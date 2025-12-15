#pragma once

#include <filesystem>
#include <string>
#include <dwhbll/collections/memory_buffer.h>
#include <dwhbll/concurrency/coroutine/task.h>

namespace dwhbll::concurrency::coroutine::wrappers {
    /**
     * @brief simple wrappers for common tasks one would want to do asynchronously
     * @note the standard constructor and destructor are sync! they may block for a little bit although usually that is
     * fine, if one wants true async behavior, await the static open, and per instance close() before destructor.
     */
    class file {
        constexpr static int batch_read_count = 65536; // read 65536 bytes at a time.

        int fd = -1;

        off_t read_head{0}, write_head{0};
        bool eof_ = false;
        collections::MemBuf rdbuf, wrbuf;

        static int compute_openmode_flags(std::ios::openmode mode);

        explicit file(int fd);

        task<bool> try_flush_wrbuf();

    public:
        file();

        file(const file &other) = delete;

        file(file &&other) noexcept;

        file & operator=(const file &other) = delete;

        file & operator=(file &&other) noexcept;

        explicit file(const char* path, std::ios::openmode mode = std::ios::in | std::ios::out);

        explicit file(const std::string& path, std::ios::openmode mode = std::ios::in | std::ios::out);

        explicit file(const std::filesystem::path& path, std::ios::openmode mode = std::ios::in | std::ios::out);

        ~file();

        static task<file> open(const char* path, std::ios::openmode mode = std::ios::in | std::ios::out);

        static task<file> open(const std::string& path, std::ios::openmode mode = std::ios::in | std::ios::out);

        static task<file> open(const std::filesystem::path& path, std::ios::openmode mode = std::ios::in | std::ios::out);

        task<> close();

        task<std::vector<char>> read(int n=-1);

        task<std::string> read_str(int n=-1);

        task<std::vector<char>> readexactly(int n);

        task<> write(const std::span<char>& data);

        task<> drain();

        void seekg(off_t head);

        void seekp(off_t head);

        bool is_eof() const;

        [[nodiscard]] bool is_open() const;
    };
}
