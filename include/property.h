/**
 * @file property.h
 * @brief Property implementation with getter/setter semantics, similar to C# properties.
 *
 * Provides Property, ReadOnlyProperty, and WriteOnlyProperty classes that
 * support member and static property binding, operator overloading, and
 * type-safe getter/setter access.
 */

#pragma once

#ifndef PROPERTY_H_INCLUDED
#define PROPERTY_H_INCLUDED

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

/*================================================================================*/

/**
 * @brief Generates a SFINAE trait that checks whether T @p OP U is valid (binary).
 * @param NAME The trait struct name to generate.
 * @param OP   The binary operator token (e.g. +, -, ==).
 */
#define _SW_DEFINE_OPERATION_HELPER(NAME, OP)                                                    \
    template <typename T, typename U, typename = void>                                           \
    struct NAME : std::false_type {                                                              \
    };                                                                                           \
    template <typename T, typename U>                                                            \
    struct NAME<T, U, decltype(void(std::declval<T>() OP std::declval<U>()))> : std::true_type { \
        using type = decltype(std::declval<T>() OP std::declval<U>());                           \
    }

/**
 * @brief Generates a SFINAE trait that checks whether @p OP T is valid (unary).
 * @param NAME The trait struct name to generate.
 * @param OP   The unary operator token (e.g. !, ~, *, +, -).
 */
#define _SW_DEFINE_UNARY_OPERATION_HELPER(NAME, OP)                         \
    template <typename T, typename = void>                                  \
    struct NAME : std::false_type {                                         \
    };                                                                      \
    template <typename T>                                                   \
    struct NAME<T, decltype(void(OP std::declval<T>()))> : std::true_type { \
        using type = decltype(OP std::declval<T>());                        \
    }

/*================================================================================*/

/**
 * Forward declarations
 */

template <typename T, typename TDerived>
class PropertyBase;

template <typename T>
class Property;

template <typename T>
class ReadOnlyProperty;

template <typename T>
class WriteOnlyProperty;

/*================================================================================*/

// SFINAE templates
_SW_DEFINE_OPERATION_HELPER(_AddOperationHelper, +);
_SW_DEFINE_OPERATION_HELPER(_SubOperationHelper, -);
_SW_DEFINE_OPERATION_HELPER(_MulOperationHelper, *);
_SW_DEFINE_OPERATION_HELPER(_DivOperationHelper, /);
_SW_DEFINE_OPERATION_HELPER(_ModOperationHelper, %);
_SW_DEFINE_OPERATION_HELPER(_EqOperationHelper, ==);
_SW_DEFINE_OPERATION_HELPER(_NeOperationHelper, !=);
_SW_DEFINE_OPERATION_HELPER(_LtOperationHelper, <);
_SW_DEFINE_OPERATION_HELPER(_LeOperationHelper, <=);
_SW_DEFINE_OPERATION_HELPER(_GtOperationHelper, >);
_SW_DEFINE_OPERATION_HELPER(_GeOperationHelper, >=);
_SW_DEFINE_OPERATION_HELPER(_BitAndOperationHelper, &);
_SW_DEFINE_OPERATION_HELPER(_BitOrOperationHelper, |);
_SW_DEFINE_OPERATION_HELPER(_BitXorOperationHelper, ^);
_SW_DEFINE_OPERATION_HELPER(_ShlOperationHelper, <<);
_SW_DEFINE_OPERATION_HELPER(_ShrOperationHelper, >>);
_SW_DEFINE_OPERATION_HELPER(_LogicAndOperationHelper, &&);
_SW_DEFINE_OPERATION_HELPER(_LogicOrOperationHelper, ||);
_SW_DEFINE_UNARY_OPERATION_HELPER(_LogicNotOperationHelper, !);
_SW_DEFINE_UNARY_OPERATION_HELPER(_BitNotOperationHelper, ~);
_SW_DEFINE_UNARY_OPERATION_HELPER(_DerefOperationHelper, *);
// _SW_DEFINE_UNARY_OPERATION_HELPER(_AddrOperationHelper, &);
_SW_DEFINE_UNARY_OPERATION_HELPER(_UnaryPlusOperationHelper, +);
_SW_DEFINE_UNARY_OPERATION_HELPER(_UnaryMinusOperationHelper, -);
// _SW_DEFINE_UNARY_OPERATION_HELPER(_PreIncOperationHelper, ++);
// _SW_DEFINE_UNARY_OPERATION_HELPER(_PreDecOperationHelper, --);

/**
 * @brief Implementation detail for _IsProperty.
 */
template <typename T>
struct _IsPropertyImpl {
private:
    template <typename U, typename V>
    static std::true_type test(const PropertyBase<U, V> *);
    static std::false_type test(...);

public:
    using type = decltype(test(std::declval<T *>()));
};

/**
 * @brief Checks whether a type is a property type.
 */
template <typename T>
struct _IsProperty : _IsPropertyImpl<typename std::decay<T>::type>::type {
};

/**
 * @brief Checks whether a type has a GetterImpl member.
 */
template <typename, typename = void>
struct _HasGetterImpl : std::false_type {
};

/**
 * @brief Specialization for types that have a GetterImpl member.
 */
template <typename T>
struct _HasGetterImpl<
    T, decltype(void(&T::GetterImpl))> : std::true_type {
};

/**
 * @brief Checks whether a type has a SetterImpl member.
 */
template <typename, typename = void>
struct _HasSetterImpl : std::false_type {
};

/**
 * @brief Specialization for types that have a SetterImpl member.
 */
template <typename T>
struct _HasSetterImpl<
    T, decltype(void(&T::SetterImpl))> : std::true_type {
};

/**
 * @brief Checks whether a type is a readable property.
 */
template <typename T>
struct _IsReadableProperty
    : std::integral_constant<bool, _IsProperty<T>::value && _HasGetterImpl<T>::value> {
};

/**
 * @brief Checks whether a type is a writable property.
 */
template <typename T>
struct _IsWritableProperty
    : std::integral_constant<bool, _IsProperty<T>::value && _HasSetterImpl<T>::value> {
};

