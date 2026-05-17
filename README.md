# CppSharp

A header-only C++ library that brings C#-style **Delegate**, **Property**, and **Event** patterns to C++.

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

### Event (`event.h`)

An event accessor that exposes a private delegate through a public event, enabling C#-style encapsulated event semantics.

- **Member events** — expose a private delegate via field pointer or getter method on the owner object
- **Static events** — bind to a global/static delegate via a free function accessor
- **`+=` / `-=`** — add or remove event handlers, just like C#
- **`EventArgs`** — a base event arguments struct (extend as needed)
- **`EventHandler<TSender, TEventArgs>`** — a convenience type alias for `Delegate<void(TSender &, TEventArgs &)>`

## Quick Start

Clone or copy the `include/` directory into your project, then:

```cpp
#include "delegate.h"
#include "property.h"
#include "event.h"
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

## Usage — Event

### Basic member event

```cpp
#include "event.h"
#include <iostream>

class Button
{
    Delegate<void()> _clicked;

public:
    Event<Delegate<void()>> Clicked{
        Event<Delegate<void()>>::Init(this)
            .Delegate<&Button::_clicked>()};

    void Click()
    {
        if (_clicked) {
            _clicked();
        }
    }
};

int main()
{
    Button btn;
    btn.Clicked += []() { std::cout << "Clicked!" << std::endl; };
    btn.Click(); // prints: Clicked!
}
```

### Member function accessor

If the delegate is accessed through a getter method instead of a field:

```cpp
class Button
{
    Delegate<void()> _clicked;

    Delegate<void()> &GetClicked() { return _clicked; }

public:
    Event<Delegate<void()>> Clicked{
        Event<Delegate<void()>>::Init(this)
            .Delegate<&Button::GetClicked>()};

    void Click() { if (_clicked) _clicked(); }
};
```

### Static event

```cpp
Delegate<void()> &GetGlobalDelegate()
{
    static Delegate<void()> d;
    return d;
}

Event<Delegate<void()>> GlobalClicked{
    Event<Delegate<void()>>::Init()
        .Delegate(GetGlobalDelegate)};
```

### EventHandler with sender and args

```cpp
class Button
{
    EventHandler<Button> _clicked;

public:
    Event<EventHandler<Button>> Clicked{
        Event<EventHandler<Button>>::Init(this)
            .Delegate<&Button::_clicked>()};

    void Click()
    {
        if (_clicked) {
            EventArgs args;
            _clicked(*this, args);
        }
    }
};

int main()
{
    Button btn;
    btn.Clicked += [](Button &sender, EventArgs &args) {
        std::cout << "Handled!" << std::endl;
    };
    btn.Click(); // prints: Handled!
}
```

## Project Structure

```text
include/
    delegate.h    — Delegate, Action, Func, Predicate
    property.h    — Property, ReadOnlyProperty, WriteOnlyProperty
    event.h       — Event, EventArgs, EventHandler
```

## License

[MIT](LICENSE)
