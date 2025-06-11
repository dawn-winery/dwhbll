#include <iostream>
#include <bits/ostream.tcc>
#include <dwhbll/collections/ring.h>

bool ring_test(std::optional<std::string> test_to_run) {
    dwhbll::collections::Ring<int> ringBuffer;

    std::size_t before = ringBuffer.size();

    for (int i = 0; i < ringBuffer.capacity(); i++) {
        // fill the entire current ring buffer size
        ringBuffer.push_back(i);
    }

    std::size_t after = ringBuffer.size();

    if (after - before != ringBuffer.capacity()) {
        // resized???
        std::cerr << "[FAILED] ring buffer got resized." << std::endl;
        return false;
    }

    before = ringBuffer.size();

    for (int i = 0; i < 5; i++) {
        ringBuffer.pop_front();
    }

    after = ringBuffer.size();

    if (after + 5 != before) {
        // resized???
        std::cerr << "[FAILED] ring buffer failed pops." << std::endl;
        return false;
    }

    before = ringBuffer.size();

    for (int i = 4; i >= 0; i--) {
        ringBuffer.push_front(i);
    }

    after = ringBuffer.size();

    if (after - 5 != before) {
        // resized???
        std::cerr << "[FAILED] ring buffer resized while still space available." << std::endl;
        return false;
    }

    before = ringBuffer.size();

    for (int i = 0; i < 5; i++) {
        ringBuffer.push_back(i);
    }

    after = ringBuffer.size();

    if (after == before) {
        // no resized???
        std::cerr << "[FAILED] ring buffer did not resize when no space available." << std::endl;
        return false;
    }

    for (int i = 0; i < 5; i++) {
        ringBuffer.pop_back();
    }

    int expected = 0;
    for (int entry : ringBuffer) {
        if (entry != expected) {
            std::cerr << "[FAILED] ring buffer resize did not correctly rearrange data / iterator broken. " << std::format("(expected: {}, got: {})", expected, entry) << std::endl;
            return false;
        }
        expected++;
    }

    return true;
}