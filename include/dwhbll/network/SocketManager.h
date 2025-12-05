#pragma once

#include <span>
#include <unordered_map>
#include <vector>

#include <arpa/inet.h>

#include "../memory/pool.h"

#define BuildIPV4(a, b, c, d) (htonl((((std::uint32_t)a) << 24 & 0xFF000000) | (((std::uint32_t)b) << 16 & 0xFF0000) | (((std::uint32_t)c) << 8 & 0xFF00) | (((std::uint32_t)d) & 0xFF)))

namespace dwhbll::network {
    class Socket {
        int fd;
        bool held = false;

        friend class SocketManager;

    public:
        enum Mode {
            NONE, ///< no mode set
            LISTEN, ///< this is a socket that is listening
            CONNECT, ///< this is a socket that is connected to a remote
        } mode; ///< the mode that the socket is currently in

        Socket(const Socket &other) = delete;

        Socket(Socket &&other) noexcept
            : fd(other.fd),
              mode(other.mode) {

            other.fd = -1;
            other.mode = NONE;
        }

        Socket & operator=(const Socket &other) = delete;

        Socket & operator=(Socket &&other) noexcept {
            if (this == &other)
                return *this;
            fd = other.fd;
            mode = other.mode;

            other.fd = -1;
            other.mode = NONE;

            return *this;
        }

        Socket() : fd(0), mode(NONE) {}
        Socket(const int fd, const Mode mode) : fd(fd), mode(mode) {}

        ~Socket();

        [[nodiscard]] int getNativeHandle() const;

        void connect(in_addr addr, unsigned short port) const;

        // TODO: bind, listen, accept wrappers

        /**
         * @brief wait for reading or writing to be available
         */
        void wait() const;

        /**
         * @brief send a set amount of data
         * @param data the buffer of data to send
         * @return sent size
         */
        size_t send(const std::span<char>& data) const;

        /**
         * @brief receive a set amount of data
        * @param data the buffer to put the data into, the receivable amount is data.size()
         * @return received size
         */
        size_t recv(std::span<char>& data) const;

        /**
         * @brief receive a set amount of data
        * @param data the buffer to put the data into, the receivable amount is data.size()
         * @return received size
         */
        size_t recv(std::vector<char>& data) const;

        /**
         * @brief send a set amount of data
         * @param data the buffer of data to send
         * @return sent size
         */
        size_t send(const std::string& data) const;

        /**
         * @brief receive a set amount of data
         * @param data the buffer to put the data into, the receivable amount is data.size()
         * @return received size
         */
        size_t recv(std::string& data) const;
    };

    // TODO: this class is not multithread safe
    // TODO: this class could give the same socket to multiple concurrent users!!!
    class SocketManager {
        memory::Pool<Socket> pool;

    public:
        memory::Pool<Socket>::ObjectWrapper getIPv4TCPSocket(in_addr addr, unsigned short port);

        memory::Pool<Socket>::ObjectWrapper getIPv4UDPSocket(in_addr addr, unsigned short port);

        /**
         * @brief Return the socket back to the manager
         * @param s the socket to return
         */
        void offer(Socket* s);
    };
}
