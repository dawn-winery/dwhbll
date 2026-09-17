#include <dwhbll/testing/testing.h>
#include <dwhbll/graphics/bitmap/bitmap.h>

#include <complex>
#include <cstdint>
#include <filesystem>
#include <print>

using namespace dwhbll::test;

namespace {

const int width = 64;
const int height = 64;
const int max_iters = 20;

int get_iters(double x, double y) {
    std::complex<double> z = std::complex(0.0, 0.0);
    std::complex<double> c = std::complex(x, y);

    for (int i = 0; i != max_iters; ++i) {
        z = z * z + c;
        if (std::abs(z) > 2)
            return i;
    }

    return -1;
}

} // namespace

namespace graphics::bitmap {

[[=test]]
void fractal()
{
    auto b = new dwhbll::graphics::bitmap::bitmap(width, height);

    for (int y = 0; y != height; ++y) {
        for (int x = 0; x != width; ++x) {
            double fx = (x - width / 2.0) * 2.0 / width;
            double fy = (y - height / 2.0) * 2.0 / height;
            int iters = get_iters(fx, fy);
            uint8_t color = 256 * iters / max_iters;
            (void)b->set_pixel(x, y, color, color, color);
        }
    }

    if (!b->write_to_file("output.bmp")) {
        std::println("Failed to write output.bmp");
        delete b;
        return;
    }

    delete b;
    std::filesystem::remove("output.bmp");
}

}

TEST_REGISTER_FILE();
