#include <condition_variable>
#include <dwhbll/collections/cache.h>

#include <mutex>
#include <thread>
#include <unordered_set>

namespace dwhbll::collections {
    std::mutex caches_change;
    std::condition_variable caches_cv;
    bool updated = false;
    std::unordered_set<generic_cache*> caches;

    generic_cache* getNextExpire() {
        generic_cache* next{nullptr};
        {
            std::chrono::system_clock::time_point point;
            std::unique_lock lock(caches_change);
            bool first = true;
            for (auto* c : caches) {
                if (first) {
                    point = c->first_expire_time;
                    next = c;
                    first = false;
                    continue;
                }
                if (point > c->first_expire_time) {
                    point = c->first_expire_time;
                    next = c;
                }
            }
        }
        return next;
    }

    void cacheCleanupWorker() {
        while (true) {
            generic_cache* wait_for = getNextExpire();

            if (!wait_for) {
                std::unique_lock lock(caches_change);
                caches_cv.wait(lock);
                continue;
            } else {
                std::unique_lock lock(caches_change);
                while (!updated) {
                    if (caches_cv.wait_until(lock, wait_for->first_expire_time) == std::cv_status::timeout)
                        goto update_cache;
                }
                continue; // restart, waiting for the earliest possible expiry.
            }
        update_cache:
            auto ctime = std::chrono::system_clock::now();
            for (auto* c : caches) {
                std::unique_lock _(c->cache_lock);
                for (auto it = c->entries.begin(); it != c->entries.end();) {
                    if (it->first < ctime) {
                        // before erasing, release the memory pool data
                        auto& [k, v] = it->second;
                        c->return_values(k, v);
                        c->entries.erase(it);
                        it = c->entries.begin();
                    } else
                        ++it;
                }
            }
        }
    }

    //std::thread worker;

    generic_cache::generic_cache() {
        std::unique_lock _(caches_change);
        caches.insert(this);
    }

    generic_cache::~generic_cache() {
        std::unique_lock _(caches_change);
        caches.erase(this);
    }

    generic_cache::generic_cache(const generic_cache &other): entries(other.entries) {
        std::unique_lock _(caches_change);
        caches.insert(this);
    }

    generic_cache::generic_cache(generic_cache &&other) noexcept: entries(std::move(other.entries)) {
        std::unique_lock _(caches_change);
        caches.erase(&other);
        caches.insert(this);
    }

    generic_cache &generic_cache::operator=(const generic_cache &other) {
        if (this == &other)
            return *this;
        entries = other.entries;
        return *this;
    }

    generic_cache &generic_cache::operator=(generic_cache &&other) noexcept {
        std::unique_lock _(caches_change);
        if (this == &other)
            return *this;
        entries = std::move(other.entries);
        caches.erase(&other);
        return *this;
    }

    void generic_cache::addEntry(std::chrono::system_clock::time_point expire_time, void *key, void *value) {
        std::unique_lock _(cache_lock);

        if (first_expire_time > expire_time)
            first_expire_time = expire_time;

        if (entries.empty()) {
            // insert one before here
            entries.push_back({expire_time, {key, value}});

            // flag the worker to let it know we potentially have a new most important
            std::unique_lock lock(caches_change);
            updated = true;
            caches_cv.notify_one();
        }

        // iterate until we find one where the current one is > than expire_time
        for (auto it = entries.begin(); it != entries.end(); ++it) {
            if (it->first > expire_time) {
                // if we are inserting before the first, this means we have a potentially new earliest cache expiry.
                bool flagWorker = it == entries.begin();

                // insert one before here
                entries.insert(it, {expire_time, {key, value}});

                if (flagWorker) {
                    // flag the worker to let it know we potentially have a new most important
                    std::unique_lock lock(caches_change);
                    updated = true;
                    caches_cv.notify_one();
                }
            }
        }
    }

    void * generic_cache::getEntry(void *key) {
        std::unique_lock _(cache_lock);
        auto expiry = std::chrono::system_clock::now();
        for (auto& [expire, data] : entries) {
            if (expiry > expire)
                continue; // expired
            if (auto& [k, v] = data; k == key)
                return v;
        }
        return nullptr;
    }
}
