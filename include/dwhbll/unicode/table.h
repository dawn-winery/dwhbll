#pragma once

#include <algorithm>
#include <stdexcept>
#include <unordered_map>

#include <dwhbll/console/debug.hpp>

namespace dwhbll::unicode {
    /**
     * @brief Range table, stores ranges [begin, end) which are non overlapping
     * this is a specialized flat search intrusive vector data-structure for
     * Unicode UCD ranges which may be generalizable in the future.
     * @tparam T type of data stored
     */
    template <typename T>
    class table {
    public:
        struct elem {
            char32_t begin;
            char32_t end;
            T data;
        };

    private:
        elem* _begin;
        elem* _end;

    public:
        class const_iterator {
            table& parent;
            elem* head;

        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = elem;
            using difference_type = std::ptrdiff_t;
            using pointer = const elem *const;
            using reference = const elem &;

            const_iterator(table& parent, elem* head) : parent(parent), head(head) {}

            [[nodiscard]] std::strong_ordering operator<=>(const const_iterator &other) const {
                ASSERT(&parent == &other.parent);

                return head <=> other.head;
            }

            const_iterator& operator+=(const difference_type &diff) {
                head += diff;
                return *this;
            }

            const_iterator& operator-=(const difference_type &diff) {
                head -= diff;
                return *this;
            }

            decltype(auto) operator++(this auto &&self) {
                ++self.head;
                return self;
            }

            decltype(auto) operator--(this auto &&self) {
                --self.head;
                return self;
            }

            decltype(auto) operator++(this auto &&self, int) {
                auto copy = self;
                ++self.head;
                return copy;
            }

            decltype(auto) operator--(this auto &&self, int) {
                auto copy = self;
                --self.head;
                return copy;
            }

            decltype(auto) operator+(this auto &&self, const difference_type &diff) {
                auto copy = self;
                copy.head += diff;
                return copy;
            }

            decltype(auto) operator-(this auto &&self, const difference_type &diff) {
                auto copy = self;
                copy.head -= diff;
                return copy;
            }

            decltype(auto) operator*(this auto &&self) {
                return *self.head;
            }

            decltype(auto) operator->(this auto &&self) {
                return self.head.operator->();
            }

            friend bool operator==(const const_iterator &lhs,
                const const_iterator &rhs) {
                ASSERT(&lhs.parent == &rhs.parent);

                return lhs.head == rhs.head;
            }

            friend bool operator!=(const const_iterator &lhs,
                const const_iterator &rhs) {
                ASSERT(&lhs.parent == &rhs.parent);

                return !(lhs == rhs);
            }
        };

        table() : _begin(), _end() {}

        explicit table(elem *begin, elem *end) : _begin(begin), _end(end) {}

        table(const table &other) = delete;

        table(table &&other) noexcept = delete;

        table & operator=(const table &other) = delete;

        table & operator=(table &&other) noexcept = delete;

        decltype(auto) begin(this auto &&self) {
            return const_iterator{self, self._begin};
        }

        decltype(auto) end(this auto &&self) {
            return const_iterator{self, self._end};
        }

        decltype(auto) find(this auto &&self, char32_t val) {
            auto bound = std::ranges::lower_bound(self._begin, self._end, val, std::ranges::less{}, [](const auto &e) { return e.end; });

            if (bound == self._end) {
                // nothing
                return self.end();
            }

            if (bound->begin > val || bound->end < val)
                // no matching range found
                return self.end();

            return const_iterator{self, bound};
        }

        decltype(auto) at(this auto &&self, char32_t val) {
            auto it = self.find(val);

            if (it == self.end())
                throw std::out_of_range("Entry not found");

            return it->data;
        }

        [[nodiscard]] bool contains(this auto &&self, char32_t c) {
            auto it = self.find(c);
            return it != self.end();
        }
    };

    struct empty_struct {};

    namespace properties {
        extern table<empty_struct> XID_Start;
        extern table<empty_struct> XID_Continue;
        extern table<empty_struct> ID_Compat_Math_Start;
        extern table<empty_struct> ID_Compat_Math_Continue;
    }

    namespace aliases {
        extern std::unordered_map<std::string, char32_t> name_aliases_to_codepoint;
    }
}
