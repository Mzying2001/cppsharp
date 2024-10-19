#ifndef _PROP_H_
#define _PROP_H_

#include <cstdint>
#include <type_traits>
#include <utility>

/**
 * @brief 伪指针，用于实现使用operator->取属性字段
 */
template <typename T>
struct FakePtr
{
    /**
     * @brief 伪指针所维护的值
     */
    T value;

    /**
     * @brief 构造伪指针
     */
    template <typename... Args>
    FakePtr(Args &&...args)
        : value(std::forward<Args>(args)...)
    {
    }

    /**
     * @brief 取字段
     */
    T *operator->()
    {
        return &value;
    }

    /**
     * @brief 取字段
     */
    const T *operator->() const
    {
        return &value;
    }
};

/*================================================================================*/

/**
 * @brief 属性基类模板
 */
template <typename T, typename TDerived>
struct PropertyBase
{
    // /**
    //  * @brief 获取属性值，由子类实现
    //  */
    // T GetterImpl() const;

    // /**
    //  * @brief 设置属性值，由子类实现
    //  */
    // void SetterImpl(const T &value) const;

    // /**
    //  * @brief 获取字段，可由子类重写
    //  */
    // FakePtr<T> ListFieldsImpl() const
    // {
    //     return FakePtr<T>(this->Get());
    // }

    /**
     * @brief 获取字段，可由子类重写
     */
    template <typename U = T>
    typename std::enable_if<!std::is_pointer<U>::value, FakePtr<T>>::type ListFieldsImpl() const
    {
        return FakePtr<T>(this->Get());
    }

    /**
     * @brief 获取字段，可由子类重写
     */
    template <typename U = T>
    typename std::enable_if<std::is_pointer<U>::value, T>::type ListFieldsImpl() const
    {
        return this->Get();
    }

    /**
     * @brief 获取属性值
     */
    T Get() const
    {
        return static_cast<const TDerived *>(this)->GetterImpl();
    }

    /**
     * @brief 设置属性值
     */
    void Set(const T &value) const
    {
        static_cast<const TDerived *>(this)->SetterImpl(value);
    }

    /**
     * @brief 取属性字段
     */
    auto operator->() const
    {
        return static_cast<const TDerived *>(this)->ListFieldsImpl();
    }

    /**
     * @brief 获取属性值
     */
    operator T() const
    {
        return this->Get();
    }

