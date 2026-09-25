// HACK: include these here prevents them from being included in the cache.h
// below, so the `#define private public` does not fuck with them. This should really
// be fixed somehow...

#include <mutex>
#include <list>
#include <chrono>

#define private public
#include <dwhbll/collections/cache.h>
#undef private
#include <dwhbll/macros/testing.h>

import dwhbll.testing;
import dwhbll.collections;
import std;

using namespace dwhbll::test;

namespace collections::cache {

[[=test]]
void add_and_find()
{
    dwhbll::collections::cache<std::string, std::string> cache;

    auto* k = cache.addEntry(std::chrono::system_clock::now() + std::chrono::seconds(1), "a", "b");
    auto* k2 = cache.keys.find("a");

    REQUIRE(k2 == k);
}

[[=test]]
void get_valid_entry()
{
    dwhbll::collections::cache<std::string, std::string> cache;

    cache.addEntry(std::chrono::system_clock::now() + std::chrono::seconds(1), "a", "b");

    EXPECT_NO_THROW(cache.getEntry("a"));
}

[[=test]]
void get_expired_entry()
{
    dwhbll::collections::cache<std::string, std::string> cache;

    cache.addEntry(std::chrono::system_clock::now() + std::chrono::seconds(1), "a", "b");

    std::this_thread::sleep_for(std::chrono::seconds(2));

    EXPECT_THROWS(std::out_of_range, cache.getEntry("a"));
}

}

TEST_REGISTER_FILE();
