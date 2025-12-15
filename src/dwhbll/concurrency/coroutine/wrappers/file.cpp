#include <dwhbll/concurrency/coroutine/wrappers/file.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>
#include <dwhbll/concurrency/coroutine/wrappers/syscall_wrappers.h>
#include <dwhbll/console/debug.hpp>
#include <dwhbll/console/Logging.h>
#include <dwhbll/sanify/coroutines.hpp>

namespace dwhbll::concurrency::coroutine::wrappers {
    int file::compute_openmode_flags(std::ios::openmode mode) {
        int flags = O_NONBLOCK;

        if ((mode & std::ios::in) != 0 && (mode & std::ios::out) != 0) {
            flags |= O_RDWR;
        } else if ((mode & std::ios::in) != 0) {
            flags |= O_RDONLY;
        } else if ((mode & std::ios::out) != 0) {
            flags |= O_WRONLY;
        } else
            debug::panic("at least one of read/write must be specified!");

        return flags;
    }

    file::file(const int fd) : fd(fd) {}

    task<bool> file::try_flush_wrbuf() {
        if (fd < 0)
            throw exceptions::rt_exception_base("writing to closed file!");

        if (wrbuf.empty())
            co_return true;

        wrbuf.get_raw_buffer().make_cont();
        auto wrote = await calls::write(fd, wrbuf.get_raw_buffer().data().data(), wrbuf.get_raw_buffer().size(), write_head);

        write_head += wrote;

        wrbuf.skip(wrote);

        co_return wrbuf.empty();
    }

    file::file() = default;

    file::file(file &&other) noexcept: fd(other.fd),
                                       read_head(other.read_head),
                                       write_head(other.write_head),
                                       rdbuf(std::move(other.rdbuf)),
                                       wrbuf(std::move(other.wrbuf)) {
        other.fd = -1;
    }

    file & file::operator=(file &&other) noexcept {
        if (this == &other)
            return *this;
        fd = other.fd;
        other.fd = -1;
        read_head = other.read_head;
        write_head = other.write_head;
        rdbuf = std::move(other.rdbuf);
        wrbuf = std::move(other.wrbuf);
        return *this;
    }

    file::file(const char *path, std::ios::openmode mode) : file(std::string(path), mode) {}

    file::file(const std::string &path, std::ios::openmode mode) {
        auto flags = compute_openmode_flags(mode);
        fd = ::open(path.c_str(), flags);

        if (fd < 0)
            debug::panic("fd open failed!");
    }

    file::file(const std::filesystem::path &path, std::ios::openmode mode) : file(path.string(), mode) {}

    file::~file() {
        if (fd > 0) {
            if (!wrbuf.empty())
                console::warn("file got closed by destructor but there was still data in the buffer!");
            ::close(fd);
        }
    }

    task<file> file::open(const char *path, std::ios::openmode mode) {
        std::string str(path);
        co_return await open(str, mode);
    }

    task<file> file::open(const std::filesystem::path &path, std::ios::openmode mode) {
        co_return await open(path.string(), mode);
    }

    task<> file::close() {
        await drain();

        await calls::close(fd);

        fd = -1;
    }

    task<std::vector<char>> file::read(int n) {
        if (fd < 0)
            throw exceptions::rt_exception_base("reading from closed file!");

        if (eof_)
            co_return {};

        if (n == -1) {
            auto buf2 = rdbuf.read_vector(rdbuf.size());
            std::vector<char> result = std::vector<char>{buf2.begin(), buf2.end()};

            char buffer[65536];

            int read;

            while (read = await calls::read(fd, buffer, 65536, read_head), read != 0) {
                read_head += read;
                result.insert(result.end(), buffer, buffer + read);
            }

            eof_ = true;

            co_return result;
        }

        if (rdbuf.size() > n) {
            // we already have enough data
            auto buf2 = rdbuf.read_vector(n);
            co_return std::vector<char>{buf2.begin(), buf2.end()};
        }

        auto buf2 = rdbuf.read_vector(rdbuf.size());
        auto b2s = buf2.size();
        std::vector<char> result = std::vector<char>{buf2.begin(), buf2.end()};
        result.resize(n);

        if (n - b2s > batch_read_count) {
            auto read = await calls::read(fd, result.data() + b2s, n - b2s, read_head);
            read_head += read;

            if (read == 0)
                eof_ = true;

            result.resize(b2s + read);

            co_return result;
        } else {
            char buffer[batch_read_count];
            auto read = await calls::read(fd, buffer, batch_read_count, read_head);
            read_head += read;

            if (n - b2s > read) {
                std::memcpy(result.data() + b2s, buffer, read);
            } else {
                std::memcpy(result.data() + b2s, buffer, n - b2s);
                auto consumed = n - b2s;
                rdbuf.write_vector(std::span{(sanify::u8*)buffer + consumed, (sanify::u8*)buffer + read});
            }

            if (read == 0)
                eof_ = true;

            co_return result;
        }
    }

    task<std::string> file::read_str(int n) {
        auto res = await read(n);

        co_return std::string(res.begin(), res.end());
    }

    task<std::vector<char>> file::readexactly(int n) {
        if (fd < 0)
            throw exceptions::rt_exception_base("reading from closed file!");

        if (eof_)
            throw exceptions::rt_exception_base("file reached eof before finishing read!");

        auto buf2 = rdbuf.read_vector(rdbuf.size());
        std::vector<char> result = std::vector<char>{buf2.begin(), buf2.end()};
        result.resize(n);

        int read = await calls::read(fd, result.data() + buf2.size(), n - buf2.size(), read_head);
        if (read != 0)
            read_head += read;

        if (read != n - buf2.size())
            throw exceptions::rt_exception_base("file reached eof before finishing the read!");

        co_return result;
    }

    task<> file::write(const std::span<char> &data) {
        if (fd < 0)
            throw exceptions::rt_exception_base("writing to closed file!");

        auto result = await try_flush_wrbuf();

        if (result) {
            int wrote = await calls::write(fd, data.data(), data.size(), write_head);

            write_head += wrote;

            if (wrote != data.size())
                wrbuf.write_vector(std::span{(sanify::u8*)data.data() + wrote, (sanify::u8*)data.data() + data.size()});
        } else
            wrbuf.write_vector(std::span{(sanify::u8*)data.data(), (sanify::u8*)data.data() + data.size()});
    }

    task<> file::drain() {
        if (fd < 0)
            throw exceptions::rt_exception_base("writing to closed file!");

        while (true) {
            auto result = await try_flush_wrbuf();

            if (result)
                co_return;

            await calls::poll(fd, POLLOUT);
        }
    }

    void file::seekg(off_t head) {
        read_head = head;
    }

    void file::seekp(off_t head) {
        write_head = head;
    }

    bool file::is_eof() const {
        return eof_;
    }

    bool file::is_open() const {
        return fd >= 0;
    }

    task<file> file::open(const std::string &path, std::ios::openmode mode) {
        std::string p = path;
        const int fd = await calls::open(p.c_str(), compute_openmode_flags(mode));

        co_return std::move(file{fd});
    }
}
