module;

#include <dwhbll/graphics/pixel.h>
#include <dwhbll/graphics/bitmap/file_header.h>
#include <dwhbll/graphics/bitmap/info_header.h>
#include <dwhbll/graphics/bitmap/bitmap.h>

export module dwhbll.graphics;

export namespace dwhbll::graphics {
    using dwhbll::graphics::pixel;
    using dwhbll::graphics::bitmap::file_header;
    using dwhbll::graphics::bitmap::info_header;

    namespace bitmap {
        using dwhbll::graphics::bitmap::bitmap;
    }
}
