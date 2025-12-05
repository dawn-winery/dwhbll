#include "../../../include/dwhbll/network/buffered_socket.h"

namespace dwhbll::network {
    inbound_network_buffer::inbound_network_buffer(memory::Pool<Socket>::ObjectWrapper &socket) : socket(socket), ParseUtils() {}

    void inbound_network_buffer::refill_buffer() {
        // TODO: zero copy this part in the future
        std::vector<char> buffer(1024);

        auto recv_count = socket->recv(buffer);

        // TODO: what is this lmao
        if (recv_count == -1 || recv_count == 0)
            return;

        buffer.resize(recv_count);
        write_vector(std::span{*reinterpret_cast<std::vector<sanify::u8> *>(&buffer)});
    }

    outbound_network_buffer::outbound_network_buffer(memory::Pool<Socket>::ObjectWrapper &socket) : socket(socket) {}

    void outbound_network_buffer::flush() {
        buffer.make_cont();

        socket->send(std::span{reinterpret_cast<char *>(buffer.data().data()), buffer.size()});

        buffer.clear();
    }

    buffered_socket::buffered_socket(memory::Pool<Socket>::ObjectWrapper &&socket) : socket(std::move(socket)), inbound(this->socket), outbound(this->socket) {}

    void buffered_socket::flush_outbound() {
        outbound.flush();
    }

    memory::Pool<Socket>::ObjectWrapper & buffered_socket::socket_ref() {
        return socket;
    }
}
