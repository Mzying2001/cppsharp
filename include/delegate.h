/**
 * @file delegate.h
 * @brief Multicast delegate implementation similar to C# Delegate.
 *
 * Provides ICallable, Delegate, Action, Predicate, and Func types that
 * support storing, combining, and invoking multiple callable objects
 * (free functions, member functions, lambdas, functors).
 */

#pragma once

#ifndef DELEGATE_H_INCLUDED
#define DELEGATE_H_INCLUDED

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <vector>

/*================================================================================*/

/**
 * Forward declarations
 */

template <typename>
struct ICallable;

template <typename>
class Delegate;

/*================================================================================*/

/**
 * @brief Interface for callable objects.
 */
template <typename TRet, typename... Args>
struct ICallable<TRet(Args...)> {
    virtual ~ICallable() = default;

    /**
     * @brief Invokes the callable object.
     * @param args Arguments to forward to the callable.
     * @return The return value of the callable.
     */
    virtual TRet Invoke(Args... args) const = 0;

    /**
     * @brief Creates a copy of this callable object.
     */
    virtual ICallable *Clone() const = 0;

    /**
     * @brief Returns the type information of the wrapped callable.
     */
    virtual std::type_index GetType() const = 0;

    /**
     * @brief Checks whether this callable is equal to another.
     * @param other The callable to compare against.
     * @return true if the two callables are equal, false otherwise.
     */
    virtual bool Equals(const ICallable &other) const = 0;
};

/*================================================================================*/

/**
 * @brief Stores and manages multiple callable objects, optimized for the single-callable case.
 */
template <typename T>
class CallableList
{
public:
    using TCallable   = ICallable<T>;
    using TSinglePtr  = std::unique_ptr<TCallable>;
    using TSharedList = std::vector<std::shared_ptr<TCallable>>;

private:
    mutable union {
        alignas(TSinglePtr) uint8_t _single[sizeof(TSinglePtr)];
        alignas(TSharedList) uint8_t _list[sizeof(TSharedList)];
    } _data = {};

    enum : uint8_t {
        STATE_NONE,   ///< No callable stored
        STATE_SINGLE, ///< Exactly one callable stored
        STATE_LIST,   ///< Multiple callables stored
    } _state = STATE_NONE;

public:
    /**
     * @brief Default constructor.
     */
    CallableList()
    {
    }

    /**
     * @brief Copy constructor.
     */
    CallableList(const CallableList &other)
    {
        *this = other;
    }

    /**
     * @brief Move constructor.
     */
    CallableList(CallableList &&other) noexcept
    {
        *this = std::move(other);
    }

    /**
     * @brief Copy assignment with strong exception safety.
     * @note All potentially-throwing work (Clone / vector copy) is done first;
     *       the commit phase (_Reset + pointer/vector move-assignment) is noexcept.
     */
    CallableList &operator=(const CallableList &other)
    {
        if (this == &other) {
            return *this;
        }

        switch (other._state) {
            case STATE_NONE: {
                _Reset();
                break;
            }
            case STATE_SINGLE: {
                std::unique_ptr<TCallable> cloned(other._GetSingle()->Clone());
                _Reset(STATE_SINGLE);
                _GetSingle() = std::move(cloned);
                break;
            }
            case STATE_LIST: {
                TSharedList copied = other._GetList();
                _Reset(STATE_LIST);
                _GetList() = std::move(copied);
                break;
            }
        }
        return *this;
    }

    /**
     * @brief Move assignment.
     */
    CallableList &operator=(CallableList &&other) noexcept
    {
        if (this == &other) {
            return *this;
        }

        _Reset(other._state);

        switch (other._state) {
            case STATE_NONE: {
                break;
            }
            case STATE_SINGLE: {
                _GetSingle() = std::move(other._GetSingle());
                other._Reset();
                break;
            }
            case STATE_LIST: {
                _GetList() = std::move(other._GetList());
                other._Reset();
                break;
            }
        }
        return *this;
    }

