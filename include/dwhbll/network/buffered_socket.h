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
    };

    class outbound_network_buffer : public files::ParseUtils {
        memory::Pool<Socket>::ObjectWrapper& socket;

    public:
        outbound_network_buffer(memory::Pool<Socket>::ObjectWrapper& socket);

        void flush();
    };

    class buffered_socket {
        memory::Pool<Socket>::ObjectWrapper socket;

    public:
        inbound_network_buffer inbound;
        outbound_network_buffer outbound;

        buffered_socket(memory::Pool<Socket>::ObjectWrapper &&socket);

        void flush_outbound();

        memory::Pool<Socket>::ObjectWrapper& socket_ref();
    };
}
