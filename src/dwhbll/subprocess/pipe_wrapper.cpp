#include <cstring>
#include <iostream>
#include <dwhbll/exceptions/sys_error.h>

#ifdef __linux__
#include <fcntl.h>
#else
#include <cstdio>
#endif
#include <unistd.h>
#include <dwhbll/subprocess/pipe_wrapper.h>

namespace dwhbll::subprocess {
    std::optional<int> pipe_wrapper::devnull;
    pipe_wrapper::pipe_wrapper(const int fd) : fd(fd) {
#ifdef __linux__
        if (!devnull.has_value())
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
            if (result < 0 && errno == EAGAIN)
                continue; // keep busy waiting until we have data.
            if (result < 0)
                throw exceptions::sys_error(strerror(errno));

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
            const auto result = ::write(fd, data.data() + head, remaining);
            if (result < 0)
                throw exceptions::sys_error(strerror(errno));

            remaining -= result;
            head += result;
        }
    }

    std::expected<std::vector<char>, bool> pipe_wrapper::ll_read(size_t count) const {
        std::vector<char> buffer(count);
        const auto result = ::read(fd, buffer.data(), count);

        if (result < 0 && errno == EAGAIN)
            // no available data
            return {};

        if (result == 0)
            return std::unexpected(true); // pipe EOF.

        if (result < 0)
            throw exceptions::sys_error(strerror(errno));

        buffer.resize(result);

        return buffer;
    }

    size_t pipe_wrapper::ll_write(const std::span<char> &data) const {
        const auto result = ::write(fd, data.data(), data.size_bytes());

        if (result < 0)
            throw exceptions::sys_error(strerror(errno));

        return result;
    }
}
