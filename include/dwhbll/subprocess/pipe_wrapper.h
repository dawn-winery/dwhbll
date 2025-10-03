#pragma once

#include <expected>
#include <memory>
#include <vector>

namespace dwhbll::subprocess {
    class pipe_wrapper {
        int fd;

#ifdef __linux__
        static std::optional<int> devnull;
#endif

    public:
        explicit pipe_wrapper(int fd);

        void available_to_null() const;

        /**
         * @brief reads count bytes from the pipe, this will read exactly this many and will block until it is satisfied.
         * @warning this is a busy wait!
         * @param count the number of bytes to read
         * @return a vector containing the read data.
         */
        [[nodiscard]] std::vector<char> read(size_t count) const;

        /**
         * @brief skips count number of bytes from the pipe, this will skip exactly count bytes and will block if not met!
         * @param count the number of bytes to read
         */
        void skip(size_t count) const;

        /**
         * @brief writes a span to the pipe, this will write the entire span's data!
         * @warning this is a busy wait!
         * @param data
         */
        void write(const std::span<char>& data) const;

        /**
         * @brief Reads n bytes from stream, or however amount is available in the pipe up to the available pipe buffer size.
         * If the requested size is bigger than the available buffer size, this will not wait for more data!
         * @param count the number of bytes to try to read
         * @return a vector containing the read data or an unexpect value of true if EOF
         */
        [[nodiscard]] std::expected<std::vector<char>, bool> ll_read(size_t count) const;

        /**
         * @brief writes a span to the pipe, this will only write up to the available amount of space in the pipe buffer
         * @param data the data to try and write to the pipe
         * @return the total number of bytes written, may not be the total length of the span!
         */
        [[nodiscard]] size_t ll_write(const std::span<char>& data) const;
    };
}
