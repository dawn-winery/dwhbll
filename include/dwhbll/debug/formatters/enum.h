#pragma once

#include <cstddef>
#include <format>
#include <string>

#include <dwhbll/sanify/sugar/enum.h>
#include <dwhbll/debug/formatters/fwd.h>


namespace {
    template <class T>
    concept sanify_enum = requires(const T& value) {
        requires dwhbll::sanify::sugar::detail::is_self_tuple<T, std::variant_alternative_t<0, typename T::storage_t>>;
        value.__impl_storage_t();
    };

}


template <sanify_enum T>
struct std::formatter<T, char> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const T& x, auto& ctx) const {
        return std::visit([&](const auto& active) {

            auto out = std::format_to(ctx.out(), "{}::",std::meta::identifier_of(^^T));

            // match helpers against discriminator id to find the name of the bloody Discriminator

            constexpr auto ctx2 = std::meta::access_context::unchecked();
            template for (constexpr auto m:
                std::define_static_array(std::meta::members_of(^^T, ctx2)))
            {
                if constexpr (std::meta::is_variable(m)) 
                    if constexpr (requires { [:m:].discriminant(); }) {
                    if constexpr (std::convertible_to<decltype([:m:]), std::string_view>)
                    {
                        if (std::string_view([:m:].discriminant()) == std::string_view(active.discriminator()))
                            out = std::format_to(out, "{}", std::meta::identifier_of(m));  
                    }
                    else{
                    
                        if ([:m:].discriminant() == active.discriminator())
                            out = std::format_to(out, "{}", std::meta::identifier_of(m));
                    }
                
            }
            
            auto x = true; 
            if constexpr (requires { active.tuple; })
            {
                out = std::apply([&](const auto&... fields) {
                     ((out = std::format_to(out, "{}{}",std::exchange(x, false) ? "(" : ", ",dwhbll::meta::dbg(fields, 0, 5, false))), ...);
                     return out;
                 }, active.tuple);
            }
            else{
                return std::format_to(out,"");
            }
            return std::format_to(out, ")");
        }, x.__impl_storage_t());
    }
};
