#include <catch2/catch_test_macros.hpp>
#include <rmx/rmx.hpp>

struct Pod
{
    int value;

    void set_value(int v) noexcept { value = v; }
    [[nodiscard]] int get_value() const noexcept { return value; }
};

TEST_CASE("Pass a POD type")
{
    rmx::Mutex<Pod, std::mutex> mutex(Pod{2});

    {
        INFO("Get constructed value");
        auto value = mutex.lock();

        REQUIRE(value->get_value() == 2);
    }

    {
        INFO("Set value through operator->()");
        auto value = mutex.lock();
        value->set_value(0);

        REQUIRE((*value).value == 0);
        REQUIRE(value->value == 0);
    }
}
