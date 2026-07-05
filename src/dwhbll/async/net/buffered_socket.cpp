#include <dwhbll/async/net/buffered_socket.h>

#include <dwhbll/sanify/coroutines.hpp>
#include <dwhbll/sanify/stl_ext.h>

namespace dwhbll::async::net {
    ssize_t buffered_socket::read_from_buffer(std::span<std::uint8_t> buffer) {
        auto copy_size = std::min(buffer.size(), inbound_size);

        std::copy_n(inbound_buffer.begin() + inbound_head, copy_size, buffer.begin());

        inbound_size -= copy_size;
        inbound_head += copy_size;
        return copy_size;
    }

    task<Result<UNIT, int>> buffered_socket::read(std::span<std::uint8_t> buffer) {
        auto buf = buffer;

        while (!buf.empty()) {
            auto r = co_await read_some(buf);

            if (r.is_err())
                co_return r.map(TO_UNIT);

            buf = buf.subspan(r.unwrap_unchecked());
        }

        co_return Ok();
    }

    task<Result<UNIT, int>> buffered_socket::write(std::span<const std::uint8_t> buffer) {
        co_return (co_await buffered_socket::write_some(buffer)).map(TO_UNIT);
    }

    task<Result<ssize_t, int>> buffered_socket::read_some(std::span<std::uint8_t> buffer) {
        auto requested = buffer.size();

        if (inbound_size != 0)
            co_return Ok(read_from_buffer(buffer));

        if (requested >= BUFFER_SIZE) {
            // huge
            co_return co_await decorated_socket::read_some(buffer);
        }

        auto r = co_await decorated_socket::read_some(inbound_buffer);
        inbound_size = BUFFER_SIZE;
        inbound_head = 0;

        co_return Ok(read_from_buffer(buffer));
    }

    task<Result<ssize_t, int>> buffered_socket::write_some(std::span<const std::uint8_t> buffer) {
        bool total_larger = outbound_size + buffer.size() >= BUFFER_SIZE;
        bool high_water = buffer.size() >= HIGH_WATERMARK;

        if (total_larger || high_water) {
            auto r = co_await flush();
            if (r.is_err())
                co_return r.map([](auto) -> ssize_t { debug::unreachable(); });
        }

        if (high_water)
            co_return (co_await decorated_socket::write(buffer)).map([&buffer](auto) -> ssize_t { return buffer.size(); });

        bool wrapped = outbound_head + outbound_size >= BUFFER_SIZE;
        bool will_wrap = outbound_head + outbound_size + buffer.size() >= BUFFER_SIZE;
        bool twocopy = will_wrap && !wrapped;

        std::size_t current_head_pos = wrapped ?
            outbound_head + outbound_size - BUFFER_SIZE :
            outbound_head + outbound_size;

        if (twocopy) {
            ssize_t first_end = BUFFER_SIZE - current_head_pos;

            std::copy_n(buffer.begin(), first_end, outbound_buffer.begin() + current_head_pos);
            std::copy(buffer.begin() + first_end, buffer.end(), outbound_buffer.begin());
        } else
            std::ranges::copy(buffer, outbound_buffer.begin() + current_head_pos);

        outbound_head += current_head_pos;
        if (outbound_head >= BUFFER_SIZE)
            outbound_head -= BUFFER_SIZE;
        outbound_size += buffer.size();

        co_return Ok(buffer.size());
    }

    task<Result<UNIT, int>> buffered_socket::flush() {
        auto r = co_await decorated_socket::write({
            outbound_buffer.begin() + outbound_head,
            outbound_buffer.begin() + std::min(BUFFER_SIZE, outbound_head + outbound_size)
        });

        if (r.is_err())
            co_return r;

        if (outbound_head + outbound_size >= BUFFER_SIZE) {
            r = co_await decorated_socket::write({
                outbound_buffer.begin(),
                outbound_buffer.begin() + outbound_head + outbound_size - BUFFER_SIZE
            });

            if (r.is_err())
                co_return r;
        }

        outbound_head = 0;
        outbound_size = 0;

        co_return Ok();
    }
}
