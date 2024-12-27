#include <catch2/catch_test_macros.hpp>
#include <rmx/rmx.hpp>

TEST_CASE("Basic mutability and access")
{
    auto mutex = rmx::Mutex(true);

    {
        INFO("Can read wrapped value");
        auto value = mutex.lock();
        REQUIRE(*value == true);
    }

    {
        INFO("Can write wrapped value");
        {
            auto value = mutex.lock();
            *value = false;
        }  // drop guard for fun
        auto value = mutex.lock();
        REQUIRE(*value == false);
    }

    {
        INFO("Writing modifies the outer Mutex's value, not just the inner MutexGuard's reference");
        auto value = mutex.lock();
        REQUIRE(*value == false);
    }

    {
        INFO("try_lock() fails if already locked");
        auto value = mutex.lock();

        auto value2 = mutex.try_lock();
        REQUIRE_FALSE(value2.has_value());
    }

    {
        INFO("try_lock() succeeds if not locked");
        auto maybe_value = mutex.try_lock();

        REQUIRE(maybe_value.has_value());
        REQUIRE(*maybe_value.value() == false);
    }
}
