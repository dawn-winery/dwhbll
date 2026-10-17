#include <dwhbll/platform/wayland/window.h>

#include <dwhbll/platform/wayland/display.h>

#include <wayland-client.h>
#include <xdg-shell-client-protocol.h>

using namespace dwhbll::stl_ext;

namespace dwhbll::platform::wayland {

const xdg_surface_listener window::surface_listener_ = {
    .configure = &window::handle_surface_configure,
};

const xdg_toplevel_listener window::toplevel_listener_ = {
    .configure = &window::handle_toplevel_configure,
    .close = &window::handle_toplevel_close,
    .configure_bounds = &window::handle_configure_bounds,
    .wm_capabilities = &window::handle_wm_capabilities,
};

void window::handle_surface_configure(void* data, xdg_surface* xdg_surf, uint32_t serial) {
    auto* self = static_cast<window*>(data);
    xdg_surface_ack_configure(xdg_surf, serial);
    self->configured_ = true;
    if (self->configure_waiter_) {
        auto h = std::exchange(self->configure_waiter_, {});
        h.resume();
    }
}

void window::handle_toplevel_configure(void* data, xdg_toplevel*,
                                       int32_t w, int32_t h, wl_array*) {
    auto* self = static_cast<window*>(data);
    if (w > 0)
        self->width_ = static_cast<uint32_t>(w);
    if (h > 0)
        self->height_ = static_cast<uint32_t>(h);
}

void window::handle_toplevel_close(void* data, xdg_toplevel*) {
    static_cast<window*>(data)->should_close_ = true;
}

void window::handle_configure_bounds(void*, xdg_toplevel*, int32_t, int32_t) {}

void window::handle_wm_capabilities(void*, xdg_toplevel*, wl_array*) {
    // TODO
}

Result<window, std::string> window::init(display& disp, uint32_t width, uint32_t height,
                                         std::string title, std::string app_id)
{
    auto* compositor = disp.compositor();
    if (!compositor)
        return "wl_compositor not available on display";

    auto* wm_base = disp.wm_base();
    if (!wm_base)
        return "xdg_wm_base not available on display";

    return Ok(window(disp, width, height, title, app_id, compositor, wm_base));
}

window::window(display& disp, uint32_t width, uint32_t height,
               std::string title, std::string app_id,
               wl_compositor* comp, xdg_wm_base* wm_base)
    : disp_(disp),
      width_(width),
      height_(height)
{
    surface_ = wl_compositor_create_surface(comp);
    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(surface_), disp_.event_queue());

    xdg_surface_ = xdg_wm_base_get_xdg_surface(wm_base, surface_);
    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(xdg_surface_), disp_.event_queue());
    xdg_surface_add_listener(xdg_surface_, &surface_listener_, this);

    xdg_toplevel_ = xdg_surface_get_toplevel(xdg_surface_);
    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(xdg_toplevel_), disp_.event_queue());
    xdg_toplevel_add_listener(xdg_toplevel_, &toplevel_listener_, this);

    xdg_toplevel_set_title(xdg_toplevel_, title.c_str());
    xdg_toplevel_set_app_id(xdg_toplevel_, app_id.c_str());

    wl_surface_commit(surface_);
    disp_.flush();
}

window::~window() {
    if (xdg_toplevel_)
        xdg_toplevel_destroy(xdg_toplevel_);
    if (xdg_surface_)
        xdg_surface_destroy(xdg_surface_);
    if (surface_)
        wl_surface_destroy(surface_);
}

window::window(window&& o)
    : disp_(o.disp_),
      surface_(std::exchange(o.surface_, nullptr)),
      xdg_surface_(std::exchange(o.xdg_surface_, nullptr)),
      xdg_toplevel_(std::exchange(o.xdg_toplevel_, nullptr)),
      width_(o.width_),
      height_(o.height_),
      configured_(o.configured_),
      should_close_(o.should_close_),
      configure_waiter_(std::exchange(o.configure_waiter_, {}))
{
    // this took me like 40 minutes to figure out ffs
    if (xdg_surface_)
        wl_proxy_set_user_data(reinterpret_cast<wl_proxy*>(xdg_surface_), this);
    if (xdg_toplevel_)
        wl_proxy_set_user_data(reinterpret_cast<wl_proxy*>(xdg_toplevel_), this);
}

window& window::operator=(window&& o) {
    if (this != &o) {
        this->~window();
        new (this) window(std::move(o));
    }

    return *this;
}

void window::commit() {
    wl_surface_commit(surface_);
    disp_.flush();
}

configure_awaiter window::wait_for_configure() noexcept {
    return configure_awaiter(*this);
}

frame_awaiter window::next_frame() noexcept {
    return frame_awaiter(*this);
}

} // namespace dwhbll::platform::wayland
