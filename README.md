# rmx

Rust-style mutex for C++17

```cpp
#include <rmx/rmx.hpp>

auto mutex = rmx::Mutex(42);

{
    auto value = mutex.lock();
    *value += 1;
}

{
    auto value = mutex.lock();
    assert(*value == 43);
}

try {
    auto value = mutex.lock();
    throw std::runtime_error("Throw an exception while the Mutex was locked");

    // This important invariant wasn't kept, the transaction was canceled by the exception
    *value += 1;
} catch (...) {
    // ...
}

assert(mutex.is_poisoned());

{
    // doesn't throw an exception
    auto value = mutex.lock_unchecked();
    assert(*value == 43);
}
{
    // does throw an exception
    auto value = mutex.lock();
}
```
