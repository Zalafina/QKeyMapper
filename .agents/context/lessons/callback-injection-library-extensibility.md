---
name: callback-injection-library-extensibility
description: Use std::function callback injection to add application-specific logic into library code without modifying library architecture
metadata:
  type: pattern
---

# Callback Injection for Library Extensibility

**Pattern**: When library code needs to perform application-specific checks (e.g., virtual gamepad detection) but shouldn't depend on application headers, inject a `std::function` callback that the library calls at the right moment.

## Structure

```cpp
// In library header:
#include <functional>
class LibraryClass {
public:
    using DetectorCallback = std::function<bool(ArgType1, ArgType2, const QString&)>;
    void setDetector(DetectorCallback cb) { m_detector = std::move(cb); }
private:
    DetectorCallback m_detector;
};

// In library implementation, at the appropriate hook point:
if (m_detector && m_detector(obj->field1, obj->field2, obj->field3)) {
    obj->flag = true;  // Mutate library-owned object safely on library's thread
}

// In application initialization:
libraryInstance->setDetector([](ArgType1 a1, ArgType2 a2, const QString& s) -> bool {
    return (a1 == APP_CONSTANT_1 && a2 == APP_CONSTANT_2)
        || (s.startsWith(APP_PREFIX));
});
```

## When to Use
- Library owns the object lifecycle and mutation timing
- Application has domain-specific knowledge (magic constants, business rules)
- Adding the logic directly to the library would create a dependency on application headers
- The check must happen at a specific point in the library's processing pipeline

## Tradeoffs vs Alternatives
- **vs subclassing**: Callback is lighter — no virtual table, no class hierarchy
- **vs #ifdef**: Callback is runtime-configurable and testable; #ifdef couples library to application at compile time
- **vs Q_PROPERTY/setting**: Callback allows complex multi-field logic, not just simple flag checks

## Use in This Project
- [[joystick-hotplug-toctou-crash]] — `SDL_Joysticks::VirtualGamepadDetector` for X360/DS4 ViGEm detection

**Why**: The alternative (giving `SDL_Joysticks` direct knowledge of ViGEm VID/PID constants) would couple a general-purpose joystick library to a specific application's virtual device concepts. **How to apply**: Add `using Callback = std::function<ReturnType(Args...)>;` and a setter. Call at the natural hook point. Register from application init code.
