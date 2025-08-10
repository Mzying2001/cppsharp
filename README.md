# cppsharp

给 C++ 引入类似 C# 的属性（Property）和委托（Delegate）支持。

## `delegate.h`

该头文件为 C++ 提供类似 C# 的委托机制。

### 示例

以下定义了一个 `Button` 类，使用委托定义一个 `Clicked` 事件，通过 `Click` 函数模拟一次按钮点击。`Action` 是 `Delegate<void(Args...)>` 的类型别名，表示无返回值的委托；此外还可以使用 `Func` 表示有返回值的委托，两者的模板参数与 C# 中的 `Action` 和 `Func` 保持一致。

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

## `property.h`

该头文件为 C++ 提供类似 C# 的属性语法。

### 示例

以下定义了一个 `Person` 类，包含一个可读可写属性 `Age` 以及一个只读属性 `AgeStr`，代码演示了属性的声明和初始化。

```cpp
class Person
{
    // 属性Age维护的字段
    int _age;

public:
    // 声明一个属性Age
    Property<int> Age;

    // 声明AgeStr只读属性，表示Age的字符串
    ReadOnlyProperty<string> AgeStr;

    // 构造函数
    Person()
        : Age(Property<int>::Init(this)
                  // 使用Property<int>::Init初始化属性
                  .Getter([](Person *self) {
                      cout << "get Age" << endl;
                      return self->_age;
                  })
                  .Setter([](Person *self, const int &value) {
                      cout << "set Age: " << value << endl;
                      if (value <= 0) {
                          cout << "error: Age can not smaller than 1" << endl;
                          return; // 若设置的Age范围不正确则输出错误，不更改值
                      }
                      self->_age = value;
                  })),

          AgeStr(Property<string>::Init(this)
                     .Getter([](Person *self) {
                         cout << "get AgeStr" << endl;
                         return to_string(self->_age);
                     }))
    {
        _age = 1; // Age默认值
    }
};
```

测试代码如下：

```cpp
int main()
{
    Person p;
    cout << p.Age << endl; // get Age

    p.Age = -1;            // error: Age can not smaller than 1
    cout << p.Age << endl; // 仍然为1

    p.Age = 10;
    cout << p.AgeStr << endl; // 10

    Person p2 = p;                      // 对象可以正常拷贝
    p2.Age++;                           // 先get后set
    cout << p2.AgeStr->c_str() << endl; // 使用->可以访问属性成员

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
