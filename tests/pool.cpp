#include <dwhbll/testing/testing.h>
#include <dwhbll/memory/pool.h>

using namespace dwhbll::test;

struct poolStruct {
    int f[1024];
};

namespace pool {

[[=test]]
void pool_struct_allocate_free()
{
    dwhbll::memory::Pool<poolStruct> testPool;

    static_assert(dwhbll::memory::Pool<poolStruct>::block_size() > 0); // block size <= 0 results in infinite loop

    auto before = testPool.used_size();

    for (int i = 0; i < 1024; i++)
        auto _ = testPool.acquire();

    auto after = testPool.used_size();

    EXPECT(before == after && after == 0);
}

[[=test]]
void pool_struct_return_memory()
{
    dwhbll::memory::Pool<poolStruct> testPool;

    auto before = testPool.used_size();

    for (int i = 0; i < 1024; i++)
        testPool.offer(testPool.acquire().disown()); // intentionally disown all the memory;

    auto after = testPool.used_size();

    EXPECT(before == after && after == 0);
}

[[=test]]
void pool_struct_leak_memory()
{
    dwhbll::memory::Pool<poolStruct> testPool;

    auto before = testPool.used_size();

    for (int i = 0; i < 1024; i++)
        testPool.acquire().disown(); // intentionally disown all the memory;

    auto after = testPool.used_size();

    EXPECT(before + 1024 == after);
}

[[=test]]
void pool_int_allocate_free()
{
    dwhbll::memory::Pool<int> testPool;

    static_assert(dwhbll::memory::Pool<int>::block_size() > 0); // block size <= 0 results in infinite loop

    auto before = testPool.used_size();

    for (int i = 0; i < 1024; i++)
        auto _ = testPool.acquire();

    auto after = testPool.used_size();

    EXPECT(before == after && after == 0);
}

[[=test]]
void pool_int_return_memory()
{
    dwhbll::memory::Pool<int> testPool;

    auto before = testPool.used_size();

    for (int i = 0; i < 1024; i++)
        testPool.offer(testPool.acquire().disown()); // intentionally disown all the memory;

    auto after = testPool.used_size();

    EXPECT(before == after && after == 0);
}

[[=test]]
void pool_int_leak_memory()
{
    dwhbll::memory::Pool<int> testPool;

    auto before = testPool.used_size();

    for (int i = 0; i < 1024; i++)
        testPool.acquire().disown(); // intentionally disown all the memory;

    auto after = testPool.used_size();

    EXPECT(before + 1024 == after);
}

[[=test]]
void pool_string_find()
{
    dwhbll::memory::Pool<std::string> testPool;

    auto* k = testPool.acquire("foo").disown();
    auto* k2 = testPool.find("foo");

    REQUIRE(k == k2);
}

[[=test]]
void pool_int_find()
{
    dwhbll::memory::Pool<int> testPool;

    auto* k = testPool.acquire(5).disown();
    auto* k2 = testPool.find(5);

    REQUIRE(k == k2);
}

}

TEST_REGISTER_FILE();
