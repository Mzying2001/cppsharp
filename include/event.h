/**
 * @file event.h
 * @brief Event implementation with member and static event binding.
 *
 * Provides the Event class template, MemberEventInitializer, StaticEventInitializer,
 * and EventHandler type alias for connecting delegates to member or static
 * event sources with type-safe accessor binding.
 */

#pragma once

#ifndef EVENT_H_INCLUDED
#define EVENT_H_INCLUDED

#include "delegate.h"
#include <cassert>
#include <cstddef>
#include <limits>

/*================================================================================*/

/**
 * Forward declarations.
 */

template <typename>
class Event;

/*================================================================================*/

/**
 * @brief SFINAE trait to check whether a delegate supports += and -= with a given type.
 */
template <typename TDelegate, typename TFunc, typename = void>
struct _DelegateCanAddSubtract : std::false_type {
};

/**
 * @brief Partial specialization that checks whether TDelegate supports += and -= with TFunc.
 */
template <typename TDelegate, typename TFunc>
struct _DelegateCanAddSubtract<
    TDelegate, TFunc,
    decltype(void(std::declval<TDelegate>() += std::declval<TFunc>()),
             void(std::declval<TDelegate>() -= std::declval<TFunc>()))>
    : std::true_type {
};

/*================================================================================*/

/**
 * @brief Initializer for member events, binding an owner object to a delegate accessor.
 */
template <typename TOwner, typename TDelegate>
class MemberEventInitializer
{
    template <typename>
    friend class Event;

private:
    /**
     * @brief Pointer to the event owner object.
     */
    TOwner *_owner;

    /**
     * @brief Function pointer that accesses the delegate on the owner object.
     */
    TDelegate &(*_accessor)(TOwner *);

public:
    /**
     * @brief Constructs a MemberEventInitializer with the given owner.
     */
    explicit MemberEventInitializer(TOwner *owner)
        : _owner(owner), _accessor(nullptr)
    {
    }

    /**
     * @brief Sets the delegate accessor using a free function.
     */
    MemberEventInitializer &Delegate(TDelegate &(*accessor)(TOwner *))
    {
        _accessor = accessor;
        return *this;
    }

    /**
     * @brief Sets the delegate accessor using a non-const member function.
     */
    template <TDelegate (TOwner::*accessor)()>
    MemberEventInitializer &Delegate()
    {
        return Delegate([](TOwner *owner) -> TDelegate & {
            return (owner->*accessor)();
        });
    }

    /**
     * @brief Sets the delegate accessor using a const member function.
     */
    template <TDelegate (TOwner::*accessor)() const>
    MemberEventInitializer &Delegate()
    {
        return Delegate([](TOwner *owner) -> TDelegate & {
            return (owner->*accessor)();
        });
    }

    /**
     * @brief Sets the delegate accessor using a member field.
     */
    template <TDelegate TOwner::*field>
    MemberEventInitializer &Delegate()
    {
        return Delegate([](TOwner *owner) -> TDelegate & {
            return owner->*field;
        });
    }
};

/**
 * @brief Initializer for static events, binding a static delegate accessor.
 */
template <typename TDelegate>
class StaticEventInitializer
{
    template <typename>
    friend class Event;

private:
    /**
     * @brief Function pointer that accesses the static delegate.
     */
    TDelegate &(*_accessor)();

public:
    /**
     * @brief Constructs a StaticEventInitializer.
     */
    StaticEventInitializer()
        : _accessor(nullptr)
    {
    }

    /**
     * @brief Sets the delegate accessor using a static function.
     */
    StaticEventInitializer &Delegate(TDelegate &(*accessor)())
    {
        _accessor = accessor;
        return *this;
    }
};

/*================================================================================*/

/**
 * @brief Event class that wraps a delegate and supports member/static binding.
 */
template <typename TRet, typename... Args>
class Event<Delegate<TRet(Args...)>> final
{
private:
    /**
     * @brief Generic function pointer type used to store any function pointer signature.
     * @note Reinterpreting between function pointer types and back is well-defined in C++;
     *       using a unified function pointer type avoids the conditionally-supported
     *       conversion between function pointers and void*.
     */
    using TFuncPtr = void (*)();

    /**
     * @brief Sentinel offset value indicating a static event.
     */
    static constexpr std::ptrdiff_t _STATICOFFSET =
        (std::numeric_limits<std::ptrdiff_t>::max)();

public:
    /**
     * @brief The delegate type associated with this event.
     */
    using TDelegate = Delegate<TRet(Args...)>;

