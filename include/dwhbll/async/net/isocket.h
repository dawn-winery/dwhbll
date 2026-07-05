#pragma once

#include <dwhbll/concurrency/coroutine/task.h>
#include <dwhbll/stl_ext/result.h>

namespace dwhbll::network {
    struct address;
}

namespace dwhbll::async::net {
    class isocket {
    public:
        virtual ~isocket() = default;

        [[nodiscard]] virtual bool is_shutdown() const noexcept = 0;
        [[nodiscard]] virtual bool has_socket() const noexcept = 0;

        virtual void set_nodelay(bool state) noexcept = 0;
        [[nodiscard]] virtual bool get_nodelay() const noexcept = 0;

        virtual void close() noexcept = 0;

        [[nodiscard]] virtual const network::address& get_address() const noexcept = 0;

        [[nodiscard]] virtual concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> read(std::span<std::uint8_t> buffer) = 0;

        [[nodiscard]] virtual concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> write(std::span<const std::uint8_t> buffer) = 0;

        [[nodiscard]] virtual concurrency::coroutine::task<stl_ext::Result<ssize_t, int>> read_some(std::span<std::uint8_t> buffer) = 0;

        [[nodiscard]] virtual concurrency::coroutine::task<stl_ext::Result<ssize_t, int>> write_some(std::span<const std::uint8_t> buffer) = 0;

        [[nodiscard]] virtual concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> flush() = 0;
    };
}
