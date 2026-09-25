import std;
import dwhbll.debug;
import dwhbll.console;

namespace dwhbll::debug {

auto perf_level = console::Level::WARN;
void set_perf_level(const console::Level level) {
    perf_level = level;
}

time::time(const std::string &stage) : stage_name(stage), start(std::chrono::steady_clock::now()) {}

time::~time() {
    auto time_taken = std::chrono::steady_clock::now() - start;

    console::log("[PERF] Spent {} ({}) executing: {}", perf_level,
        time_taken,
        std::chrono::duration_cast<std::chrono::milliseconds>(time_taken),
        stage_name
    );
}

}
