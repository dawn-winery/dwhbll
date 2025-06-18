#include <dwhbll/network/SocketManager.h>

#include <cstring>
#include <stdexcept>

#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <bits/this_thread_sleep.h>

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

    size_t Socket::send(const std::vector<char> &data) const {
        return ::send(fd, data.data(), data.size(), MSG_NOSIGNAL);
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

    Socket* SocketManager::getIPv4TCPSocket(in_addr addr, unsigned short port) {
        // TODO: DON'T BUSY WAIT!!!
        auto& byAddr = ipv4_sockets[addr.s_addr];
        auto& byPort = byAddr[port];
        auto& [tcp, udp] = byPort;
        if (tcp != nullptr) {
            while (tcp->held) {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(1ms);
            }
            pollfd req = {};
            req.fd = tcp->getNativeHandle();
            req.events = 0;
            if (tcp->mode != Socket::NONE) {
                poll(&req, 1, 0);
                if (req.revents & POLLIN != 0)
                    tcp->mode = Socket::NONE;
            }
            if (tcp->mode == Socket::NONE) {
                pool.offer(tcp);
                tcp = pool.acquire(::socket(AF_INET, SOCK_STREAM, 0), Socket::CONNECT).disown();
                tcp->connect(addr, port);
            }
        } else {
            tcp = pool.acquire(::socket(AF_INET, SOCK_STREAM, 0), Socket::CONNECT).disown();
            tcp->connect(addr, port);
        }
        return tcp;
    }

    Socket* SocketManager::getIPv4UDPSocket(in_addr addr, unsigned short port) {
        // TODO: DON'T BUSY WAIT!!!
        // TODO: implement ungetting and allow multiple users of the socket at once
        auto& byAddr = ipv4_sockets[addr.s_addr];
        auto& byPort = byAddr[port];
        auto& [tcp, udp] = byPort;
        if (udp != nullptr) {
            while (udp->held) {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(1ms);
            }
            pollfd req = {};
            req.fd = udp->getNativeHandle();
            req.events = 0;
            if (udp->mode != Socket::NONE) {
                poll(&req, 1, 0);
                if (req.revents & POLLIN != 0)
                    udp->mode = Socket::NONE;
            }
            if (udp->mode == Socket::NONE) {
                pool.offer(udp);
                udp = pool.acquire(::socket(AF_INET, SOCK_DGRAM, 0), Socket::CONNECT).disown();
                udp->connect(addr, port);
            }
        } else {
            auto i = ::socket(AF_INET, SOCK_DGRAM, 0);
            auto test = pool.acquire(i, Socket::CONNECT);
            udp = test.disown();
            udp->connect(addr, port);
        }
        return udp;
    }

    void SocketManager::offer(Socket *s) {
        // force the socket to be closed
        if (s->mode != Socket::NONE) {
            close(s->getNativeHandle());
            s->mode = Socket::NONE;
        }
        s->held = false;
    }
}
