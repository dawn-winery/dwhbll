#include <dwhbll/platform/wayland/shm_swapchain.h>
#include <dwhbll/platform/wayland/display.h>
#include <dwhbll/platform/wayland/window.h>

#include <utility>

using namespace dwhbll::stl_ext;

namespace dwhbll::platform::wayland {

Result<shm_swapchain, std::string>
shm_swapchain::create(window& win, uint32_t buffer_count, uint32_t format) {
    return create(win.display(), win.surface(), win.width(), win.height(),
                  buffer_count, format);
}

Result<shm_swapchain, std::string>
shm_swapchain::create(display& disp, wl_surface* surf, uint32_t width, uint32_t height,
                      uint32_t buffer_count, uint32_t format)
{
    if (!surf)
        return "surface cannot be null";
    if (width == 0 || height == 0)
        return "width and height must be greater than 0";
    if (buffer_count == 0)
        return "buffer_count must be greater than 0";

    shm_swapchain chain(disp, surf, width, height, buffer_count, format);
    auto res = chain.allocate_buffers();
    if (!res.is_ok())
        return res.unwrap_err();

    return Ok(std::move(chain));
}

shm_swapchain::shm_swapchain(display& disp, wl_surface* surf, uint32_t w, uint32_t h,
                             uint32_t count, uint32_t format)
    : disp_(&disp),
      surface_(surf),
      width_(w),
      height_(h),
      format_(format),
      buffer_count_(count)
{}

shm_swapchain::shm_swapchain(shm_swapchain&& o) noexcept
    : disp_(std::exchange(o.disp_, nullptr)),
      surface_(std::exchange(o.surface_, nullptr)),
      width_(std::exchange(o.width_, 0)),
      height_(std::exchange(o.height_, 0)),
      format_(std::exchange(o.format_, WL_SHM_FORMAT_ARGB8888)),
      buffer_count_(std::exchange(o.buffer_count_, 0)),
      pool_(std::move(o.pool_)),
      buffers_(std::move(o.buffers_)),
      acquire_waiters_(std::move(o.acquire_waiters_)),
      on_buffer_released_(std::move(o.on_buffer_released_))
{
    for (auto& buf : buffers_) {
        buf.set_on_release([this](shm_buffer& released) {
            this->handle_buffer_released(released);
        });
    }
}

shm_swapchain& shm_swapchain::operator=(shm_swapchain&& o) noexcept {
    if (this != &o) {
        disp_ = std::exchange(o.disp_, nullptr);
        surface_ = std::exchange(o.surface_, nullptr);
        width_ = std::exchange(o.width_, 0);
        height_ = std::exchange(o.height_, 0);
        format_ = std::exchange(o.format_, WL_SHM_FORMAT_ARGB8888);
        buffer_count_ = std::exchange(o.buffer_count_, 0);
        pool_ = std::move(o.pool_);
        buffers_ = std::move(o.buffers_);
        acquire_waiters_ = std::move(o.acquire_waiters_);
        on_buffer_released_ = std::move(o.on_buffer_released_);

        for (auto& buf : buffers_) {
            buf.set_on_release([this](shm_buffer& released) {
                this->handle_buffer_released(released);
            });
        }
    }
    return *this;
}

Result<UNIT, std::string> shm_swapchain::allocate_buffers() {
    uint32_t stride = width_ * 4;
    size_t buf_size = stride * height_;
    size_t total_size = buf_size * buffer_count_;

    auto pool_res = shm_pool::create(*disp_, total_size);
    if (!pool_res.is_ok())
        return pool_res.unwrap_err();

    pool_ = Some(std::move(pool_res.unwrap()));
    buffers_.clear();
    buffers_.reserve(buffer_count_);

    for (uint32_t i = 0; i < buffer_count_; ++i) {
        auto buf_res = pool_.unwrap().create_buffer(i * buf_size, width_,
                                                    height_, stride, format_);
        if (!buf_res.is_ok())
            return buf_res.unwrap_err();

        buffers_.push_back(std::move(buf_res.unwrap()));
    }

    for (auto& buf : buffers_) {
        buf.set_on_release([this](shm_buffer& released) {
            this->handle_buffer_released(released);
        });
    }
    return Ok();
}

void shm_swapchain::handle_buffer_released(shm_buffer& buf) {
    if (!acquire_waiters_.empty()) {
        auto h = acquire_waiters_.front();
        acquire_waiters_.pop_front();
        if (h)
            h.resume();
    }

    if (on_buffer_released_)
        on_buffer_released_(buf);
}

shm_buffer* shm_swapchain::try_acquire() noexcept {
    for (auto& b : buffers_) {
        if (!b.is_busy())
            return &b;
    }
    return nullptr;
}

shm_acquire_awaiter shm_swapchain::acquire() noexcept {
    return shm_acquire_awaiter(*this);
}

void shm_swapchain::present(shm_buffer& buf) noexcept {
    buf.set_busy(true);
    wl_surface_attach(surface_, buf.wl_buffer(), 0, 0);
    wl_surface_damage(surface_, 0, 0, static_cast<int32_t>(buf.width()), static_cast<int32_t>(buf.height()));
    wl_surface_commit(surface_);
    disp_->flush();
}

Result<UNIT, std::string> shm_swapchain::resize(uint32_t width, uint32_t height) {
    if (width == width_ && height == height_)
        return Ok();

    width_ = width;
    height_ = height;
    return allocate_buffers();
}

} // namespace dwhbll::platform::wayland
