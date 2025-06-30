#include <dwhbll/collections/streams.hpp>

#include <functional>
#include <optional>
#include <unordered_map>
#include <string>
#include <iostream>
#include <span>
#include <vector>
#include <fstream>
#include <filesystem> // For file operations
#include <numeric>    // For std::iota
#include <stdexcept>
#include <type_traits> // For std::is_enum_v
#include <sstream>     // For std::ostringstream

// Alias for brevity
namespace sc = dwhbll::collections::stream;
namespace fs = std::filesystem; // Alias for filesystem

// --- Helper Macros for Testing ---
// These macros provide a basic assertion framework for unit tests.
// They print a message to cerr and return false on failure.

#define TEST_NAME(name) \
    std::cout << "Running test: " << (name) << std::endl;

#define ASSERT_TRUE(condition) \
    if (!(condition)) {        \
        std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false;          \
    }

#define ASSERT_FALSE(condition) \
    if ((condition)) {          \
        std::cerr << "Assertion failed: !" << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false;           \
    }

// Helper to print enum class values correctly
template<typename T>
auto print_value(T val) {
    if constexpr (std::is_enum_v<T>) {
        return static_cast<std::underlying_type_t<T>>(val);
    } else {
        return val;
    }
}

// Helper to print vector values correctly
template<typename T>
std::string print_value_vec(const std::vector<T>& val) {
    std::ostringstream ss;
    ss << "[";
    for(size_t i = 0; i < val.size(); ++i) {
        ss << static_cast<int>(val[i]); // Cast to int for numeric types like uint8_t
        if (i < val.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}

#define ASSERT_EQ(val1, val2) \
    if (!((val1) == (val2))) { \
        std::cerr << "Assertion failed: " << #val1 << " (" << print_value(val1) << ") != " << #val2 << " (" << print_value(val2) << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false;          \
    }

#define ASSERT_EQ_VEC(val1, val2) \
    if (!((val1) == (val2))) { \
        std::cerr << "Assertion failed: " << #val1 << " (" << print_value_vec(val1) << ") != " << #val2 << " (" << print_value_vec(val2) << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false;          \
    }

#define ASSERT_NE(val1, val2) \
    if (!((val1) == (val2))) { \
        std::cerr << "Assertion failed: " << #val1 << " (" << print_value(val1) << ") == " << #val2 << " (" << print_value(val2) << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false;          \
    }

#define ASSERT_ERROR_EQ(buffer_or_reader, expected_error) \
    if (!((buffer_or_reader).fail() && (buffer_or_reader).last_error() == (expected_error))) { \
        std::cerr << "Assertion failed: Expected error " << static_cast<int>(expected_error) << " but got " \
                  << static_cast<int>((buffer_or_reader).last_error()) << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    }

#define ASSERT_NO_ERROR(buffer_or_reader) \
    if ((buffer_or_reader).fail()) { \
        std::cerr << "Assertion failed: Expected no error but got " \
                  << static_cast<int>((buffer_or_reader).last_error()) << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    }

// --- Forward Declarations for Test Functions ---
bool test_memory_buffer();
bool test_file_buffer();
bool test_stream_reader();
bool test_cached_reader();

// --- Helper Functions for FileBuffer Tests ---
const std::string TEST_FILENAME = "temp_test_file.bin";

// Creates a temporary binary file with the given data
void create_test_file(const std::string& filename, const std::vector<uint8_t>& data) {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs.is_open()) {
        throw std::runtime_error("Failed to create test file: " + filename);
    }
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
    ofs.close();
}

// Deletes the temporary test file
void delete_test_file(const std::string& filename) {
    fs::remove(filename);
}

// --- MemoryBuffer Tests ---
bool test_memory_buffer() {
    TEST_NAME("MemoryBuffer Tests");

    // Test 1: Constructor from std::vector<uint8_t> and basic read
    std::vector<uint8_t> data1 = {0x01, 0x02, 0x03, 0x04, 0x05};
    sc::MemoryBuffer mb1(data1);
    ASSERT_TRUE(mb1.good());
    ASSERT_EQ(mb1.size(), 5);
    ASSERT_EQ(mb1.position(), 0);
    ASSERT_EQ(mb1.remaining(), 5);

    std::vector<uint8_t> read_buf1(3);
    std::size_t bytes_read1 = mb1.read_raw_bytes(read_buf1);
    ASSERT_EQ(bytes_read1, 3);
    ASSERT_EQ(read_buf1[0], 0x01);
    ASSERT_EQ(read_buf1[1], 0x02);
    ASSERT_EQ(read_buf1[2], 0x03);
    ASSERT_EQ(mb1.position(), 3);
    ASSERT_EQ(mb1.remaining(), 2);
    ASSERT_NO_ERROR(mb1);

    bytes_read1 = mb1.read_raw_bytes(read_buf1); // Try to read 3 more, but only 2 remain
    ASSERT_EQ(bytes_read1, 2);
    ASSERT_EQ(read_buf1[0], 0x04);
    ASSERT_EQ(read_buf1[1], 0x05);
    ASSERT_EQ(mb1.position(), 5);
    ASSERT_EQ(mb1.remaining(), 0);
    ASSERT_ERROR_EQ(mb1, dwhbll::collections::stream::Error::EndOfData);

    bytes_read1 = mb1.read_raw_bytes(read_buf1); // Try to read past end
    ASSERT_EQ(bytes_read1, 0);
    ASSERT_TRUE(mb1.eof());
    ASSERT_EQ(mb1.last_error(), sc::Error::EndOfData);
    mb1.clear_error();
    ASSERT_NO_ERROR(mb1);

    // Test 2: Constructor from std::span<const uint8_t>
    std::vector<uint8_t> data2 = {0x10, 0x11, 0x12};
    sc::MemoryBuffer mb2(data2);
    ASSERT_TRUE(mb2.good());
    ASSERT_EQ(mb2.size(), 3);
    ASSERT_EQ(mb2.position(), 0);

    // Test 3: Constructor from std::string
    std::string str_data = "Hello World";
    sc::MemoryBuffer mb3(str_data);
    ASSERT_TRUE(mb3.good());
    ASSERT_EQ(mb3.size(), str_data.length());
    ASSERT_EQ(mb3.position(), 0);

    std::vector<uint8_t> read_str_buf(5);
    mb3.read_raw_bytes(read_str_buf);
    ASSERT_EQ(std::string(reinterpret_cast<char*>(read_str_buf.data()), 5), "Hello");
    ASSERT_EQ(mb3.position(), 5);

    // Test 4: Seek and Skip
    sc::MemoryBuffer mb4(data1); // {0x01, 0x02, 0x03, 0x04, 0x05}
    ASSERT_TRUE(mb4.seek(2));
    ASSERT_EQ(mb4.position(), 2);
    std::vector<uint8_t> single_byte(1);
    mb4.read_raw_bytes(single_byte);
    ASSERT_EQ(single_byte[0], 0x03);
    ASSERT_NO_ERROR(mb4);

    ASSERT_TRUE(mb4.skip(1));
    ASSERT_EQ(mb4.position(), 4);
    mb4.read_raw_bytes(single_byte);
    ASSERT_EQ(single_byte[0], 0x05);
    ASSERT_NO_ERROR(mb4);

    ASSERT_FALSE(mb4.seek(10)); // Seek past end
    ASSERT_TRUE(mb4.fail());
    ASSERT_TRUE(mb4.eof());
    mb4.clear_error();

    ASSERT_FALSE(mb4.skip(10)); // Skip past end
    ASSERT_TRUE(mb4.eof()); // pos_ should be at size()
    ASSERT_EQ(mb4.position(), mb4.size());
    mb4.clear_error();

    // Test 5: Peek
    sc::MemoryBuffer mb5(data1); // {0x01, 0x02, 0x03, 0x04, 0x05}
    std::vector<uint8_t> peek_buf(3);
    std::size_t bytes_peeked = mb5.peek_raw_bytes(peek_buf);
    ASSERT_EQ(bytes_peeked, 3);
    ASSERT_EQ(peek_buf[0], 0x01);
    ASSERT_EQ(peek_buf[1], 0x02);
    ASSERT_EQ(peek_buf[2], 0x03);
    ASSERT_EQ(mb5.position(), 0); // Position should not change
    ASSERT_NO_ERROR(mb5);

    bytes_peeked = mb5.peek_raw_bytes(peek_buf, 2); // Peek with offset
    ASSERT_EQ(bytes_peeked, 3);
    ASSERT_EQ(peek_buf[0], 0x03);
    ASSERT_EQ(peek_buf[1], 0x04);
    ASSERT_EQ(peek_buf[2], 0x05);
    ASSERT_EQ(mb5.position(), 0);
    ASSERT_NO_ERROR(mb5);

    bytes_peeked = mb5.peek_raw_bytes(peek_buf, 4); // Peek near end (peek 3, but only 1 remains)
    ASSERT_EQ(bytes_peeked, 1);
    ASSERT_EQ(peek_buf[0], 0x05);
    ASSERT_EQ(mb5.position(), 0);
    ASSERT_EQ(mb5.last_error(), sc::Error::EndOfData);
    mb5.clear_error();

    bytes_peeked = mb5.peek_raw_bytes(peek_buf, 5); // Peek past end
    ASSERT_EQ(bytes_peeked, 0);
    ASSERT_TRUE(mb5.eof());
    ASSERT_EQ(mb5.last_error(), sc::Error::EndOfData);
    mb5.clear_error();

    // Test 6: Empty buffer
    std::vector<uint8_t> empty_data;
    sc::MemoryBuffer mb_empty(empty_data);
    ASSERT_EQ(mb_empty.size(), 0);
    ASSERT_EQ(mb_empty.position(), 0);
    ASSERT_EQ(mb_empty.remaining(), 0);

    std::vector<uint8_t> read_buf_empty(0);
    bytes_read1 = mb_empty.read_raw_bytes(read_buf_empty);
    ASSERT_EQ(bytes_read1, 0);
    ASSERT_TRUE(mb_empty.fail());
    ASSERT_TRUE(mb_empty.last_error() == sc::Error::GenericError);
    mb_empty.clear_error();

    bytes_peeked = mb_empty.peek_raw_bytes(read_buf_empty);
    ASSERT_EQ(bytes_peeked, 0);
    ASSERT_TRUE(mb_empty.fail());
    ASSERT_TRUE(mb_empty.last_error() == sc::Error::GenericError);
    mb_empty.clear_error();

    ASSERT_FALSE(mb_empty.seek(1));
    ASSERT_TRUE(mb_empty.fail());
    ASSERT_TRUE(mb_empty.eof());
    mb_empty.clear_error();

    std::cout << "MemoryBuffer tests passed." << std::endl;
    return true;
}

// --- FileBuffer Tests ---
bool test_file_buffer() {
    TEST_NAME("FileBuffer Tests");

    std::vector<uint8_t> test_data = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33};

    create_test_file(TEST_FILENAME, test_data); // Create file for subsequent tests

    // Test 1: Constructor and basic properties
    sc::FileBuffer fb1(TEST_FILENAME);
    ASSERT_TRUE(fb1.good());
    ASSERT_EQ(fb1.size(), test_data.size());
    ASSERT_EQ(fb1.position(), 0);
    ASSERT_EQ(fb1.remaining(), test_data.size());
    ASSERT_NO_ERROR(fb1);

    // Test 2: read_raw_bytes
    std::vector<uint8_t> read_buf(5);
    std::size_t bytes_read = fb1.read_raw_bytes(read_buf);
    ASSERT_EQ(bytes_read, 5);
    std::vector<uint8_t> expected_read = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    ASSERT_EQ_VEC(read_buf, expected_read);
    ASSERT_EQ(fb1.position(), 5);
    ASSERT_EQ(fb1.remaining(), 5);
    ASSERT_NO_ERROR(fb1);

    bytes_read = fb1.read_raw_bytes(read_buf); // Read remaining 5 bytes
    ASSERT_EQ(bytes_read, 5);
    ASSERT_EQ_VEC(read_buf, std::vector<uint8_t>({0xFF, 0x00, 0x11, 0x22, 0x33}));
    ASSERT_EQ(fb1.position(), 10);
    ASSERT_EQ(fb1.remaining(), 0);
    ASSERT_NO_ERROR(fb1);

    bytes_read = fb1.read_raw_bytes(read_buf); // Read past EOF
    ASSERT_EQ(bytes_read, 0);
    ASSERT_TRUE(fb1.eof());
    ASSERT_EQ(fb1.last_error(), sc::Error::EndOfData);
    fb1.clear_error();

    // Test 3: seek
    sc::FileBuffer fb2(TEST_FILENAME);
    ASSERT_TRUE(fb2.seek(3));
    ASSERT_EQ(fb2.position(), 3);
    read_buf.resize(2);
    bytes_read = fb2.read_raw_bytes(read_buf);
    ASSERT_EQ(bytes_read, 2);
    ASSERT_EQ_VEC(read_buf, std::vector<uint8_t>({0xDD, 0xEE}));
    ASSERT_NO_ERROR(fb2);

    ASSERT_FALSE(fb2.seek(test_data.size() + 1)); // Seek past end
    ASSERT_TRUE(fb2.fail());
    ASSERT_TRUE(fb2.eof());
    fb2.clear_error();
    ASSERT_EQ(fb2.position(), 5); // Position should remain unchanged if seek fails

    // Test 4: skip
    sc::FileBuffer fb3(TEST_FILENAME);
    ASSERT_TRUE(fb3.skip(5));
    ASSERT_EQ(fb3.position(), 5);
    read_buf.resize(3);
    bytes_read = fb3.read_raw_bytes(read_buf);
    ASSERT_EQ(bytes_read, 3);
    ASSERT_EQ_VEC(read_buf, std::vector<uint8_t>({0xFF, 0x00, 0x11}));
    ASSERT_NO_ERROR(fb3);

    ASSERT_FALSE(fb3.skip(100)); // Skip past end
    ASSERT_TRUE(fb3.eof());
    ASSERT_EQ(fb3.position(), test_data.size()); // Position should go to end
    fb3.clear_error();

    // Test 5: peek_raw_bytes
    sc::FileBuffer fb4(TEST_FILENAME);
    read_buf.resize(5);
    std::size_t bytes_peeked = fb4.peek_raw_bytes(read_buf);
    ASSERT_EQ(bytes_peeked, 5);
    ASSERT_EQ_VEC(read_buf, std::vector<uint8_t>({0xAA, 0xBB, 0xCC, 0xDD, 0xEE}));
    ASSERT_EQ(fb4.position(), 0); // Position unchanged
    ASSERT_NO_ERROR(fb4);

    bytes_peeked = fb4.peek_raw_bytes(read_buf, 7); // Peek with offset
    ASSERT_EQ(bytes_peeked, 3); // Only 3 bytes remain from offset 7
    ASSERT_EQ(read_buf[0], 0x11);
    ASSERT_EQ(read_buf[1], 0x22);
    ASSERT_EQ(read_buf[2], 0x33);
    ASSERT_EQ(fb4.position(), 0);
    ASSERT_ERROR_EQ(fb4, sc::Error::EndOfData);

    bytes_peeked = fb4.peek_raw_bytes(read_buf, test_data.size()); // Peek at exact end, should be 0 bytes
    ASSERT_EQ(bytes_peeked, 0);
    ASSERT_TRUE(fb4.eof()); // Error should be set if trying to peek *at* end if dest.size() > 0
    fb4.clear_error();

    bytes_peeked = fb4.peek_raw_bytes(read_buf, test_data.size() + 1); // Peek past end
    ASSERT_EQ(bytes_peeked, 0);
    ASSERT_TRUE(fb4.eof()); // Should be EndOfData as peek cannot read
    fb4.clear_error();

    // Test 6: Empty file
    delete_test_file(TEST_FILENAME);
    create_test_file(TEST_FILENAME, {});
    sc::FileBuffer fb_empty(TEST_FILENAME);
    ASSERT_EQ(fb_empty.size(), 0);
    ASSERT_EQ(fb_empty.remaining(), 0);

    std::vector<uint8_t> empty_read_buf(1);
    bytes_read = fb_empty.read_raw_bytes(empty_read_buf);
    ASSERT_EQ(bytes_read, 0);
    ASSERT_TRUE(fb_empty.eof());
    fb_empty.clear_error();

    bytes_peeked = fb_empty.peek_raw_bytes(empty_read_buf);
    ASSERT_EQ(bytes_peeked, 0);
    ASSERT_TRUE(fb_empty.eof());
    fb_empty.clear_error();

    delete_test_file(TEST_FILENAME); // Clean up

    std::cout << "FileBuffer tests passed." << std::endl;
    return true;
}

