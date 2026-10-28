#pragma once

#include <dwhbll/platform/wayland/awaiters.h>
#include <dwhbll/stl_ext/result.h>

#include <xdg-shell-client-protocol.h>
#include <string>

namespace dwhbll::platform::wayland {

class display;
class shm_swapchain;
class shm_buffer;

class window {
public:
    static stl_ext::Result<window, std::string>
    init(class display& disp, uint32_t width = 1280, uint32_t height = 720,
         std::string title = "dwhbll window", std::string app_id = "dwhbll");
    ~window();

    window(const window&) = delete;
    window& operator=(const window&) = delete;
    window(window&&);
    window& operator=(window&&);

    [[nodiscard]] uint32_t width() const noexcept { return width_; }
    [[nodiscard]] uint32_t height() const noexcept { return height_; }
    [[nodiscard]] bool should_close() const noexcept { return should_close_; }
    [[nodiscard]] bool is_configured() const noexcept { return configured_; }

    [[nodiscard]] class display& display() const noexcept { return disp_; }
    [[nodiscard]] wl_surface* surface() const noexcept { return surface_; }

    void commit();
    void damage(int32_t x, int32_t y, int32_t width, int32_t height);

    void set_on_configure(std::function<void(uint32_t w, uint32_t h)> cb) {
        on_configure_ = std::move(cb);
    }
    void set_on_close(std::function<void()> cb) {
        on_close_ = std::move(cb);
    }
    void request_frame(std::function<void(uint32_t time)> cb);

    [[nodiscard]] configure_awaiter wait_for_configure() noexcept;
    [[nodiscard]] frame_awaiter next_frame() noexcept;

    stl_ext::Result<shm_swapchain*, std::string>
    init_shm(uint32_t buffer_count = 2, uint32_t format = WL_SHM_FORMAT_ARGB8888);

    [[nodiscard]] bool has_shm() const noexcept { return shm_ != nullptr; }
    [[nodiscard]] shm_swapchain* shm() noexcept { return shm_.get(); }

    [[nodiscard]] shm_buffer* try_acquire_buffer() noexcept;
    // UB if shm is not initialized
    shm_acquire_awaiter acquire() noexcept;

    void present(shm_buffer& buf) noexcept;

private:
    class display& disp_;
    wl_surface* surface_ = nullptr;
    xdg_surface* xdg_surface_ = nullptr;
    xdg_toplevel* xdg_toplevel_ = nullptr;

    uint32_t width_;
    uint32_t height_;
    bool configured_ = false;
    bool should_close_ = false;

    std::coroutine_handle<> configure_waiter_{};
    std::unique_ptr<shm_swapchain> shm_;

    std::function<void(uint32_t, uint32_t)> on_configure_;
    std::function<void()> on_close_;

    window(class display& disp, uint32_t width, uint32_t height,
           std::string title, std::string app_id,
           wl_compositor* comp, xdg_wm_base* wm_base);

    static const xdg_surface_listener surface_listener_;
    static const xdg_toplevel_listener toplevel_listener_;

    static void handle_surface_configure(void*, xdg_surface*, uint32_t);
    static void handle_toplevel_configure(void*, xdg_toplevel*, int32_t, int32_t, wl_array*);
    static void handle_toplevel_close(void*, xdg_toplevel*);
    static void handle_configure_bounds(void*, xdg_toplevel*, int32_t, int32_t);
    static void handle_wm_capabilities(void*, xdg_toplevel*, wl_array*);

    friend class configure_awaiter;
    friend class frame_awaiter;
};

} // namespace dwhbll::platform::wayland
