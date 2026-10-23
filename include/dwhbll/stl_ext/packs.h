#pragma once

#include <cstddef>

namespace dwhbll::stl_ext {
    namespace __detail {
        template <std::size_t N, typename... Ts>
        struct template_pack_nth_helper;

        template <std::size_t N, typename T0, typename... Ts>
        requires (N == 0)
        struct template_pack_nth_helper<N, T0, Ts...> {
            using type = T0;
        };

        template <std::size_t N, typename T0, typename... Ts>
        requires (N != 0)
        struct template_pack_nth_helper<N, T0, Ts...> {
            using type = typename template_pack_nth_helper<N - 1, Ts...>::type;
        };

        template <size_t N, typename... Ts>
        requires (N != 0)
		struct pack_cnt_divisible_by {
			static constexpr bool value = (sizeof...(Ts) > 0) && (sizeof...(Ts) % N == 0);
		};

    }

    template <std::size_t N, typename... Ts>
    struct template_pack_nth {
        static_assert(N < sizeof...(Ts), "N is out of range!");

        using type = typename __detail::template_pack_nth_helper<N, Ts...>::type;
    };

    template <size_t N, typename... Ts>
   	constexpr bool pack_cnt_divisible_by_v = __detail::pack_cnt_divisible_by<N, Ts...>::value;
}
