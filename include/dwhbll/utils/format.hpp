#pragma once

#include <dwhbll/utils/utils.hpp>
#include <dwhbll/utils/json.hpp>
#include <dwhbll/console/Logging.h>
#include <cassert>


namespace dwhbll::debug {

#ifdef DWHBLL_REFLECTION
struct DebugFmt {};
inline constexpr auto Debug = DebugFmt();

template <typename T>
requires std::is_class_v<T>
constexpr json::json debugfmt_internal(T val) {
    json::json json;
    
    constexpr auto ctx = std::meta::access_context::current();
    template for(constexpr auto m : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx))) {
        using V = [:std::meta::type_of(m):];
        constexpr std::string_view id = std::meta::identifier_of(m);

        if constexpr (utils::has_annotation(std::meta::type_of(m), ^^DebugFmt)) {
            json[id] = debugfmt_internal<V>(val.[:m:]);
        }
        else {
            json[id] = std::format("{}", val.[:m:]);
        }
    }

    return json;
}

template <typename T>
requires std::is_class_v<T>
constexpr std::string debugfmt(T val) {
    if constexpr (!utils::has_annotation(^^T, ^^DebugFmt)) {
        throw "Missing annotation";
    }
    return debugfmt_internal(val).dump();
}
#endif

} // namespace dwhbll::debug
