#pragma once

#include <dwhbll/async/net/socket.h>

namespace dwhbll::async::net {
    class buffered_socket : public socket {
        constexpr static std::size_t BUFFER_SIZE = 4096;
        constexpr static std::size_t HIGH_WATERMARK = (BUFFER_SIZE / 4 * 3);

        std::array<std::uint8_t, BUFFER_SIZE> inbound_buffer{};
        size_t inbound_head = 0, inbound_size = 0;

        std::array<std::uint8_t, BUFFER_SIZE> outbound_buffer{};
        size_t outbound_head = 0, outbound_size = 0;

    public:
        using socket::socket;

        // Promoting constructor.
        explicit buffered_socket(socket&& sock);

        concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> read(std::span<std::uint8_t> buffer) override;

        concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> write(std::span<const std::uint8_t> buffer) override;

        concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> flush() override;
    };
}