    /**
     * @brief Type alias for the current event type.
     */
    using TEvent = Event<TDelegate>;

public:
    /**
     * @brief Constructs a member event from an initializer.
     * @param initializer The member event initializer.
     */
    template <typename TOwner>
    explicit Event(const MemberEventInitializer<TOwner, TDelegate> &initializer)
    {
        assert(initializer._owner != nullptr);
        assert(initializer._accessor != nullptr);

        SetOwner(initializer._owner);
        _accessor = reinterpret_cast<TFuncPtr>(initializer._accessor);

        _extractor = [](void *owner, TFuncPtr accessor) -> TDelegate & {
            return reinterpret_cast<TDelegate &(*)(TOwner *)>(accessor)(reinterpret_cast<TOwner *>(owner));
        };
    }

    /**
     * @brief Constructs a static event from an initializer.
     * @param initializer The static event initializer.
     */
    explicit Event(const StaticEventInitializer<TDelegate> &initializer)
    {
        assert(initializer._accessor != nullptr);

        SetOwner(nullptr);
        _accessor = reinterpret_cast<TFuncPtr>(initializer._accessor);

        _extractor = [](void * /*owner*/, TFuncPtr accessor) -> TDelegate & {
            return reinterpret_cast<TDelegate &(*)()>(accessor)();
        };
    }

    /**
     * @brief Adds an event handler.
     * @param handler The handler to add; can be any callable object.
     */
    template <typename T>
    auto operator+=(T &&handler) const
        -> typename std::enable_if<_DelegateCanAddSubtract<TDelegate, T>::value>::type
    {
        this->GetDelegate() += std::forward<T>(handler);
    }

    /**
     * @brief Removes an event handler.
     * @param handler The handler to remove; can be any callable object.
     */
    template <typename T>
    auto operator-=(T &&handler) const
        -> typename std::enable_if<_DelegateCanAddSubtract<TDelegate, T>::value>::type
    {
        this->GetDelegate() -= std::forward<T>(handler);
    }

public:
    /**
     * @brief Creates a MemberEventInitializer for binding a member event.
     */
    template <typename TOwner>
    static MemberEventInitializer<TOwner, TDelegate> Init(TOwner *owner)
    {
        return MemberEventInitializer<TOwner, TDelegate>(owner);
    }

    /**
     * @brief Creates a StaticEventInitializer for binding a static event.
     */
    static StaticEventInitializer<TDelegate> Init()
    {
        return StaticEventInitializer<TDelegate>();
    }

private:
    /**
     * @brief Byte offset from this event to its owner object.
     */
    std::ptrdiff_t _offset;

    /**
     * @brief Type-erased pointer to the delegate accessor function.
     */
    TFuncPtr _accessor;

    /**
     * @brief Type-erased delegate extractor used to obtain a reference to the delegate.
     */
    TDelegate &(*_extractor)(void *owner, TFuncPtr accessor);

    /**
     * @brief Checks whether this event is a static event.
     */
    bool IsStatic() const noexcept
    {
        return _offset == _STATICOFFSET;
    }

    /**
     * @brief Sets the owner object for this event.
     * @param owner Pointer to the owner object, or nullptr for static events.
     */
    void SetOwner(void *owner) noexcept
    {
        if (owner == nullptr) {
            _offset = _STATICOFFSET;
        } else {
            _offset = reinterpret_cast<uint8_t *>(owner) - reinterpret_cast<uint8_t *>(this);
        }
    }

    /**
     * @brief Gets the owner object, or nullptr if this is a static event.
     */
    void *GetOwner() const noexcept
    {
        if (IsStatic()) {
            return nullptr;
        } else {
            return const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(this)) + _offset;
        }
    }

    /**
     * @brief Gets a reference to the underlying delegate.
     */
    TDelegate &GetDelegate() const
    {
        return _extractor(GetOwner(), _accessor);
    }
};

/*================================================================================*/

/**
 * @brief Base event arguments structure.
 */
struct EventArgs {
};

/**
 * @brief Type alias for event handler delegates with sender and event args.
 */
template <typename TSender, typename TEventArgs = EventArgs>
using EventHandler = Delegate<void(TSender &, TEventArgs &)>;

/*================================================================================*/

#endif // !EVENT_H_INCLUDED
