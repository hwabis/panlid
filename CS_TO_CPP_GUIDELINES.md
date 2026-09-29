Many C# idioms don't map cleanly to C++. Here are how to handle some of these scenarios:

- Generally, prefix getters/setters with `Get`/`Set`
  - For bool getters, use `Is` if appropriate
- Internal visibility
  - Classes - add to the end of its namespace `::detail`
  - Class methods - use public and prefix the name with `DETAIL`
- Interface inheritance
  - If the interface is used for polymorphism (usually the case), create the interface as normal
  - If the interface is a marker interface (o!f only uses it like `if (value is IMyMarker))`):
    - Create a C++20 concept `MyMarker` representing the C# interface
    - Under the class declaration, use `static_assert(ConceptName<Type>, "...")`, and fulfill it with a free function
    - At the site checking the interface, check like `if constexpr (MyMarker<T>) {`
  - In general, feel free to template-ize any C# code to be more concise
- If there is `IMyClass` and `IMyClass<T>`, name the latter to `IMyClassVal<T>`
- If a class is meant to only be used as a shared_ptr, give a private ctor and a `::Create(...)` returning a shared_ptr
- If class A needs another class B (not needing B's full size/members), forward-declare B instead of including its header
