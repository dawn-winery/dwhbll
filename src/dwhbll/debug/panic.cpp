#include <dwhbll/debug/panic.h>

#include <dwhbll/debug/debug.h>

#include <atomic>
#include <iostream>
#include <stacktrace>
#include <filesystem>
#include <ranges>

// TODO: other architectures like i386 exists too
#if defined(__x86_64) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace dwhbll::debug {

[[noreturn]] void panic(const std::string& msg) {
    static std::atomic_flag panicking = false;

    // busy wait if there's already a thread panicking, this way we don't spit
    // out multiple traces at a time and interleave them.
    while (panicking.test_and_set(std::memory_order_acq_rel)) {
#if defined(__x86_64) || defined(__x86_64__)
        _mm_pause();
#endif
    }

    std::cerr << "\n\e[1;91m============ [PANIC] ============\n";
    std::cerr << msg << "\n\n";
    std::cerr << "Traceback (most recent call first):" << "\n";

    std::stacktrace trace = std::stacktrace::current();
    for(auto& entry : trace) {
        const auto function = entry.description().substr(0, entry.description().find("("));

        std::string sourcePosition;
        if (entry.source_file().size() > 0) {
            const auto sourcePath = std::filesystem::path(entry.source_file());
            const auto relativePath = sourcePath.lexically_relative(std::filesystem::current_path());
#if __cpp_lib_format_path >= 202506L
            const auto filename = relativePath.display_string().starts_with("../..") ? sourcePath : relativePath;
#else
            const auto filename = relativePath.string().starts_with("../..") ? sourcePath : relativePath;
#endif
            sourcePosition = std::format(
                    "{} at {}:{}",
                    function.data(), filename.c_str(), entry.source_line());
        } else if (!function.empty()) {
            sourcePosition = function;
        } else if (!entry.description().empty()) {
            sourcePosition = entry.description();
        } else {
            sourcePosition = "???";
        }

        // when under modules for some reason width specifiers are broken :xdd:
        const auto info = std::format(
                "[{:#x}] {}\n",
                reinterpret_cast<std::uintptr_t>(entry.native_handle()), sourcePosition);
        std::cerr << (info);
    }

#ifdef NDEBUG
    std::cerr << "Context Stack unavailable in release mode.\n";
#else
    if (!running_tasks().empty()) {
        std::cerr << "Context Stack (most recent task first):" << "\n";

        for (const auto& [index, task] : running_tasks() | std::views::reverse | std::views::enumerate) {
            std::cerr << std::format("  #{}: {}\n", index, task->get_name());
        }
    }
#endif

    std::cerr << "\e[0;0m" << std::endl;

    exit(1);
}

void panic() {
    panic("");
}

void panic(bool condition) {
    if (!condition)
        panic("");
}

} // namespace dwhbll::debug
