#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <algorithm>
#include <wayland-client.h>

namespace dwhbll::platform::wayland {

class shm_pool;
class shm_swapchain;

class shm_buffer {
public:
    shm_buffer() noexcept = default;
    ~shm_buffer() noexcept;

    shm_buffer(const shm_buffer&) = delete;
    shm_buffer& operator=(const shm_buffer&) = delete;

    shm_buffer(shm_buffer&& o) noexcept;
    shm_buffer& operator=(shm_buffer&& o) noexcept;

    [[nodiscard]] bool valid() const noexcept { return buffer_ != nullptr; }
    [[nodiscard]] struct wl_buffer* wl_buffer() const noexcept { return buffer_; }

    [[nodiscard]] uint32_t width() const noexcept { return width_; }
    [[nodiscard]] uint32_t height() const noexcept { return height_; }
    [[nodiscard]] uint32_t stride() const noexcept { return stride_; }
    [[nodiscard]] uint32_t format() const noexcept { return format_; }
    [[nodiscard]] uint32_t offset() const noexcept { return offset_; }
    [[nodiscard]] size_t size() const noexcept { return stride_ * height_; }

    [[nodiscard]] bool is_busy() const noexcept { return busy_; }
    void set_busy(bool busy) noexcept { busy_ = busy; }

    // TODO: add const variants of these 4 methods?
    [[nodiscard]] uint8_t* data() noexcept { return data_; }

    template <typename T = uint32_t>
    [[nodiscard]] std::span<T> pixels_as() noexcept {
        if (!data_ || sizeof(T) == 0)
            return {};
        return std::span<T>(reinterpret_cast<T*>(data_), (stride_ * height_) / sizeof(T));
    }

    [[nodiscard]] std::span<uint32_t> pixels() noexcept {
        return pixels_as<uint32_t>();
    }

    template <typename T = uint32_t>
    [[nodiscard]] std::span<T> row(uint32_t y) noexcept {
        if (!data_ || y >= height_ || sizeof(T) == 0)
            return {};
        return std::span<T>(reinterpret_cast<T*>(data_ + y * stride_), stride_ / sizeof(T));
    }

    void clear(uint32_t argb) noexcept {
        auto px = pixels();
        std::fill(px.begin(), px.end(), argb);
    }

    void set_on_release(std::function<void(shm_buffer&)> cb) {
        on_release_ = std::move(cb);
    }

private:
    struct wl_buffer* buffer_ = nullptr;
    uint8_t* data_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t stride_ = 0;
    uint32_t format_ = 0;
    uint32_t offset_ = 0;
    bool busy_ = false;

    std::function<void(shm_buffer&)> on_release_;

    static const struct wl_buffer_listener listener_;

    shm_buffer(struct wl_buffer* buf, uint8_t* data, uint32_t w, uint32_t h,
               uint32_t stride, uint32_t format, uint32_t offset);

    friend class shm_pool;
    friend class shm_swapchain;
};

} // namespace dwhbll::platform::wayland
