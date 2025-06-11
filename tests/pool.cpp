#include <iostream>

#include <dwhbll/memory/pool.h>
#include <optional>

struct poolStruct {
    int f[1024];
};

bool pool_test(std::optional<std::string> test_to_run) {
    bool failed = false;
    {
        dwhbll::memory::Pool<poolStruct> testPool;

        static_assert(dwhbll::memory::Pool<poolStruct>::block_size() > 0); // block size <= 0 results in infinite loop

        auto before = testPool.used_size();

        for (int i = 0; i < 1024; i++)
            auto _ = testPool.acquire();

        auto after = testPool.used_size();

        if (before != after || after != 0) {
            std::cerr << std::format("[ERR] (Pool<poolStruct>) Allocate then free failure: Before size: {} != After size: {}", before, after) << std::endl;
            failed = true;
        }

        before = testPool.used_size();

        for (int i = 0; i < 1024; i++)
            testPool.offer(testPool.acquire().disown()); // intentionally disown all the memory;

        after = testPool.used_size();

        if (before != after || after != 0) {
            std::cerr << std::format("[ERR] (Pool<poolStruct>) Return memory failure: Before size: {} != After size: {}", before, after) << std::endl;
            failed = true;
        }

        before = testPool.used_size();

        for (int i = 0; i < 1024; i++)
            testPool.acquire().disown(); // intentionally disown all the memory;

        after = testPool.used_size();

        if (before + 1024 != after) {
            std::cerr << std::format("[ERR] (Pool<poolStruct>) Leak memory failure (memory not leaked): Before size: {} != After size: {}", before, after) << std::endl;
            failed = true;
        }
    }

    {
        dwhbll::memory::Pool<int> testPool;

        static_assert(dwhbll::memory::Pool<int>::block_size() > 0); // block size <= 0 results in infinite loop

        auto before = testPool.used_size();

        for (int i = 0; i < 1024; i++)
            auto _ = testPool.acquire();

        auto after = testPool.used_size();

        if (before != after || after != 0) {
            std::cerr << std::format("[ERR] (Pool<int>) Allocate then free failure: Before size: {} != After size: {}", before, after) << std::endl;
            failed = true;
        }

        before = testPool.used_size();

        for (int i = 0; i < 1024; i++)
            testPool.offer(testPool.acquire().disown()); // intentionally disown all the memory;

        after = testPool.used_size();

        if (before != after || after != 0) {
            std::cerr << std::format("[ERR] (Pool<int>) Return memory failure: Before size: {} != After size: {}", before, after) << std::endl;
            failed = true;
        }

        before = testPool.used_size();

        for (int i = 0; i < 1024; i++)
            testPool.acquire().disown(); // intentionally disown all the memory;

        after = testPool.used_size();

        if (before + 1024 != after) {
            std::cerr << std::format("[ERR] (Pool<int>) Leak memory failure (memory not leaked): Before size: {} != After size: {}", before, after) << std::endl;
            failed = true;
        }
    }
    return failed;
}
