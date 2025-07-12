#include <iostream>
#include <dwhbll/collections/cache.h>
#include <thread>

bool cache_test(std::optional<std::string> test_to_run) {
    dwhbll::collections::cache<std::string, std::string> cache;

    auto* k = cache.addEntry(std::chrono::system_clock::now() + std::chrono::seconds(1), "a", "b");
    auto* k2 = cache.keys.find("a");

    if (k2 != k)
        return false; // broke something else

    try {
        std::cout << cache.getEntry("a");
    } catch (std::exception& e) {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    try {
        std::cout << cache.getEntry("a");
    } catch (std::out_of_range& e) {
        std::cout << e.what();
        return true;
    }

    return false;
}
