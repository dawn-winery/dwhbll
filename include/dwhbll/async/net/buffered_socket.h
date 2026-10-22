#pragma once

#include <dwhbll/async/net/decorated_socket.h>
#include <dwhbll/async/net/socket.h>

// TODO: make a templated IO buffer to use instead of reimplementing.
namespace dwhbll::async::net {
    class buffered_socket : public decorated_socket {
        constexpr static std::size_t BUFFER_SIZE = 4096;
        constexpr static std::size_t HIGH_WATERMARK = (BUFFER_SIZE / 4 * 3);

        std::array<std::uint8_t, BUFFER_SIZE> inbound_buffer{};
        size_t inbound_head = 0, inbound_size = 0;

        std::array<std::uint8_t, BUFFER_SIZE> outbound_buffer{};
        size_t outbound_head = 0, outbound_size = 0;

        ssize_t read_from_buffer(std::span<std::uint8_t> buffer);

    public:
        // Promoting constructor.
        buffered_socket(std::unique_ptr<isocket>&& super) : decorated_socket(std::move(super)) { }

        buffered_socket(const buffered_socket &other) = delete;

        buffered_socket(buffered_socket &&other) noexcept
            : decorated_socket(std::move(other)),
              inbound_buffer(other.inbound_buffer),
              inbound_head(other.inbound_head),
              inbound_size(other.inbound_size),
              outbound_buffer(other.outbound_buffer),
              outbound_head(other.outbound_head),
              outbound_size(other.outbound_size) {
        }

        buffered_socket & operator=(const buffered_socket &other) = delete;

        buffered_socket & operator=(buffered_socket &&other) noexcept {
            if (this == &other)
                return *this;
            decorated_socket::operator =(std::move(other));
            inbound_buffer = other.inbound_buffer;
            inbound_head = other.inbound_head;
            inbound_size = other.inbound_size;
            outbound_buffer = other.outbound_buffer;
            outbound_head = other.outbound_head;
            outbound_size = other.outbound_size;
            return *this;
        }

        concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> read(std::span<std::uint8_t> buffer) override;

        concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> write(std::span<const std::uint8_t> buffer) override;

        concurrency::coroutine::task<stl_ext::Result<ssize_t, int>> read_some(std::span<std::uint8_t> buffer) override;

        concurrency::coroutine::task<stl_ext::Result<ssize_t, int>> write_some(std::span<const std::uint8_t> buffer) override;

        concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> flush() override;
    };
}
