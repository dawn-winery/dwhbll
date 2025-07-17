#include <dwhbll/console/debug.hpp>
#include <dwhbll/utils/stacktrace.hpp>
#include <iostream>
#include <fstream>
#include <regex>
#include <version>

#ifdef __cpp_lib_stacktrace
#include <stacktrace>
#include <filesystem>
#endif

namespace dwhbll::debug {

[[noreturn]] void panic(const std::string& msg) {
    std::cerr << "\n\e[1;91m============ [PANIC] ============\n";
    std::cerr << msg << "\n\n";

    #ifndef __cpp_lib_stacktrace
    using namespace dwhbll::stacktrace;
    std::vector<Entry> trace = current(1);
    for(auto& entry : trace) {
        const auto function = entry.symbol_name.has_value() ? entry.symbol_name.value() : "???";

        std::string sourcePosition;
        if (entry.path.has_value()) {
            if(entry.line.has_value()) {
                sourcePosition = std::format(
                    "{} at {}:{}",
                    function, entry.path.value(), entry.line.value());
            }
            else {
                sourcePosition = std::format(
                    "{} at {}", function.data(), entry.path.value());
            }
        } else {
            sourcePosition = function;
        }

        const auto info = std::format(
                "[{:#018x}] {}\n",
                reinterpret_cast<std::uintptr_t>(entry.address), sourcePosition.data());
        std::cerr << (info);
    }
    #else
    std::stacktrace trace = std::stacktrace::current();
    for(auto& entry : trace) {
        const auto function = entry.description().substr(0, entry.description().find("("));

        std::string sourcePosition;
        if (entry.source_file().size() > 0) {
            const auto sourcePath = std::filesystem::path(entry.source_file());
            const auto relativePath = sourcePath.lexically_relative(std::filesystem::current_path());
            const auto filename = relativePath.string().starts_with("../..") ? sourcePath : relativePath;
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

        const auto info = std::format(
                "[{:#018x}] {}\n",
                reinterpret_cast<std::uintptr_t>(entry.native_handle()), sourcePosition.data());
        std::cerr << (info);
    }
    #endif

    exit(1);
}

bool is_being_debugged() {
    std::ifstream f("/proc/self/status");
    std::string line;
    while(std::getline(f, line)) {
        if (line.starts_with("TracerPid:")) {
            int tracer_pid = std::stoi(line.substr(10));
            return tracer_pid != 0;
        }
    }

    return false;
}

} // namespace dwhbll::debug
