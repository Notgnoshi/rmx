# rmx

Rust-style mutex for C++17

## API showcase

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

## Developer info

Install the sanitizer dependencies. You can opt out of sanitizers with the `RMX_ENABLE_ASAN` and
`RMX_ENABLE_TSAN` CMake options.

```sh
sudo dnf install libasan libubsan libtsan
```

Build, test, lint, and format with

```sh
cmake -B ./build/
cmake --build ./build/
run-clang-tidy -p ./build/ '/(tests|include)/'
git ls-files '*.hpp' '*.cpp' | xargs clang-format -i
```

Run the tests with

```sh
ctest --test-dir ./build/
```

Note that the testing philosophy is to trust that `std::mutex` is correct. Since `rmx::Mutex` is
really just a wrapper type that defers synchronization to the C++ standard library, the tests are
largely single-threaded, and designed around checking that the `rmx::Mutex` API works as expected.
