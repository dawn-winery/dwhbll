#pragma once

#include <dwhbll/async/net/socket.h>
#include <dwhbll/collections/streams.hpp>
#include <dwhbll/concurrency/coroutine/task.h>
#include <dwhbll/stl_ext/result.h>

namespace dwhbll::network {
    struct address;
}

// TODO: move constructors delete copy.

namespace dwhbll::async::net {
    class tcp_listener {
        int fd_{-1};

        bool shutdown {false};
        bool want_reuseaddr {false};

    public:
        tcp_listener();

        ~tcp_listener();

        [[nodiscard]] bool is_closed() const noexcept;
        [[nodiscard]] bool has_socket() const noexcept;

        void close() noexcept;

        /**
         * @brief Setup this socket as a listening socket.
         * @param endpoint Bind point
         * @param backlog Number of connections waiting for the process.
         * @return Ok on success or errno on Err.
         */
        [[nodiscard]] stl_ext::Result<stl_ext::UNIT, int> listen(const network::address &endpoint, int backlog = 4096);

        [[nodiscard]] concurrency::coroutine::task<stl_ext::Result<std::unique_ptr<socket>, int>> accept() const noexcept;

        void set_reuseaddr() noexcept;
    };
}