    /**
     * @brief Destructor.
     */
    ~CallableList()
    {
        _Reset();
    }

    /**
     * @brief Returns the number of stored callables.
     * @return The number of callables.
     */
    size_t Count() const noexcept
    {
        switch (_state) {
            case STATE_SINGLE: {
                return 1;
            }
            case STATE_LIST: {
                return _GetList().size();
            }
            default: {
                return 0;
            }
        }
    }

    /**
     * @brief Checks whether no callables are stored.
     * @return true if empty, false otherwise.
     */
    bool IsEmpty() const noexcept
    {
        return _state == STATE_NONE;
    }

    /**
     * @brief Removes all stored callables.
     */
    void Clear() noexcept
    {
        _Reset();
    }

    /**
     * @brief Adds a callable to the list.
     * @note Ownership of the raw pointer is transferred to the CallableList.
     * @note Exception safety:
     *       - SINGLE->LIST upgrade uses reserve(2) to avoid reallocation during
     *         emplace_back; the only failure path is shared_ptr control-block
     *         allocation, where the shared_ptr constructor guarantees the raw
     *         pointer is deleted on exception.
     *       - STATE_LIST branch wraps the raw pointer in a local shared_ptr
     *         first; even if vector reallocation fails, the local shared_ptr
     *         destructor correctly frees the object.
     */
    void Add(TCallable *callable)
    {
        if (callable == nullptr) {
            return;
        }

        switch (_state) {
            case STATE_NONE: {
                _Reset(STATE_SINGLE);
                _GetSingle().reset(callable);
                break;
            }
            case STATE_SINGLE: {
                TSharedList list;
                list.reserve(2);
                list.emplace_back(_GetSingle().release());
                list.emplace_back(callable);
                _Reset(STATE_LIST);
                _GetList() = std::move(list);
                break;
            }
            case STATE_LIST: {
                std::shared_ptr<TCallable> sp(callable);
                _GetList().emplace_back(std::move(sp));
                break;
            }
        }
    }

    /**
     * @brief Removes the callable at the specified index.
     * @return true if the element was removed, false otherwise.
     */
    bool RemoveAt(size_t index) noexcept
    {
        switch (_state) {
            case STATE_SINGLE: {
                if (index != 0) {
                    return false;
                } else {
                    _Reset();
                    return true;
                }
            }
            case STATE_LIST: {
                auto &list = _GetList();
                if (index >= list.size()) {
                    return false;
                }
                list.erase(list.begin() + index);
                if (list.empty()) {
                    _Reset();
                }
                // Note: when the list shrinks to one element we do NOT downgrade
                // back to SINGLE.  shared_ptr cannot be moved into unique_ptr,
                // and a forced Clone would add unnecessary overhead.  Keeping a
                // single-element LIST state is acceptable both functionally and
                // performance-wise.
                return true;
            }
            default: {
                return false;
            }
        }
    }

    /**
     * @brief Returns the callable at the specified index.
     * @return Pointer to the callable, or nullptr if the index is out of range.
     */
    TCallable *GetAt(size_t index) const noexcept
    {
        switch (_state) {
            case STATE_SINGLE: {
                return index == 0 ? _GetSingle().get() : nullptr;
            }
            case STATE_LIST: {
                auto &list = _GetList();
                return (index < list.size()) ? list[index].get() : nullptr;
            }
            default: {
                return nullptr;
            }
        }
    }

    /**
     * @brief Returns the callable at the specified index.
     * @return Pointer to the callable, or nullptr if the index is out of range.
     */
    TCallable *operator[](size_t index) const noexcept
    {
        return GetAt(index);
    }

private:
    /**
     * @brief Returns a reference to the single callable (valid only in STATE_SINGLE).
     */
    constexpr TSinglePtr &_GetSingle() const noexcept
    {
        return *reinterpret_cast<TSinglePtr *>(_data._single);
    }

