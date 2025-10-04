#include <complex>
#include <cstdint>
#include <dwhbll/graphics/bitmap/bitmap.h>
#include <functional>
#include <print>
#include <unordered_map>

const int width = 4096;
const int height = 2160;
const int max_iters = 1000;

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

bool fractal_test() {
  auto b = new dwhbll::graphics::bitmap::bitmap(4096, 2160);

  for (int y = 0; y != height; ++y) {
    for (int x = 0; x != width; ++x) {
      double fx = (x - width / 2.0) * 2.0 / width;
      double fy = (y - height / 2.0) * 2.0 / height;
      int iters = get_iters(fx, fy);
      uint8_t color = 256 * iters / max_iters;
      b->set_pixel(x, y, color, color, color);
    }
  }

  if (!b->write_to_file("output.bmp")) {
    std::println("Failed to write output.bmp");
    delete b;
    return true;
  }

  delete b;
  return false;
}

std::unordered_map<std::string, std::function<bool()>> bitmap_dispatch{
    {"fractal", fractal_test},
};

bool bitmap_test(std::optional<std::string> test_to_run) {
  if (test_to_run.has_value() && bitmap_dispatch.contains(test_to_run.value()))
    return bitmap_dispatch.at(test_to_run.value())();
  return 1;
}
