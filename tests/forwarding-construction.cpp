#include <catch2/catch_test_macros.hpp>
#include <rmx/rmx.hpp>

#include <cstddef>
#include <vector>

namespace {
struct Aggregate
{
    int a;
    int b;
};
}  // namespace

TEST_CASE("Single-arg construction calls the matching constructor, not initializer_list")
{
    // Would build a one-element vector containing {3} if the ctor used brace-init.
    rmx::Mutex<std::vector<int>> mutex(std::size_t{3});
    auto guard = mutex.lock();
    REQUIRE(guard->size() == 3);
}

TEST_CASE("Multi-arg construction forwards to the matching constructor")
{
    rmx::Mutex<std::vector<int>> mutex(std::size_t{3}, 4);
    auto guard = mutex.lock();
    REQUIRE(guard->size() == 3);
    REQUIRE(guard->front() == 4);
}

TEST_CASE("Aggregate types initialize without a matching constructor")
{
    rmx::Mutex<Aggregate> mutex(1, 2);
    auto guard = mutex.lock();
    REQUIRE(guard->a == 1);
    REQUIRE(guard->b == 2);
}

TEST_CASE("Default construction zero-initializes through the forwarding ctor")
{
    rmx::Mutex<int> mutex;
    auto guard = mutex.lock();
    REQUIRE(*guard == 0);
}
