#include <dwhbll/network/SocketManager.h>

#include <cstring>
#include <stdexcept>

#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <dwhbll/concurrency/coroutine/wrappers/syscall_wrappers.h>
#include <dwhbll/sanify/coroutines.hpp>

namespace dwhbll::network {
    Socket::~Socket() {
        if (mode != NONE) {
            shutdown(fd, SHUT_RDWR);
            close(fd);
            mode = NONE;
        }
    }

    int Socket::getNativeHandle() const {
        return fd;
    }

    /**
     * @brief connect this socket
     * @param addr the address to connect to
     * @param port the port to connect to
     * @throws std::runtime_error when socket connection failed.
     */
    void Socket::connect(in_addr addr, unsigned short port) const {
        sockaddr_in a = {
            AF_INET,
            htons(port),
            addr
        };
        auto res = ::connect(fd, reinterpret_cast<sockaddr *>(&a), sizeof(a));
        if (res == -1) {
            // failure
            auto e = errno;
            throw std::runtime_error("Failed to connect to socket");
        }
    }

    task<> Socket::connect_async(in_addr addr, unsigned short port) const {
        sockaddr_in a = {
            AF_INET,
            htons(port),
            addr
        };

        await calls::connect(fd, reinterpret_cast<sockaddr *>(&a), sizeof(a));
    }

    void Socket::wait() const {
        if (mode == NONE) {
            throw std::runtime_error("cannot wait on a socket with mode NONE");
        }

        pollfd req = {};
        req.fd = fd;
        if (mode == LISTEN) {
            req.events = POLLIN;
            poll(&req, 1, -1);
        } else if (mode == CONNECT) {
            req.events = POLLOUT;
            poll(&req, 1, -1);
        }
    }

    task<> Socket::wait_async() const {
        if (mode == NONE) {
            throw std::runtime_error("cannot wait on a socket with mode NONE");
        }

        if (mode == LISTEN)
            await calls::poll(fd, POLLIN);
        else if (mode == CONNECT)
            await calls::poll(fd, POLLOUT);
    }

    ssize_t Socket::send(const std::span<char> &data) const {
        return ::send(fd, data.data(), data.size(), MSG_NOSIGNAL);
    }

    // todo: return the awaitable directly let the user handle it instead.
    task<ssize_t> Socket::send_async(const std::span<char> &data) const {
        return calls::send(fd, data.data(), data.size(), MSG_NOSIGNAL);
    }

    ssize_t Socket::recv(std::span<char> &data) const {
        return ::recv(fd, data.data(), data.size(), 0);
    }

    task<ssize_t> Socket::recv_async(std::span<char> &data) const {
        return calls::recv(fd, data.data(), data.size(), 0);
    }

    ssize_t Socket::recv(std::vector<char> &data) const {
        return ::recv(fd, data.data(), data.size(), 0);
    }

    task<ssize_t> Socket::recv_async(std::vector<char> &data) const {
        return calls::recv(fd, data.data(), data.size(), 0);
    }

    ssize_t Socket::read(std::vector<char> &data) const {
        return ::read(fd, data.data(), data.size());
    }

    task<ssize_t> Socket::read_async(std::vector<char> &data) const {
        return calls::read(fd, data.data(), data.size(), 0);
    }

    ssize_t Socket::send(const std::string &data) const {
        return ::send(fd, data.data(), data.size(), MSG_NOSIGNAL);
    }

    task<ssize_t> Socket::send_async(const std::string &data) const {
        return calls::send(fd, data.data(), data.size(), MSG_NOSIGNAL);
    }

    ssize_t Socket::recv(std::string &data) const {
        return ::recv(fd, data.data(), data.size(), 0);
    }

    task<ssize_t> Socket::recv_async(std::string &data) const {
        return calls::recv(fd, data.data(), data.size(), 0);
    }

    task<Socket> Socket::accept() const {
        auto f = await calls::accept(fd, nullptr, nullptr, 0);
        co_return std::move(Socket{f, CONNECT});
    }

    SocketManager::socket_t SocketManager::getIPv4TCPSocket(in_addr addr, unsigned short port) {
        auto tcp = pool.acquire(::socket(AF_INET, SOCK_STREAM, 0), Socket::CONNECT);
        tcp->connect(addr, port);
        return std::move(tcp);
    }

    task<SocketManager::socket_t> SocketManager::getIPv4TCPSocket_async(in_addr addr,
    unsigned short port) {
        auto tcp = pool.acquire(::socket(AF_INET, SOCK_STREAM, 0), Socket::CONNECT);
        await tcp->connect_async(addr, port);
        co_return std::move(tcp);
    }

    SocketManager::socket_t SocketManager::getIPv4UDPSocket(in_addr addr, unsigned short port) {
        auto i = ::socket(AF_INET, SOCK_DGRAM, 0);
        auto udp = pool.acquire(i, Socket::CONNECT);
        udp->connect(addr, port);
        return std::move(udp);
    }

    task<SocketManager::socket_t> SocketManager::getIPv4UDPSocket_async(in_addr addr,
    unsigned short port) {
        auto i = ::socket(AF_INET, SOCK_DGRAM, 0);
        auto udp = pool.acquire(i, Socket::CONNECT);
        await udp->connect_async(addr, port);
        co_return std::move(udp);
    }

    SocketManager::socket_t SocketManager::listenTCP(in_addr_t addr, unsigned short port) {
        auto i = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        auto tcp = pool.acquire(i, Socket::LISTEN);
        struct sockaddr_in a = {
            .sin_family = AF_INET,
            .sin_port   = htons(8080),
            .sin_addr   = { .s_addr = addr }
        };
        ::bind(i, (struct sockaddr *)&a, sizeof(a));
        listen(i, 64);

        return std::move(tcp);
    }

    void SocketManager::offer(Socket *s) {
        // force the socket to be closed
        if (s->mode != Socket::NONE) {
            shutdown(s->fd, SHUT_RDWR);
            close(s->getNativeHandle());
            s->mode = Socket::NONE;
        }
        s->held = false;

        pool.offer(s);
    }
}
