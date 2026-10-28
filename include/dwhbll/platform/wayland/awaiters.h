#pragma once

#include <dwhbll/concurrency/coroutine/cancellable_base.h>
#include <dwhbll/stl_ext/result.h>

#include <coroutine>
#include <wayland-client-protocol.h>

namespace dwhbll::platform::wayland {

class display;
class window;
class shm_swapchain;
class shm_buffer;

class sync_awaiter : public concurrency::coroutine::cancellable_base {
public:
    sync_awaiter(display& disp);
    ~sync_awaiter();

    [[nodiscard]] bool await_ready() const noexcept { return false; };
    void await_suspend(std::coroutine_handle<> h) noexcept;
    void await_resume();

private:
    display& disp_;
    wl_callback* cb_ = nullptr;
    std::coroutine_handle<> waiter_ {};

    static const wl_callback_listener listener_;
    static void on_done(void*, wl_callback*, uint32_t);
};


class frame_awaiter : public concurrency::coroutine::cancellable_base {
public:
    frame_awaiter(window& win) : win_(win) {};
    ~frame_awaiter();

    [[nodiscard]] bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) noexcept;
    uint32_t await_resume();

private:
    window& win_;
    wl_callback* cb_ = nullptr;
    std::coroutine_handle<> waiter_{};
    uint32_t frame_time_ = 0;

    static const wl_callback_listener listener_;
    static void on_done(void* data, wl_callback* cb, uint32_t time);
};

class configure_awaiter : public concurrency::coroutine::cancellable_base {
public:
    configure_awaiter(window& win) : win_(win) {};
    ~configure_awaiter();

    [[nodiscard]] bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> h) noexcept;
    void await_resume();

private:
    window& win_;
    std::coroutine_handle<> waiter_{};
    friend class window;
};

class registry;

class wait_global_awaiter : public concurrency::coroutine::cancellable_base {
public:
    wait_global_awaiter(registry& reg, const wl_interface* iface, uint32_t version);
    ~wait_global_awaiter();

    [[nodiscard]] bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> h) noexcept;
    stl_ext::Result<void*, std::string> await_resume();

private:
    registry& reg_;
    const wl_interface* iface_;
    uint32_t version_;
    uint64_t obs_id_ = 0;
    std::coroutine_handle<> waiter_{};
    bool done_ = false;
};

class shm_acquire_awaiter : public concurrency::coroutine::cancellable_base {
public:
    shm_acquire_awaiter(shm_swapchain& chain) : chain_(chain) {}
    ~shm_acquire_awaiter();

    [[nodiscard]] bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> h) noexcept;
    shm_buffer* await_resume();

private:
    shm_swapchain& chain_;
    std::coroutine_handle<> waiter_{};
    shm_buffer* acquired_ = nullptr;
    friend class shm_swapchain;
};

} // namespace dwhbll::platform::wayland
