#include <dwhbll/macros/testing.h>

import dwhbll.testing;
import dwhbll.collections;
import std;

using namespace dwhbll::test;

namespace collections::ring {

[[=test]]
void fill_capacity()
{
    dwhbll::collections::Ring<int> ringBuffer;

    std::size_t before = ringBuffer.size();

    for (std::size_t i = 0; i < ringBuffer.capacity(); i++) {
        // fill the entire current ring buffer size
        ringBuffer.push_back(i);
    }

    std::size_t after = ringBuffer.size();

    // resized???
    EXPECT(after - before == ringBuffer.capacity());
}

[[=test]]
void pop_front()
{
    dwhbll::collections::Ring<int> ringBuffer;

    for (std::size_t i = 0; i < ringBuffer.capacity(); i++)
        ringBuffer.push_back(i);

    std::size_t before = ringBuffer.size();

    for (int i = 0; i < 5; i++) {
        ringBuffer.pop_front();
    }

    std::size_t after = ringBuffer.size();

    // resized???
    EXPECT(after + 5 == before);
}

[[=test]]
void push_front_no_resize()
{
    dwhbll::collections::Ring<int> ringBuffer;

    for (std::size_t i = 0; i < ringBuffer.capacity(); i++)
        ringBuffer.push_back(i);
    for (int i = 0; i < 5; i++)
        ringBuffer.pop_front();

    std::size_t before = ringBuffer.size();

    for (int i = 4; i >= 0; i--) {
        ringBuffer.push_front(i);
    }

    std::size_t after = ringBuffer.size();

    // resized???
    EXPECT(after - 5 == before);
}

[[=test]]
void push_back_triggers_resize()
{
    dwhbll::collections::Ring<int> ringBuffer;

    for (std::size_t i = 0; i < ringBuffer.capacity(); i++)
        ringBuffer.push_back(i);
    for (int i = 4; i >= 0; i--)
        ringBuffer.push_front(i);

    std::size_t before = ringBuffer.size() - 5;

    for (int i = 0; i < 5; i++) {
        ringBuffer.push_back(i);
    }

    std::size_t after = ringBuffer.size();

    // no resized???
    EXPECT(after != before);
}

[[=test]]
void iterator_after_resize()
{
    dwhbll::collections::Ring<int> ringBuffer;

    for (std::size_t i = 0; i < ringBuffer.capacity(); i++)
        ringBuffer.push_back(i);

    for (int i = 0; i < 5; i++)
        ringBuffer.pop_front();

    for (int i = 4; i >= 0; i--)
        ringBuffer.push_front(i);

    // trigger resize
    for (int i = 0; i < 5; i++)
        ringBuffer.push_back(i);

    for (int i = 0; i < 5; i++)
        ringBuffer.pop_back();

    int expected = 0;
    for (int entry : ringBuffer) {
        EXPECT_EQ(entry, expected);
        expected++;
    }
}

}

TEST_REGISTER_FILE();
