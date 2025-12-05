#include <dwhbll/collections/memory_buffer.h>
#include <dwhbll/exceptions/rt_exception_base.h>

namespace dwhbll::collections {
    MemBuf::~MemBuf() = default;

    MemBuf::MemBuf(std::span<sanify::u8> buffer) {
        this->buffer.assign(buffer.begin(), buffer.end());
    }

    MemBuf::MemBuf(std::span<const sanify::u8> buffer) {
        this->buffer.assign(buffer.begin(), buffer.end());
    }

    MemBuf::MemBuf(std::size_t reserved_size) : buffer(reserved_size) {}

    MemBuf::MemBuf() : buffer(1024) {}

    void MemBuf::set_big_endian(bool endian) {
        big_endian = endian;
    }

    sanify::u8 MemBuf::read_u8() {
        sanify::u8 data = buffer.front();
        buffer.pop_front();
        return data;
    }

    sanify::u16 MemBuf::read_u16() {
        sanify::u16 a = static_cast<sanify::u16>(read_u8()) & 0xFF;
        sanify::u16 b = static_cast<sanify::u16>(read_u8()) & 0xFF;

        return big_endian ? a << 8 | b : b << 8 | a;
    }

    sanify::u32 MemBuf::read_u32() {
        sanify::u32 a = static_cast<sanify::u32>(read_u16()) & 0xFFFF;
        sanify::u32 b = static_cast<sanify::u32>(read_u16()) & 0xFFFF;

        return big_endian ? a << 16 | b : b << 16 | a;
    }

    sanify::u64 MemBuf::read_u64() {
        sanify::u64 a = static_cast<sanify::u64>(read_u32()) & 0xFFFFFFFF;
        sanify::u64 b = static_cast<sanify::u64>(read_u32()) & 0xFFFFFFFF;

        return big_endian ? a << 32 | b : b << 32 | a;
    }

    std::vector<sanify::u8> MemBuf::read_vector(std::size_t size) {
        std::vector<sanify::u8> result(size);

        // TODO: directly std::copy from the ring buffer!
        for (std::size_t i = 0; i < size; ++i)
            result[i] = read_u8();

        return result;
    }

    sanify::u8 MemBuf::peek_u8(std::size_t index) {
        return buffer[index];
    }

    sanify::u16 MemBuf::peek_u16(std::size_t index) {
        sanify::u16 a = static_cast<sanify::u16>(peek_u8(index)) & 0xFF;
        sanify::u16 b = static_cast<sanify::u16>(peek_u8(index + 1)) & 0xFF;

        return big_endian ? a << 8 | b : b << 8 | a;
    }

    sanify::u32 MemBuf::peek_u32(std::size_t index) {
        sanify::u32 a = static_cast<sanify::u32>(peek_u16(index)) & 0xFFFF;
        sanify::u32 b = static_cast<sanify::u32>(peek_u16(index + 1)) & 0xFFFF;

        return big_endian ? a << 16 | b : b << 16 | a;
    }

    sanify::u64 MemBuf::peek_u64(std::size_t index) {
        sanify::u64 a = static_cast<sanify::u64>(peek_u32(index)) & 0xFFFFFFFF;
        sanify::u64 b = static_cast<sanify::u64>(peek_u32(index + 1)) & 0xFFFFFFFF;

        return big_endian ? a << 32 | b : b << 32 | a;
    }

    std::vector<sanify::u8> MemBuf::peek_vector(std::size_t size, std::size_t index) {
        std::vector<sanify::u8> result(size);

        // TODO: directly std::copy from the ring buffer!
        for (std::size_t i = 0; i < size; ++i)
            result[i] = peek_u8(index + i);

        return result;
    }

    void MemBuf::write_u8(sanify::u8 data) {
        buffer.push_back(data);
    }

    void MemBuf::write_u16(sanify::u16 data) {
        if (big_endian) {
            write_u8(data >> 8);
            write_u8(data & 0xFF);
        } else {
            write_u8(data & 0xFF);
            write_u8(data >> 8);
        }
    }

    void MemBuf::write_u32(sanify::u32 data) {
        if (big_endian) {
            write_u16(data >> 16);
            write_u16(data & 0xFFFF);
        } else {
            write_u16(data & 0xFFFF);
            write_u16(data >> 16);
        }
    }

    void MemBuf::write_u64(sanify::u64 data) {
        if (big_endian) {
            write_u32(data >> 32);
            write_u32(data & 0xFFFFFFFF);
        } else {
            write_u32(data & 0xFFFFFFFF);
            write_u32(data >> 32);
        }
    }

    void MemBuf::write_vector(const std::span<sanify::u8> &data) {
        for (const auto& d : data)
            write_u8(d);
    }

    void MemBuf::write_string(const std::string &data) {
        for (const auto& c : data)
            write_u8(c);
    }

    std::size_t MemBuf::size() const {
        return buffer.size();
    }

    bool MemBuf::empty() const {
        return size() == 0;
    }

    void MemBuf::clear() {
        buffer.clear();
    }

    sanify::deferred MemBuf::lock() {
        return std::move(_lock.lock());
    }

    void MemBuf::refill_buffer() {
        throw exceptions::rt_exception_base("MemoryBuffer::refill_buffer is not implemented by default!");
    }
}
