#include <dwhbll/testing/testing.h>
#include <dwhbll/debug/debug.h>
#include <mutex>

namespace dwhbll::test {

namespace detail {

static std::mutex failure_mutex;

void result::add_failure(std::string msg, std::source_location loc) {
    std::lock_guard lock(failure_mutex);
    failures_.push_back({std::move(msg), loc});
}

bool result::passed() const {
    std::lock_guard lock(failure_mutex);
    return failures_.empty();
}

const std::vector<failure>& result::failures() const {
    return failures_;
}

result* current_result = nullptr;

void report_failure(std::string msg, std::source_location loc) {
    ASSERT(current_result);
    current_result->add_failure(std::move(msg), loc);
}

std::vector<entry>& registry() {
    static std::vector<entry> r;
    return r;
}

} // namespace detail

bool expect(bool cond, std::string_view msg, std::source_location loc) {
    if (!cond)
        detail::report_failure(msg.empty() ? std::string("expect failed")
                                        : std::string(msg), loc);
    return cond;
}

int run_all(const options& options) {
    return runner().run(options);
}

} // namespace dwhbll::test
