#include <rmx/rmx.hpp>
#include <iostream>

int main(int argc, const char** argv)
{
    rmx::Mutex<int> value(42);
    auto guard = value.lock();

    std::cout << "value: " << *guard << std::endl;
    *guard += 1;
    std::cout << "value: " << *guard << std::endl;

    return 0;
}
