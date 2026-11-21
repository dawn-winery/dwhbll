#pragma once

#include <tuple>

namespace dwhbll::stl_ext {
    /**
     * @brief Extract template by type.
     * @tparam V value param, the data to extract from
     */
    template <typename V>
    struct template_info : std::false_type {
    };

    template<template<typename...> typename TV, typename... ArgsV>
    struct template_info<TV<ArgsV...>> : std::true_type {
        template <typename... Args>
        using rebind = TV<Args...>;

        using args = std::tuple<ArgsV...>;
    };
}
