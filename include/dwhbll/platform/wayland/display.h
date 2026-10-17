#pragma once

#include <dwhbll/stl_ext/option.h>

#include <wayland-client.h>
#include <xdg-shell-client-protocol.h>

namespace dwhbll::platform::wayland {

class registry;
class sync_awaiter;

class display {
public:
    static stl_ext::Result<display, std::string>
    init(stl_ext::Option<std::string> name = stl_ext::None()) noexcept;

    ~display() noexcept;

    display(const display&) = delete;
    display& operator=(const display&) = delete;
    display(display&&) noexcept;
    display& operator=(display&&) noexcept;

    struct wl_display* wl_display() const noexcept { return display_; }
    wl_event_queue* event_queue() const noexcept { return queue_; }
    int get_fd() const noexcept { return fd_; }
    bool is_alive() const noexcept { return alive_; }

    registry& get_registry() noexcept;

    wl_compositor* compositor() noexcept;
    xdg_wm_base* wm_base() noexcept;

    bool prepare_poll() noexcept;
    int flush() noexcept;
    void on_readable() noexcept;
    void on_cancel() noexcept;
    void dispatch() noexcept;

    [[nodiscard]] sync_awaiter sync();

private:
    bool alive_ = true;
    struct wl_display* display_ = nullptr;
    struct wl_event_queue* queue_ = nullptr;
    int fd_ = -1;
    bool reading_ready_ = false;
    std::unique_ptr<registry> registry_;

    wl_compositor* compositor_ = nullptr;
    xdg_wm_base* wm_base_ = nullptr;

    display(struct wl_display* d, wl_event_queue* q);
};

} // namespace dwhbll::platform::wayland
