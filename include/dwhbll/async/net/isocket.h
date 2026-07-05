#pragma once

#include <dwhbll/concurrency/coroutine/task.h>
#include <dwhbll/stl_ext/result.h>

namespace dwhbll::async::net {
    class isocket {
    public:
        virtual ~isocket();

        [[nodiscard]] virtual concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> read(std::span<std::uint8_t> buffer) = 0;

        [[nodiscard]] virtual concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> write(std::span<const std::uint8_t> buffer) = 0;

        [[nodiscard]] virtual concurrency::coroutine::task<stl_ext::Result<ssize_t, int>> read_some(std::span<std::uint8_t> buffer) = 0;

        [[nodiscard]] virtual concurrency::coroutine::task<stl_ext::Result<ssize_t, int>> write_some(std::span<const std::uint8_t> buffer) = 0;

        [[nodiscard]] virtual concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> flush() = 0;
    };
}