    /**
     * @brief Returns a reference to the callable list (valid only in STATE_LIST).
     */
    constexpr TSharedList &_GetList() const noexcept
    {
        return *reinterpret_cast<TSharedList *>(_data._list);
    }

    /**
     * @brief Destroys the stored callable(s) and resets to STATE_NONE.
     */
    void _Reset() noexcept
    {
        switch (_state) {
            case STATE_NONE: {
                break;
            }
            case STATE_SINGLE: {
                _GetSingle().~TSinglePtr();
                _state = STATE_NONE;
                break;
            }
            case STATE_LIST: {
                _GetList().~TSharedList();
                _state = STATE_NONE;
                break;
            }
        }
    }

    /**
     * @brief Destroys current state and re-initializes to the given state.
     */
    void _Reset(uint8_t state) noexcept
    {
        _Reset();

        switch (state) {
            case STATE_SINGLE: {
                new (_data._single) TSinglePtr();
                _state = STATE_SINGLE;
                break;
            }
            case STATE_LIST: {
                new (_data._list) TSharedList();
                _state = STATE_LIST;
                break;
            }
        }
    }
};

/*================================================================================*/

/**
 * @brief Multicast delegate similar to C# Delegate, capable of storing and
 *        invoking one or more callable objects.
 */
template <typename TRet, typename... Args>
class Delegate<TRet(Args...)> final : public ICallable<TRet(Args...)>
{
private:
    using _ICallable = ICallable<TRet(Args...)>;

    template <typename T, typename = void>
    struct _IsEqualityComparable : std::false_type {
    };

    template <typename T>
    struct _IsEqualityComparable<
        T, decltype(void(std::declval<T>() == std::declval<T>()))> : std::true_type {
    };

    template <typename T, typename = void>
    struct _IsMemcmpSafe : std::false_type {
    };

    template <typename T>
    struct _IsMemcmpSafe<
        T,
        typename std::enable_if</*std::is_trivial<T>::value &&*/ std::is_standard_layout<T>::value, void>::type> : std::true_type {
    };

    template <typename T>
    class _CallableWrapperImpl final : public _ICallable
    {
        alignas(T) mutable uint8_t _storage[sizeof(T)];

    public:
        _CallableWrapperImpl(const T &value)
        {
            memset(_storage, 0, sizeof(_storage));
            new (_storage) T(value);
        }
        _CallableWrapperImpl(T &&value)
        {
            memset(_storage, 0, sizeof(_storage));
            new (_storage) T(std::move(value));
        }
        virtual ~_CallableWrapperImpl()
        {
            GetValue().~T();
            // memset(_storage, 0, sizeof(_storage));
        }
        T &GetValue() const noexcept
        {
            return *reinterpret_cast<T *>(_storage);
        }
        TRet Invoke(Args... args) const override
        {
            return GetValue()(std::forward<Args>(args)...);
        }
        _ICallable *Clone() const override
        {
            return new _CallableWrapperImpl(GetValue());
        }
        virtual std::type_index GetType() const override
        {
            return typeid(T);
        }
        bool Equals(const _ICallable &other) const override
        {
            return EqualsImpl(other);
        }
        template <typename U = T>
        auto EqualsImpl(const _ICallable &other) const
            -> typename std::enable_if<_IsEqualityComparable<U>::value, bool>::type
        {
            if (this == &other) {
                return true;
            }
            if (GetType() != other.GetType()) {
                return false;
            }
            const auto &otherWrapper = static_cast<const _CallableWrapperImpl &>(other);
            return GetValue() == otherWrapper.GetValue();
        }
        template <typename U = T>
        auto EqualsImpl(const _ICallable &other) const
            -> typename std::enable_if<!_IsEqualityComparable<U>::value && _IsMemcmpSafe<U>::value, bool>::type
        {
            if (this == &other) {
                return true;
            }
            if (GetType() != other.GetType()) {
                return false;
            }
            const auto &otherWrapper = static_cast<const _CallableWrapperImpl &>(other);
            return memcmp(_storage, otherWrapper._storage, sizeof(_storage)) == 0;
        }
        template <typename U = T>
        auto EqualsImpl(const _ICallable &other) const
            -> typename std::enable_if<!_IsEqualityComparable<U>::value && !_IsMemcmpSafe<U>::value, bool>::type
        {
            return this == &other;
        }

