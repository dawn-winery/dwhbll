#pragma once

#include <dwhbll/files/parse_utils.h>
#include <dwhbll/memory/pool.h>
#include <dwhbll/network/SocketManager.h>

namespace dwhbll::network {
    class inbound_network_buffer : public files::ParseUtils {
        memory::Pool<Socket>::ObjectWrapper& socket;

    public:
        inbound_network_buffer(memory::Pool<Socket>::ObjectWrapper& socket);

        void refill_buffer() override;

        concurrency::coroutine::task<> refill_buffer_async() override;
    };

    class outbound_network_buffer : public files::ParseUtils {
        memory::Pool<Socket>::ObjectWrapper& socket;

    public:
        outbound_network_buffer(memory::Pool<Socket>::ObjectWrapper& socket);

        void flush();

        concurrency::coroutine::task<> flush_async();
    };

    class buffered_socket {
        memory::Pool<Socket>::ObjectWrapper socket;

    public:
        inbound_network_buffer inbound;
        outbound_network_buffer outbound;

        buffered_socket();

        buffered_socket(memory::Pool<Socket>::ObjectWrapper &&socket);

        buffered_socket(const buffered_socket &other) = delete;

        buffered_socket(buffered_socket &&other) noexcept;

        buffered_socket & operator=(const buffered_socket &other) = delete;

        buffered_socket & operator=(buffered_socket &&other) noexcept = delete;

        void flush_outbound();

        concurrency::coroutine::task<> flush_outbound_async();

        memory::Pool<Socket>::ObjectWrapper& socket_ref();
    };
}
