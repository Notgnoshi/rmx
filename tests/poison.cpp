#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <rmx/rmx.hpp>

#include <stdexcept>

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
        // @expected: the test deliberately threw above to drive the Mutex into poisoned state.
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
        REQUIRE_THROWS_WITH(mutex.lock(),
                            "Mutex poisoned: exception thrown while Mutex was locked");
    }
}

TEST_CASE("Guard constructed in a catch block does not falsely poison")
{
    rmx::Mutex<int> mutex(0);

    try
    {
        throw std::runtime_error("outer");
    } catch (...)
    {
        // There's no stack unwinding in progress when mutex is locked; does not poison
        auto guard = mutex.lock();
        *guard = 1;
    }

    REQUIRE_FALSE(mutex.is_poisoned());
    REQUIRE(*mutex.lock() == 1);
}

TEST_CASE("Guard constructed during stack unwinding does not falsely poison")
{
    rmx::Mutex<int> mutex(0);

    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions): don't care about 3/5/0 in tests
    struct Locker
    {
        explicit Locker(rmx::Mutex<int>* m) noexcept : mutex(m) {}
        ~Locker()
        {
            auto guard = mutex->lock_unchecked();
            *guard = 2;
        }

        rmx::Mutex<int>* mutex;
    };

    try
    {
        // Throwing an exception while the mutex is not locked shouldn't be a problem; that's not
        // what poisons it. Poisoning is about guaranteeing that the full transaction that the mutex
        // lock guarded completed successfully.
        Locker locker(&mutex);
        throw std::runtime_error("outer");
    } catch (...)
    {
        // @expected: the test deliberately threw to trigger locking the mutex during unwinding
    }

    REQUIRE_FALSE(mutex.is_poisoned());
    REQUIRE(*mutex.lock() == 2);
}
