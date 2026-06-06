#pragma once
#include <ranges>

namespace dwhbll::stl_ext {
    template <typename T, typename V>
    requires std::ranges::range<V> && std::ranges::forward_range<T> && std::integral<typename V::value_type>
    constexpr void erase_inplace_with_sorted_index_list(T& container, const V& erase_index, bool ascending = true) {
        std::size_t write_head = 0;
        std::size_t read_head = 0;
        std::size_t to_remove_head = ascending ? 0 : erase_index.size();

        while (read_head < container.size()) {
            if ((ascending ? to_remove_head != erase_index.size() : to_remove_head != 0) &&
                (ascending ? erase_index[to_remove_head] : erase_index[to_remove_head - 1]) == read_head) {
                read_head++;
                if (ascending)
                    ++to_remove_head;
                else
                    --to_remove_head;
                continue;
            }

            if (write_head != read_head)
                container[write_head] = std::move(container[read_head]);

            write_head++;
            read_head++;
        }

        container.resize(write_head);
    }

    template <typename T, typename V>
    requires std::ranges::range<V> && std::ranges::forward_range<T> && std::integral<typename V::value_type>
    constexpr void erase_inplace_with_unsorted_index_list(T& container, V& erase_index) {
        std::sort(erase_index.begin(), erase_index.end());

        erase_inplace_with_sorted_index_list(container, erase_index, true);
    }
}
