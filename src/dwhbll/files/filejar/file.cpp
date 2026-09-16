#include <dwhbll/files/filejar/file.h>

#include <cstring>
#include <fstream>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <dwhbll/hash/xxh/xxh3_64.h>

namespace dwhbll::files::filejar {
    file::file(const std::filesystem::path &path) {
        metadata = {canonical(path)};
        metadata._exists = std::filesystem::exists(metadata._path);

        if (metadata._exists) {
            metadata.len = std::filesystem::file_size(metadata._path);
            metadata.last_write_time = std::filesystem::last_write_time(metadata._path);

            fd = open(metadata._path.c_str(), O_RDONLY);

            if (fd < 0)
                debug::panic("Attempted to open {}, got error: {}", metadata._path, strerror(errno));

            data = (sanify::u8*)mmap(nullptr, metadata.len, PROT_READ, MAP_PRIVATE, fd, 0);
        }
    }

    file::~file() {
        if (fd != -1) {
            close(fd);
            fd = -1;
        }

        if (data) {
            munmap(data, metadata.len);
            data = nullptr;
        }
    }

    void file::compute_hash() const {
        if (!metadata._exists || !data)
            debug::panic("File doesn't exist!");

        metadata.hash = hash::xxh::detail3_64::hash(data, metadata.len);
    }

    bool file::file_unchanged_against(const file_metadata &other) const {
        if (metadata.probably_unchanged(other))
            return true;

        if (!metadata.hash.has_value())
            compute_hash();

        if (!other.hash.has_value())
            debug::panic("Other metadata doesn't have has hvalue!");

        return metadata.hash == other.hash;
    }
}
