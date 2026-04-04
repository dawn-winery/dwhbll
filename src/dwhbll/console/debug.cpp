#include <dwhbll/console/debug.hpp>
#include <dwhbll/utils/stacktrace.hpp>
#include <iostream>
#include <format>
#include <fstream>
#include <ranges>
#include <vector>
#include <version>

#ifdef __cpp_lib_stacktrace
#include <stacktrace>
#include <filesystem>
#endif

namespace dwhbll::debug {
#ifndef NDEBUG
thread_local std::vector<task_deferral*> _running_tasks;

task_deferral::task_deferral(const std::string &name) : name(name) {
    _running_tasks.push_back(this);
}

task_deferral::~task_deferral() {
    ASSERT(_running_tasks.back() == this);

    _running_tasks.pop_back();
}

const std::string & task_deferral::get_name() const {
    return name;
}
#endif

[[noreturn]] void panic(const std::string& msg) {
    std::cerr << "\n\e[1;91m============ [PANIC] ============\n";
    std::cerr << msg << "\n\n";
    std::cerr << "Traceback (most recent call first):" << "\n";

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

        // when under modules for some reason width specifiers are broken :xdd:
        const auto info = std::format(
                "[{:#x}] {}\n",
                reinterpret_cast<std::uintptr_t>(entry.native_handle()), sourcePosition);
        std::cerr << (info);
    }
    #endif

#ifdef NDEBUG
    std::cerr << "Context Stack unavailable in release mode.\n";
#else
    if (!_running_tasks.empty()) {
        std::cerr << "Context Stack (most recent task first):" << "\n";

        for (const auto& [index, task] : _running_tasks | std::views::reverse | std::views::enumerate) {
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

void cond_assert(bool condition) {
    cond_assert(condition, "");
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
