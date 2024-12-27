#include <catch2/catch_test_macros.hpp>
#include <rmx/rmx.hpp>

struct Dummy
{
    int value = 0;

    Dummy() = default;
    Dummy(int v) : value(v) {}

    void set_value(int v) noexcept { value = v; }
    [[nodiscard]] int get_value() const noexcept { return value; }
};

TEST_CASE("Pointer access for a more sophisticated type")
{
    rmx::Mutex<Dummy> mutex;

    {
        INFO("Get default constructed value");
        auto value = mutex.lock();

        REQUIRE(value->get_value() == 0);
    }

    {
        INFO("Set value through operator->()");
        auto value = mutex.lock();
        value->set_value(42);

        REQUIRE((*value).value == 42);
        REQUIRE(value->value == 42);
    }
}
