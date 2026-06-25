#include <dwhbll/async/net/socket.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <dwhbll/concurrency/coroutine/wrappers/syscall_wrappers.h>
#include <dwhbll/network/address.h>
#include <dwhbll/sanify/coroutines.hpp>
#include <dwhbll/stl_ext/option.h>

#include <netinet/tcp.h>

namespace dwhbll::async::net {
    task<stl_ext::Result<socket, int>> socket::connect_internal(bool use_ipv6, const network::address &endpoint, int socktype) {
        sockaddr_storage addr{};
        std::size_t addrlen{};
        switch (endpoint.type) {
        case network::address::DOMAIN:
            debug::todo();
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
        }

        auto sock = ::socket(use_ipv6 ? AF_INET6 : AF_INET, socktype, 0);

        if (sock == -1)
            co_return stl_ext::Err(errno);

        co_return (co_await calls::connect(sock, reinterpret_cast<sockaddr *>(&addr), addrlen)).map([sock](auto) {
            return socket{sock};
        });
    }

    socket::socket() = default;

    socket::socket(int fd) : fd(fd) {}

    socket::~socket() {
        close();
    }

    bool socket::is_shutdown() const noexcept {
        return shutdown;
    }

    bool socket::has_socket() const noexcept {
        return fd != -1;
    }

    void socket::set_nodelay(bool state) noexcept {
        nodelay_ = state;
        int val = state;
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) < 0) {
            debug::panic(strerror(errno));
        }
    }

    bool socket::get_nodelay() const noexcept {
        return nodelay_;
    }

    void socket::close() noexcept {
        if (fd == -1)
            return;

        ::shutdown(fd, SHUT_RDWR);
        ::close(fd);
        shutdown = true;
        fd = -1;
    }

    task<stl_ext::Result<socket, int>> socket::connect_tcp(bool use_ipv6, const network::address &endpoint) {
        return connect_internal(use_ipv6, endpoint, SOCK_STREAM);
    }

    task<stl_ext::Result<socket, int>> socket::connect_udp(bool use_ipv6, const network::address &endpoint) {
        return connect_internal(use_ipv6, endpoint, SOCK_DGRAM);
    }

    task<stl_ext::Result<ssize_t, int>> socket::read(std::span<std::uint8_t> buffer) const {
        if (!has_socket())
            debug::panic();
        co_return (co_await calls::recv(fd, buffer.data(), buffer.size(), 0));
    }

    task<stl_ext::Result<ssize_t, int>> socket::write(std::span<const std::uint8_t> buffer) const {
        if (!has_socket())
            debug::panic();
        co_return (co_await calls::send(fd, buffer.data(), buffer.size(), 0));
    }

    task<stl_ext::Result<stl_ext::UNIT, int>> socket::read_exact(std::span<std::uint8_t> buffer) const {
        if (!has_socket())
            debug::panic();

        auto* head = buffer.data();
        auto size = buffer.size();

        while (size != 0) {
            auto res = co_await calls::recv(fd, head, size, 0);
            if (res.is_err())
                co_return res.map(stl_ext::TO_UNIT);

            auto count = res.ok().unwrap();
            head += count;
            size -= count;
        }

        co_return stl_ext::Ok();
    }

    task<stl_ext::Result<stl_ext::UNIT, int>> socket::write_exact(std::span<const std::uint8_t> buffer) const {
        if (!has_socket())
            debug::panic();

        auto* head = buffer.data();
        auto size = buffer.size();

        while (size != 0) {
            auto res = co_await calls::send(fd, head, size, 0);
            if (res.is_err())
                co_return res.map(stl_ext::TO_UNIT);

            auto count = res.ok().unwrap();
            head += count;
            size -= count;
        }

        co_return stl_ext::Ok();
    }
}
