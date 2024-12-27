#include <catch2/catch_test_macros.hpp>
#include <rmx/rmx.hpp>

TEST_CASE("Detects exceptions while locked")
{
    auto mutex = rmx::Mutex(true);

    try
    {
        INFO("Get constructed value");
        auto value = mutex.lock();
        REQUIRE(*value == true);

        throw std::runtime_error("Throwing an exception while the Mutex is locked");

        // This important part of the transaction was skipped!
        *value = false;
    } catch (...)
    {
        // ...
    }

    INFO("Throwing an exception while locked poisons the mutex");
    REQUIRE(mutex.is_poisoned());

    {
        INFO(
            "lock_unchecked() doesn't throw, but the transaction was cancelled, so the value never "
            "changed");
        auto value = mutex.lock_unchecked();
        REQUIRE(*value == true);
    }

    {
        INFO("lock() does throw");
        REQUIRE_THROWS(mutex.lock(), "Mutex poisoned: exception thrown while Mutex was locked");
    }
}