    public:
        // Disable copy/move: the default implementation would byte-copy _storage
        // without calling T's constructor, breaking invariants for non-trivial types.
        // Use Clone() instead.
        _CallableWrapperImpl(const _CallableWrapperImpl &)            = delete;
        _CallableWrapperImpl(_CallableWrapperImpl &&)                 = delete;
        _CallableWrapperImpl &operator=(const _CallableWrapperImpl &) = delete;
        _CallableWrapperImpl &operator=(_CallableWrapperImpl &&)      = delete;
    };

    template <typename T>
    using _CallableWrapper = _CallableWrapperImpl<typename std::decay<T>::type>;

    template <typename T>
    class _MemberFuncWrapper final : public _ICallable
    {
        T *obj;
        TRet (T::*func)(Args...);

    public:
        _MemberFuncWrapper(T &obj, TRet (T::*func)(Args...))
            : obj(&obj), func(func)
        {
        }
        TRet Invoke(Args... args) const override
        {
            return (obj->*func)(std::forward<Args>(args)...);
        }
        _ICallable *Clone() const override
        {
            return new _MemberFuncWrapper(*obj, func);
        }
        virtual std::type_index GetType() const override
        {
            return typeid(func);
        }
        bool Equals(const _ICallable &other) const override
        {
            if (this == &other) {
                return true;
            }
            if (GetType() != other.GetType()) {
                return false;
            }
            const auto &otherWrapper = static_cast<const _MemberFuncWrapper &>(other);
            return obj == otherWrapper.obj && func == otherWrapper.func;
        }

    public:
        // Disable copy/move — consistent with _CallableWrapperImpl. Use Clone().
        _MemberFuncWrapper(const _MemberFuncWrapper &)            = delete;
        _MemberFuncWrapper(_MemberFuncWrapper &&)                 = delete;
        _MemberFuncWrapper &operator=(const _MemberFuncWrapper &) = delete;
        _MemberFuncWrapper &operator=(_MemberFuncWrapper &&)      = delete;
    };

    template <typename T>
    class _ConstMemberFuncWrapper final : public _ICallable
    {
        const T *obj;
        TRet (T::*func)(Args...) const;

    public:
        _ConstMemberFuncWrapper(const T &obj, TRet (T::*func)(Args...) const)
            : obj(&obj), func(func)
        {
        }
        TRet Invoke(Args... args) const override
        {
            return (obj->*func)(std::forward<Args>(args)...);
        }
        _ICallable *Clone() const override
        {
            return new _ConstMemberFuncWrapper(*obj, func);
        }
        virtual std::type_index GetType() const override
        {
            return typeid(func);
        }
        bool Equals(const _ICallable &other) const override
        {
            if (this == &other) {
                return true;
            }
            if (GetType() != other.GetType()) {
                return false;
            }
            const auto &otherWrapper = static_cast<const _ConstMemberFuncWrapper &>(other);
            return obj == otherWrapper.obj && func == otherWrapper.func;
        }

    public:
        // Disable copy/move — consistent with _CallableWrapperImpl. Use Clone().
        _ConstMemberFuncWrapper(const _ConstMemberFuncWrapper &)            = delete;
        _ConstMemberFuncWrapper(_ConstMemberFuncWrapper &&)                 = delete;
        _ConstMemberFuncWrapper &operator=(const _ConstMemberFuncWrapper &) = delete;
        _ConstMemberFuncWrapper &operator=(_ConstMemberFuncWrapper &&)      = delete;
    };

private:
    CallableList<TRet(Args...)> _data;

public:
    /**
     * @brief Default constructor; creates an empty delegate.
     */
    Delegate(std::nullptr_t = nullptr)
    {
    }