// --- StreamReader Tests ---
bool test_stream_reader() {
    TEST_NAME("StreamReader Tests");

    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x00, 0x07, 0x08, 0x09}; // 0x00 is null terminator
    std::unique_ptr<sc::Buffer> mem_buffer = std::make_unique<sc::MemoryBuffer>(data);
    sc::StreamReader sr(std::move(mem_buffer));
    ASSERT_TRUE(sr.good());
    ASSERT_EQ(sr.size(), data.size());
    ASSERT_EQ(sr.position(), 0);
    ASSERT_EQ(sr.remaining(), data.size());
    ASSERT_NO_ERROR(sr);

    // Test 1: read_byte
    uint8_t byte;
    sr.read_byte(byte);
    ASSERT_TRUE(sr.good());
    ASSERT_EQ(byte, 0x01);
    ASSERT_EQ(sr.position(), 1);

    sr.read_byte(byte);
    ASSERT_TRUE(sr.good());
    ASSERT_EQ(byte, 0x02);
    ASSERT_EQ(sr.position(), 2);

    // Test 2: read_bytes
    std::vector<uint8_t> read_vec;
    sr.read_bytes(read_vec, 3);
    ASSERT_TRUE(sr.good());
    ASSERT_EQ(read_vec.size(), 3);
    ASSERT_EQ(read_vec[0], 0x03);
    ASSERT_EQ(read_vec[1], 0x04);
    ASSERT_EQ(read_vec[2], 0x05);
    ASSERT_EQ(sr.position(), 5);

    // Read remaining bytes with read_bytes, should cause EOF for next read
    sr.read_bytes(read_vec, 5); // Should read 0x06, 0x00, 0x07, 0x08, 0x09
    ASSERT_TRUE(sr.good());
    ASSERT_EQ(read_vec.size(), 5);
    ASSERT_EQ(read_vec[0], 0x06);
    ASSERT_EQ(read_vec[1], 0x00);
    ASSERT_EQ(read_vec[2], 0x07);
    ASSERT_EQ(read_vec[3], 0x08);
    ASSERT_EQ(read_vec[4], 0x09);
    ASSERT_EQ(sr.position(), 10);
    ASSERT_EQ(sr.remaining(), 0);

    sr.read_bytes(read_vec, 1); // Read past end
    ASSERT_TRUE(sr.fail());
    ASSERT_TRUE(sr.eof());
    ASSERT_EQ(sr.last_error(), sc::Error::EndOfData);
    sr.clear_error();
    ASSERT_NO_ERROR(sr);
    ASSERT_EQ(read_vec.size(), 0); // Should be empty on error

    // Test 3: seek and position
    sr.seek(0);
    ASSERT_TRUE(sr.good());
    ASSERT_EQ(sr.position(), 0);
    ASSERT_EQ(sr.remaining(), data.size());

    sr.seek(5);
    ASSERT_TRUE(sr.good());
    ASSERT_EQ(sr.position(), 5);
    ASSERT_EQ(sr.remaining(), data.size() - 5);

    sr.seek(data.size() + 1); // Seek past end
    ASSERT_TRUE(sr.fail());
    ASSERT_TRUE(sr.eof()); // Error propagated from MemoryBuffer
    sr.clear_error();

    // Test 4: skip
    sr.seek(0);
    sr.skip(4);
    ASSERT_TRUE(sr.good());
    ASSERT_EQ(sr.position(), 4);
    sr.read_byte(byte);
    ASSERT_EQ(byte, 0x05);

    sr.skip(100); // Skip past end
    ASSERT_TRUE(sr.fail());
    ASSERT_TRUE(sr.eof());
    sr.clear_error();
    ASSERT_EQ(sr.position(), data.size()); // Position goes to end

    // Test 5: read_until (uint8_t)
    std::unique_ptr<sc::Buffer> mem_buffer2 = std::make_unique<sc::MemoryBuffer>(data);
    sc::StreamReader sr2(std::move(mem_buffer2));

    std::vector<uint8_t> until_vec;
    sr2.read_until(until_vec, 0x06, true); // Consume delimiter
    ASSERT_TRUE(sr2.good());
    ASSERT_EQ_VEC(until_vec, std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04, 0x05}));
    ASSERT_EQ(sr2.position(), 6); // Position after 0x06

    sr2.seek(0);
    sr2.read_until(until_vec, 0x06, false); // Do not consume delimiter
    ASSERT_TRUE(sr2.good());
    ASSERT_EQ_VEC(until_vec, std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04, 0x05}));
    ASSERT_EQ(sr2.position(), 5); // Position before 0x06

    sr2.seek(0);
    sr2.read_until(until_vec, 0x99); // Delimiter not found (read until EOF)
    ASSERT_TRUE(sr2.fail());
    ASSERT_TRUE(sr2.eof());
    ASSERT_EQ_VEC(until_vec, data); // Should have read all data
    ASSERT_EQ(sr2.position(), data.size());
    sr2.clear_error();

    // Test 6: read_until (char)
    std::string char_data_str = "abc:def:ghi";
    std::vector<uint8_t> char_data(char_data_str.begin(), char_data_str.end());
    std::unique_ptr<sc::Buffer> mem_buffer_char = std::make_unique<sc::MemoryBuffer>(char_data);
    sc::StreamReader sr_char(std::move(mem_buffer_char));

    std::vector<uint8_t> char_vec;
    sr_char.read_until(char_vec, ':');
    ASSERT_TRUE(sr_char.good());
    ASSERT_EQ(std::string(char_vec.begin(), char_vec.end()), "abc");
    ASSERT_EQ(sr_char.position(), 4); // After ':'

    // Test 7: read_string
    std::vector<uint8_t> str_data_vec = {'H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd', '\0', 'T', 'e', 's', 't'};
    std::unique_ptr<sc::Buffer> mem_buffer_str = std::make_unique<sc::MemoryBuffer>(str_data_vec);
    sc::StreamReader sr_str(std::move(mem_buffer_str));

    std::string read_str;
    sr_str.read_string(read_str);
    ASSERT_TRUE(sr_str.good());
    ASSERT_EQ(read_str, "HelloWorld");
    ASSERT_EQ(sr_str.position(), 11); // After the null terminator

    sr_str.seek(12); // Seek to 'e' in "Test"
    sr_str.read_string(read_str);
    ASSERT_TRUE(sr_str.fail());
    ASSERT_TRUE(sr_str.eof());
    ASSERT_EQ(read_str, "est");
    sr_str.clear_error();

    // Test 8: read_all
    std::unique_ptr<sc::Buffer> mem_buffer3 = std::make_unique<sc::MemoryBuffer>(data);
    sc::StreamReader sr3(std::move(mem_buffer3));
    std::vector<uint8_t> all_data;
    sr3.read_all(all_data);
    ASSERT_TRUE(sr3.good());
    ASSERT_EQ_VEC(all_data, data);
    ASSERT_EQ(sr3.position(), data.size());

    // Test 9: peek_byte
    std::unique_ptr<sc::Buffer> mem_buffer4 = std::make_unique<sc::MemoryBuffer>(data);
    sc::StreamReader sr4(std::move(mem_buffer4));
    uint8_t peeked_byte;
    sr4.peek_byte(peeked_byte);
    ASSERT_TRUE(sr4.good());
    ASSERT_EQ(peeked_byte, 0x01);
    ASSERT_EQ(sr4.position(), 0); // Position unchanged

    sr4.skip(data.size() - 1); // Skip to last byte
    sr4.peek_byte(peeked_byte);
    ASSERT_TRUE(sr4.good());
    ASSERT_EQ(peeked_byte, 0x09);
    ASSERT_EQ(sr4.position(), data.size() - 1);

    sr4.skip(1); // Now at EOF
    sr4.peek_byte(peeked_byte);
    ASSERT_TRUE(sr4.fail());
    ASSERT_TRUE(sr4.eof());
    sr4.clear_error();

    // Test 10: peek_bytes
    std::unique_ptr<sc::Buffer> mem_buffer5 = std::make_unique<sc::MemoryBuffer>(data);
    sc::StreamReader sr5(std::move(mem_buffer5));
    std::vector<uint8_t> peeked_vec;
    sr5.peek_bytes(peeked_vec, 5);
    ASSERT_TRUE(sr5.good());
    ASSERT_EQ_VEC(peeked_vec, std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04, 0x05}));
    ASSERT_EQ(sr5.position(), 0);

    sr5.skip(7); // Position at 0x08
    sr5.peek_bytes(peeked_vec, 5); // Only 3 bytes left
    ASSERT_TRUE(sr5.fail()); // Fail because it couldn't peek 'count' bytes
    ASSERT_TRUE(sr5.eof());
    ASSERT_EQ_VEC(peeked_vec, std::vector<uint8_t>({0x07, 0x08, 0x09}));
    ASSERT_EQ(sr5.position(), 7); // Position unchanged
    sr5.clear_error();

    // Test 11: Null source buffer in StreamReader constructor
    try {
        sc::StreamReader sr_null(nullptr);
        // Should set an error internally, but not throw based on current code
        ASSERT_TRUE(sr_null.fail());
        ASSERT_EQ(sr_null.last_error(), sc::Error::GenericError); // Error from internal check
    } catch (const std::exception& e) {
        // This case should ideally not throw from StreamReader constructor, but rather set error state.
        // If it throws (e.g. from a custom unique_ptr with strict assertions), then catch it.
        std::cerr << "Unexpected exception for null StreamReader: " << e.what() << std::endl;
        return false;
    }


    std::cout << "StreamReader tests passed." << std::endl;
    return true;
}

