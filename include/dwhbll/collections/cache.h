#pragma once

#include <chrono>
#include <list>

#include <dwhbll/memory/pool.h>

bool cache_test(std::optional<std::string> test_to_run);

namespace dwhbll::collections {
    class generic_cache {
        std::mutex cache_lock;

        std::chrono::system_clock::time_point first_expire_time;
        std::list<std::pair<std::chrono::system_clock::time_point, std::pair<void*, void*>>> entries;

        friend generic_cache* getNextExpire();
        friend void cacheCleanupWorker();

    public:
        generic_cache();

        virtual ~generic_cache();

        generic_cache(const generic_cache &other);

        generic_cache(generic_cache &&other) noexcept;

        generic_cache & operator=(const generic_cache &other);

        generic_cache & operator=(generic_cache &&other) noexcept;

        void addEntry(std::chrono::system_clock::time_point expire_time, void* key, void* value);

        void* getEntry(void* key);

    protected:
        virtual void return_values(void* key, void* value) = 0;
    };

    /**
     * @brief a simple cache class that... probably works?
     */
    template <typename K, typename V>
    requires std::copy_constructible<K> && std::copy_constructible<V>
    class cache : generic_cache {
        memory::Pool<K> keys;
        memory::Pool<V> values;

        friend bool ::cache_test(std::optional<std::string> test_to_run);

    public:
        cache() = default;

        void* addEntry(std::chrono::system_clock::time_point expire_time, K key, V value) {
            auto* k = keys.acquire(key).disown();
            generic_cache::addEntry(expire_time, k, values.acquire(value).disown());
            return k;
        }

        V& getEntry(const K& key) {
            auto* r = static_cast<V*>(generic_cache::getEntry(keys.find(key)));
            if (r == nullptr)
                throw std::out_of_range("key not found (probably expired)");
            return *r;
        }

    protected:
        void return_values(void *key, void *value) override {
            keys.offer(static_cast<K*>(key));
            values.offer(static_cast<V*>(value));
        }
    };
}
