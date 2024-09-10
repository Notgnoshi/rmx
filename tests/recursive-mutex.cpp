#include <catch2/catch_test_macros.hpp>
#include <rmx/rmx.hpp>

struct Dummy
{
    int value;

    explicit Dummy(int v) : value(v) {}

    void set_value(int v) noexcept { value = v; }
    [[nodiscard]] int get_value() const noexcept { return value; }
};

TEST_CASE("Use a std::recursive_mutex")
{
    rmx::Mutex<Dummy, std::recursive_mutex> mutex(42);

    {
        INFO("Get constructed value");
        auto value = mutex.lock();

        REQUIRE(value->get_value() == 42);
    }

    {
        INFO("Set value through operator->()");
        auto value = mutex.lock();
        value->set_value(0);

        REQUIRE((*value).value == 0);
        REQUIRE(value->value == 0);
    }
}
