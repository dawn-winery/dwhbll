#pragma once

#include <dwhbll/files/filejar/file_mgr.h>

namespace dwhbll::lang {
    struct cursor {
        files::filejar::fileid file{};
        std::size_t line{};
        std::size_t column{};

        cursor(files::filejar::fileid id) : file(id) {}

        constexpr void next_line() {
            line++;
            column = 0;
        }

        constexpr void next_col() {
            column++;
        }
    };

    struct span {
        cursor begin;
        cursor end;
    };
}