// --- CachedReader Tests ---
bool test_cached_reader() {
    TEST_NAME("CachedReader Tests");

    std::vector<uint8_t> data_long;
    data_long.reserve(200);
    for (int i = 0; i < 200; ++i) {
        data_long.push_back(static_cast<uint8_t>(i % 256));
    }

    // Test 1: Basic read_byte - cache hit and miss
    std::unique_ptr<sc::Buffer> mb1 = std::make_unique<sc::MemoryBuffer>(data_long);
    sc::CachedReader cr1(std::move(mb1), 10); // Cache capacity 10

    uint8_t byte;
    // Read first 10 bytes (cache fill then hits)
    for (int i = 0; i < 10; ++i) {
        cr1.read_byte(byte);
        ASSERT_TRUE(cr1.good());
        ASSERT_EQ(byte, data_long[i]);
        ASSERT_EQ(cr1.position(), i + 1);
    }
    ASSERT_NO_ERROR(cr1);

    // Read 11th byte (should cause cache miss and refill)
    cr1.read_byte(byte);
    ASSERT_TRUE(cr1.good());
    ASSERT_EQ(byte, data_long[10]);
    ASSERT_EQ(cr1.position(), 11);
    ASSERT_NO_ERROR(cr1);

    // Test 2: read_bytes - smaller than cache
    std::unique_ptr<sc::Buffer> mb2 = std::make_unique<sc::MemoryBuffer>(data_long);
    sc::CachedReader cr2(std::move(mb2), 20); // Cache capacity 20

    std::vector<uint8_t> read_buf;
    cr2.read_bytes(read_buf, 5); // Read 5 bytes, cache_capacity_ is 20
    ASSERT_TRUE(cr2.good());
    ASSERT_EQ(read_buf.size(), 5);
    ASSERT_EQ(read_buf[0], data_long[0]);
    ASSERT_EQ(cr2.position(), 5);

    cr2.read_bytes(read_buf, 10); // Read 10 bytes, still within first cache block
    ASSERT_TRUE(cr2.good());
    ASSERT_EQ(read_buf.size(), 10);
    ASSERT_EQ(read_buf[0], data_long[5]);
    ASSERT_EQ(cr2.position(), 15);
    ASSERT_NO_ERROR(cr2);

    // Test 3: read_bytes - larger than cache
    std::unique_ptr<sc::Buffer> mb3 = std::make_unique<sc::MemoryBuffer>(data_long);
    sc::CachedReader cr3(std::move(mb3), 10); // Cache capacity 10

    cr3.read_bytes(read_buf, 25); // Read 25 bytes (2.5 cache fills)
    ASSERT_TRUE(cr3.good());
    ASSERT_EQ(read_buf.size(), 25);
    for (int i = 0; i < 25; ++i) {
        ASSERT_EQ(read_buf[i], data_long[i]);
    }
    ASSERT_EQ(cr3.position(), 25);
    ASSERT_NO_ERROR(cr3);

    // Test 4: seek - invalidates cache
    std::unique_ptr<sc::Buffer> mb4 = std::make_unique<sc::MemoryBuffer>(data_long);
    sc::CachedReader cr4(std::move(mb4), 10);

    cr4.read_byte(byte); // Fill cache
    ASSERT_EQ(cr4.position(), 1);
    cr4.seek(50); // Seek to new position
    ASSERT_TRUE(cr4.good());
    ASSERT_EQ(cr4.position(), 50);

    // Read from new position - should trigger cache fill
    cr4.read_byte(byte);
    ASSERT_TRUE(cr4.good());
    ASSERT_EQ(byte, data_long[50]);
    ASSERT_EQ(cr4.position(), 51);
    ASSERT_NO_ERROR(cr4);

    cr4.seek(data_long.size() + 1); // Seek past end
    ASSERT_TRUE(cr4.fail());
    ASSERT_TRUE(cr4.eof()); // From underlying reader seek error
    cr4.clear_error();

    // Test 5: skip - invalidates cache
    std::unique_ptr<sc::Buffer> mb5 = std::make_unique<sc::MemoryBuffer>(data_long);
    sc::CachedReader cr5(std::move(mb5), 10);

    cr5.read_byte(byte); // Fill cache
    ASSERT_EQ(cr5.position(), 1);
    cr5.skip(50); // Skip forward
    ASSERT_TRUE(cr5.good());
    ASSERT_EQ(cr5.position(), 51); // 1 (initial pos) + 50 (skipped)

    cr5.read_byte(byte); // Read from new position - should trigger cache fill
    ASSERT_TRUE(cr5.good());
    ASSERT_EQ(byte, data_long[51]);
    ASSERT_EQ(cr5.position(), 52);
    ASSERT_NO_ERROR(cr5);

    cr5.skip(data_long.size()); // Skip way past end
    ASSERT_TRUE(cr5.fail());
    ASSERT_TRUE(cr5.eof());
    ASSERT_EQ(cr5.last_error(), sc::Error::EndOfData);
    ASSERT_EQ(cr5.position(), data_long.size()); // Position goes to end
    cr5.clear_error();

    // Test 6: read_all
    std::unique_ptr<sc::Buffer> mb6 = std::make_unique<sc::MemoryBuffer>(data_long);
    sc::CachedReader cr6(std::move(mb6), 10);
    std::vector<uint8_t> all_data_read;
    cr6.read_all(all_data_read);
    ASSERT_TRUE(cr6.good());
    ASSERT_EQ_VEC(all_data_read, data_long);
    ASSERT_EQ(cr6.position(), data_long.size());
    ASSERT_NO_ERROR(cr6);

    // Test 7: peek_byte - cache hit and miss (without advancing position)
    std::unique_ptr<sc::Buffer> mb7 = std::make_unique<sc::MemoryBuffer>(data_long);
    sc::CachedReader cr7(std::move(mb7), 10);

    // Peek first byte (cache fill, then peek)
    cr7.peek_byte(byte);
    ASSERT_TRUE(cr7.good());
    ASSERT_EQ(byte, data_long[0]);
    ASSERT_EQ(cr7.position(), 0); // Position unchanged
    ASSERT_NO_ERROR(cr7);

    cr7.skip(9); // Position at 9
    cr7.peek_byte(byte); // Peek at 9 (still within first cache block)
    ASSERT_TRUE(cr7.good());
    ASSERT_EQ(byte, data_long[9]);
    ASSERT_EQ(cr7.position(), 9); // Position unchanged by peek
    ASSERT_NO_ERROR(cr7);

    cr7.skip(1); // Position at 10 (cache boundary)
    cr7.peek_byte(byte); // Peek at 10 (should cause underlying read, no cache fill)
    ASSERT_TRUE(cr7.good());
    ASSERT_EQ(byte, data_long[10]);
    ASSERT_EQ(cr7.position(), 10); // Position unchanged by peek
    ASSERT_NO_ERROR(cr7);

    cr7.seek(data_long.size() - 1); // Seek to last byte
    cr7.peek_byte(byte);
    ASSERT_TRUE(cr7.good());
    ASSERT_EQ(byte, data_long.back());
    ASSERT_EQ(cr7.position(), data_long.size() - 1);

    cr7.skip(1); // Now at EOF
    cr7.peek_byte(byte);
    ASSERT_TRUE(cr7.fail());
    ASSERT_TRUE(cr7.eof());
    cr7.clear_error();

    // Test 8: peek_bytes - multiple reads from underlying source
    std::unique_ptr<sc::Buffer> mb8 = std::make_unique<sc::MemoryBuffer>(data_long);
    sc::CachedReader cr8(std::move(mb8), 10); // Cache capacity 10

    std::vector<uint8_t> peek_vec;
    cr8.peek_bytes(peek_vec, 25); // Peek 25 bytes (2.5 cache blocks worth)
    ASSERT_TRUE(cr8.good());
    ASSERT_EQ(peek_vec.size(), 25);
    for (int i = 0; i < 25; ++i) {
        ASSERT_EQ(peek_vec[i], data_long[i]);
    }
    ASSERT_EQ(cr8.position(), 0); // Position must be unchanged
    ASSERT_NO_ERROR(cr8);

    cr8.skip(data_long.size() - 2); // Position at last two bytes
    cr8.peek_bytes(peek_vec, 5); // Try to peek 5, but only 2 available
    ASSERT_TRUE(cr8.fail());
    ASSERT_TRUE(cr8.eof());
    ASSERT_EQ_VEC(peek_vec, std::vector<uint8_t>({data_long[data_long.size() - 2], data_long[data_long.size() - 1]}));
    ASSERT_EQ(cr8.position(), data_long.size() - 2);
    cr8.clear_error();

    std::cout << "CachedReader tests passed." << std::endl;
    return true;
}

// --- Main Test Dispatcher ---
bool stream_test(std::optional<std::string> test_to_run) {
    if (!test_to_run || *test_to_run == "MemoryBuffer") {
        if (!test_memory_buffer()) return false;
    }
    if (!test_to_run || *test_to_run == "FileBuffer") {
        if (!test_file_buffer()) return false;
    }
    if (!test_to_run || *test_to_run == "StreamReader") {
        if (!test_stream_reader()) return false;
    }
    if (!test_to_run || *test_to_run == "CachedReader") {
        if (!test_cached_reader()) return false;
    }
    return true;
}
