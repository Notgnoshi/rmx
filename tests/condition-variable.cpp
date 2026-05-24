#include <catch2/catch_test_macros.hpp>
#include <rmx/rmx.hpp>

#include <chrono>
#include <condition_variable>
#include <thread>

TEST_CASE("MutexGuard::wait blocks until the predicate is satisfied")
{
    rmx::Mutex<int> mutex(0);
    std::condition_variable cv;

    std::thread producer([&mutex, &cv] {
        auto g = mutex.lock();
        *g = 1;
        cv.notify_one();
    });

    {
        auto guard = mutex.lock();
        guard.wait(cv, [&guard] {
            return *guard != 0;
        });
        REQUIRE(*guard == 1);
    }

    producer.join();
}

TEST_CASE("MutexGuard::wait_for returns false on timeout, true once satisfied")
{
    rmx::Mutex<int> mutex(0);
    std::condition_variable cv;

    {
        auto guard = mutex.lock();
        bool ok = guard.wait_for(cv, std::chrono::milliseconds(1), [&guard] {
            return *guard != 0;
        });
        REQUIRE_FALSE(ok);
    }

    std::thread producer([&mutex, &cv] {
        auto g = mutex.lock();
        *g = 2;
        cv.notify_one();
    });

    {
        auto guard = mutex.lock();
        bool ok = guard.wait_for(cv, std::chrono::seconds(1), [&guard] {
            return *guard != 0;
        });
        REQUIRE(ok);
        REQUIRE(*guard == 2);
    }

    producer.join();
}
