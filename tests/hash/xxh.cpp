#include <dwhbll/testing/testing.h>
#include <dwhbll/testing/config.h>

#include <filesystem>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <dwhbll/console/logging.h>
#include <dwhbll/debug/debug.h>
#include <dwhbll/hash/xxh/xxh32.h>
#include <dwhbll/hash/xxh/xxh3_64.h>
#include <dwhbll/hash/xxh/xxh3_128.h>
#include <dwhbll/hash/xxh/xxh64.h>
#include <dwhbll/sanify/deferred.h>

using namespace dwhbll::test;

namespace xxh {
    uint16_t consume_u16_le(uint8_t* &ptr) {
        uint16_t result = (uint16_t)ptr[0] | ((uint16_t)ptr[1] << 8);
        ptr += 2;
        return result;
    }

    uint32_t consume_u32_le(uint8_t* &ptr) {
        return (uint32_t)consume_u16_le(ptr) | ((uint32_t)consume_u16_le(ptr) << 16);
    }

    uint64_t consume_u64_le(uint8_t* &ptr) {
        return (uint64_t)consume_u32_le(ptr) | ((uint64_t)consume_u32_le(ptr) << 32);
    }

    void test_xxh32(uint8_t* &ptr, const std::string &buffer, uint64_t seed, std::size_t &pass, std::size_t &total) {
        auto len = consume_u16_le(ptr);

        for (int i = 0; i < len; i++) {
            auto calculated = dwhbll::hash::xxh::detail32::hash(reinterpret_cast<const dwhbll::sanify::u8 *>(buffer.data()), i, seed);
            auto expected = consume_u32_le(ptr);
            if (calculated == expected)
                pass++;
            else
                dwhbll::debug::panic("Failed xxh32 (calculated({:#x}) != expected({:#x})) with seed {:#x}, buffer len({}) and data(\"{}\")!", calculated, expected, seed, i, buffer.substr(0, i));
            total++;
        }
    }

    void test_xxh64(uint8_t* &ptr, const std::string &buffer, uint64_t seed, std::size_t &pass, std::size_t &total) {
        auto len = consume_u16_le(ptr);

        for (int i = 0; i < len; i++) {
            auto calculated = dwhbll::hash::xxh::detail64::hash(reinterpret_cast<const dwhbll::sanify::u8 *>(buffer.data()), i, seed);
            auto expected = consume_u64_le(ptr);
            if (calculated == expected)
                pass++;
            else
                dwhbll::debug::panic("Failed xxh64 (calculated({:#x}) != expected({:#x})) with seed {:#x}, buffer len({}) and data(\"{}\")!", calculated, expected, seed, i, buffer.substr(0, i));
            total++;
        }
    }

    void test_xxh364seed(uint8_t* &ptr, const std::string &buffer, uint64_t seed, std::size_t &pass, std::size_t &total) {
        auto len = consume_u16_le(ptr);

        for (int i = 0; i < len; i++) {
            auto calculated = dwhbll::hash::xxh::detail3_64::hash(reinterpret_cast<const dwhbll::sanify::u8 *>(buffer.data()), i, seed);
            auto expected = consume_u64_le(ptr);
            if (calculated == expected)
                pass++;
            else
                dwhbll::debug::panic("Failed xxh3_64 (calculated({:#x}) != expected({:#x})) with seed {:#x}, buffer len({}) and data(\"{}\")!", calculated, expected, seed, i, buffer.substr(0, i));
            total++;
        }
    }

    void test_xxh364secret(uint8_t* &ptr, const std::string &buffer, std::span<dwhbll::sanify::u8> secret, std::size_t &pass, std::size_t &total) {
        auto len = consume_u16_le(ptr);

        for (int i = 0; i < len; i++) {
            auto calculated = dwhbll::hash::xxh::detail3_64::hash(reinterpret_cast<const dwhbll::sanify::u8 *>(buffer.data()), i, secret);
            auto expected = consume_u64_le(ptr);
            if (calculated == expected)
                pass++;
            else
                dwhbll::debug::panic("Failed xxh3_64 (calculated({:#x}) != expected({:#x})) with secret, buffer len({}) and data(\"{}\")!", calculated, expected, i, buffer.substr(0, i));
            total++;
        }
    }