/**
 * @brief Checks whether operator[] is available for the given types.
 */
template <typename T, typename U, typename = void>
struct _BracketOperationHelper : std::false_type {
};

/**
 * @brief Specialization for types that support operator[].
 */
template <typename T, typename U>
struct _BracketOperationHelper<
    T, U, decltype(void(std::declval<T>()[std::declval<U>()]))> : std::true_type {
    using type = decltype(std::declval<T>()[std::declval<U>()]);
};

/**
 * @brief Checks whether a type has operator->.
 */
template <typename T, typename = void>
struct _HasArrowOperator : std::false_type {
};

/**
 * @brief Specialization for types that have operator->.
 */
template <typename T>
struct _HasArrowOperator<
    T, decltype(void(std::declval<T>().operator->()))> : std::true_type {
    using type = decltype(std::declval<T>().operator->());
};

/**
 * @brief Helper to select the setter parameter type (pass scalars by value, others by const ref).
 */
template <typename T>
struct _PropertySetterParamTypeHelper {
    using type = typename std::conditional<
        std::is_scalar<T>::value, T, const T &>::type;
};

/**
 * @brief Deduced parameter type for property setters.
 */
template <typename T>
using _PropertySetterParamType =
    typename _PropertySetterParamTypeHelper<typename std::decay<T>::type>::type;

/*================================================================================*/

/**
 * @brief Provides operator-> access to the underlying value's fields.
 */
template <typename T>
struct FieldsAccessor {
    /**
     * @brief The stored value.
     */
    T value;

    /**
     * @brief Constructs the accessor by forwarding arguments to the value.
     */
    template <typename... Args>
    FieldsAccessor(Args &&...args)
        : value(std::forward<Args>(args)...)
    {
    }

    /**
     * @brief Pointer type: returns the pointer directly.
     */
    template <typename U = T>
    auto operator->()
        -> typename std::enable_if<std::is_pointer<U>::value, U>::type
    {
        return this->value;
    }

    /**
     * @brief Non-pointer type without operator->: returns the address of the value.
     */
    template <typename U = T>
    auto operator->()
        -> typename std::enable_if<!std::is_pointer<U>::value && !_HasArrowOperator<U>::value, U *>::type
    {
        return &this->value;
    }

    /**
     * @brief Non-pointer type with operator->: forwards to the value's operator->.
     */
    template <typename U = T>
    auto operator->()
        -> typename std::enable_if<!std::is_pointer<U>::value && _HasArrowOperator<U>::value, typename _HasArrowOperator<U>::type>::type
    {
        return this->value.operator->();
    }
};

/**
 * @brief Initializer for member (non-static) properties.
 */
template <typename TOwner, typename TValue>
class MemberPropertyInitializer
{
    friend class Property<TValue>;
    friend class ReadOnlyProperty<TValue>;
    friend class WriteOnlyProperty<TValue>;

private:
    /**
     * @brief Pointer to the owning object.
     */
    TOwner *_owner;

    /**
     * @brief Getter function pointer.
     */
    TValue (*_getter)(TOwner *);

    /**
     * @brief Setter function pointer.
     */
    void (*_setter)(TOwner *, _PropertySetterParamType<TValue>);

public:
    /**
     * @brief Constructs the initializer with the given owner.
     */
    MemberPropertyInitializer(TOwner *owner)
        : _owner(owner), _getter(nullptr), _setter(nullptr)
    {
    }

    /**
     * @brief Sets the getter via a free function pointer.
     */
    MemberPropertyInitializer &Getter(TValue (*getter)(TOwner *))
    {
        this->_getter = getter;
        return *this;
    }

    /**
     * @brief Sets the setter via a free function pointer.
     */
    MemberPropertyInitializer &Setter(void (*setter)(TOwner *, _PropertySetterParamType<TValue>))
    {
        this->_setter = setter;
        return *this;
    }

    /**
     * @brief Sets the getter via a non-const member function pointer.
     */
    template <TValue (TOwner::*getter)()>
    MemberPropertyInitializer &Getter()
    {
        return this->Getter(
            [](TOwner *owner) -> TValue {
                return (owner->*getter)();
            });
    }

    /**
     * @brief Sets the getter via a const member function pointer.
     */
    template <TValue (TOwner::*getter)() const>
    MemberPropertyInitializer &Getter()
    {
        return this->Getter(
            [](TOwner *owner) -> TValue {
                return (owner->*getter)();
            });
    }

    /**
     * @brief Sets the setter via a non-const member function pointer.
     */
    template <void (TOwner::*setter)(_PropertySetterParamType<TValue>)>
    MemberPropertyInitializer &Setter()
    {
        return this->Setter(
            [](TOwner *owner, _PropertySetterParamType<TValue> value) {
                (owner->*setter)(value);
            });
    }

    /**
     * @brief Sets the setter via a const member function pointer.
     */
    template <void (TOwner::*setter)(_PropertySetterParamType<TValue>) const>
    MemberPropertyInitializer &Setter()
    {
        return this->Setter(
            [](TOwner *owner, _PropertySetterParamType<TValue> value) {
                (owner->*setter)(value);
            });
    }

    /**
     * @brief Sets the getter to a direct field access.
     */
    template <TValue TOwner::*field>
    MemberPropertyInitializer &Getter()
    {
        return this->Getter(
            [](TOwner *owner) -> TValue {
                return owner->*field;
            });
    }

    /**
     * @brief Sets the setter to a direct field assignment.
     */
    template <TValue TOwner::*field>
    MemberPropertyInitializer &Setter()
    {
        return this->Setter(
            [](TOwner *owner, _PropertySetterParamType<TValue> value) {
                owner->*field = value;
            });
    }
};

/**
 * @brief Initializer for static (non-member) properties.
 */
template <typename TValue>
class StaticPropertyInitializer
{
    friend class Property<TValue>;
    friend class ReadOnlyProperty<TValue>;
    friend class WriteOnlyProperty<TValue>;

private:
    /**
     * @brief Static getter function pointer.
     */
    TValue (*_getter)();

