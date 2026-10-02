#include <dwhbll/testing/testing.h>

namespace dwhbll::test {

namespace detail {

std::vector<entry>& registry() {
    static std::vector<entry> r;
    return r;
}

} // namespace detail

int run_all(const options& options) {
    return runner().run(options);
}

} // namespace dwhbll::test
