# `prop.h`

一个头文件为 C++ 提供类似 C# 的属性语法。

## 示例

以下定义了一个 `Person` 类，包含一个可读可写属性 `Age` 以及一个只读属性 `AgeStr`，代码演示了属性的声明和初始化。

```C++
class Person
{
    // 为当前类启用成员属性宏支持
    ENABLE_PROPERTY(Person);

    // 属性Age维护的字段
    int _age;

public:
    // 声明一个属性Age
    PROPERTY_RW(int, Age);

    // 声明AgeStr只读属性，表示Age的字符串
    PROPERTY_R(string, AgeStr);

    // 构造函数
    Person()
        : Age(
              // 使用GETTER宏和SETTER宏设置属性的getter和setter
              GETTER(int) {
                  cout << "get Age" << endl;
                  return self._age; // self表示当前对象
              },
              SETTER(int) {
                  cout << "set Age: " << value << endl; // value表示给属性设置的值
                  if (value <= 0) {
                      cout << "error: Age can not smaller than 1" << endl;
                      return; // 若设置的Age范围不正确则输出错误，不更改值
                  }
                  self._age = value;
              }),

          AgeStr(
              // 只读属性只有getter
              GETTER(string) {
                  cout << "get AgeStr" << endl;
                  return to_string(self._age);
              })
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
    cout << p.Age << endl; // 10

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
get Age
10
get Age
set Age: 11
get AgeStr
11
```