    /**
     * @brief Static setter function pointer.
     */
    void (*_setter)(_PropertySetterParamType<TValue>);

public:
    /**
     * @brief Default constructor.
     */
    StaticPropertyInitializer()
        : _getter(nullptr), _setter(nullptr)
    {
    }

    /**
     * @brief Sets the static getter function.
     */
    StaticPropertyInitializer &Getter(TValue (*getter)())
    {
        this->_getter = getter;
        return *this;
    }

    /**
     * @brief Sets the static setter function.
     */
    StaticPropertyInitializer &Setter(void (*setter)(_PropertySetterParamType<TValue>))
    {
        this->_setter = setter;
        return *this;
    }
};

/*================================================================================*/

/**
 * @brief Base class template for properties.
 */
template <typename T, typename TDerived>
class PropertyBase
{
public:
    using TValue       = T;
    using TSetterParam = _PropertySetterParamType<T>;

    // /**
    //  * @brief Gets the property value (implemented by derived class).
    //  */
    // T GetterImpl() const;

    // /**
    //  * @brief Sets the property value (implemented by derived class).
    //  */
    // void SetterImpl(TSetterParam value) const;

    /**
     * @brief Returns a FieldsAccessor for member access via operator->.
     */
    FieldsAccessor<T> AccessFields() const
    {
        return FieldsAccessor<T>(this->Get());
    }

    /**
     * @brief Gets the property value.
     */
    T Get() const
    {
        return static_cast<const TDerived *>(this)->GetterImpl();
    }

    /**
     * @brief Sets the property value.
     */
    void Set(TSetterParam value) const
    {
        static_cast<const TDerived *>(this)->SetterImpl(value);
    }

    /**
     * @brief Arrow operator for field access.
     */
    auto operator->() const
    {
        return static_cast<const TDerived *>(this)->AccessFields();
    }

    /**
     * @brief Implicit conversion to T.
     */
    operator T() const
    {
        return this->Get();
    }

    /**
     * @brief Implicit conversion to U (for non-arithmetic, convertible types).
     */
    template <
        typename U = T,
        typename   = typename std::enable_if<!std::is_arithmetic<T>::value && std::is_convertible<T, U>::value, U>::type>
    operator U() const
    {
        return static_cast<U>(this->Get());
    }

    /**
     * @brief Explicit conversion to U (for non-arithmetic, non-convertible but explicitly-convertible types).
     */
    template <
        typename U = T,
        typename   = typename std::enable_if<!std::is_arithmetic<T>::value && !std::is_convertible<T, U>::value, U>::type,
        typename   = typename std::enable_if<!std::is_arithmetic<T>::value && _IsExplicitlyConvertable<T, U>::value, U>::type>
    explicit operator U() const
    {
        return static_cast<U>(this->Get());
    }

