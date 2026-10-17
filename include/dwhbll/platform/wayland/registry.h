#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>
#include <wayland-client.h>

#include <dwhbll/platform/wayland/awaiters.h>

namespace dwhbll::platform::wayland {

class display;

class registry {
public:
    struct global {
        uint32_t name;
        std::string interface;
        uint32_t version;
    };

    explicit registry(display& disp);
    ~registry();

    registry(const registry&) = delete;
    registry& operator=(const registry&) = delete;

    bool has(std::string_view interface_name) const noexcept;

    stl_ext::Result<void*, std::string> bind_raw(const wl_interface* iface, uint32_t version);

    template <typename T>
    stl_ext::Result<T*, std::string> bind(const wl_interface* iface, uint32_t version) {
        return bind_raw(iface, version).map([](void* p) {
            return static_cast<T*>(p);
        });
    }

    // Returns the id that can used to remove it
    std::uint64_t add_observer(std::function<void(const global&)> on_add,
                               std::function<void(uint32_t)> on_remove = {});
    void remove_observer(uint64_t id);

    /// Suspends the calling coroutine until the requested global is advertised.
    template <typename T = void>
    [[nodiscard]] auto wait_global(const wl_interface* iface, uint32_t version) {
        struct typed_awaiter : public wait_global_awaiter {
            using wait_global_awaiter::wait_global_awaiter;
            stl_ext::Result<T*, std::string> await_resume() {
                auto res = wait_global_awaiter::await_resume();
                if (!res.is_ok())
                    return res.unwrap_err();
                return static_cast<T*>(res.unwrap());
            }
        };
        return typed_awaiter(*this, iface, version);
    }

private:
    display* display_;
    wl_registry* registry_ = nullptr;

    std::unordered_map<uint32_t, global> globals_by_name_;
    std::unordered_map<std::string, std::vector<uint32_t>> globals_by_iface_;

    struct observer {
        std::function<void(const global&)> on_add;
        std::function<void(uint32_t)> on_remove;
    };
    std::unordered_map<uint64_t, observer> observers_;

    static const wl_registry_listener listener_;
    static void handle_global(void*, wl_registry*, uint32_t, const char*, uint32_t);
    static void handle_global_remove(void*, wl_registry*, uint32_t);

    friend class display;
};

} // namespace dwhbll::platform::wayland
