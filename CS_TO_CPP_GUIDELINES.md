Many C# idioms don't map cleanly to C++. Here are how to handle some of these scenarios:

- Generally, prefix getters/setters with `Get`/`Set`
  - For bool getters, use `Is` if appropriate
- Internal visibility
  - Classes - add to the end of its namespace `::detail`
  - Class methods - use public and prefix the name with `DETAIL`, or use friend class if more appopriate
- Interface inheritance
  - If the interface is used for polymorphism (usually the case), create the interface as normal
  - If the interface is a marker interface (o!f only uses it like `if (value is IMyMarker))`):
    - Create a C++20 concept representing it
    - Require its methods as members (`T::Foo(...)` for static, `x.Foo(...)` for instance), not free functions
    - Under the class: `static_assert(MyConcept<Type>, "...")`; check with `if constexpr` or constrain templates with it
  - In general, feel free to template-ize any C# code to be more concise
  - Remember in C#, events in interfaces need to be re-declared. Don't do that in C++
- If there is `IMyClass` and `IMyClass<T>`, name the latter to `IMyClassVal<T>`
- If a class is meant to only be used as a shared_ptr, change its ctor to private and add `::Create(...)`
  that returns a shared_ptr at the same visilbity the ctor originally was
- If class A needs another class B (not needing B's full size/members), forward-declare B instead of including its header
- C# readonly struct: use const methods, non-const data members
