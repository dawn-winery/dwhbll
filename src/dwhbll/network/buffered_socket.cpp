#include "../../../include/dwhbll/network/buffered_socket.h"

#include <dwhbll/console/debug.hpp>
#include <dwhbll/exceptions/rt_exception_base.h>
#include <dwhbll/sanify/coroutines.hpp>

namespace dwhbll::network {
    inbound_network_buffer::inbound_network_buffer(memory::Pool<Socket>::ObjectWrapper &socket) : socket(socket), ParseUtils() {}

    void inbound_network_buffer::refill_buffer() {
        // TODO: zero copy this part in the future
        std::vector<char> buffer(1024);

        auto recv_count = socket->read(buffer);

        // TODO: what is this lmao
        if ((recv_count == -1 && (errno != EAGAIN && errno != EWOULDBLOCK)) || recv_count == 0)
            return;

        if (recv_count == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return;

        buffer.resize(recv_count);
        write_vector(std::span{*reinterpret_cast<std::vector<sanify::u8> *>(&buffer)});
    }

    task<> inbound_network_buffer::refill_buffer_async() {
        // TODO: zero copy this part in the future
        std::vector<char> buffer(1024);

        auto recv_count = await socket->read_async(buffer);

        // TODO: what is this lmao
        if ((recv_count == -1 && (errno != EAGAIN && errno != EWOULDBLOCK)) || recv_count == 0)
            co_return;

        if (recv_count == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            co_return;
    }

    outbound_network_buffer::outbound_network_buffer(memory::Pool<Socket>::ObjectWrapper &socket) : socket(socket) {}

    void outbound_network_buffer::flush() {
        buffer.make_cont();

        socket->send(std::span{reinterpret_cast<char *>(buffer.data().data()), buffer.size()});

        buffer.clear();
    }

    task<> outbound_network_buffer::flush_async() {
        buffer.make_cont();

        await socket->send_async(std::span{reinterpret_cast<char *>(buffer.data().data()), buffer.size()});

        buffer.clear();
    }

    buffered_socket::buffered_socket() : socket(nullptr, nullptr), inbound(this->socket), outbound(this->socket) {}

    buffered_socket::buffered_socket(memory::Pool<Socket>::ObjectWrapper &&socket) : socket(std::move(socket)), inbound(this->socket), outbound(this->socket) {
    }

    buffered_socket::buffered_socket(buffered_socket &&other) noexcept: socket(std::move(other.socket)),
                                                                        inbound(this->socket),
                                                                        outbound(this->socket) {
        // TODO: fix this by using std::shared_ptr.
        if (!other.inbound.empty() || !other.outbound.empty())
            debug::panic("buffered_socket was moved with data inside the buffers! this is not allowed!");
    }

    void buffered_socket::flush_outbound() {
        outbound.flush();
    }

    task<> buffered_socket::flush_outbound_async() {
        return outbound.flush_async();
    }

    memory::Pool<Socket>::ObjectWrapper & buffered_socket::socket_ref() {
        return socket;
    }
}