    /**
     * @brief Constructs a delegate from an ICallable object.
     */
    Delegate(const ICallable<TRet(Args...)> &callable)
    {
        Add(callable);
    }

    /**
     * @brief Constructs a delegate from a function pointer.
     */
    Delegate(TRet (*func)(Args...))
    {
        Add(func);
    }

    /**
     * @brief Constructs a delegate from a callable object.
     */
    template <typename T, typename std::enable_if<!std::is_base_of<_ICallable, T>::value, int>::type = 0>
    Delegate(const T &callable)
    {
        Add(callable);
    }

    /**
     * @brief Constructs a delegate bound to a member function.
     */
    template <typename T>
    Delegate(T &obj, TRet (T::*func)(Args...))
    {
        Add(obj, func);
    }

    /**
     * @brief Constructs a delegate bound to a const member function.
     */
    template <typename T>
    Delegate(const T &obj, TRet (T::*func)(Args...) const)
    {
        Add(obj, func);
    }

    /**
     * @brief Copy constructor; deep-copies all callables.
     */
    Delegate(const Delegate &other)
    {
        for (size_t i = 0; i < other._data.Count(); ++i) {
            _data.Add(other._data[i]->Clone());
        }
    }

    /**
     * @brief Move constructor.
     */
    Delegate(Delegate &&other) noexcept
        : _data(std::move(other._data))
    {
    }

    /**
     * @brief Copy assignment; deep-copies all callables from @p other.
     */
    Delegate &operator=(const Delegate &other)
    {
        if (this == &other) {
            return *this;
        }
        _data.Clear();
        for (size_t i = 0; i < other._data.Count(); ++i) {
            _data.Add(other._data[i]->Clone());
        }
        return *this;
    }

    /**
     * @brief Move assignment.
     */
    Delegate &operator=(Delegate &&other) noexcept
    {
        if (this != &other) {
            _data = std::move(other._data);
        }
        return *this;
    }

    /**
     * @brief Adds a callable to this delegate.
     * @note When the argument is a Delegate of the same type:
     *       - Empty delegate: no-op.
     *       - Exactly one element: the inner element is cloned and added
     *         directly (equivalent to single-cast add).
     *       - Two or more elements: the delegate is added as a nested whole
     *         without flattening.  This preserves add/remove symmetry so that
     *         `*this -= callable` can still match and undo the add.  Therefore
     *         `b += a; b == a` is false when a has >=2 elements (different
     *         count), but `b += a; b -= a` restores b to its original state.
     */
    void Add(const ICallable<TRet(Args...)> &callable)
    {
        if (callable.GetType() == GetType()) {
            auto &delegate = static_cast<const Delegate &>(callable);
            if (delegate._data.IsEmpty()) {
                return;
            } else if (delegate._data.Count() == 1) {
                _data.Add(delegate._data[0]->Clone());
                return;
            }
        }
        _data.Add(callable.Clone());
    }

    /**
     * @brief Adds a function pointer to this delegate.
     */
    void Add(TRet (*func)(Args...))
    {
        if (func != nullptr) {
            _data.Add(new _CallableWrapper<decltype(func)>(func));
        }
    }

    /**
     * @brief Adds a callable object to this delegate.
     */
    template <typename T>
    auto Add(const T &callable)
        -> typename std::enable_if<!std::is_base_of<_ICallable, T>::value, void>::type
    {
        _data.Add(new _CallableWrapper<T>(callable));
    }

    /**
     * @brief Adds a member function pointer to this delegate.
     */
    template <typename T>
    void Add(T &obj, TRet (T::*func)(Args...))
    {
        _data.Add(new _MemberFuncWrapper<T>(obj, func));
    }

