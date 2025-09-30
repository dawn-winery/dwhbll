#pragma once

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
         * @param data
         */
        void write(const std::span<char>& data) const;
    };
}
