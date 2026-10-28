#include <dwhbll/platform/wayland/shm_pool.h>
#include <dwhbll/platform/wayland/display.h>

#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <utility>

using namespace dwhbll::stl_ext;

namespace dwhbll::platform::wayland {

Result<shm_pool, std::string> shm_pool::create(display& disp, size_t size) {
    wl_shm* shm = disp.shm();
    if (!shm)
        return "wl_shm is not available";

    if (size == 0)
        return "pool size must be greater than 0";

    int fd = memfd_create("dwhbll_shm_pool", MFD_CLOEXEC);
    if (fd < 0)
        return "memfd_create failed: " + std::string(strerror(errno));

    if (ftruncate(fd, size) < 0) {
        int err = errno;
        close(fd);
        return "ftruncate failed: " + std::string(strerror(err));
    }

    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        int err = errno;
        close(fd);
        return "mmap failed: " + std::string(strerror(err));
    }

    wl_shm_pool* pool = wl_shm_create_pool(shm, fd, size);
    if (!pool) {
        munmap(ptr, size);
        close(fd);
        return "wl_shm_create_pool failed";
    }

    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(pool), disp.event_queue());

    return Ok(shm_pool(disp, fd, size, static_cast<uint8_t*>(ptr), pool));
}

shm_pool::shm_pool(display& disp, int fd, size_t size, uint8_t* data, wl_shm_pool* pool)
    : disp_(&disp), fd_(fd), size_(size), data_(data), pool_(pool) {}

shm_pool::~shm_pool() noexcept {
    if (pool_)
        wl_shm_pool_destroy(pool_);
    if (data_ && data_ != MAP_FAILED)
        munmap(data_, size_);
    if (fd_ >= 0)
        close(fd_);
}

shm_pool::shm_pool(shm_pool&& o) noexcept
    : disp_(std::exchange(o.disp_, nullptr)),
      fd_(std::exchange(o.fd_, -1)),
      size_(std::exchange(o.size_, 0)),
      data_(std::exchange(o.data_, nullptr)),
      pool_(std::exchange(o.pool_, nullptr))
{}

shm_pool& shm_pool::operator=(shm_pool&& o) noexcept {
    if (this != &o) {
        if (pool_)
            wl_shm_pool_destroy(pool_);
        if (data_ && data_ != MAP_FAILED)
            munmap(data_, size_);
        if (fd_ >= 0)
            close(fd_);

        disp_ = std::exchange(o.disp_, nullptr);
        fd_ = std::exchange(o.fd_, -1);
        size_ = std::exchange(o.size_, 0);
        data_ = std::exchange(o.data_, nullptr);
        pool_ = std::exchange(o.pool_, nullptr);
    }
    return *this;
}

Result<UNIT, std::string> shm_pool::resize(size_t new_size) {
    if (new_size <= size_)
        return Ok();

    if (ftruncate(fd_, new_size) < 0)
        return "ftruncate failed: " + std::string(strerror(errno));

    void* new_ptr = mremap(data_, size_, new_size, MREMAP_MAYMOVE);
    if (new_ptr == MAP_FAILED) {
        munmap(data_, size_);
        new_ptr = mmap(nullptr, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (new_ptr == MAP_FAILED)
            return "mmap failed: " + std::string(strerror(errno));
    }

    data_ = static_cast<uint8_t*>(new_ptr);
    size_ = new_size;
    wl_shm_pool_resize(pool_, new_size);
    return Ok();
}

Result<shm_buffer, std::string>
shm_pool::create_buffer(uint32_t offset, uint32_t width, uint32_t height,
                        uint32_t stride, uint32_t format)
{
    size_t required = offset + stride * height;
    if (required > size_)
        return "buffer size is over capacity";

    wl_buffer* buf = wl_shm_pool_create_buffer(pool_, offset, width,
                                               height, stride, format);
    if (!buf)
        return "wl_shm_pool_create_buffer failed";

    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(buf), disp_->event_queue());

    return Ok(shm_buffer(buf, data_ + offset, width, height, stride, format, offset));
}

} // namespace dwhbll::platform::wayland
