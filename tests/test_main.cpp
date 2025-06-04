#include <iostream>

extern bool pool_test();

int main(int argc, char** argv) {
    if (!pool_test()) {
        std::cerr << "[ERR] memory pool testing failed." << std::endl;
        return 1;
    } else
        std::cerr << "[PASSED] memory pool." << std::endl;

    return 0;
}