    /**
     * @brief Adds a const member function pointer to this delegate.
     */
    template <typename T>
    void Add(const T &obj, TRet (T::*func)(Args...) const)
    {
        _data.Add(new _ConstMemberFuncWrapper<T>(obj, func));
    }

    /**
     * @brief Removes all callables from this delegate.
     */
    void Clear()
    {
        _data.Clear();
    }

    /**
     * @brief Removes a callable from this delegate.
     * @return true if a matching callable was found and removed, false otherwise.
     * @note Searches from the most-recently-added callable backward.
     *       Symmetric with Add — when the argument is a Delegate of the same type:
     *       - Empty delegate: returns false.
     *       - Exactly one element: attempts to match and remove that element itself.
     *       - Two or more elements: matches and removes the delegate as a nested
     *         whole, mirroring the non-flattening behavior of Add.
     */
    bool Remove(const ICallable<TRet(Args...)> &callable)
    {
        if (callable.GetType() == GetType()) {
            auto &delegate = static_cast<const Delegate &>(callable);
            if (delegate._data.IsEmpty()) {
                return false;
            } else if (delegate._data.Count() == 1) {
                return _Remove(*delegate._data[0]);
            }
        }
        return _Remove(callable);
    }

    /**
     * @brief Removes a function pointer from this delegate.
     * @return true if a matching function pointer was found and removed, false otherwise.
     * @note Searches from the most-recently-added callable backward.
     */
    bool Remove(TRet (*func)(Args...))
    {
        if (func == nullptr) {
            return false;
        }
        return _Remove(_CallableWrapper<decltype(func)>(func));
    }

    /**
     * @brief Removes a callable object from this delegate.
     * @return true if a matching callable was found and removed, false otherwise.
     * @note Searches from the most-recently-added callable backward.
     */
    template <typename T>
    auto Remove(const T &callable)
        -> typename std::enable_if<!std::is_base_of<_ICallable, T>::value, bool>::type
    {
        return _Remove(_CallableWrapper<T>(callable));
    }

    /**
     * @brief Removes a member function pointer from this delegate.
     * @return true if a matching callable was found and removed, false otherwise.
     * @note Searches from the most-recently-added callable backward.
     */
    template <typename T>
    bool Remove(T &obj, TRet (T::*func)(Args...))
    {
        return _Remove(_MemberFuncWrapper<T>(obj, func));
    }

    /**
     * @brief Removes a const member function pointer from this delegate.
     * @return true if a matching callable was found and removed, false otherwise.
     * @note Searches from the most-recently-added callable backward.
     */
    template <typename T>
    bool Remove(const T &obj, TRet (T::*func)(Args...) const)
    {
        return _Remove(_ConstMemberFuncWrapper<T>(obj, func));
    }

    /**
     * @brief Invokes all stored callables.
     * @param args Arguments forwarded to each callable.
     * @return The return value of the last callable.
     * @throw std::runtime_error if the delegate is empty.
     */
    TRet operator()(Args... args) const
    {
        return _InvokeImpl(std::forward<Args>(args)...);
    }

    /**
     * @brief Checks whether this delegate is equal to another.
     * @param other The delegate to compare against.
     * @return true if both delegates contain the same callables in the same order.
     */
    bool operator==(const Delegate &other) const
    {
        return Equals(other);
    }

    /**
     * @brief Checks whether this delegate is not equal to another.
     * @param other The delegate to compare against.
     * @return true if the delegates differ, false otherwise.
     */
    bool operator!=(const Delegate &other) const
    {
        return !Equals(other);
    }

    /**
     * @brief Checks whether this delegate is null (empty).
     * @return true if the delegate has no callables, false otherwise.
     */
    bool operator==(std::nullptr_t) const noexcept
    {
        return _data.IsEmpty();
    }

    /**
     * @brief Checks whether this delegate is not null (non-empty).
     * @return true if the delegate has at least one callable, false otherwise.
     */
    bool operator!=(std::nullptr_t) const noexcept
    {
        return !_data.IsEmpty();
    }

