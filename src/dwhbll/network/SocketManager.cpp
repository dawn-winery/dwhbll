#include <dwhbll/network/SocketManager.h>

#include <cstring>
#include <stdexcept>

#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace dwhbll::network {
    Socket::~Socket() {
        close(fd);
        mode = NONE;
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

    size_t Socket::send(const std::span<char> &data) const {
        return ::send(fd, data.data(), data.size(), MSG_NOSIGNAL);
    }

    size_t Socket::recv(std::span<char> &data) const {
        return ::recv(fd, data.data(), data.size(), 0);
    }

    size_t Socket::recv(std::vector<char> &data) const {
        return ::recv(fd, data.data(), data.size(), 0);
    }

    size_t Socket::send(const std::string &data) const {
        return ::send(fd, data.data(), data.size(), MSG_NOSIGNAL);
    }

    size_t Socket::recv(std::string &data) const {
        return ::recv(fd, data.data(), data.size(), 0);
    }

    memory::Pool<Socket>::ObjectWrapper SocketManager::getIPv4TCPSocket(in_addr addr, unsigned short port) {
        auto tcp = pool.acquire(::socket(AF_INET, SOCK_STREAM, 0), Socket::CONNECT);
        tcp->connect(addr, port);
        return std::move(tcp);
    }

    memory::Pool<Socket>::ObjectWrapper SocketManager::getIPv4UDPSocket(in_addr addr, unsigned short port) {
        auto i = ::socket(AF_INET, SOCK_DGRAM, 0);
        auto udp = pool.acquire(i, Socket::CONNECT);
        udp->connect(addr, port);
        return std::move(udp);
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
