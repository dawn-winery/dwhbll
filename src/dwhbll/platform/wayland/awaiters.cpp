#include <dwhbll/platform/wayland/awaiters.h>

#include <dwhbll/concurrency/coroutine/cancellation_exception.h>

#include <dwhbll/platform/wayland/display.h>
#include <dwhbll/platform/wayland/registry.h>
#include <dwhbll/platform/wayland/window.h>
#include <wayland-client.h>

namespace dwhbll::platform::wayland {

const wl_callback_listener sync_awaiter::listener_ = {
    .done = &sync_awaiter::on_done
};

sync_awaiter::sync_awaiter(display& disp) : disp_(disp) {
    cb_ = wl_display_sync(disp_.wl_display());
    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(cb_), disp_.event_queue());
    wl_callback_add_listener(cb_, &listener_, this);
    disp_.flush();
}

sync_awaiter::~sync_awaiter() {
    if (cb_)
        wl_callback_destroy(cb_);
}

void sync_awaiter::await_suspend(std::coroutine_handle<> h) noexcept {
    waiter_ = h;
}

void sync_awaiter::await_resume() {
    if (is_cancelled())
        throw concurrency::coroutine::cancellation_exception();
}

void sync_awaiter::on_done(void* data, wl_callback*, uint32_t) {
    auto* self = static_cast<sync_awaiter*>(data);
    wl_callback_destroy(self->cb_);
    self->cb_ = nullptr;
    if (self->waiter_)
        self->waiter_.resume();
}

const wl_callback_listener frame_awaiter::listener_ = {
    .done = &frame_awaiter::on_done
};

frame_awaiter::~frame_awaiter() {
    if (cb_)
        wl_callback_destroy(cb_);
}

void frame_awaiter::await_suspend(std::coroutine_handle<> h) noexcept {
    waiter_ = h;
    cb_ = wl_surface_frame(win_.surface_);
    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(cb_), win_.disp_.event_queue());
    wl_callback_add_listener(cb_, &listener_, this);
    win_.commit();
}

uint32_t frame_awaiter::await_resume() {
    if (is_cancelled())
        throw concurrency::coroutine::cancellation_exception();
    return frame_time_;
}

void frame_awaiter::on_done(void* data, wl_callback* cb, uint32_t time) {
    auto* self = static_cast<frame_awaiter*>(data);
    wl_callback_destroy(cb);
    self->cb_ = nullptr;
    self->frame_time_ = time;
    if (self->waiter_)
        self->waiter_.resume();
}

configure_awaiter::~configure_awaiter() {
    if (win_.configure_waiter_ == waiter_) {
        win_.configure_waiter_ = {};
    }
}

bool configure_awaiter::await_ready() const noexcept {
    return win_.configured_;
}

void configure_awaiter::await_suspend(std::coroutine_handle<> h) noexcept {
    waiter_ = h;
    win_.configure_waiter_ = h;
}

void configure_awaiter::await_resume() {
    if (is_cancelled())
        throw concurrency::coroutine::cancellation_exception();
}

wait_global_awaiter::wait_global_awaiter(registry& reg, const wl_interface* iface, uint32_t version)
    : reg_(reg), iface_(iface), version_(version) {}

wait_global_awaiter::~wait_global_awaiter() {
    if (obs_id_ != 0) {
        reg_.remove_observer(obs_id_);
    }
}

bool wait_global_awaiter::await_ready() const noexcept {
    return reg_.has(iface_->name);
}

void wait_global_awaiter::await_suspend(std::coroutine_handle<> h) noexcept {
    waiter_ = h;
    obs_id_ = reg_.add_observer(
        [this](const registry::global& g) {
            if (g.interface == iface_->name) {
                done_ = true;
                if (waiter_) {
                    auto h = std::exchange(waiter_, {});
                    h.resume();
                }
            }
        },
        {}
    );
}

stl_ext::Result<void*, std::string> wait_global_awaiter::await_resume() {
    if (is_cancelled())
        throw concurrency::coroutine::cancellation_exception();
    if (obs_id_ != 0) {
        reg_.remove_observer(obs_id_);
        obs_id_ = 0;
    }
    return reg_.bind_raw(iface_, version_);
}

} // namespace dwhbll::platform::wayland
