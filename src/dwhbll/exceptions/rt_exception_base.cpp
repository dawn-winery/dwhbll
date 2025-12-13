#include "../../../include/dwhbll/exceptions/rt_exception_base.h"

#include <filesystem>
#include <iostream>

#include <cxxabi.h>
#include <dwhbll/utils/stacktrace.hpp>

namespace dwhbll::exceptions {
    void prettyprint_rtexcept(std::stringstream& ss, const rt_exception_base& exception);

    void prettyprint_stdexcept(std::stringstream& ss, const std::exception& exception) {
        ss << stacktrace::demangle(typeid(exception).name()) << ": " << exception.what() << std::endl;
        try {
            std::rethrow_if_nested(exception);
        } catch (const rt_exception_base& base) {
            ss << "With nested exception: " << std::endl;
            prettyprint_rtexcept(ss, base);
        } catch (const std::exception& e) {
            ss << "With nested exception: " << std::endl;
            prettyprint_stdexcept(ss, e);
        }
    }

    void prettyprint_rtexcept(std::stringstream& ss, const rt_exception_base& exception) {
        ss << stacktrace::demangle(typeid(exception).name()) << ": " << exception.what() << std::endl;
        ss << exception.get_prettyprint_trace();
        try {
            std::rethrow_if_nested(exception);
        } catch (const rt_exception_base& base) {
            ss << "With nested exception: " << std::endl;
            prettyprint_rtexcept(ss, base);
        } catch (const std::exception& e) {
            ss << "With nested exception: " << std::endl;
            prettyprint_stdexcept(ss, e);
        }
    }

    const bool autotraceprint_initd = [] {
        std::set_terminate(rt_exception_base::traceback_terminate_handler);
        return true;
    }();

    void rt_exception_base::populate_trace() {
#ifdef __cpp_lib_stacktrace
        trace = std::stacktrace::current();
#else
        trace = dwhbll::stacktrace::current_nodemangle();
#endif
    }

    std::string rt_exception_base::get_prettyprint_trace() const {
        std::stringstream stream;
        stream << "Traceback (most recent call first):" << "\n";
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
            stream << (info);
        }
        #else
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
            stream << (info);
        }
        #endif

        return stream.str();
    }

    void rt_exception_base::trace_to_stderr() const {
        std::cerr << get_prettyprint_trace() << std::endl;
    }

    void rt_exception_base::traceback_terminate_handler() {
        if (auto ptr = std::current_exception(); ptr != nullptr) {
            // we are exiting with an exception.
            std::stringstream ss;
            ss << "Terminating with an uncaught exception!" << std::endl;
            try {
                std::rethrow_exception(ptr);
            } catch (const rt_exception_base& base) {
                prettyprint_rtexcept(ss, base);
            } catch (const std::exception& e) {
                prettyprint_stdexcept(ss, e);
            }

            std::cerr << ss.str() << std::endl;
        }
    }
}
