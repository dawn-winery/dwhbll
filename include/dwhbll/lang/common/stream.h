#pragma once

#include <cstddef>
#include <optional>
#include <dwhbll/console/debug.hpp>

namespace dwhbll::lang::common {
    /**
     * @brief Stream type specialized for use in compilation tasks
     * @tparam T Type the stream provides
     * @tparam PeekCount Number of available lookahead tokens
     */
    template <typename T, std::size_t PeekCount>
    class Stream {
        struct RingObject {
            RingObject *next;
            std::optional<T> obj;
        };

        RingObject* head;
        RingObject* tail;
        std::size_t avail{};

        void prepare_ring() {
            tail = head = new RingObject;

            // We already created one
            for (std::size_t i = 1; i < PeekCount; i++) {
                tail->next = new RingObject;
                tail = tail->next;
            }

            // Create ring
            tail->next = head;

            // Empty the ring.
            tail = head;
            avail = 0;
        }

        void destroy_ring() {
            for (std::size_t i = 0; i < PeekCount; i++) {
                auto* cur = head;
                head->next = head;
                delete cur;
            }
        }

    protected:
        /**
         * @return The next object
         */
        virtual T next0() = 0;

        /**
         * @return If there exists another object
         */
        virtual bool has_next0() = 0;

    private:
        constexpr void _refill() {
            if (avail == PeekCount)
                return;

            while (avail < PeekCount && has_next0()) {
                tail->obj = next0();
                tail = tail->next;
                avail++;
            }
        }

    public:
        virtual ~Stream() {
            destroy_ring();
        }

        Stream() {
            prepare_ring();
        }

        constexpr bool has_next() {
            if (avail != 0)
                return true;

            _refill();

            return avail > 0;
        }

        constexpr T next() {
            if (!has_next())
                debug::panic("No more available tokens!");

            // extract data
            T val = std::move(head->obj.value());
            head->obj.reset();

            // advance head
            head = head->next;
            avail--;

            return std::move(val);
        }

        struct PeekTool {
            Stream* parent;
            RingObject* head;
            std::size_t avail;

            explicit PeekTool(Stream* parent) : parent(parent) {
                parent->_refill();

                head = parent->head;
                avail = parent->avail;
            }

            constexpr bool expect(const T& value) {
                if (avail == 0)
                    debug::panic("Nothing left to peek!");

                if (head->obj != value)
                    return false;

                // advance head
                head = head->next;
                avail--;
                return true;
            }

            template <typename R>
            constexpr bool expect(R&& values) {
                auto* prev_head = head;
                const auto prev_avail = avail;

                for (const T& value : values) {
                    if (!expect(value)) {
                        head = prev_head;
                        avail = prev_avail;
                        return false;
                    }
                }

                return true;
            }

            constexpr bool is(const T& value) {
                if (avail == 0)
                    debug::panic("Nothing left to peek!");

                return head->obj != value;
            }

            constexpr T peek() {
                if (avail == 0)
                    debug::panic("Nothing left to peek!");

                return head->obj;
            }

            [[nodiscard]] constexpr bool has_next() const {
                return avail > 0;
            }

            [[nodiscard]] constexpr std::size_t left() const {
                return avail;
            }

            /**
             * @brief Consume all the tokens we have peeked so far, refilling the
             * stream and resetting this PeekTool
             */
            constexpr void consume() {
                while (parent->head != head) {
                    parent->head = parent->head->next;
                    --parent->avail;
                }

                parent->_refill();
                head = parent->head;
                avail = parent->avail;
            }
        };

        constexpr PeekTool get_tool() {
            return PeekTool(this);
        }
    };
}
