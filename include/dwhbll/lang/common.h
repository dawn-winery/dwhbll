#pragma once

#include <dwhbll/files/filejar/file_mgr.h>

namespace dwhbll::lang {
    struct cursor {
        files::filejar::fileid file;
        std::size_t line;
        std::size_t column;
    };

    struct span {
        cursor begin;
        cursor end;
    };
}