    /**
     * @brief Assigns a value to the property.
     */
    TDerived &operator=(TSetterParam value)
    {
        this->Set(value);
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Assigns a value to the property (const overload).
     */
    const TDerived &operator=(TSetterParam value) const
    {
        this->Set(value);
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Assigns from another property.
     */
    TDerived &operator=(const PropertyBase &prop)
    {
        this->Set(prop.Get());
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Assigns from another property (const overload).
     */
    const TDerived &operator=(const PropertyBase &prop) const
    {
        this->Set(prop.Get());
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound add assignment.
     */
    template <typename U>
    auto operator+=(U &&value)
        -> typename std::enable_if<_AddOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() + std::forward<U>(value));
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound add assignment (const overload).
     */
    template <typename U>
    auto operator+=(U &&value) const
        -> typename std::enable_if<_AddOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() + std::forward<U>(value));
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound add assignment from another property.
     */
    template <typename D, typename U>
    auto operator+=(const PropertyBase<U, D> &prop)
        -> typename std::enable_if<_AddOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() + prop.Get());
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound add assignment from another property (const overload).
     */
    template <typename D, typename U>
    auto operator+=(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_AddOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() + prop.Get());
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound subtract assignment.
     */
    template <typename U>
    auto operator-=(U &&value)
        -> typename std::enable_if<_SubOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() - std::forward<U>(value));
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound subtract assignment (const overload).
     */
    template <typename U>
    auto operator-=(U &&value) const
        -> typename std::enable_if<_SubOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() - std::forward<U>(value));
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound subtract assignment from another property.
     */
    template <typename D, typename U>
    auto operator-=(const PropertyBase<U, D> &prop)
        -> typename std::enable_if<_SubOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() - prop.Get());
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound subtract assignment from another property (const overload).
     */
    template <typename D, typename U>
    auto operator-=(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_SubOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() - prop.Get());
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound multiply assignment.
     */
    template <typename U>
    auto operator*=(U &&value)
        -> typename std::enable_if<_MulOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() * std::forward<U>(value));
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound multiply assignment (const overload).
     */
    template <typename U>
    auto operator*=(U &&value) const
        -> typename std::enable_if<_MulOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() * std::forward<U>(value));
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound multiply assignment from another property.
     */
    template <typename D, typename U>
    auto operator*=(const PropertyBase<U, D> &prop)
        -> typename std::enable_if<_MulOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() * prop.Get());
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound multiply assignment from another property (const overload).
     */
    template <typename D, typename U>
    auto operator*=(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_MulOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() * prop.Get());
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound divide assignment.
     */
    template <typename U>
    auto operator/=(U &&value)
        -> typename std::enable_if<_DivOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() / std::forward<U>(value));
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound divide assignment (const overload).
     */
    template <typename U>
    auto operator/=(U &&value) const
        -> typename std::enable_if<_DivOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() / std::forward<U>(value));
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound divide assignment from another property.
     */
    template <typename D, typename U>
    auto operator/=(const PropertyBase<U, D> &prop)
        -> typename std::enable_if<_DivOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() / prop.Get());
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound divide assignment from another property (const overload).
     */
    template <typename D, typename U>
    auto operator/=(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_DivOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() / prop.Get());
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Pre-increment.
     */
    template <typename U = T>
    auto operator++()
        -> typename std::enable_if<_AddOperationHelper<U, int>::value, TDerived &>::type
    {
        this->Set(this->Get() + 1);
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Pre-increment (const overload).
     */
    template <typename U = T>
    auto operator++() const
        -> typename std::enable_if<_AddOperationHelper<U, int>::value, const TDerived &>::type
    {
        this->Set(this->Get() + 1);
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Pre-decrement.
     */
    template <typename U = T>
    auto operator--()
        -> typename std::enable_if<_SubOperationHelper<U, int>::value, TDerived &>::type
    {
        this->Set(this->Get() - 1);
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Pre-decrement (const overload).
     */
    template <typename U = T>
    auto operator--() const
        -> typename std::enable_if<_SubOperationHelper<U, int>::value, const TDerived &>::type
    {
        this->Set(this->Get() - 1);
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Post-increment.
     */
    template <typename U = T>
    auto operator++(int) const
        -> typename std::enable_if<_AddOperationHelper<U, int>::value, T>::type
    {
        T oldval = this->Get();
        this->Set(oldval + 1);
        return oldval;
    }

    /**
     * @brief Post-decrement.
     */
    template <typename U = T>
    auto operator--(int) const
        -> typename std::enable_if<_SubOperationHelper<U, int>::value, T>::type
    {
        T oldval = this->Get();
        this->Set(oldval - 1);
        return oldval;
    }

    /**
     * @brief Compound bitwise AND assignment.
     */
    template <typename U>
    auto operator&=(U &&value)
        -> typename std::enable_if<_BitAndOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() & std::forward<U>(value));
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound bitwise AND assignment (const overload).
     */
    template <typename U>
    auto operator&=(U &&value) const
        -> typename std::enable_if<_BitAndOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() & std::forward<U>(value));
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound bitwise AND assignment from another property.
     */
    template <typename D, typename U>
    auto operator&=(const PropertyBase<U, D> &prop)
        -> typename std::enable_if<_BitAndOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() & prop.Get());
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound bitwise AND assignment from another property (const overload).
     */
    template <typename D, typename U>
    auto operator&=(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_BitAndOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() & prop.Get());
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound bitwise OR assignment.
     */
    template <typename U>
    auto operator|=(U &&value)
        -> typename std::enable_if<_BitOrOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() | std::forward<U>(value));
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound bitwise OR assignment (const overload).
     */
    template <typename U>
    auto operator|=(U &&value) const
        -> typename std::enable_if<_BitOrOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() | std::forward<U>(value));
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound bitwise OR assignment from another property.
     */
    template <typename D, typename U>
    auto operator|=(const PropertyBase<U, D> &prop)
        -> typename std::enable_if<_BitOrOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() | prop.Get());
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound bitwise OR assignment from another property (const overload).
     */
    template <typename D, typename U>
    auto operator|=(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_BitOrOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() | prop.Get());
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound bitwise XOR assignment.
     */
    template <typename U>
    auto operator^=(U &&value)
        -> typename std::enable_if<_BitXorOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() ^ std::forward<U>(value));
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound bitwise XOR assignment (const overload).
     */
    template <typename U>
    auto operator^=(U &&value) const
        -> typename std::enable_if<_BitXorOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() ^ std::forward<U>(value));
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound bitwise XOR assignment from another property.
     */
    template <typename D, typename U>
    auto operator^=(const PropertyBase<U, D> &prop)
        -> typename std::enable_if<_BitXorOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() ^ prop.Get());
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound bitwise XOR assignment from another property (const overload).
     */
    template <typename D, typename U>
    auto operator^=(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_BitXorOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() ^ prop.Get());
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound left-shift assignment.
     */
    template <typename U>
    auto operator<<=(U &&value)
        -> typename std::enable_if<_ShlOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() << std::forward<U>(value));
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound left-shift assignment (const overload).
     */
    template <typename U>
    auto operator<<=(U &&value) const
        -> typename std::enable_if<_ShlOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() << std::forward<U>(value));
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound left-shift assignment from another property.
     */
    template <typename D, typename U>
    auto operator<<=(const PropertyBase<U, D> &prop)
        -> typename std::enable_if<_ShlOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() << prop.Get());
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound left-shift assignment from another property (const overload).
     */
    template <typename D, typename U>
    auto operator<<=(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_ShlOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() << prop.Get());
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound right-shift assignment.
     */
    template <typename U>
    auto operator>>=(U &&value)
        -> typename std::enable_if<_ShrOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() >> std::forward<U>(value));
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound right-shift assignment (const overload).
     */
    template <typename U>
    auto operator>>=(U &&value) const
        -> typename std::enable_if<_ShrOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() >> std::forward<U>(value));
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Compound right-shift assignment from another property.
     */
    template <typename D, typename U>
    auto operator>>=(const PropertyBase<U, D> &prop)
        -> typename std::enable_if<_ShrOperationHelper<T, U>::value, TDerived &>::type
    {
        this->Set(this->Get() >> prop.Get());
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief Compound right-shift assignment from another property (const overload).
     */
    template <typename D, typename U>
    auto operator>>=(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_ShrOperationHelper<T, U>::value, const TDerived &>::type
    {
        this->Set(this->Get() >> prop.Get());
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief Logical NOT.
     */
    template <typename U = T>
    auto operator!() const
        -> typename std::enable_if<_LogicNotOperationHelper<U>::value, typename _LogicNotOperationHelper<U>::type>::type
    {
        return !this->Get();
    }

    /**
     * @brief Bitwise NOT.
     */
    template <typename U = T>
    auto operator~() const
        -> typename std::enable_if<_BitNotOperationHelper<U>::value, typename _BitNotOperationHelper<U>::type>::type
    {
        return ~this->Get();
    }

    /**
     * @brief Dereference operator.
     * @note Enabled only when T::operator* returns a non-reference type, because
     *       Get() may return a temporary — returning a reference would dangle.
     */
    template <typename U = T>
    auto operator*() const
        -> typename std::enable_if<
            _DerefOperationHelper<U>::value &&
                !std::is_reference<typename _DerefOperationHelper<U>::type>::value,
            typename _DerefOperationHelper<U>::type>::type
    {
        return *this->Get();
    }

    /**
     * @brief Pointer dereference operator.
     * @note When T is a pointer, the pointer returned by Get() is a temporary,
     *       but the memory it points to is independent, so returning a reference is safe.
     */
    template <typename U = T>
    auto operator*() const
        -> typename std::enable_if<
            _DerefOperationHelper<U>::value && std::is_pointer<T>::value,
            typename _DerefOperationHelper<U>::type>::type
    {
        return *this->Get();
    }

    /**
     * @brief Unary plus.
     */
    template <typename U = T>
    auto operator+() const
        -> typename std::enable_if<_UnaryPlusOperationHelper<U>::value, typename _UnaryPlusOperationHelper<U>::type>::type
    {
        return +this->Get();
    }

    /**
     * @brief Unary minus.
     */
    template <typename U = T>
    auto operator-() const
        -> typename std::enable_if<_UnaryMinusOperationHelper<U>::value, typename _UnaryMinusOperationHelper<U>::type>::type
    {
        return -this->Get();
    }

    /**
     * @brief Addition.
     */
    template <typename U>
    auto operator+(U &&value) const
        -> typename std::enable_if<_AddOperationHelper<T, U>::value, typename _AddOperationHelper<T, U>::type>::type
    {
        return this->Get() + std::forward<U>(value);
    }

    /**
     * @brief Addition with another property.
     */
    template <typename D, typename U>
    auto operator+(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_AddOperationHelper<T, U>::value, typename _AddOperationHelper<T, U>::type>::type
    {
        return this->Get() + prop.Get();
    }

    /**
     * @brief Subtraction.
     */
    template <typename U>
    auto operator-(U &&value) const
        -> typename std::enable_if<_SubOperationHelper<T, U>::value, typename _SubOperationHelper<T, U>::type>::type
    {
        return this->Get() - std::forward<U>(value);
    }

    /**
     * @brief Subtraction with another property.
     */
    template <typename D, typename U>
    auto operator-(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_SubOperationHelper<T, U>::value, typename _SubOperationHelper<T, U>::type>::type
    {
        return this->Get() - prop.Get();
    }

    /**
     * @brief Multiplication.
     */
    template <typename U>
    auto operator*(U &&value) const
        -> typename std::enable_if<_MulOperationHelper<T, U>::value, typename _MulOperationHelper<T, U>::type>::type
    {
        return this->Get() * std::forward<U>(value);
    }

    /**
     * @brief Multiplication with another property.
     */
    template <typename D, typename U>
    auto operator*(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_MulOperationHelper<T, U>::value, typename _MulOperationHelper<T, U>::type>::type
    {
        return this->Get() * prop.Get();
    }

    /**
     * @brief Division.
     */
    template <typename U>
    auto operator/(U &&value) const
        -> typename std::enable_if<_DivOperationHelper<T, U>::value, typename _DivOperationHelper<T, U>::type>::type
    {
        return this->Get() / std::forward<U>(value);
    }

    /**
     * @brief Division with another property.
     */
    template <typename D, typename U>
    auto operator/(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_DivOperationHelper<T, U>::value, typename _DivOperationHelper<T, U>::type>::type
    {
        return this->Get() / prop.Get();
    }

    /**
     * @brief Modulo.
     */
    template <typename U>
    auto operator%(U &&value) const
        -> typename std::enable_if<_ModOperationHelper<T, U>::value, typename _ModOperationHelper<T, U>::type>::type
    {
        return this->Get() % std::forward<U>(value);
    }

    /**
     * @brief Modulo with another property.
     */
    template <typename D, typename U>
    auto operator%(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_ModOperationHelper<T, U>::value, typename _ModOperationHelper<T, U>::type>::type
    {
        return this->Get() % prop.Get();
    }

    /**
     * @brief Equality comparison.
     */
    template <typename U>
    auto operator==(U &&value) const
        -> typename std::enable_if<_EqOperationHelper<T, U>::value, typename _EqOperationHelper<T, U>::type>::type
    {
        return this->Get() == std::forward<U>(value);
    }

    /**
     * @brief Equality comparison with another property.
     */
    template <typename D, typename U>
    auto operator==(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_EqOperationHelper<T, U>::value, typename _EqOperationHelper<T, U>::type>::type
    {
        return this->Get() == prop.Get();
    }

    /**
     * @brief Inequality comparison.
     * @note Implemented as !(==) to avoid conflicts with C++20 synthesized !=.
     */
    template <typename U>
    auto operator!=(U &&value) const
        -> typename std::enable_if<_EqOperationHelper<T, U>::value, typename _EqOperationHelper<T, U>::type>::type
    {
        return !(*this == std::forward<U>(value));
    }

    /**
     * @brief Inequality comparison with another property.
     * @note Implemented as !(==) to avoid conflicts with C++20 synthesized !=.
     */
    template <typename D, typename U>
    auto operator!=(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_EqOperationHelper<T, U>::value, typename _EqOperationHelper<T, U>::type>::type
    {
        return !(*this == prop);
    }

    /**
     * @brief Less-than comparison.
     */
    template <typename U>
    auto operator<(U &&value) const
        -> typename std::enable_if<_LtOperationHelper<T, U>::value, typename _LtOperationHelper<T, U>::type>::type
    {
        return this->Get() < std::forward<U>(value);
    }

    /**
     * @brief Less-than comparison with another property.
     */
    template <typename D, typename U>
    auto operator<(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_LtOperationHelper<T, U>::value, typename _LtOperationHelper<T, U>::type>::type
    {
        return this->Get() < prop.Get();
    }

    /**
     * @brief Less-than-or-equal comparison.
     */
    template <typename U>
    auto operator<=(U &&value) const
        -> typename std::enable_if<_LeOperationHelper<T, U>::value, typename _LeOperationHelper<T, U>::type>::type
    {
        return this->Get() <= std::forward<U>(value);
    }

    /**
     * @brief Less-than-or-equal comparison with another property.
     */
    template <typename D, typename U>
    auto operator<=(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_LeOperationHelper<T, U>::value, typename _LeOperationHelper<T, U>::type>::type
    {
        return this->Get() <= prop.Get();
    }

    /**
     * @brief Greater-than comparison.
     */
    template <typename U>
    auto operator>(U &&value) const
        -> typename std::enable_if<_GtOperationHelper<T, U>::value, typename _GtOperationHelper<T, U>::type>::type
    {
        return this->Get() > std::forward<U>(value);
    }

    /**
     * @brief Greater-than comparison with another property.
     */
    template <typename D, typename U>
    auto operator>(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_GtOperationHelper<T, U>::value, typename _GtOperationHelper<T, U>::type>::type
    {
        return this->Get() > prop.Get();
    }

    /**
     * @brief Greater-than-or-equal comparison.
     */
    template <typename U>
    auto operator>=(U &&value) const
        -> typename std::enable_if<_GeOperationHelper<T, U>::value, typename _GeOperationHelper<T, U>::type>::type
    {
        return this->Get() >= std::forward<U>(value);
    }

    /**
     * @brief Greater-than-or-equal comparison with another property.
     */
    template <typename D, typename U>
    auto operator>=(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_GeOperationHelper<T, U>::value, typename _GeOperationHelper<T, U>::type>::type
    {
        return this->Get() >= prop.Get();
    }

    /**
     * @brief Bitwise AND.
     */
    template <typename U>
    auto operator&(U &&value) const
        -> typename std::enable_if<_BitAndOperationHelper<T, U>::value, typename _BitAndOperationHelper<T, U>::type>::type
    {
        return this->Get() & std::forward<U>(value);
    }

    /**
     * @brief Bitwise AND with another property.
     */
    template <typename D, typename U>
    auto operator&(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_BitAndOperationHelper<T, U>::value, typename _BitAndOperationHelper<T, U>::type>::type
    {
        return this->Get() & prop.Get();
    }

    /**
     * @brief Bitwise OR.
     */
    template <typename U>
    auto operator|(U &&value) const
        -> typename std::enable_if<_BitOrOperationHelper<T, U>::value, typename _BitOrOperationHelper<T, U>::type>::type
    {
        return this->Get() | std::forward<U>(value);
    }

    /**
     * @brief Bitwise OR with another property.
     */
    template <typename D, typename U>
    auto operator|(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_BitOrOperationHelper<T, U>::value, typename _BitOrOperationHelper<T, U>::type>::type
    {
        return this->Get() | prop.Get();
    }

    /**
     * @brief Bitwise XOR.
     */
    template <typename U>
    auto operator^(U &&value) const
        -> typename std::enable_if<_BitXorOperationHelper<T, U>::value, typename _BitXorOperationHelper<T, U>::type>::type
    {
        return this->Get() ^ std::forward<U>(value);
    }

    /**
     * @brief Bitwise XOR with another property.
     */
    template <typename D, typename U>
    auto operator^(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_BitXorOperationHelper<T, U>::value, typename _BitXorOperationHelper<T, U>::type>::type
    {
        return this->Get() ^ prop.Get();
    }

    /**
     * @brief Left shift.
     */
    template <typename U>
    auto operator<<(U &&value) const
        -> typename std::enable_if<_ShlOperationHelper<T, U>::value, typename _ShlOperationHelper<T, U>::type>::type
    {
        return this->Get() << std::forward<U>(value);
    }

    /**
     * @brief Left shift with another property.
     */
    template <typename D, typename U>
    auto operator<<(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_ShlOperationHelper<T, U>::value, typename _ShlOperationHelper<T, U>::type>::type
    {
        return this->Get() << prop.Get();
    }

    /**
     * @brief Right shift.
     */
    template <typename U>
    auto operator>>(U &&value) const
        -> typename std::enable_if<_ShrOperationHelper<T, U>::value, typename _ShrOperationHelper<T, U>::type>::type
    {
        return this->Get() >> std::forward<U>(value);
    }

    /**
     * @brief Right shift with another property.
     */
    template <typename D, typename U>
    auto operator>>(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_ShrOperationHelper<T, U>::value, typename _ShrOperationHelper<T, U>::type>::type
    {
        return this->Get() >> prop.Get();
    }

    /**
     * @brief Logical AND.
     */
    template <typename U>
    auto operator&&(U &&value) const
        -> typename std::enable_if<_LogicAndOperationHelper<T, U>::value, typename _LogicAndOperationHelper<T, U>::type>::type
    {
        return this->Get() && std::forward<U>(value);
    }

    /**
     * @brief Logical AND with another property.
     */
    template <typename D, typename U>
    auto operator&&(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_LogicAndOperationHelper<T, U>::value, typename _LogicAndOperationHelper<T, U>::type>::type
    {
        return this->Get() && prop.Get();
    }

    /**
     * @brief Logical OR.
     */
    template <typename U>
    auto operator||(U &&value) const
        -> typename std::enable_if<_LogicOrOperationHelper<T, U>::value, typename _LogicOrOperationHelper<T, U>::type>::type
    {
        return this->Get() || std::forward<U>(value);
    }

    /**
     * @brief Logical OR with another property.
     */
    template <typename D, typename U>
    auto operator||(const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<_LogicOrOperationHelper<T, U>::value, typename _LogicOrOperationHelper<T, U>::type>::type
    {
        return this->Get() || prop.Get();
    }

    /**
     * @brief Subscript operator.
     * @note Enabled only when T::operator[] returns a non-reference type, because
     *       Get() may return a temporary — returning a reference would dangle.
     */
    template <typename U>
    auto operator[](U &&value) const
        -> typename std::enable_if<
            _BracketOperationHelper<T, U>::value &&
                !std::is_reference<typename _BracketOperationHelper<T, U>::type>::value,
            typename _BracketOperationHelper<T, U>::type>::type
    {
        return this->Get()[std::forward<U>(value)];
    }

    /**
     * @brief Subscript operator with another property as index.
     * @note Enabled only when T::operator[] returns a non-reference type.
     */
    template <typename D, typename U>
    auto operator[](const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<
            _BracketOperationHelper<T, U>::value &&
                !std::is_reference<typename _BracketOperationHelper<T, U>::type>::value,
            typename _BracketOperationHelper<T, U>::type>::type
    {
        return this->Get()[prop.Get()];
    }

    /**
     * @brief Pointer subscript operator.
     * @note When T is a pointer, the pointer returned by Get() is a temporary,
     *       but the memory it points to is independent, so returning a reference is safe.
     */
    template <typename U>
    auto operator[](U &&value) const
        -> typename std::enable_if<
            _BracketOperationHelper<T, U>::value && std::is_pointer<T>::value,
            typename _BracketOperationHelper<T, U>::type>::type
    {
        return this->Get()[std::forward<U>(value)];
    }

    /**
     * @brief Pointer subscript operator with another property as index.
     * @note When T is a pointer, returning a reference is safe.
     */
    template <typename D, typename U>
    auto operator[](const PropertyBase<U, D> &prop) const
        -> typename std::enable_if<
            _BracketOperationHelper<T, U>::value && std::is_pointer<T>::value,
            typename _BracketOperationHelper<T, U>::type>::type
    {
        return this->Get()[prop.Get()];
    }

protected:
    /**
     * @brief Generic function pointer type used for storage.
     * @note Converting between function pointer types via reinterpret_cast and back
     *       is well-defined in C++.  Using a uniform type avoids the
     *       conditionally-supported conversion between function pointers and void*.
     */
    using TFuncPtr = void (*)();

    /**
     * @brief Sentinel offset value indicating a static property.
     */
    static constexpr std::ptrdiff_t _STATICOFFSET =
        (std::numeric_limits<std::ptrdiff_t>::max)();

    /**
     * @brief Byte offset from this property to its owning object.
     */
    std::ptrdiff_t _offset{_STATICOFFSET};

    /**
     * @brief Returns true if this is a static property.
     */
    bool IsStatic() const noexcept
    {
        return _offset == _STATICOFFSET;
    }

    /**
     * @brief Sets the owning object; nullptr makes this a static property.
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
     * @brief Returns the owning object, or nullptr if this is a static property.
     */
    void *GetOwner() const noexcept
    {
        if (this->IsStatic()) {
            return nullptr;
        } else {
            return const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(this)) + _offset;
        }
    }

public:
    /**
     * @brief Creates a MemberPropertyInitializer for binding a member property.
     */
    template <typename TOwner>
    static auto Init(TOwner *owner)
        -> MemberPropertyInitializer<TOwner, T>
    {
        return MemberPropertyInitializer<TOwner, T>(owner);
    }

    /**
     * @brief Creates a StaticPropertyInitializer for binding a static property.
     */
    static auto Init()
        -> StaticPropertyInitializer<T>
    {
        return StaticPropertyInitializer<T>();
    }
};

/*================================================================================*/

/**
 * @brief Addition (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator+(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _AddOperationHelper<T, U>::value, typename _AddOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) + right.Get();
}

/**
 * @brief Subtraction (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator-(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _SubOperationHelper<T, U>::value, typename _SubOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) - right.Get();
}

/**
 * @brief Multiplication (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator*(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _MulOperationHelper<T, U>::value, typename _MulOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) * right.Get();
}

/**
 * @brief Division (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator/(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _DivOperationHelper<T, U>::value, typename _DivOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) / right.Get();
}

/**
 * @brief Modulo (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator%(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _ModOperationHelper<T, U>::value, typename _ModOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) % right.Get();
}

/**
 * @brief Equality comparison (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator==(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _EqOperationHelper<T, U>::value, typename _EqOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) == right.Get();
}

/**
 * @brief Inequality comparison (non-property left operand).
 * @note Implemented as !(==) to avoid conflicts with C++20 synthesized !=.
 */
template <typename D, typename T, typename U>
auto operator!=(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _EqOperationHelper<T, U>::value, typename _EqOperationHelper<T, U>::type>::type
{
    return !(std::forward<T>(left) == right);
}

/**
 * @brief Less-than comparison (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator<(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _LtOperationHelper<T, U>::value, typename _LtOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) < right.Get();
}

/**
 * @brief Less-than-or-equal comparison (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator<=(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _LeOperationHelper<T, U>::value, typename _LeOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) <= right.Get();
}

/**
 * @brief Greater-than comparison (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator>(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _GtOperationHelper<T, U>::value, typename _GtOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) > right.Get();
}

/**
 * @brief Greater-than-or-equal comparison (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator>=(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _GeOperationHelper<T, U>::value, typename _GeOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) >= right.Get();
}

/**
 * @brief Bitwise AND (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator&(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _BitAndOperationHelper<T, U>::value, typename _BitAndOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) & right.Get();
}

/**
 * @brief Bitwise OR (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator|(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _BitOrOperationHelper<T, U>::value, typename _BitOrOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) | right.Get();
}

/**
 * @brief Bitwise XOR (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator^(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _BitXorOperationHelper<T, U>::value, typename _BitXorOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) ^ right.Get();
}

/**
 * @brief Left shift (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator<<(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _ShlOperationHelper<T, U>::value, typename _ShlOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) << right.Get();
}

/**
 * @brief Right shift (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator>>(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _ShrOperationHelper<T, U>::value, typename _ShrOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) >> right.Get();
}

/**
 * @brief Logical AND (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator&&(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _LogicAndOperationHelper<T, U>::value, typename _LogicAndOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) && right.Get();
}

/**
 * @brief Logical OR (non-property left operand).
 */
template <typename D, typename T, typename U>
auto operator||(T &&left, const PropertyBase<U, D> &right)
    -> typename std::enable_if<!_IsProperty<T>::value && _LogicOrOperationHelper<T, U>::value, typename _LogicOrOperationHelper<T, U>::type>::type
{
    return std::forward<T>(left) || right.Get();
}

/*================================================================================*/

/**
 * @brief Read-write property.
 */
template <typename T>
class Property : public PropertyBase<T, Property<T>>
{
public:
    using TBase         = PropertyBase<T, Property<T>>;
    using TValue        = typename TBase::TValue;
    using TSetterParam  = typename TBase::TSetterParam;
    using TFuncPtr      = typename TBase::TFuncPtr;
    using TGetter       = T (*)(void *);
    using TSetter       = void (*)(void *, TSetterParam);
    using TStaticGetter = T (*)();
    using TStaticSetter = void (*)(TSetterParam);

private:
    /**
     * @brief Getter function pointer.
     */
    TFuncPtr _getter;

    /**
     * @brief Setter function pointer.
     */
    TFuncPtr _setter;

public:
    /**
     * @brief Inherit assignment operators from the base class.
     */
    using TBase::operator=;

    /**
     * @brief Constructs a member property from an initializer.
     */
    template <typename TOwner>
    explicit Property(const MemberPropertyInitializer<TOwner, T> &initializer)
    {
        assert(initializer._owner != nullptr);
        assert(initializer._getter != nullptr);
        assert(initializer._setter != nullptr);

        this->SetOwner(initializer._owner);
        this->_getter = reinterpret_cast<TFuncPtr>(initializer._getter);
        this->_setter = reinterpret_cast<TFuncPtr>(initializer._setter);
    }

    /**
     * @brief Constructs a static property from an initializer.
     */
    explicit Property(const StaticPropertyInitializer<T> &initializer)
    {
        assert(initializer._getter != nullptr);
        assert(initializer._setter != nullptr);

        this->SetOwner(nullptr);
        this->_getter = reinterpret_cast<TFuncPtr>(initializer._getter);
        this->_setter = reinterpret_cast<TFuncPtr>(initializer._setter);
    }

    /**
     * @brief Gets the property value.
     */
    T GetterImpl() const
    {
        if (this->IsStatic()) {
            return reinterpret_cast<TStaticGetter>(this->_getter)();
        } else {
            return reinterpret_cast<TGetter>(this->_getter)(this->GetOwner());
        }
    }

    /**
     * @brief Sets the property value.
     */
    void SetterImpl(TSetterParam value) const
    {
        if (this->IsStatic()) {
            reinterpret_cast<TStaticSetter>(this->_setter)(value);
        } else {
            reinterpret_cast<TSetter>(this->_setter)(this->GetOwner(), value);
        }
    }
};

/**
 * @brief Read-only property (no setter).
 */
template <typename T>
class ReadOnlyProperty : public PropertyBase<T, ReadOnlyProperty<T>>
{
public:
    using TBase         = PropertyBase<T, ReadOnlyProperty<T>>;
    using TValue        = typename TBase::TValue;
    using TSetterParam  = typename TBase::TSetterParam;
    using TFuncPtr      = typename TBase::TFuncPtr;
    using TGetter       = T (*)(void *);
    using TStaticGetter = T (*)();

private:
    /**
     * @brief Getter function pointer.
     */
    TFuncPtr _getter;

public:
    /**
     * @brief Constructs a member property from an initializer.
     */
    template <typename TOwner>
    explicit ReadOnlyProperty(const MemberPropertyInitializer<TOwner, T> &initializer)
    {
        assert(initializer._owner != nullptr);
        assert(initializer._getter != nullptr);

        this->SetOwner(initializer._owner);
        this->_getter = reinterpret_cast<TFuncPtr>(initializer._getter);
    }

    /**
     * @brief Constructs a static property from an initializer.
     */
    explicit ReadOnlyProperty(const StaticPropertyInitializer<T> &initializer)
    {
        assert(initializer._getter != nullptr);

        this->SetOwner(nullptr);
        this->_getter = reinterpret_cast<TFuncPtr>(initializer._getter);
    }

    /**
     * @brief Gets the property value.
     */
    T GetterImpl() const
    {
        if (this->IsStatic()) {
            return reinterpret_cast<TStaticGetter>(this->_getter)();
        } else {
            return reinterpret_cast<TGetter>(this->_getter)(this->GetOwner());
        }
    }
};

/**
 * @brief Write-only property (no getter).
 */
template <typename T>
class WriteOnlyProperty : public PropertyBase<T, WriteOnlyProperty<T>>
{
public:
    using TBase         = PropertyBase<T, WriteOnlyProperty<T>>;
    using TValue        = typename TBase::TValue;
    using TSetterParam  = typename TBase::TSetterParam;
    using TFuncPtr      = typename TBase::TFuncPtr;
    using TSetter       = void (*)(void *, TSetterParam);
    using TStaticSetter = void (*)(TSetterParam);

private:
    /**
     * @brief Setter function pointer.
     */
    TFuncPtr _setter;

public:
    /**
     * @brief Inherit assignment operators from the base class.
     */
    using TBase::operator=;

    /**
     * @brief Constructs a member property from an initializer.
     */
    template <typename TOwner>
    explicit WriteOnlyProperty(const MemberPropertyInitializer<TOwner, T> &initializer)
    {
        assert(initializer._owner != nullptr);
        assert(initializer._setter != nullptr);

        this->SetOwner(initializer._owner);
        this->_setter = reinterpret_cast<TFuncPtr>(initializer._setter);
    }

    /**
     * @brief Constructs a static property from an initializer.
     */
    explicit WriteOnlyProperty(const StaticPropertyInitializer<T> &initializer)
    {
        assert(initializer._setter != nullptr);

        this->SetOwner(nullptr);
        this->_setter = reinterpret_cast<TFuncPtr>(initializer._setter);
    }

    /**
     * @brief Sets the property value.
     */
    void SetterImpl(TSetterParam value) const
    {
        if (this->IsStatic()) {
            reinterpret_cast<TStaticSetter>(this->_setter)(value);
        } else {
            reinterpret_cast<TSetter>(this->_setter)(this->GetOwner(), value);
        }
    }
};

#endif // PROPERTY_H_INCLUDED
