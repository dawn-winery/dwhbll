#include <dwhbll/async/net/tcp_listener.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <dwhbll/concurrency/coroutine/wrappers/syscall_wrappers.h>
#include <dwhbll/network/address.h>
#include <dwhbll/sanify/coroutines.hpp>
#include <dwhbll/sanify/stl_ext.h>

namespace dwhbll::async::net {
    tcp_listener::tcp_listener() = default;

    tcp_listener::~tcp_listener() {
        close();
    }

    bool tcp_listener::is_closed() const noexcept {
        return shutdown;
    }

    bool tcp_listener::has_socket() const noexcept {
        return fd_ != -1;
    }

    void tcp_listener::close() noexcept {
        ::close(fd_);
        shutdown = true;
        fd_ = -1;
    }

    Result<UNIT, int> tcp_listener::listen(const network::address &endpoint, int backlog) {
        sockaddr_storage addr{};
        std::size_t addrlen{};
        bool use_ipv6{};

        switch (endpoint.type) {
        case network::address::DOMAIN:
            debug::panic("binding to domain is not allowed.");
        case network::address::IPV4: {
            use_ipv6 = false;
            auto& v4addr = std::get<std::array<std::uint8_t, 4>>(endpoint.host);
            auto* v4 = reinterpret_cast<sockaddr_in*>(&addr);
            v4->sin_family = AF_INET;
            v4->sin_addr.s_addr = v4addr[3] << 24 | v4addr[2] << 16 | v4addr[1] << 8 | v4addr[0];
            v4->sin_port = htons(endpoint.port);
            addrlen = sizeof(sockaddr_in);
            break;
        }
        case network::address::IPV6:
            use_ipv6 = true;
            debug::todo();
            break;
        case network::address::EMPTY:
            debug::panic();
        }

        auto sock = ::socket(use_ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);

        if (sock < 0)
            return Err(errno);

        if (bind(sock, reinterpret_cast<sockaddr*>(&addr), addrlen) < 0)
            return Err(errno);

        if (::listen(sock, backlog) < 0)
            return Err(errno);

        fd_ = sock;

        return Ok();
    }

    task<Result<socket, int>> tcp_listener::accept() const noexcept {
        if (fd_ < 0)
            debug::panic("Socket not listening!");

        sockaddr_storage addr{};
        socklen_t addrlen{sizeof(sockaddr_storage)};

        auto sock = co_await calls::accept(fd_, reinterpret_cast<sockaddr*>(&addr), &addrlen, 0);

        if (sock.is_err())
            co_return Err(sock.unwrap_err_unchecked());

        co_return Ok(socket(sock.unwrap_unchecked(), [&] -> network::address {
            switch (addr.ss_family) {
                case AF_INET: {
                    auto* a = reinterpret_cast<sockaddr_in*>(&addr);
                    auto addr = a->sin_addr.s_addr;
                    return network::address(std::array{
                        static_cast<std::uint8_t>(addr & 0xFF),
                        static_cast<std::uint8_t>((addr >> 8) & 0xFF),
                        static_cast<std::uint8_t>((addr >> 16) & 0xFF),
                        static_cast<std::uint8_t>((addr >> 24) & 0xFF)
                    }, a->sin_port);
                }
                case AF_INET6:
                default:
                    debug::panic("Unrecognized sa_family type!");
            }
        }()));
    }

    Result<UNIT, int> tcp_listener::set_reuseaddr() const noexcept {
        if (!has_socket())
            debug::panic();

        int one = 1;
        auto r = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

        if (r == 0)
            return Ok();
        return Err(errno);
    }
}
