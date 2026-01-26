#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace dwhbll::collections {
    /**
     * @brief A simple ring buffer
     * @tparam T The inner type
     * @note emplace operations will invalidate
     */
    template <typename T>
    class Ring {
        // these hold the information on the current front and backs.
        // head holds the first index
        // tail holds the first index past
        // head == tail means the buffer is full (or entire thing is empty);
        std::size_t head{}, tail{};
        std::size_t sz{};
        std::vector<T> _M_data;
        bool needsResize{false};

        void inc_tail() {
            tail++;
            if (tail >= _M_data.size()) {
                tail = 0; // wrap around
            }
            if (tail == head) {
                // next insertion is going to run the head into the tail.
                needsResize = true;
            }
        }

        void dec_tail() {
            if (tail == 0)
                tail = _M_data.size() - 1;
            else
                tail--;
            if (needsResize) {
                // we needed more space, now we don't
                needsResize = false;
            }
        }

        void inc_head() {
            head++;
            if (head >= _M_data.size()) {
                head = 0; // wrap around
            }
            if (needsResize) {
                // we needed more space, now we don't
                needsResize = false;
            }
        }

        void dec_head() {
            if (head == 0)
                head = _M_data.size() - 1;
            else
                head--;
            if (tail == head) {
                // next insertion is going to run the head into the tail.
                needsResize = true;
            }
        }

        void resize() {
            std::vector<T> newData;
            if (_M_data.size() == 0)
                newData.resize(2);
            else
                newData.resize(_M_data.size() * 2); // double size
            // put it all in order
            if (tail >= head) {
                // it's actually already in order.
                std::memcpy(newData.data(), _M_data.data() + head, sz * sizeof(T));
            } else {
                // it's not quite in order
                std::memcpy(newData.data(), _M_data.data() + head, (_M_data.size() - head) * sizeof(T));
                std::memcpy(newData.data() + (_M_data.size() - head), _M_data.data(), tail * sizeof(T));
            }
            _M_data = std::move(newData);
            head = 0;
            tail = sz;
            needsResize = false;
        }

    public:
        using value_type = typename std::vector<T>::value_type;
        using reference = typename std::vector<T>::reference;
        using const_reference = typename std::vector<T>::const_reference;
        using pointer = typename std::vector<T>::pointer;

        explicit Ring(std::size_t defaultSize) : _M_data(defaultSize) {}

        Ring() : _M_data(16) {}

        void push_back(T data) {
            if (needsResize)
                resize();
            this->_M_data[tail] = data;
            inc_tail();
            sz++;
        }

        void move_back(T&& data) {
            if (needsResize)
                resize();
            this->_M_data[tail] = std::move(data);
            inc_tail();
            sz++;
        }

        void pop_back() {
            if (sz == 0)
                throw std::out_of_range("size is already zero.");
            dec_tail();
            sz--;
        }

        void push_front(T data) {
            if (needsResize)
                resize();
            dec_head();
            this->_M_data[head] = data;
            sz++;
        }

        void pop_front() {
            if (sz == 0)
                throw std::out_of_range("size is already zero.");
            inc_head();
            sz--;
        }

        void clear() {
            head = 0;
            tail = 0;
            sz = 0;
        }

        /**
         * Rebuild the buffer such that the data inside is now continuous
         */
        void make_cont() {
            if (tail >= head && head == 0)
                return;

            std::vector<T> newData;
            newData.resize(_M_data.size());

            // put it all in order
            if (tail >= head) {
                // it's actually already in order.
                std::memcpy(newData.data(), _M_data.data() + head, sz * sizeof(T));
            } else {
                // it's not quite in order
                std::memcpy(newData.data(), _M_data.data() + head, (_M_data.size() - head) * sizeof(T));
                std::memcpy(newData.data() + (_M_data.size() - head), _M_data.data(), tail * sizeof(T));
            }
            _M_data = std::move(newData);
            head = 0;
            tail = sz;
        }

        template <typename IterType>
        void assign(IterType start, IterType end) {
            sz = end - start;
            _M_data.resize(sz);
            std::copy(start, end, _M_data.begin());
            head = 0;
            tail = end - start;
        }

        void resize(std::size_t target) {
            std::vector<T> newData;
            newData.resize(target);
            if (target > _M_data.size()) {
                // put it all in order
                if (tail >= head) {
                    // it's actually already in order.
                    std::memcpy(newData.data(), _M_data.data() + head, sz * sizeof(T));
                } else {
                    // it's not quite in order
                    std::memcpy(newData.data(), _M_data.data() + head, (_M_data.size() - head) * sizeof(T));
                    std::memcpy(newData.data() + (_M_data.size() - head), _M_data.data(), tail * sizeof(T));
                }
            } else {
                // we gonna run out of space, chop off the end
                if (tail >= head) {
                    // it's actually already in order.
                    std::memcpy(newData.data(), _M_data.data() + head, target * sizeof(T));
                } else {
                    // it's not quite in order
                    std::memcpy(newData.data(), _M_data.data() + head, std::min((_M_data.size() - head), target) * sizeof(T));
                    if (target > _M_data.size() - head)
                        std::memcpy(newData.data() + (_M_data.size() - head), _M_data.data(), (target - (_M_data.size() - head)) * sizeof(T));
                }
            }
            _M_data = std::move(newData);
            head = 0;
            tail = sz;
            needsResize = false;
        }

        void used(std::size_t count) {
            if (count > _M_data.size()) {
                resize(count);
            }
            tail = count;
            sz = count;
            if (sz == _M_data.size())
                needsResize = true;
        }

        std::vector<T>& data() {
            return _M_data;
        }

        [[nodiscard]] std::size_t size() const {
            return sz;
            //if (tail > head)
            //    return tail - head;
            //return data.size() - head + tail;
        }

        [[nodiscard]] std::size_t capacity() const {
            return _M_data.size();
        }

        class iterator {
            Ring* parent;
            long long index;

            iterator(Ring* parent, const long long index) : parent(parent), index(index) {}

            friend class Ring;

        public:
            using value_type = typename std::vector<T>::value_type;
            using difference_type = std::ptrdiff_t;
            using reference = typename std::vector<T>::reference;
            using pointer = typename std::vector<T>::pointer;
            using iterator_category = std::random_access_iterator_tag;

            iterator operator+(long long count) {
                iterator n = *this;
                n.index += count;
                if (n.index < 0)
                    n.index += parent->_M_data.size();
                if (n.index > parent->_M_data.size())
                    n.index -= parent->_M_data.size();
                return n;
            }

            iterator operator-(const long long count) {
                return operator+(-count);
            }

            difference_type operator-(const iterator& other) {
#ifdef DWHBLL_HARDEN
                if (parent != other.parent) {
                    throw std::invalid_argument("taking the difference between iterators from different objects make no sense.");
                }
#endif
                if (parent->tail > parent->head)
                    return index - other.index;

                // flatten out the indices.
                std::size_t ia = index;
                std::size_t ib = other.index;
                if (ia < parent->head)
                    ia += parent->_M_data.size();
                if (ib < parent->head)
                    ib += parent->_M_data.size();
                return static_cast<long>(ia - ib);
            }

            reference operator*() {
                return parent->_M_data[index];
            }

            pointer operator->() {
                return parent->_M_data.data() + index;
            }

            iterator& operator++() {
                index += 1;
                if (index >= parent->_M_data.size())
                    index -= parent->_M_data.size();
                return *this;
            }

            iterator operator++(int) {
                iterator tmp = *this;
                index += 1;
                if (index >= parent->_M_data.size())
                    index -= parent->_M_data.size();
                return tmp;
            }

            iterator& operator--() {
                index -= 1;
                if (index < 0)
                    index += parent->_M_data.size();
                return *this;
            }

            iterator operator--(int) {
                iterator tmp = *this;
                index -= 1;
                if (index < 0)
                    index += parent->_M_data.size();
                return tmp;
            }

            friend bool operator==(const iterator &lhs, const iterator &rhs) {
                return lhs.parent == rhs.parent
                       && lhs.index == rhs.index;
            }

            friend bool operator!=(const iterator &lhs, const iterator &rhs) {
                return !(lhs == rhs);
            }
        };

        class reverse_iterator {
            Ring* parent;
            long long index;

            reverse_iterator(Ring* parent, const long long index) : parent(parent), index(index) {}

            friend class Ring;

        public:
            using value_type = typename std::vector<T>::value_type;
            using difference_type = std::ptrdiff_t;
            using reference = typename std::vector<T>::reference;
            using pointer = typename std::vector<T>::pointer;
            using iterator_category = std::random_access_iterator_tag;

            reverse_iterator operator-(long long count) {
                reverse_iterator n = *this;
                n.index += count;
                if (n.index < 0)
                    n.index += parent->_M_data.size();
                if (n.index > parent->_M_data.size())
                    n.index -= parent->_M_data.size();
                return n;
            }

            reverse_iterator operator+(const long long count) {
                return operator-(-count);
            }

            // TODO: taking the difference between reverse iterators :xdd:
//             difference_type operator-(const reverse_iterator& other) {
// #ifdef DWHBLL_HARDEN
//                 if (parent != other.parent) {
//                     throw std::invalid_argument("taking the difference between iterators from different objects make no sense.");
//                 }
// #endif
//                 if (parent->tail > parent->head)
//                     return index - other.index;
//
//                 // flatten out the indices.
//                 std::size_t ia = index;
//                 std::size_t ib = other.index;
//                 if (ia < parent->head)
//                     ia += parent->data.size();
//                 if (ib < parent->head)
//                     ib += parent->data.size();
//                 return ia - ib;
//             }

            reference operator*() {
                return parent->_M_data[index];
            }

            pointer operator->() {
                return parent->_M_data.data() + index;
            }

            reverse_iterator& operator--() {
                index += 1;
                if (index >= parent->_M_data.size())
                    index -= parent->_M_data.size();
                return *this;
            }

            reverse_iterator operator--(int) {
                reverse_iterator tmp = *this;
                index += 1;
                if (index >= parent->_M_data.size())
                    index -= parent->_M_data.size();
                return tmp;
            }

            reverse_iterator& operator++() {
                index -= 1;
                if (index < 0)
                    index += parent->_M_data.size();
                return *this;
            }

            reverse_iterator operator++(int) {
                reverse_iterator tmp = *this;
                index -= 1;
                if (index < 0)
                    index += parent->_M_data.size();
                return tmp;
            }

            friend bool operator==(const reverse_iterator &lhs, const reverse_iterator &rhs) {
                return lhs.parent == rhs.parent
                       && lhs.index == rhs.index;
            }

            friend bool operator!=(const reverse_iterator &lhs, const reverse_iterator &rhs) {
                return !(lhs == rhs);
            }
        };

        class const_iterator {
            Ring* parent;
            long long index;

            const_iterator(Ring* parent, const long long index) : parent(parent), index(index) {}

            friend class Ring;

        public:
            using value_type = typename std::vector<T>::value_type;
            using difference_type = std::ptrdiff_t;
            using reference = typename std::vector<T>::reference;
            using pointer = typename std::vector<T>::pointer;
            using const_pointer = typename std::vector<T>::const_pointer;
            using iterator_category = std::random_access_iterator_tag;

            const_iterator operator+(long long count) {
                const_iterator n = *this;
                n.index += count;
                if (n.index < 0)
                    n.index += parent->_M_data.size();
                if (n.index > parent->_M_data.size())
                    n.index -= parent->_M_data.size();
                return n;
            }

            const_iterator operator-(const long long count) {
                return operator+(-count);
            }

            difference_type operator-(const const_iterator& other) {
#ifdef DWHBLL_HARDEN
                if (parent != other.parent) {
                    throw std::invalid_argument("taking the difference between iterators from different objects make no sense.");
                }
#endif
                if (parent->tail > parent->head)
                    return index - other.index;

                // flatten out the indices.
                std::size_t ia = index;
                std::size_t ib = other.index;
                if (ia < parent->head)
                    ia += parent->_M_data.size();
                if (ib < parent->head)
                    ib += parent->_M_data.size();
                return static_cast<long>(ia - ib);
            }

            const_reference operator*() const {
                return parent->_M_data[index];
            }

            const_pointer operator->() const {
                return parent->_M_data.data() + index;
            }

            const_iterator& operator++() {
                index += 1;
                if (index >= parent->_M_data.size())
                    index -= parent->_M_data.size();
                return *this;
            }

            const_iterator operator++(int) {
                const_iterator tmp = *this;
                index += 1;
                if (index >= parent->_M_data.size())
                    index -= parent->_M_data.size();
                return tmp;
            }

            const_iterator& operator--() {
                index -= 1;
                if (index < 0)
                    index += parent->_M_data.size();
                return *this;
            }

            const_iterator operator--(int) {
                const_iterator tmp = *this;
                index -= 1;
                if (index < 0)
                    index += parent->_M_data.size();
                return tmp;
            }

            friend bool operator==(const const_iterator &lhs, const const_iterator &rhs) {
                return lhs.parent == rhs.parent
                       && lhs.index == rhs.index;
            }

            friend bool operator!=(const const_iterator &lhs, const const_iterator &rhs) {
                return !(lhs == rhs);
            }
        };

        class const_reverse_iterator {
            Ring* parent;
            long long index;

            const_reverse_iterator(Ring* parent, const long long index) : parent(parent), index(index) {}

            friend class Ring;

        public:
            using value_type = typename std::vector<T>::value_type;
            using difference_type = std::ptrdiff_t;
            using reference = typename std::vector<T>::reference;
            using pointer = typename std::vector<T>::pointer;
            using const_pointer = typename std::vector<T>::const_pointer;
            using iterator_category = std::random_access_iterator_tag;

            const_reverse_iterator operator-(long long count) {
                const_reverse_iterator n = *this;
                n.index += count;
                if (n.index < 0)
                    n.index += parent->_M_data.size();
                if (n.index > parent->_M_data.size())
                    n.index -= parent->_M_data.size();
                return n;
            }

            const_reverse_iterator operator+(const long long count) {
                return operator-(-count);
            }

            // TODO: taking the difference between reverse iterators :xdd:
//             difference_type operator-(const const_reverse_iterator& other) {
// #ifdef DWHBLL_HARDEN
//                 if (parent != other.parent) {
//                     throw std::invalid_argument("taking the difference between iterators from different objects make no sense.");
//                 }
// #endif
//                 if (parent->tail > parent->head)
//                     return index - other.index;
//
//                 // flatten out the indices.
//                 std::size_t ia = index;
//                 std::size_t ib = other.index;
//                 if (ia < parent->head)
//                     ia += parent->data.size();
//                 if (ib < parent->head)
//                     ib += parent->data.size();
//                 return ia - ib;
//             }

            const_reference operator*() const {
                return parent->_M_data[index];
            }

            const_pointer operator->() {
                return parent->_M_data.data() + index;
            }

            const_reverse_iterator& operator--() {
                index += 1;
                if (index >= parent->_M_data.size())
                    index -= parent->_M_data.size();
                return *this;
            }

            const_reverse_iterator operator--(int) {
                const_reverse_iterator tmp = *this;
                index += 1;
                if (index >= parent->_M_data.size())
                    index -= parent->_M_data.size();
                return tmp;
            }

            const_reverse_iterator& operator++() {
                index -= 1;
                if (index < 0)
                    index += parent->_M_data.size();
                return *this;
            }

            const_reverse_iterator operator++(int) {
                const_reverse_iterator tmp = *this;
                index -= 1;
                if (index < 0)
                    index += parent->_M_data.size();
                return tmp;
            }

            friend bool operator==(const const_reverse_iterator &lhs, const const_reverse_iterator &rhs) {
                return lhs.parent == rhs.parent
                       && lhs.index == rhs.index;
            }

            friend bool operator!=(const const_reverse_iterator &lhs, const const_reverse_iterator &rhs) {
                return !(lhs == rhs);
            }
        };

        reference operator[](std::size_t index)
#ifndef DWHBLL_HARDEN
            noexcept
#endif
        {
            // compute the index
            index += head;
            if (index > _M_data.size())
                index -= _M_data.size();
#ifdef DWHBLL_HARDEN
            // bounds check
            if (index >= tail)
                throw std::out_of_range("index is out of range.");
#endif
            return _M_data[index];
        }

        const_reference operator[](std::size_t index) const
#ifndef DWHBLL_HARDEN
            noexcept
#endif
        {
            // compute the index
            index += head;
            if (index > _M_data.size())
                index -= _M_data.size();
#ifdef DWHBLL_HARDEN
            // bounds check
            if (index >= tail)
                throw std::out_of_range("index is out of range.");
#endif
            return _M_data[index];
        }

        /**
         * @brief unlike operator[], this one is ALWAYS bounds checked
         * @param index the index to access
         * @throws std::out_of_range when index is out of bounds
         * @return reference to the data.
         */
        reference at(std::size_t index) {
            // compute the index
            index += head;
            if (index > _M_data.size())
                index -= _M_data.size();
            // bounds check
            if (index >= tail)
                throw std::out_of_range("index is out of range.");
            return _M_data[index];
        }

        /**
         * @brief unlike operator[], this one is ALWAYS bounds checked
         * @param index the index to access
         * @throws std::out_of_range when index is out of bounds
         * @return reference to the data.
         */
        const_reference at(std::size_t index) const {
            // compute the index
            index += head;
            if (index > _M_data.size())
                index -= _M_data.size();
            // bounds check
            if (index >= tail)
                throw std::out_of_range("index is out of range.");
            return _M_data[index];
        }

        [[nodiscard]] bool empty() const noexcept {
            return sz == 0;
        }

        reference front()
#ifndef DWHBLL_HARDEN
            noexcept
#endif
        {
#ifdef DWHBLL_HARDEN
            if (empty())
                throw std::out_of_range("ring buffer is empty.");
#endif
            return _M_data[head];
        }

        const_reference front() const
#ifndef DWHBLL_HARDEN
            noexcept
#endif
        {
#ifdef DWHBLL_HARDEN
            if (empty())
                throw std::out_of_range("ring buffer is empty.");
#endif
            return _M_data[head];
        }

        reference back()
#ifndef DWHBLL_HARDEN
            noexcept
#endif
        {
#ifdef DWHBLL_HARDEN
            if (empty())
                throw std::out_of_range("ring buffer is empty.");
#endif
            return _M_data[tail == 0 ? _M_data.size() - 1 : tail - 1];
        }

        const_reference back() const
#ifndef DWHBLL_HARDEN
            noexcept
#endif
        {
#ifdef DWHBLL_HARDEN
            if (empty())
                throw std::out_of_range("ring buffer is empty.");
#endif
            return _M_data[tail == 0 ? _M_data.size() - 1 : tail - 1];
        }

        constexpr iterator begin() noexcept {
            return iterator(this, head);
        }

        constexpr const_iterator begin() const noexcept {
            return const_iterator(this, head);
        }

        constexpr const_iterator cbegin() const noexcept {
            return const_iterator(this, head);
        }

        constexpr iterator end() noexcept {
            return iterator(this, tail);
        }

        constexpr const_iterator end() const noexcept {
            return const_iterator(this, tail);
        }

        constexpr const_iterator cend() const noexcept {
            return const_iterator(this, tail);
        }

        // TODO: reverse iterators are not yet provided.
    };
}