    /**
     * @brief 设置属性值
     */
    TDerived &operator=(const T &value)
    {
        this->Set(value);
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief 设置属性值
     */
    const TDerived &operator=(const T &value) const
    {
        this->Set(value);
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief 设置属性值
     */
    TDerived &operator=(const PropertyBase &prop)
    {
        this->Set(prop.Get());
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief 设置属性值
     */
    const TDerived &operator=(const PropertyBase &prop) const
    {
        this->Set(prop.Get());
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief 加赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_arithmetic<U>::value, TDerived &>::type operator+=(T value)
    {
        this->Set(this->Get() + value);
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief 加赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_arithmetic<U>::value, const TDerived &>::type operator+=(T value) const
    {
        this->Set(this->Get() + value);
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief 减赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_arithmetic<U>::value, TDerived &>::type operator-=(T value)
    {
        this->Set(this->Get() - value);
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief 减赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_arithmetic<U>::value, const TDerived &>::type operator-=(T value) const
    {
        this->Set(this->Get() - value);
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief 乘赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_arithmetic<U>::value, TDerived &>::type operator*=(T value)
    {
        this->Set(this->Get() * value);
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief 乘赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_arithmetic<U>::value, const TDerived &>::type operator*=(T value) const
    {
        this->Set(this->Get() * value);
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief 除赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_arithmetic<U>::value, TDerived &>::type operator/=(T value)
    {
        this->Set(this->Get() / value);
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief 除赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_arithmetic<U>::value, const TDerived &>::type operator/=(T value) const
    {
        this->Set(this->Get() / value);
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief 前置自增运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_arithmetic<U>::value, TDerived &>::type operator++()
    {
        this->Set(this->Get() + 1);
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief 前置自增运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_arithmetic<U>::value, const TDerived &>::type operator++() const
    {
        this->Set(this->Get() + 1);
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief 前置自减运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_arithmetic<U>::value, TDerived &>::type operator--()
    {
        this->Set(this->Get() - 1);
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief 前置自减运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_arithmetic<U>::value, const TDerived &>::type operator--() const
    {
        this->Set(this->Get() - 1);
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief 后置自增运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_arithmetic<U>::value, T>::type operator++(int) const
    {
        T oldval = this->Get();
        this->Set(oldval + 1);
        return oldval;
    }

    /**
     * @brief 后置自减运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_arithmetic<U>::value, T>::type operator--(int) const
    {
        T oldval = this->Get();
        this->Set(oldval - 1);
        return oldval;
    }

    /**
     * @brief 按位与赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_integral<U>::value, TDerived &>::type operator&=(T value)
    {
        this->Set(this->Get() & value);
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief 按位与赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_integral<U>::value, const TDerived &>::type operator&=(T value) const
    {
        this->Set(this->Get() & value);
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief 按位或赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_integral<U>::value, TDerived &>::type operator|=(T value)
    {
        this->Set(this->Get() | value);
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief 按位或赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_integral<U>::value, const TDerived &>::type operator|=(T value) const
    {
        this->Set(this->Get() | value);
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief 按位异或赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_integral<U>::value, TDerived &>::type operator^=(T value)
    {
        this->Set(this->Get() ^ value);
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief 按位异或赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_integral<U>::value, const TDerived &>::type operator^=(T value) const
    {
        this->Set(this->Get() ^ value);
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief 左移赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_integral<U>::value, TDerived &>::type operator<<=(T value)
    {
        this->Set(this->Get() << value);
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief 左移赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_integral<U>::value, const TDerived &>::type operator<<=(T value) const
    {
        this->Set(this->Get() << value);
        return *static_cast<const TDerived *>(this);
    }

    /**
     * @brief 右移赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_integral<U>::value, TDerived &>::type operator>>=(T value)
    {
        this->Set(this->Get() >> value);
        return *static_cast<TDerived *>(this);
    }

    /**
     * @brief 右移赋值运算
     */
    template <typename U = T>
    typename std::enable_if<std::is_integral<U>::value, const TDerived &>::type operator>>=(T value) const
    {
        this->Set(this->Get() >> value);
        return *static_cast<const TDerived *>(this);
    }
};

/*================================================================================*/

/**
 * @brief 成员属性Getter函数指针类型
 */
template <typename T, typename TOwner>
using PropertyGetter = T (*)(TOwner &);

/**
 * @brief 成员属性Setter函数指针类型
 */
template <typename T, typename TOwner>
using PropertySetter = void (*)(TOwner &, const T &);

/**
 * @brief 成员属性
 */
template <typename T, typename TOwner, typename TDerived>
class MemberProperty : public PropertyBase<T, TDerived>
{
private:
    const off_t _offOwner; // 属性与所有者对象的偏移值

public:
    /**
     * @brief 构造函数，指定所有者对象
     */
    explicit MemberProperty(TOwner &owner) noexcept
        : _offOwner(reinterpret_cast<uint8_t *>(&owner) - reinterpret_cast<uint8_t *>(this)) {}

    /**
     * @brief 继承operator=
     */
    using PropertyBase<T, TDerived>::operator=;

protected:
    /**
     * @brief 获取属性的所有者对象
     */
    TOwner &GetOwner() const noexcept
    {
        return *reinterpret_cast<TOwner *>(const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(this)) + this->_offOwner);
    }
};

/**
 * @brief 属性
 */
template <typename T, typename TOwner>
class Property : public MemberProperty<T, TOwner, Property<T, TOwner>>
{
public:
    using TBase = MemberProperty<T, TOwner, Property<T, TOwner>>;
    using FnGet = PropertyGetter<T, TOwner>;
    using FnSet = PropertySetter<T, TOwner>;
    using TBase::operator=;

private:
    const FnGet _getter;
    const FnSet _setter;

public:
    /**
     * @brief 构造属性
     */
    Property(TOwner &owner, FnGet getter, FnSet setter)
        : TBase(owner), _getter(getter), _setter(setter)
    {
    }

    /**
     * @brief 获取属性值
     */
    T GetterImpl() const
    {
        return this->_getter(this->GetOwner());
    }

    /**
     * @brief 设置属性值
     */
    void SetterImpl(const T &value) const
    {
        this->_setter(this->GetOwner(), value);
    }
};

/**
 * @brief 只读属性
 */
template <typename T, typename TOwner>
class ReadOnlyProperty : public MemberProperty<T, TOwner, ReadOnlyProperty<T, TOwner>>
{
public:
    using TBase = MemberProperty<T, TOwner, ReadOnlyProperty<T, TOwner>>;
    using FnGet = PropertyGetter<T, TOwner>;

private:
    const FnGet _getter;

public:
    /**
     * @brief 构造属性
     */
    ReadOnlyProperty(TOwner &owner, FnGet getter)
        : TBase(owner), _getter(getter)
    {
    }

    /**
     * @brief 获取属性值
     */
    T GetterImpl() const
    {
        return this->_getter(this->GetOwner());
    }
};

/**
 * @brief 只写属性
 */
template <typename T, typename TOwner>
class WriteOnlyProperty : public MemberProperty<T, TOwner, WriteOnlyProperty<T, TOwner>>
{
public:
    using TBase = MemberProperty<T, TOwner, WriteOnlyProperty<T, TOwner>>;
    using FnSet = PropertySetter<T, TOwner>;
    using TBase::operator=;

private:
    const FnSet _setter;

public:
    /**
     * @brief 构造属性
     */
    WriteOnlyProperty(TOwner &owner, FnSet setter)
        : TBase(owner), _setter(setter)
    {
    }

    /**
     * @brief 设置属性值
     */
    void SetterImpl(const T &value) const
    {
        this->_setter(this->GetOwner(), value);
    }
};

/*================================================================================*/

/**
 * @brief 静态属性Getter函数指针类型
 */
template <typename T>
using StaticPropertyGetter = T (*)();

/**
 * @brief 静态属性Setter函数指针类型
 */
template <typename T>
using StaticPropertySetter = void (*)(const T &);

/**
 * @brief 静态属性
 */
template <typename T>
class StaticProperty : public PropertyBase<T, StaticProperty<T>>
{
public:
    using TBase = PropertyBase<T, StaticProperty<T>>;
    using FnGet = StaticPropertyGetter<T>;
    using FnSet = StaticPropertySetter<T>;
    using TBase::operator=;

private:
    const FnGet _getter;
    const FnSet _setter;

public:
    /**
     * @brief 构造属性
     */
    StaticProperty(FnGet getter, FnSet setter)
        : _getter(getter), _setter(setter)
    {
    }

    /**
     * @brief 获取属性值
     */
    T GetterImpl() const
    {
        return this->_getter();
    }

    /**
     * @brief 设置属性值
     */
    void SetterImpl(const T &value) const
    {
        this->_setter(value);
    }
};

/**
 * @brief 静态只读属性
 */
template <typename T>
class StaticReadOnlyProperty : public PropertyBase<T, StaticReadOnlyProperty<T>>
{
public:
    using TBase = PropertyBase<T, StaticReadOnlyProperty<T>>;
    using FnGet = StaticPropertyGetter<T>;

private:
    const FnGet _getter;

public:
    /**
     * @brief 构造属性
     */
    StaticReadOnlyProperty(FnGet getter)
        : _getter(getter)
    {
    }

    /**
     * @brief 获取属性值
     */
    T GetterImpl() const
    {
        return this->_getter();
    }
};

/**
 * @brief 静态只写属性
 */
template <typename T>
class StaticWriteOnlyProperty : public PropertyBase<T, StaticWriteOnlyProperty<T>>
{
public:
    using TBase = PropertyBase<T, StaticWriteOnlyProperty<T>>;
    using FnSet = StaticPropertySetter<T>;
    using TBase::operator=;

private:
    const FnSet _setter;

public:
    /**
     * @brief 构造属性
     */
    StaticWriteOnlyProperty(FnSet setter)
        : _setter(setter)
    {
    }

    /**
     * @brief 设置属性值
     */
    void SetterImpl(const T &value) const
    {
        this->_setter(value);
    }
};

/*================================================================================*/

/**
 * @brief 属性类型辅助模板，用于防止宏直接拼接导致的const引用类型不正确的问题
 */
template <typename T>
struct PropertyTypeHelper
{
    using Type = T;
    using TypeConstRef = const T &;
};

/**
 * @brief 为当前类启用成员属性宏支持
 */
#define ENABLE_PROPERTY(TSELF) \
    using _TSelf = typename PropertyTypeHelper<TSELF>::Type;

/**
 * @brief 声明一个可读可写属性
 */
#define PROPERTY_RW(T, NAME) \
    const Property<typename PropertyTypeHelper<T>::Type, _TSelf> NAME

/**
 * @brief 声明一个只读属性
 */
#define PROPERTY_R(T, NAME) \
    const ReadOnlyProperty<typename PropertyTypeHelper<T>::Type, _TSelf> NAME

/**
 * @brief 声明一个只写属性
 */
#define PROPERTY_W(T, NAME) \
    const WriteOnlyProperty<typename PropertyTypeHelper<T>::Type, _TSelf> NAME

/**
 * @brief 同PROPERTY_RW，声明一个可读可写属性
 */
#define PROPERTY(T, NAME) \
    PROPERTY_RW(T, NAME)

/**
 * @brief 属性的Getter，self表示当前对象
 */
#define GETTER(T) \
    *this, [](_TSelf & self) -> typename PropertyTypeHelper<T>::Type

/**
 * @brief 属性的Setter，self表示当前对象，value表示要设置的值
 */
#define SETTER(T) \
    [](_TSelf & self, typename PropertyTypeHelper<T>::TypeConstRef value) -> void

/**
 * @brief 声明一个静态可读可写属性
 */
#define PROPERTY_RW_STATIC(T, NAME) \
    static const StaticProperty<typename PropertyTypeHelper<T>::Type> NAME

/**
 * @brief 声明一个静态只读属性
 */
#define PROPERTY_R_STATIC(T, NAME) \
    static const StaticReadOnlyProperty<typename PropertyTypeHelper<T>::Type> NAME

/**
 * @brief 声明一个静态只写属性
 */
#define PROPERTY_W_STATIC(T, NAME) \
    static const StaticWriteOnlyProperty<typename PropertyTypeHelper<T>::Type> NAME

/**
 * @brief 同PROPERTY_RW_STATIC，声明一个静态可读可写属性
 */
#define PROPERTY_STATIC(T, NAME) \
    PROPERTY_RW_STATIC(T, NAME)

/**
 * @brief 静态属性的Getter
 */
#define GETTER_STATIC(T) \
    []() -> typename PropertyTypeHelper<T>::Type

/**
 * @brief 静态属性的Setter，value表示要设置的值
 */
#define SETTER_STATIC(T) \
    [](typename PropertyTypeHelper<T>::TypeConstRef value) -> void

#endif // _PROP_H_
