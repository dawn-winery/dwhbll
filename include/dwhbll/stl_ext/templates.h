#pragma once

#include <tuple>

namespace dwhbll::stl_ext {
    /**
     * @brief Extract template by type.
     * @tparam V value param, the data to extract from
     */
    template <typename V>
    struct template_info {
        /// Whether is templated type
        constexpr static bool IS_TYPE = false;
    };

    template<template<typename...> typename TV, typename... ArgsV>
    struct template_info<TV<ArgsV...>> : std::true_type {
        /// Whether is templated type
        constexpr static bool IS_TYPE = true;

        template <typename... Args>
        using rebind = TV<Args...>;

        using args = std::tuple<ArgsV...>;
    };
}
