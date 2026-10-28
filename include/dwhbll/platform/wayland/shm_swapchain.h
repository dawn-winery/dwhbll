#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>
#include <deque>
#include <coroutine>

#include <dwhbll/stl_ext/result.h>
#include <dwhbll/stl_ext/option.h>
#include <dwhbll/platform/wayland/shm_pool.h>
#include <dwhbll/platform/wayland/shm_buffer.h>
#include <dwhbll/platform/wayland/awaiters.h>

#include <wayland-client.h>

namespace dwhbll::platform::wayland {

class display;
class window;

class shm_swapchain {
public:
    static stl_ext::Result<shm_swapchain, std::string>
    create(window& win, uint32_t buffer_count = 2, uint32_t format = WL_SHM_FORMAT_ARGB8888);

    static stl_ext::Result<shm_swapchain, std::string>
    create(display& disp, wl_surface* surf, uint32_t width, uint32_t height,
           uint32_t buffer_count = 2, uint32_t format = WL_SHM_FORMAT_ARGB8888);

    // TODO: Should probably cancel the acquire_awaiters coroutines
    // ~shm_swapchain() noexcept;

    shm_swapchain(const shm_swapchain&) = delete;
    shm_swapchain& operator=(const shm_swapchain&) = delete;

    shm_swapchain(shm_swapchain&& o) noexcept;
    shm_swapchain& operator=(shm_swapchain&& o) noexcept;

    [[nodiscard]] uint32_t buffer_count() const noexcept { return buffer_count_; }
    [[nodiscard]] uint32_t width() const noexcept { return width_; }
    [[nodiscard]] uint32_t height() const noexcept { return height_; }
    [[nodiscard]] uint32_t format() const noexcept { return format_; }

    [[nodiscard]] std::span<shm_buffer> buffers() noexcept { return buffers_; }

    [[nodiscard]] shm_buffer* try_acquire() noexcept;
    [[nodiscard]] shm_acquire_awaiter acquire() noexcept;

    void present(shm_buffer& buf) noexcept;

    stl_ext::Result<stl_ext::UNIT, std::string> resize(uint32_t width, uint32_t height);

    void set_on_buffer_released(std::function<void(shm_buffer&)> cb) {
        on_buffer_released_ = std::move(cb);
    }

private:
    display* disp_ = nullptr;
    wl_surface* surface_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t format_ = WL_SHM_FORMAT_ARGB8888;
    uint32_t buffer_count_ = 0;

    stl_ext::Option<shm_pool> pool_ = stl_ext::None();
    std::vector<shm_buffer> buffers_;
    std::deque<std::coroutine_handle<>> acquire_waiters_;
    std::function<void(shm_buffer&)> on_buffer_released_;

    shm_swapchain(display& disp, wl_surface* surf, uint32_t w, uint32_t h,
                  uint32_t count, uint32_t format);

    stl_ext::Result<stl_ext::UNIT, std::string> allocate_buffers();
    void handle_buffer_released(shm_buffer& buf);

    friend class shm_acquire_awaiter;
};

} // namespace dwhbll::platform::wayland
