#include <dwhbll/platform/wayland/display.h>

#include <dwhbll/platform/wayland/awaiters.h>
#include <dwhbll/platform/wayland/registry.h>

#include <dwhbll/stl_ext/result.h>

using namespace dwhbll::stl_ext;

namespace dwhbll::platform::wayland {

const xdg_wm_base_listener wm_base_listener_ = {
    .ping = [](void*, xdg_wm_base* wm, uint32_t serial) {
        xdg_wm_base_pong(wm, serial);
    }
};

const wl_shm_listener display::shm_listener_ = {
    .format = [](void* data, wl_shm*, uint32_t format) {
        auto* self = static_cast<display*>(data);
        self->shm_formats_.push_back(format);
    }
};

Result<display, std::string> display::init(Option<std::string> name) noexcept {
    struct wl_display* disp = wl_display_connect(std::move(name).map_or(nullptr,
                [](std::string&& s) {
                    return s.c_str();
                }));
    if (!disp)
        return "failed to connect to display";

    wl_event_queue* queue = wl_display_create_queue_with_name(disp, "dwhbll_wl");
    if (!queue)
        return "failed to create dwhbll event queue";

    return Ok(display(disp, queue));
}

display::~display() noexcept {
    if (shm_)
        wl_shm_destroy(shm_);
    if (wm_base_)
        xdg_wm_base_destroy(wm_base_);
    if (compositor_)
        wl_compositor_destroy(compositor_);
    registry_.reset();
    if (queue_)
        wl_event_queue_destroy(queue_);
    if (display_)
        wl_display_disconnect(display_);
}

display::display(display&& o) noexcept
    : alive_(std::exchange(o.alive_, false)),
      display_(std::exchange(o.display_, nullptr)),
      queue_(std::exchange(o.queue_, nullptr)),
      fd_(std::exchange(o.fd_, -1)),
      reading_ready_(std::exchange(o.reading_ready_, false)),
      registry_(std::move(o.registry_)),
      compositor_(std::exchange(o.compositor_, nullptr)),
      wm_base_(std::exchange(o.wm_base_, nullptr)),
      shm_(std::exchange(o.shm_, nullptr)),
      shm_formats_(std::move(o.shm_formats_))
{
    if (registry_)
        registry_->display_ = this;
    if (shm_)
        wl_proxy_set_user_data(reinterpret_cast<wl_proxy*>(shm_), this);
}

display& display::operator=(display&& o) noexcept {
    if (this != &o) {
        this->~display();
        new (this) display(std::move(o));
    }

    return *this;
}

bool display::prepare_poll() noexcept {
    if (!alive_)
        return false;

    while (wl_display_prepare_read_queue(display_, queue_) != 0)
        wl_display_dispatch_queue_pending(display_, queue_);

    reading_ready_ = true;
    flush();
    return true;
}

int display::flush() noexcept {
    if (!alive_)
        return -1;

    int ret = wl_display_flush(display_);
    if (ret < 0 && errno != EAGAIN)
        alive_ = false;
    return ret;
}

void display::on_readable() noexcept {
    if (!alive_)
        return;
    ASSERT(reading_ready_);

    reading_ready_ = false;
    if (wl_display_read_events(display_) == 0)
        wl_display_dispatch_queue_pending(display_, queue_);
    else
        alive_ = false;
}

void display::on_cancel() noexcept {
    if (reading_ready_) {
        wl_display_cancel_read(display_);
        reading_ready_ = false;
    }
}

void display::dispatch() noexcept {
    if (alive_)
        wl_display_dispatch_queue_pending(display_, queue_);
}

sync_awaiter display::sync() {
    return sync_awaiter(*this);
}

void display::sync(std::function<void()> on_done) {
    // TODO: Surely there's gotta be a better way to do this?
    struct sync_ctx {
        std::function<void()> cb;
    };
    auto* ctx = new sync_ctx{std::move(on_done)};
    static const wl_callback_listener sync_cb_listener = {
        .done = [](void* data, wl_callback* cb, uint32_t) {
            auto* c = static_cast<sync_ctx*>(data);
            if (c->cb)
                c->cb();
            delete c;
            wl_callback_destroy(cb);
        }
    };
    wl_callback* cb = wl_display_sync(display_);
    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(cb), queue_);
    wl_callback_add_listener(cb, &sync_cb_listener, ctx);
    flush();
}

display::display(struct wl_display* d, wl_event_queue* q)
    : display_(d),
      queue_(q),
      fd_(wl_display_get_fd(display_)),
      registry_(std::make_unique<registry>(*this))
{
    registry_->add_observer(
        [this](const registry::global& g) {
            if (g.interface == "wl_compositor" && !compositor_) {
                auto res = registry_->bind_raw(&wl_compositor_interface, 4);
                if (res.is_ok())
                    compositor_ = static_cast<wl_compositor*>(res.unwrap());
            } else if (g.interface == "xdg_wm_base" && !wm_base_) {
                auto res = registry_->bind_raw(&xdg_wm_base_interface, 3);
                if (res.is_ok()) {
                    wm_base_ = static_cast<xdg_wm_base*>(res.unwrap());
                    xdg_wm_base_add_listener(wm_base_, &wm_base_listener_, nullptr);
                }
            } else if (g.interface == "wl_shm" && !shm_) {
                auto res = registry_->bind_raw(&wl_shm_interface, 1);
                if (res.is_ok()) {
                    shm_ = static_cast<wl_shm*>(res.unwrap());
                    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(shm_), queue_);
                    wl_shm_add_listener(shm_, &shm_listener_, this);
                }
            }
        });
}

registry& display::get_registry() noexcept {
    return *registry_;
}

wl_compositor* display::compositor() noexcept {
    if (!compositor_ && registry_ && registry_->has("wl_compositor")) {
        auto res = registry_->bind_raw(&wl_compositor_interface, 4);
        if (res.is_ok())
            compositor_ = static_cast<wl_compositor*>(res.unwrap());
    }
    return compositor_;
}

xdg_wm_base* display::wm_base() noexcept {
    if (!wm_base_ && registry_ && registry_->has("xdg_wm_base")) {
        auto res = registry_->bind_raw(&xdg_wm_base_interface, 1);
        if (res.is_ok()) {
            wm_base_ = static_cast<xdg_wm_base*>(res.unwrap());
            xdg_wm_base_add_listener(wm_base_, &wm_base_listener_, nullptr);
        }
    }
    return wm_base_;
}

wl_shm* display::shm() noexcept {
    if (!shm_ && registry_ && registry_->has("wl_shm")) {
        auto res = registry_->bind_raw(&wl_shm_interface, 1);
        if (res.is_ok()) {
            shm_ = static_cast<wl_shm*>(res.unwrap());
            wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(shm_), queue_);
            wl_shm_add_listener(shm_, &shm_listener_, this);
        }
    }
    return shm_;
}

} // namespace dwhbll::platform::wayland
