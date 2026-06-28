#pragma once

#include <dwhbll/concurrency/coroutine/task.h>
#include <dwhbll/network/address.h>
#include <dwhbll/stl_ext/result.h>

namespace dwhbll::async::net {
    class socket {
        int fd{-1};
        network::address addr{};

        bool shutdown {false};
        bool nodelay_ {false};

        static concurrency::coroutine::task<stl_ext::Result<socket, int>> connect_internal(bool use_ipv6, const network::address &endpoint, int socktype);

        [[nodiscard]] concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> read_exact0(std::span<std::uint8_t> buffer);

        [[nodiscard]] concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> write_exact0(std::span<const std::uint8_t> buffer) const;

    public:
        socket();

        explicit socket(int fd);

        socket(int fd, network::address addr);

        virtual ~socket();

        socket(const socket &other) = delete;

        socket(socket &&other) noexcept;

        socket & operator=(const socket &other) = delete;

        socket & operator=(socket &&other) noexcept;

        [[nodiscard]] bool is_shutdown() const noexcept;
        [[nodiscard]] bool has_socket() const noexcept;

        void set_nodelay(bool state) noexcept;
        [[nodiscard]] bool get_nodelay() const noexcept;

        void close() noexcept;

        [[nodiscard]] const network::address& get_address() const noexcept;

        /**
         * @brief Connect TCP Socket
         * @param use_ipv6 Whether to use IPv6, if the endpoint is specified as v4 ip it doesn't matter, if DNS resolves to only v4 it also doesn't matter
         * @param endpoint Address to connect to, or domain, domain will be resolved
         * @return Socket if connected
         */
        [[nodiscard]] static concurrency::coroutine::task<stl_ext::Result<socket, int>> connect_tcp(bool use_ipv6, const network::address &endpoint);

        /**
         * @brief Connect UDP Socket
         * @param use_ipv6 Whether to use IPv6, if the endpoint is specified as v4 ip it doesn't matter, if DNS resolves to only v4 it also doesn't matter
         * @param endpoint Address to connect to, or domain, domain will be resolved
         * @return Socket if connected
         */
        [[nodiscard]] static concurrency::coroutine::task<stl_ext::Result<socket, int>> connect_udp(bool use_ipv6, const network::address &endpoint);

        [[nodiscard]] virtual concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> read(std::span<std::uint8_t> buffer);

        [[nodiscard]] virtual concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> write(std::span<const std::uint8_t> buffer);

        [[nodiscard]] virtual concurrency::coroutine::task<stl_ext::Result<ssize_t, int>> read_some(std::span<std::uint8_t> buffer);

        [[nodiscard]] virtual concurrency::coroutine::task<stl_ext::Result<ssize_t, int>> write_some(std::span<const std::uint8_t> buffer);

        [[nodiscard]] virtual concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> flush();
    };
}
