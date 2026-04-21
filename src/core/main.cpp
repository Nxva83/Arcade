#include <iostream>
#include "Core.hpp"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: ./arcade <graphics_library.so>" << std::endl;
        return 84;
    }
    try {
        Arcade::Core core(argv[1]);
        core.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 84;
    }
    return 0;
}
