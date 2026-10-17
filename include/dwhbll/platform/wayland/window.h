#pragma once

#include <dwhbll/platform/wayland/awaiters.h>
#include <dwhbll/stl_ext/result.h>

#include <xdg-shell-client-protocol.h>
#include <string>

namespace dwhbll::platform::wayland {

class display;

class window {
public:
    static stl_ext::Result<window, std::string>
    init(display& disp, uint32_t width = 1280, uint32_t height = 720,
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

    [[nodiscard]] wl_surface* surface() const noexcept { return surface_; }

    void commit();

    [[nodiscard]] configure_awaiter wait_for_configure() noexcept;
    [[nodiscard]] frame_awaiter next_frame() noexcept;

private:
    display& disp_;
    wl_surface* surface_ = nullptr;
    xdg_surface* xdg_surface_ = nullptr;
    xdg_toplevel* xdg_toplevel_ = nullptr;

    uint32_t width_;
    uint32_t height_;
    bool configured_ = false;
    bool should_close_ = false;

    std::coroutine_handle<> configure_waiter_{};

    window(display& disp, uint32_t width, uint32_t height,
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