    void test_xxh3128seed(uint8_t* &ptr, const std::string &buffer, uint64_t seed, std::size_t &pass, std::size_t &total) {
        auto len = consume_u16_le(ptr);

        for (int i = 0; i < len; i++) {
            auto calculated = dwhbll::hash::xxh::detail3_128::hash(reinterpret_cast<const dwhbll::sanify::u8 *>(buffer.data()), i, seed);
            auto expected_low = consume_u64_le(ptr);
            auto expected_high = consume_u64_le(ptr);
            if (calculated.high == expected_high && calculated.low == expected_low)
                pass++;
            else
                dwhbll::debug::panic("Failed xxh3_128 (calculated({:#x}{:x}) != expected({:#x}{:x})) with seed {:#x}, buffer len({}) and data(\"{}\")!", calculated.high, calculated.low, expected_high, expected_low, seed, i, buffer.substr(0, i));
            total++;
        }
    }

    void test_xxh3128secret(uint8_t* &ptr, const std::string &buffer, std::span<dwhbll::sanify::u8> secret, std::size_t &pass, std::size_t &total) {
        auto len = consume_u16_le(ptr);

        for (int i = 0; i < len; i++) {
            auto calculated = dwhbll::hash::xxh::detail3_128::hash(reinterpret_cast<const dwhbll::sanify::u8 *>(buffer.data()), i, secret);
            auto expected_low = consume_u64_le(ptr);
            auto expected_high = consume_u64_le(ptr);
            if (calculated.high == expected_high && calculated.low == expected_low)
                pass++;
            else
                dwhbll::debug::panic("Failed xxh3_128 (calculated({:#x}{:x}) != expected({:#x}{:x})) with secret, buffer len({}) and data(\"{}\")!", calculated.high, calculated.low, expected_high, expected_low, i, buffer.substr(0, i));
            total++;
        }
    }

    std::string consume_len_prefixed(uint8_t* &ptr) {
        auto len = consume_u16_le(ptr);
        std::string data;

        for (std::size_t i = 0; i < len; i++)
            data += (char)ptr[i];

        ptr += len;

        return data;
    }

    [[=test]]
    void xxh_smoke() {
        std::filesystem::path src_file(config::XXH_TEST_FILE);
        auto src_file_len = std::filesystem::file_size(src_file);
        int fd = open(src_file.c_str(), O_RDONLY);

        REQUIRE(fd != -1);

        auto bytes = (std::uint8_t*)mmap(nullptr, src_file_len, PROT_READ, MAP_PRIVATE, fd, 0);

        dwhbll::sanify::deferred _([&]() -> void {
            munmap(bytes, src_file_len);
            close(fd);
        });

        std::size_t pass = 0, total = 0;

        // drop file comment
        consume_len_prefixed(bytes);

        auto text = consume_len_prefixed(bytes);

        auto seed = consume_u64_le(bytes);

        auto secret_len = consume_u16_le(bytes);
        std::vector<uint8_t> secret(secret_len);
        for (int i = 0; i < secret_len; i++) {
            secret[i] = *bytes++;
        }

        ASSERT(consume_u16_le(bytes) == 4);

        // XXH32
        ASSERT(consume_u16_le(bytes) == 2);
        test_xxh32(bytes, text, 0, pass, total);
        test_xxh32(bytes, text, seed, pass, total);

        // XXH64
        ASSERT(consume_u16_le(bytes) == 2);
        test_xxh64(bytes, text, 0, pass, total);
        test_xxh64(bytes, text, seed, pass, total);

        // XXH3 64
        ASSERT(consume_u16_le(bytes) == 3);
        test_xxh364seed(bytes, text, 0, pass, total);
        test_xxh364seed(bytes, text, seed, pass, total);
        test_xxh364secret(bytes, text, secret, pass, total);

        // XXH3 128
        ASSERT(consume_u16_le(bytes) == 3);
        test_xxh3128seed(bytes, text, 0, pass, total);
        test_xxh3128seed(bytes, text, seed, pass, total);
        test_xxh3128secret(bytes, text, secret, pass, total);

        dwhbll::console::info("OVERALL STATUS: PASS: {}, TOTAL: {} (FAIL: {})", pass, total, total - pass);

        EXPECT(pass == total);
    }
}

TEST_REGISTER_FILE();
