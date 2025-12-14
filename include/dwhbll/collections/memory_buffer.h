#pragma once

#include <span>

#include <dwhbll/collections/ring.h>
#include <dwhbll/concurrency/spinlock.h>
#include <dwhbll/concurrency/coroutine/task.h>
#include <dwhbll/sanify/types.hpp>

namespace dwhbll::collections {
    /**
     * @brief Small memory buffer for buffers to be worked on in memory.
     */
    class MemBuf {
    protected:
        Ring<sanify::u8> buffer;

        std::unique_ptr<concurrency::spinlock> _lock;

        bool big_endian = false;

    public:
        virtual ~MemBuf();

        explicit MemBuf(std::span<sanify::u8> buffer);

        explicit MemBuf(std::span<const sanify::u8> buffer);

        explicit MemBuf(std::size_t reserved_size);

        MemBuf(const MemBuf &other) = delete;

        MemBuf(MemBuf &&other) noexcept;

        MemBuf & operator=(const MemBuf &other) = delete;

        MemBuf & operator=(MemBuf &&other) noexcept;

        /**
         * @brief if no data is given, the buffer is set to a size of 1024 by default.
         */
        MemBuf();

        /**
         * @return the current endianness of this stream.
         */
        [[nodiscard]] constexpr bool get_is_big_endian() const {
            return big_endian;
        }

        /**
         * @param endian whether this stream should be read/write as big_endian
         */
        void set_big_endian(bool endian);

        sanify::u8 read_u8();

        sanify::u16 read_u16();

        sanify::u32 read_u32();

        sanify::u64 read_u64();

        std::vector<sanify::u8> read_vector(std::size_t size);

        void skip(std::size_t count);

        sanify::u8 peek_u8(std::size_t index = 0);

        sanify::u16 peek_u16(std::size_t index = 0);

        sanify::u32 peek_u32(std::size_t index = 0);

        sanify::u64 peek_u64(std::size_t index = 0);

        std::vector<sanify::u8> peek_vector(std::size_t size, std::size_t index = 0);

        void write_u8(sanify::u8 data);

        void write_u16(sanify::u16 data);

        void write_u32(sanify::u32 data);

        void write_u64(sanify::u64 data);

        void write_vector(const std::span<sanify::u8>& data);

        void write_string(const std::string& data);

        [[nodiscard]] std::size_t size() const;

        [[nodiscard]] bool empty() const;

        void clear();

        [[nodiscard]] sanify::deferred lock();

        virtual void refill_buffer();

        virtual concurrency::coroutine::task<> refill_buffer_async();

        Ring<sanify::u8>& get_raw_buffer();
    };
}
