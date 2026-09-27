Many C# idioms don't map cleanly to C++. Here are how to handle some of these scenarios:

- Generally, prefix getters/setters with `Get`/`Set`
- Internal visibility
  - Classes - add to the end of its namespace `::detail`
  - Class methods - use public and comment `// detail` outside the method
- Interface inheritance
  - If the interface is used for polymorphism (usually the case), create the interface as normal
  - If the interface is a marker interface (o!f only uses it like `if (value is IMyMarker))`):
    - Create a C++20 concept `MyMarker` representing the C# interface
    - Under the class declaration, use `static_assert(ConceptName<Type>, "...")`, and fulfill it with a free function
    - At the site checking the interface, check like `if constexpr (MyMarker<T>) {`
- Feel free to template-ize any C# code that manually checks int/float/double to a `std::is_arithmetic_v<T>`