    /**
     * @brief Explicit bool conversion; returns true if the delegate is non-empty.
     */
    explicit operator bool() const noexcept
    {
        return !_data.IsEmpty();
    }

    /**
     * @brief Adds a callable to this delegate (delegates to Add).
     */
    Delegate &operator+=(const ICallable<TRet(Args...)> &callable)
    {
        Add(callable);
        return *this;
    }

    /**
     * @brief Adds a function pointer to this delegate (delegates to Add).
     */
    Delegate &operator+=(TRet (*func)(Args...))
    {
        Add(func);
        return *this;
    }

    /**
     * @brief Adds a callable object to this delegate (delegates to Add).
     */
    template <typename T>
    auto operator+=(const T &callable)
        -> typename std::enable_if<!std::is_base_of<_ICallable, T>::value, Delegate &>::type
    {
        Add(callable);
        return *this;
    }

    /**
     * @brief Removes a callable from this delegate (delegates to Remove).
     */
    Delegate &operator-=(const ICallable<TRet(Args...)> &callable)
    {
        Remove(callable);
        return *this;
    }

    /**
     * @brief Removes a function pointer from this delegate (delegates to Remove).
     */
    Delegate &operator-=(TRet (*func)(Args...))
    {
        Remove(func);
        return *this;
    }

    /**
     * @brief Removes a callable object from this delegate (delegates to Remove).
     */
    template <typename T>
    auto operator-=(const T &callable)
        -> typename std::enable_if<!std::is_base_of<_ICallable, T>::value, Delegate &>::type
    {
        Remove(callable);
        return *this;
    }

    /**
     * @brief Invokes all stored callables.
     * @param args Arguments forwarded to each callable.
     * @return The return value of the last callable.
     * @throw std::runtime_error if the delegate is empty.
     */
    virtual TRet Invoke(Args... args) const override
    {
        return _InvokeImpl(std::forward<Args>(args)...);
    }

    /**
     * @brief Creates a deep copy of this delegate.
     * @return A new Delegate containing cloned copies of all callables.
     */
    virtual ICallable<TRet(Args...)> *Clone() const override
    {
        return new Delegate(*this);
    }

    /**
     * @brief Returns the type information for this delegate.
     * @return typeid(Delegate<TRet(Args...)>).
     */
    virtual std::type_index GetType() const override
    {
        return typeid(Delegate<TRet(Args...)>);
    }

