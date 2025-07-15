#pragma once

#ifdef DWHBLL_LIBCPP

#include <dwhbll/sanify/types.hpp>
#include <optional>
#include <execinfo.h>
#include <elfutils/libdwfl.h>
#include <unistd.h>
#include <cxxabi.h>

/*
 * Everything here is temporary until libc++ adds <stacktrace>
 * We need libc++ in case the library is built with reflection features
 */

// Probably a sane default?
#define MAX_STACK_SIZE 512

namespace dwhbll::stacktrace {

using namespace sanify;

struct Entry {
    void* address;
    std::optional<std::string> symbol_name;
    std::optional<std::string> path;
    std::optional<u32> line;
};

inline std::string demangle(const char* name) {
    int status = 0;
    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(name, nullptr, nullptr, &status),
        std::free
    };
    return (status == 0) ? res.get() : name;
}

inline std::vector<Entry> current(u32 skip = 0) {
    skip++;

    // Will only build on glibc, but who cares
    void *buffer[MAX_STACK_SIZE];
    int n = backtrace(buffer, MAX_STACK_SIZE);

    std::vector<Entry> entries(n - skip);
    for(int i = 0; i < entries.size(); i++) {
        entries[i] = Entry { .address = buffer[i + skip] };
    }

    Dwfl_Callbacks callbacks = {
        .find_elf = dwfl_linux_proc_find_elf,
        .find_debuginfo = dwfl_standard_find_debuginfo,
    };

    Dwfl *dwfl = dwfl_begin(&callbacks);
    // On failure, return only the addresses
    if(dwfl_linux_proc_report(dwfl, getpid()))
        goto end;

    if(dwfl_report_end(dwfl, NULL, NULL))
        goto end;

    for(int i = skip; i < n; i++) {
        Dwfl_Module *mod = dwfl_addrmodule(dwfl, (u64) buffer[i]);
        const char *func;
        Dwarf_Addr addr = (u64) buffer[i];
        func = dwfl_module_addrname(mod, (u64) buffer[i]);
        if(func)
            entries[i - skip].symbol_name = demangle(func);

        Dwfl_Line *line = dwfl_module_getsrc(mod, (uintptr_t)buffer[i]);

        if (line) {
            int lineno;
            const char *filename = dwfl_lineinfo(line, NULL, &lineno, NULL, NULL, NULL);
            if(filename[0])
                entries[i - skip].path = filename;
            entries[i - skip].line = lineno;
        } 
    }

end:
    dwfl_end(dwfl);
    return entries;
}

}

#endif
