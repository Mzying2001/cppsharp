# cppsharp

给 C++ 引入类似 C# 的委托（Delegate）和属性（Property）支持。

## [`delegate.h`](./include/delegate.h)

该头文件为 C++ 提供类似 C# 的委托机制。

### 示例

以下定义了一个 `Button` 类，使用委托定义一个 `Clicked` 事件，通过 `Click` 函数模拟一次按钮点击。`Action` 是 `Delegate<void(Args...)>` 的类型别名，表示无返回值的委托，此外还可以使用 `Func` 表示有返回值的委托，两者的模板参数与 C# 中的 `Action` 和 `Func` 保持一致。

```cpp
class Button
{
public:
    // 点击事件
    Action<Button *> Clicked;

    // 模拟点击按钮
    void Click()
    {
        if (Clicked) {
            Clicked(this);
        }
    }
};
```

测试代码如下：

```cpp
void ClickedHandler(Button *b)
{
    std::cout << "Button clicked!" << std::endl;
}

int main()
{
    Button btn;

    // 绑定事件处理函数
    btn.Clicked += ClickedHandler;

    // 支持多播，这里添加一个lambda处理函数
    // 委托支持任意可调用对象，包括函数指针、成员函数（通过Add方法）、lambda表达式等
    btn.Clicked += [](Button *b) {
        std::cout << "Lambda: Button clicked!" << std::endl;
    };

    // 模拟点击按钮，触发事件
    btn.Click();

    // 通过-=操作符移除事件处理函数
    btn.Clicked -= ClickedHandler;

    // 再次点击按钮
    btn.Click();

    return 0;
}
```

运行结果：

```plaintext
Button clicked!
Lambda: Button clicked!
Lambda: Button clicked!
```

## [`property.h`](./include/property.h)

该头文件为 C++ 提供类似 C# 的属性语法。

### 示例

以下定义了一个 `Person` 类，包含一个可读可写属性 `Age` 以及一个只读属性 `AgeStr`，代码演示了属性的声明和初始化。

```cpp
class Person
{
    // 属性Age维护的字段
    int _age = 1;

    // getter函数
    int getAge()
    {
        std::cout << "get Age" << std::endl;
        return _age;
    }

    // setter函数
    void setAge(int value)
    {
        std::cout << "set Age: " << value << std::endl;

        if (value >= 0) { // 对Age的值进行范围检查
            _age = value;
        } else {
            std::cout << "error: Age can not smaller than 0" << std::endl;
        }
    }

public:
    // 属性Age，通过成员函数初始化
    Property<int> Age{
        Property<int>::Init(this)
            .Getter<&Person::getAge>()
            .Setter<&Person::setAge>()};

    // 只读属性AgeStr，表示Age的字符串
    // 这里使用lambda表达式初始化，第一个参数即属性所有者对象指针，若有setter同理
    ReadOnlyProperty<std::string> AgeStr{
        Property<std::string>::Init(this)
            .Getter([](Person *self) {
                std::cout << "get AgeStr" << std::endl;
                return std::to_string(self->_age);
            })};
};
```

测试代码如下：

```cpp
int main()
{
    Person p;
    std::cout << p.Age << std::endl; // get Age

    p.Age = -1;                      // error: Age can not smaller than 0
    std::cout << p.Age << std::endl; // 仍然为1

    p.Age = 10;
    std::cout << p.AgeStr << std::endl; // 10

    Person p2 = p;                                // 对象可以正常拷贝
    p2.Age++;                                     // 先get后set
    std::cout << p2.AgeStr->c_str() << std::endl; // 使用->可以访问属性成员

    return 0;
}
```

运行结果：

```plaintext
get Age
1
set Age: -1
error: Age can not smaller than 1
get Age
1
set Age: 10
get AgeStr
10
get Age
set Age: 11
get AgeStr
11
```
