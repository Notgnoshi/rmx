#include <rmx/rmx.hpp>

#include <iostream>

int main(int argc, const char** argv)
{
    const auto result = rmx::add(2, 2);
    std::cout << result << std::endl;

    return 0;
}
