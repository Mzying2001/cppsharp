# CppSharp

A header-only C++ library that brings C#-style **Delegate** and **Property** patterns to C++.

No dependencies, no build step — just drop the headers into your project and `#include` them.

## Features

### Delegate (`delegate.h`)

A multicast delegate that supports storing and invoking multiple callable objects, closely modeled after C# `Delegate`.

- **Multicast** — attach multiple handlers via `+=`, invoke them all with `()`
- **Type aliases** — `Action<Args...>`, `Func<T1, ..., TResult>`, `Predicate<T>` mirror their C# counterparts
- **Any callable** — free functions, member functions (const and non-const), lambdas, and functors
- **Equality** — delegates can be compared (`==`/`!=`) and checked against `nullptr`
- **Exception-safe** — copy assignment provides strong exception safety guarantee

### Property (`property.h`)

A property abstraction with getter/setter semantics, closely modeled after C# properties.

- **Read-write** — `Property<T>` with both getter and setter
- **Read-only** — `ReadOnlyProperty<T>` with getter only
- **Write-only** — `WriteOnlyProperty<T>` with setter only
- **Member and static** — bind to instance methods or static/free functions
- **Field binding** — directly bind to a data member as a shortcut
- **Rich operators** — arithmetic, bitwise, comparison, increment/decrement, subscript, dereference, and compound assignment all work transparently through the property
- **`operator->`** — access the underlying value's fields without calling `Get()` first

## Quick Start

Clone or copy the `include/` directory into your project, then:

```cpp
#include "delegate.h"
#include "property.h"
```

Requires **C++11** or later.

## Usage — Delegate

### Basic event pattern

```cpp
#include "delegate.h"
#include <iostream>

class Button
{
public:
    Action<Button *> Clicked;

    void Click()
    {
        if (Clicked) {
            Clicked(this);
        }
    }
};

void OnClick(Button *)
{
    std::cout << "Button clicked!" << std::endl;
}

int main()
{
    Button btn;
    btn.Clicked += OnClick;
    btn.Click(); // prints: Button clicked!
}
```

### Multicast

Multiple handlers can be attached; all of them are invoked in order:

```cpp
btn.Clicked += OnClick;
btn.Clicked += [](Button *) {
    std::cout << "Lambda handler" << std::endl;
};
btn.Click();
// Output:
//   Button clicked!
//   Lambda handler
```

### Removing a handler

```cpp
btn.Clicked -= OnClick; // removes the exact callable
```

### Member function binding

```cpp
class Logger
{
public:
    void Log(Button *)
    {
        std::cout << "Logger::Log" << std::endl;
    }
};

Logger logger;
btn.Clicked.Add(logger, &Logger::Log);
```

### Func with return value

```cpp
Func<int, int, int> add;
add += [](int a, int b) { return a + b; };
int result = add(3, 4); // result == 7
```

## Usage — Property

### Basic read-write property

```cpp
#include "property.h"
#include <iostream>
#include <string>

class Person
{
    int _age = 0;

public:
    Property<int> Age{
        Property<int>::Init(this)
            .Getter([](Person *self) { return self->_age; })
            .Setter([](Person *self, int value) {
                if (value >= 0) self->_age = value;
            })};
};

int main()
{
    Person p;
    p.Age = 25;
    std::cout << p.Age << std::endl; // 25
}
```

### Using member functions

```cpp
class Person
{
    int _age = 0;

    int getAge() const { return _age; }
    void setAge(int value) { _age = value; }

public:
    Property<int> Age{
        Property<int>::Init(this)
            .Getter<&Person::getAge>()
            .Setter<&Person::setAge>()};
};
```

### Direct field binding

If no custom logic is needed, bind directly to a data member:

```cpp
Property<int> Age{
    Property<int>::Init(this)
        .Getter<&Person::_age>()
        .Setter<&Person::_age>()};
```

### Read-only property

```cpp
class Circle
{
    double _radius = 1.0;

public:
    ReadOnlyProperty<double> Area{
        Property<double>::Init(this)
            .Getter([](Circle *self) {
                return 3.14159265 * self->_radius * self->_radius;
            })};
};
```

### Static property

```cpp
class App
{
    static std::string &nameStorage()
    {
        static std::string name = "MyApp";
        return name;
    }

public:
    static Property<std::string> Name{
        Property<std::string>::Init()
            .Getter([]() { return nameStorage(); })
            .Setter([](const std::string &v) { nameStorage() = v; })};
};
```

### Operator usage

Properties support arithmetic, comparison, and compound operators:

```cpp
Person p;
p.Age = 10;
p.Age += 5;       // Age is now 15
p.Age++;          // Age is now 16
std::cout << p.Age * 2 << std::endl; // 32
```

### Accessing fields via `operator->`

```cpp
class Config
{
public:
    struct Data { int x; int y; };
    Data _data{1, 2};

    Property<Data> Values{
        Property<Data>::Init(this)
            .Getter<&Config::_data>()
            .Setter<&Config::_data>()};
};

Config cfg;
std::cout << cfg.Values->x << std::endl; // 1
```

## Project Structure

```text
include/
    delegate.h    — Delegate, Action, Func, Predicate
    property.h    — Property, ReadOnlyProperty, WriteOnlyProperty
```

## License

[MIT](LICENSE)
