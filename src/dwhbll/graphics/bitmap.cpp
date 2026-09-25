import std;
import dwhbll.graphics;
import dwhbll.sanify;

namespace dwhbll::graphics::bitmap {
    bitmap::bitmap(const int width, const int height) : width(width), height(height) {
        pixels = new u8[width * height * 3]();
    }

    bitmap::~bitmap() {
        delete[] pixels;
    }

    bool bitmap::set_pixel(const int x, const int y, const u8 r, const u8 g, const u8 b) const {
        if (const int offset = y * width * 3 + x * 3; offset + 2 < width * height * 3) {
            pixels[offset] = b;
            pixels[offset + 1] = g;
            pixels[offset + 2] = r;
        } else {
            return false;
        }
        return true;
    }

    bool bitmap::write_to_file(const std::string &filename) const {
        std::ofstream file(filename);
        if (!file.is_open())
            return false;

        file_header fh = {};
        fh.size = sizeof(file_header) + sizeof(info_header) + width * height * 3;
        fh.offset = sizeof(file_header) + sizeof(info_header);

        info_header ih = {};
        ih.width = width;
        ih.height = height;

        file.write(reinterpret_cast<char *>(&fh), sizeof(file_header));
        file.write(reinterpret_cast<char *>(&ih), sizeof(info_header));
        file.write(reinterpret_cast<char *>(pixels), width * height * 3);

        return true;
    }
} // namespace dwhbll::graphics::bitmap
