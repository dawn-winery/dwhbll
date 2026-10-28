#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <dwhbll/stl_ext/result.h>
#include <dwhbll/platform/wayland/shm_buffer.h>

#include <wayland-client.h>

namespace dwhbll::platform::wayland {

class display;

class shm_pool {
public:
    static stl_ext::Result<shm_pool, std::string> create(display& disp, size_t size);

    ~shm_pool() noexcept;

    shm_pool(const shm_pool&) = delete;
    shm_pool& operator=(const shm_pool&) = delete;

    shm_pool(shm_pool&& o) noexcept;
    shm_pool& operator=(shm_pool&& o) noexcept;

    [[nodiscard]] size_t size() const noexcept { return size_; }
    [[nodiscard]] int fd() const noexcept { return fd_; }
    [[nodiscard]] uint8_t* data() noexcept { return data_; }
    [[nodiscard]] const uint8_t* data() const noexcept { return data_; }
    [[nodiscard]] wl_shm_pool* wl_pool() const noexcept { return pool_; }
    [[nodiscard]] display& get_display() const noexcept { return *disp_; }

    stl_ext::Result<stl_ext::UNIT, std::string> resize(size_t new_size);

    stl_ext::Result<shm_buffer, std::string>
    create_buffer(uint32_t offset, uint32_t width, uint32_t height,
                  uint32_t stride, uint32_t format = WL_SHM_FORMAT_ARGB8888);

private:
    display* disp_ = nullptr;
    int fd_ = -1;
    size_t size_ = 0;
    uint8_t* data_ = nullptr;
    wl_shm_pool* pool_ = nullptr;

    shm_pool(display& disp, int fd, size_t size, uint8_t* data, wl_shm_pool* pool);
};

} // namespace dwhbll::platform::wayland
