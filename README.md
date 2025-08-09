# `property.h`

一个头文件为 C++ 提供类似 C# 的属性语法。

## 示例

以下定义了一个 `Person` 类，包含一个可读可写属性 `Age` 以及一个只读属性 `AgeStr`，代码演示了属性的声明和初始化。

```C++
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

```C++
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

```
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

