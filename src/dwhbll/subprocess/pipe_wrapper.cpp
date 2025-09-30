#ifdef __linux__
#include <fcntl.h>
#else
#include <cstdio>
#endif
#include <unistd.h>
#include <dwhbll/subprocess/pipe_wrapper.h>

namespace dwhbll::subprocess {
    pipe_wrapper::pipe_wrapper(const int fd) : fd(fd) {
#ifdef __linux__
        if (devnull.has_value())
            devnull = open("/dev/null", O_WRONLY);
#endif
    }

    void pipe_wrapper::available_to_null() const {
#ifdef __linux__
        while (splice(fd, nullptr, devnull.value(), nullptr, 65535, SPLICE_F_MOVE) != 0)
            ;
#else
        char buffer[65536];
        while (read(fd, buffer, sizeof(buffer)) == sizeof(buffer))
            ;
#endif
    }

    std::vector<char> pipe_wrapper::read(const size_t count) const {
        std::vector<char> buffer(count);
        size_t remaining = count;
        size_t head = 0;

        while (remaining != 0) {
            const auto result = ::read(fd, buffer.data() + head, count);
            if (result < 0)
                throw std::system_error(errno, std::system_category());

            remaining -= result;
            head += result;
        }

        return buffer;
    }

    void pipe_wrapper::skip(const size_t count) const {
        size_t remaining = count;
#ifdef __linux__
        while (remaining != 0) {
            const auto result = splice(fd, nullptr, devnull.value(), nullptr, remaining, SPLICE_F_MOVE);
            if (result < 0)
                throw std::runtime_error("some error occurred");
            remaining -= result;
        }
#else
        char buffer[65536];
        while (remaining != 0) {
            const auto result = read(fd, buffer, remaining);
            if (result < 0)
                throw std::runtime_error("some error occurred");
            remaining -= result;
        }
#endif
    }

    void pipe_wrapper::write(const std::span<char> &data) const {
        size_t remaining = data.size_bytes();
        size_t head = 0;
        while (remaining != 0) {
            const auto result = ::write(fd, data.data() + head, data.size_bytes());
            if (result < 0)
                throw std::system_error(errno, std::system_category());

            remaining -= result;
            head += result;
        }
    }
}
