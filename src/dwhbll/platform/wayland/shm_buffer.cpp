#include <dwhbll/platform/wayland/shm_buffer.h>

#include <utility>

namespace dwhbll::platform::wayland {

const wl_buffer_listener shm_buffer::listener_ = {
    .release = [](void* data, struct wl_buffer*) {
        auto* self = static_cast<shm_buffer*>(data);
        self->busy_ = false;
        if (self->on_release_)
            self->on_release_(*self);
    }
};

shm_buffer::shm_buffer(struct wl_buffer* buf, uint8_t* data, uint32_t w, uint32_t h,
                       uint32_t stride, uint32_t format, uint32_t offset)
    : buffer_(buf),
      data_(data),
      width_(w),
      height_(h),
      stride_(stride),
      format_(format),
      offset_(offset),
      busy_(false)
{
    if (buffer_)
        wl_buffer_add_listener(buffer_, &listener_, this);
}

shm_buffer::~shm_buffer() noexcept {
    if (buffer_) {
        wl_buffer_destroy(buffer_);
        buffer_ = nullptr;
    }
}

shm_buffer::shm_buffer(shm_buffer&& o) noexcept
    : buffer_(std::exchange(o.buffer_, nullptr)),
      data_(std::exchange(o.data_, nullptr)),
      width_(o.width_),
      height_(o.height_),
      stride_(o.stride_),
      format_(o.format_),
      offset_(o.offset_),
      busy_(o.busy_),
      on_release_(std::move(o.on_release_))
{
    if (buffer_)
        wl_proxy_set_user_data(reinterpret_cast<wl_proxy*>(buffer_), this);
}

shm_buffer& shm_buffer::operator=(shm_buffer&& o) noexcept {
    if (this != &o) {
        if (buffer_)
            wl_buffer_destroy(buffer_);
        buffer_ = std::exchange(o.buffer_, nullptr);
        data_ = std::exchange(o.data_, nullptr);
        width_ = o.width_;
        height_ = o.height_;
        stride_ = o.stride_;
        format_ = o.format_;
        offset_ = o.offset_;
        busy_ = o.busy_;
        on_release_ = std::move(o.on_release_);

        if (buffer_)
            wl_proxy_set_user_data(reinterpret_cast<wl_proxy*>(buffer_), this);
    }
    return *this;
}

} // namespace dwhbll::platform::wayland
