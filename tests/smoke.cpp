#include <catch2/catch_test_macros.hpp>
#include <rmx/rmx.hpp>

#include <algorithm>
#include <numeric>
#include <thread>
#include <vector>

// The test philosophy for this project is to trust that std::mutex is correct, and only test the
// additional niceties that rmx::Mutex provides. This test is intended to be a smoke test that TSAN
// can analyze.
TEST_CASE("Smoke test: rmx::Mutex can be used safely from multiple threads")
{
    constexpr int thread_count = 7;
    constexpr int per_thread = 113;
    constexpr int total = thread_count * per_thread;

    rmx::Mutex<std::vector<int>> counters;

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (int t = 0; t < thread_count; ++t)
    {
        threads.emplace_back([&counters, t] {
            for (int i = 0; i < per_thread; ++i)
            {
                auto guard = counters.lock();
                guard->push_back((t * per_thread) + i);
            }
        });
    }
    for (auto& th : threads)
    {
        th.join();
    }

    auto guard = counters.lock();
    REQUIRE_FALSE(counters.is_poisoned());
    REQUIRE(guard->size() == total);

    std::sort(guard->begin(), guard->end());
    std::vector<int> expected(total);
    std::iota(expected.begin(), expected.end(), 0);
    REQUIRE(*guard == expected);
}
