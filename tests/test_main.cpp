#include <functional>
#include <optional>
#include <unordered_map>
#include <string>
#include <iostream>

extern bool pool_test(std::optional<std::string> test_to_run);
extern bool matrix_test(std::optional<std::string> test_to_run);
extern bool ring_test(std::optional<std::string> test_to_run);
extern bool cache_test(std::optional<std::string> test_to_run);

// The optional string argument is for the subtests to run
using TestFunc = std::function<bool(std::optional<std::string>)>;

std::unordered_map<std::string, TestFunc> module_dispatch {
    { "pool", pool_test },
    { "matrix", matrix_test },
    { "ring", ring_test },
    {"cache", cache_test},
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " test_name [subtest name]" << std::endl;
        return 1;
    }

    if(module_dispatch.contains(argv[1])) {
        auto fun = module_dispatch.at(argv[1]);
        auto ret = (argc == 3) ? fun(argv[2]) : fun(std::nullopt);
        std::cout << argv[1] << ":" << ret << std::endl;
        return ret;
    }

    return 0;
}