    /**
     * @brief Checks whether this delegate equals another callable.
     * @param other The callable to compare against.
     * @return true if both delegates contain the same callables in the same order.
     */
    virtual bool Equals(const ICallable<TRet(Args...)> &other) const override
    {
        if (this == &other) {
            return true;
        }
        if (GetType() != other.GetType()) {
            return false;
        }
        const auto &otherDelegate = static_cast<const Delegate &>(other);
        if (_data.Count() != otherDelegate._data.Count()) {
            return false;
        }
        for (size_t i = _data.Count(); i > 0; --i) {
            if (!_data[i - 1]->Equals(*otherDelegate._data[i - 1])) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Invokes all stored callables and collects their return values.
     * @param args Arguments forwarded to each callable.
     * @return A vector containing the return value of each callable.
     * @note During multicast invocation the first N-1 callables receive args as
     *       lvalues; only the last invocation uses std::forward.  This avoids
     *       repeatedly moving the same object when dealing with move-only types
     *       or rvalue-reference parameters.
     */
    template <typename U = TRet>
    auto InvokeAll(Args... args) const
        -> typename std::enable_if<!std::is_void<U>::value, std::vector<U>>::type
    {
        std::vector<U> results;
        size_t count = _data.Count();
        if (count == 0) {
            _ThrowEmptyDelegateError();
        } else if (count == 1) {
            results.emplace_back(_data[0]->Invoke(std::forward<Args>(args)...));
        } else {
            auto list = _data;
            results.reserve(count);
            for (size_t i = 0; i + 1 < count; ++i) {
                results.emplace_back(list[i]->Invoke(args...));
            }
            results.emplace_back(list[count - 1]->Invoke(std::forward<Args>(args)...));
        }
        return results;
    }

private:
    /**
     * @brief Searches backward for the first callable equal to the given one and removes it.
     */
    bool _Remove(const _ICallable &callable)
    {
        for (size_t i = _data.Count(); i > 0; --i) {
            if (_data[i - 1]->Equals(callable)) {
                return _data.RemoveAt(i - 1);
            }
        }
        return false;
    }

    /**
     * @brief Throws a runtime error indicating the delegate is empty.
     */
    [[noreturn]] void _ThrowEmptyDelegateError() const
    {
        throw std::runtime_error("Delegate is empty");
    }

    /**
     * @brief Shared implementation for Invoke() and operator().
     * @note During multicast invocation the first N-1 callables receive args as
     *       lvalues; only the last invocation uses std::forward.
     */
    inline TRet _InvokeImpl(Args... args) const
    {
        size_t count = _data.Count();
        if (count == 0) {
            _ThrowEmptyDelegateError();
        } else if (count == 1) {
            return _data[0]->Invoke(std::forward<Args>(args)...);
        } else {
            auto list = _data;
            for (size_t i = 0; i + 1 < count; ++i)
                list[i]->Invoke(args...);
            return list[count - 1]->Invoke(std::forward<Args>(args)...);
        }
    }
};

/*================================================================================*/

/**
 * @brief Compares nullptr with a delegate for equality.
 * @return true if the delegate is empty, false otherwise.
 */
template <typename TRet, typename... Args>
inline bool operator==(std::nullptr_t, const Delegate<TRet(Args...)> &d) noexcept
{
    return d == nullptr;
}

/**
 * @brief Compares nullptr with a delegate for inequality.
 * @return true if the delegate is non-empty, false otherwise.
 */
template <typename TRet, typename... Args>
inline bool operator!=(std::nullptr_t, const Delegate<TRet(Args...)> &d) noexcept
{
    return d != nullptr;
}

/*================================================================================*/

/**
 * @brief Type alias for a void-returning delegate (similar to C# Action).
 */
template <typename... Args>
using Action = Delegate<void(Args...)>;

/**
 * @brief Type alias for a single-parameter bool-returning delegate (similar to C# Predicate).
 */
template <typename T>
using Predicate = Delegate<bool(T)>;

/*================================================================================*/

/**
 * @brief Helper to extract return type and argument types from a variadic type list.
 */
template <typename...>
struct _FuncTraits;

/**
 * @brief Base case: the last type is the return type.
 */
template <typename Last>
struct _FuncTraits<Last> {
    using TRet       = Last;
    using TArgsTuple = std::tuple<>;
};

/**
 * @brief Recursive case: accumulate argument types and forward the return type.
 */
template <typename First, typename... Rest>
struct _FuncTraits<First, Rest...> {
    using TRet       = typename _FuncTraits<Rest...>::TRet;
    using TArgsTuple = decltype(std::tuple_cat(std::declval<std::tuple<First>>(), std::declval<typename _FuncTraits<Rest...>::TArgsTuple>()));
};

/**
 * @brief Maps an argument tuple to a Delegate type.
 */
template <typename TArgsTuple>
struct _FuncTypeHelper;

/**
 * @brief Specialization that unpacks the tuple into a Delegate parameter list.
 */
template <typename... Args>
struct _FuncTypeHelper<std::tuple<Args...>> {
    template <typename TRet>
    using TFunc = Delegate<TRet(Args...)>;
};

/**
 * @brief Type alias similar to C# Func<T1, T2, ..., TResult>.
 */
template <typename... Types>
using Func = typename _FuncTypeHelper<typename _FuncTraits<Types...>::TArgsTuple>::template TFunc<typename _FuncTraits<Types...>::TRet>;

/*================================================================================*/

#endif // DELEGATE_H_INCLUDED
