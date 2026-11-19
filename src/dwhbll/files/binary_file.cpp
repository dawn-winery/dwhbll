#include <dwhbll/files/binary_file.h>

#include <dwhbll/debug/debug.h>
#include <dwhbll/console/logging.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

namespace dwhbll::files {
    stl_ext::errno_result<stl_ext::UNIT> binary_file::load(const std::filesystem::path &path) {
        if (file) {
#if __cpp_lib_format_path >= 202506L
            console::warn("There is already a file loaded?! Unloading before loading {}", path.display_string());
#else
            console::warn("There is already a file loaded?! Unloading before loading {}", path.string());
#endif
            unload();
        }

#if __cpp_lib_format_path >= 202506L
        auto fname = path.display_string();
#else
        auto fname = path.string();
#endif
        int fd = open(fname.c_str(), O_RDONLY);

        if (fd == -1)
            return stl_ext::Err(errno);

        struct stat st{};
        if (fstat(fd, &st) == -1)
            return stl_ext::Err(errno);

        len = st.st_size;

        void* mapping = mmap(
            nullptr,
            len,
            PROT_READ,
            MAP_SHARED,
            fd,
            0
        );

        close(fd);

        if (mapping == MAP_FAILED)
            return stl_ext::Err(errno);

        file = static_cast<std::uint8_t*>(mapping);

        return stl_ext::Ok();
    }

    void binary_file::unload() {
        if (!file)
            return;

        munmap(file, len);

        file = nullptr;
        len = 0;
    }

    binary_file::binary_file() {}

    binary_file::binary_file(const std::filesystem::path &path) {
        if (load(path).is_err())
#if __cpp_lib_format_path >= 202506L
            console::warn("Failed to load file {}", path.display_string());
#else
            console::warn("Failed to load file {}", path.string());
#endif
    }

    binary_file::~binary_file() {
        unload();
    }

    binary_file::binary_file(binary_file &&other) noexcept : file(other.file),
        len(other.len) {
        other.file = nullptr;
        other.len = 0;
    }

    binary_file & binary_file::operator=(binary_file &&other) noexcept {
        if (this == &other)
            return *this;
        file = other.file;
        len = other.len;
        other.file = nullptr;
        other.len = 0;
        return *this;
    }
}
