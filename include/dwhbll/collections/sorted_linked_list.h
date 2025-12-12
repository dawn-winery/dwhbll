#pragma once
#include <concepts>
#include <dwhbll/memory/pool.h>

namespace dwhbll::collections {
    /**
     * @brief A sorted linked list,
     * @tparam T The type stored by this linked list, the type needs to be orderable.
     */
    template <typename T, class Allocator = std::allocator<T>>
    class SortedLinkedList : protected std::list<T, Allocator> {
    public:
        using list_t = std::list<T, Allocator>;

        using value_type = list_t::value_type;
        using allocator_type = list_t::allocator_type;
        using size_type = list_t::size_type;
        using difference_type = list_t::difference_type;
        using reference = list_t::reference;
        using const_reference = list_t::const_reference;
        using pointer = list_t::pointer;
        using const_pointer = list_t::const_pointer;
        using iterator = list_t::iterator;
        using const_iterator = list_t::const_iterator;
        using reverse_iterator = list_t::reverse_iterator;
        using const_reverse_iterator = list_t::const_reverse_iterator;

        using list_t::list_t;

        template <typename InputIt>
        constexpr SortedLinkedList(InputIt first, InputIt last, const Allocator& alloc = Allocator()) : list_t(first, last, alloc) {
            list_t::sort();
        }

        template <typename R>
        constexpr SortedLinkedList(std::from_range_t, R&& rg, const Allocator& alloc = Allocator()) : list_t(std::forward<R>(rg), std::forward<Allocator>(alloc)) {
            list_t::sort();
        }

        constexpr SortedLinkedList(std::initializer_list<T> init, const Allocator& alloc = Allocator()) : list_t(init, alloc) {
            list_t::sort();
        }

        constexpr ~SortedLinkedList() = default;

        using list_t::operator=;

        constexpr SortedLinkedList& operator=(std::initializer_list<value_type> ilist) {
            list_t::operator=(ilist);
            list_t::sort();
            return *this;
        }

        using list_t::assign;

        template <typename InputIt>
        constexpr void assign(InputIt first, InputIt last) {
            list_t::assign(first, last);
            list_t::sort();
        }

        constexpr void assign(std::initializer_list<value_type> ilist) {
            list_t::assign(ilist);
            list_t::sort();
        }

        using list_t::get_allocator;
        using list_t::front;
        using list_t::back;
        using list_t::begin;
        using list_t::cbegin;
        using list_t::end;
        using list_t::cend;
        using list_t::rbegin;
        using list_t::crbegin;
        using list_t::rend;
        using list_t::crend;

        using list_t::empty;
        using list_t::size;
        using list_t::max_size;

        using list_t::clear;

        constexpr iterator insert(const T& value) {
            auto it = std::lower_bound(begin(), end(), value);
            return list_t::insert(it, value);
        }

        constexpr iterator insert(T&& value) {
            auto it = std::lower_bound(begin(), end(), value);
            return list_t::insert(it, std::move(value));
        }

        constexpr iterator insert(size_type count, const T& value) {
            auto it = std::lower_bound(begin(), end(), value);
            return list_t::insert(it, count, value);
        }

        // TODO: is insert then sort better here?
        template <typename InputIt>
        constexpr iterator insert(InputIt first, InputIt last) {
            iterator first_insert = list_t::end();
            if (first != last) {
                first_insert = insert(first);
                ++first;
            }
            for (; first != last; ++first)
                insert(first);

            return first_insert;
        }

        constexpr iterator insert(std::initializer_list<T> ilist) {
            return insert(ilist.begin(), ilist.end());
        }

        template <typename... Args>
        constexpr iterator emplace(Args&&... args) {
            T value;
            std::allocator_traits<Allocator>::construct(get_allocator(), &value, std::forward<Args>(args)...);
            return insert(std::move(value));
        }

        using list_t::erase;
        using list_t::pop_back;
        using list_t::pop_front;
        using list_t::resize;
        using list_t::swap;
        using list_t::merge;
        using list_t::remove;
        using list_t::remove_if;
        using list_t::unique;

        constexpr void sort() {}

        template <typename Compare>
        constexpr void sort(Compare comp) {}

        template <class Alloc>
        friend bool operator==(const SortedLinkedList<T, Alloc>& lhs,
            const SortedLinkedList<T, Alloc>& rhs) {
            return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        }

        // TODO: no spaceship operator provided (yet)
    };
}
