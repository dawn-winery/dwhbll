#include <dwhbll/collections/streams.hpp>

#include <functional>
#include <optional>
#include <string>
#include <iostream>
#include <span>
#include <print>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <filesystem>

using namespace dwhbll::collections::stream;

std::filesystem::path create_test_file(const std::string& content) {
    auto temp_path = std::filesystem::temp_directory_path() / "stream_test.tmp";
    std::ofstream file(temp_path, std::ios::binary);
    file.write(content.data(), content.size());
    file.close();
    return temp_path;
}

std::vector<uint8_t> generate_test_data(size_t size) {
    std::vector<uint8_t> data;
    data.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        data.push_back(static_cast<uint8_t>(i % 256));
    }
    return data;
}

bool test_memory_buffer() {
    std::string test_string = "Hello, World!\nThis is a test string.";
    MemoryBuffer buffer(test_string);
    
    // Test size and position
    {
        auto size = buffer.size();
        assert(size.has_value());
        assert(size.value() == test_string.size());
        
        auto pos = buffer.position();
        assert(pos.has_value());
        assert(pos.value() == 0);
        
        auto remaining = buffer.remaining();
        assert(remaining.has_value());
        assert(remaining.value() == test_string.size());
    }
    
    // Test reading bytes
    {
        std::array<uint8_t, 5> read_buffer;
        auto result = buffer.read_raw_bytes(read_buffer);
        assert(result.has_value());
        assert(result.value() == 5);
        assert(std::string(read_buffer.begin(), read_buffer.end()) == "Hello");
        
        auto pos = buffer.position();
        assert(pos.has_value());
        assert(pos.value() == 5);
    }
    
    // Test peeking bytes
    {
        std::array<uint8_t, 3> peek_buffer;
        auto result = buffer.peek_raw_bytes(peek_buffer);
        assert(result.has_value());
        assert(result.value() == 3);
        assert(std::string(peek_buffer.begin(), peek_buffer.end()) == ", W");
        
        auto pos = buffer.position();
        assert(pos.has_value());
        assert(pos.value() == 5);
    }
    
    // Test seeking
    {
        auto seek_result = buffer.seek(7);
        assert(seek_result.has_value());
        
        auto pos = buffer.position();
        assert(pos.has_value());
        assert(pos.value() == 7);
        
        std::array<uint8_t, 5> read_buffer;
        auto result = buffer.read_raw_bytes(read_buffer);
        assert(result.has_value());
        assert(result.value() == 5);
        assert(std::string(read_buffer.begin(), read_buffer.end()) == "World");
    }
    
    // Test skipping
    {
        auto skip_result = buffer.skip(1);
        assert(skip_result.has_value());
        
        auto pos = buffer.position();
        assert(pos.has_value());
        assert(pos.value() == 13);
        
        std::array<uint8_t, 1> read_buffer;
        auto result = buffer.read_raw_bytes(read_buffer);
        assert(result.has_value());
        assert(result.value() == 1);
        assert(read_buffer[0] == '\n');
    }
    
    // Test error conditions
    {
        // Invalid seek
        auto seek_result = buffer.seek(test_string.size() + 1);
        assert(!seek_result.has_value());
        assert(seek_result.error() == Error::InvalidPositionError);
        
        // Invalid skip
        buffer.seek(test_string.size() - 1);
        auto skip_result = buffer.skip(2);
        assert(!skip_result.has_value());
        assert(skip_result.error() == Error::InvalidPositionError);
        
        // Empty span
        std::span<uint8_t> empty_span;
        auto read_result = buffer.read_raw_bytes(empty_span);
        assert(!read_result.has_value());
        assert(read_result.error() == Error::GenericError);
    }
    
    // Test with binary data
    {
        std::vector<uint8_t> binary_data = generate_test_data(1000);
        MemoryBuffer binary_buffer(binary_data);
        
        auto size = binary_buffer.size();
        assert(size.has_value());
        assert(size.value() == 1000);
        
        std::array<uint8_t, 256> read_buffer;
        auto result = binary_buffer.read_raw_bytes(read_buffer);
        assert(result.has_value());
        assert(result.value() == 256);
        
        for (size_t i = 0; i < 256; ++i) {
            assert(read_buffer[i] == static_cast<uint8_t>(i));
        }
    }
    
    return true;
}

bool test_file_buffer() {
    std::string test_content = "File buffer test content.\nSecond line.\nThird line with some data: 12345";
    auto temp_file = create_test_file(test_content);
    
    try {
        FileBuffer buffer(temp_file);
        
        // Test size
        {
            auto size = buffer.size();
            assert(size.has_value());
            assert(size.value() == test_content.size());
        }
        
        // Test reading
        {
            std::array<uint8_t, 10> read_buffer;
            auto result = buffer.read_raw_bytes(read_buffer);
            assert(result.has_value());
            assert(result.value() == 10);
            assert(std::string(read_buffer.begin(), read_buffer.end()) == "File buffe");
        }
        
        // Test peeking
        {
            std::array<uint8_t, 5> peek_buffer;
            auto result = buffer.peek_raw_bytes(peek_buffer);
            assert(result.has_value());
            assert(result.value() == 5);
            assert(std::string(peek_buffer.begin(), peek_buffer.end()) == "r tes");
            
            // Position should not have changed
            auto pos = buffer.position();
            assert(pos.has_value());
            assert(pos.value() == 10);
        }
        
        // Test seeking
        {
            auto seek_result = buffer.seek(26);
            assert(seek_result.has_value());
            
            std::array<uint8_t, 11> read_buffer;
            auto result = buffer.read_raw_bytes(read_buffer);
            assert(result.has_value());
            assert(result.value() == 11);
            assert(std::string(read_buffer.begin(), read_buffer.end()) == "Second line");
        }
        
        // Test skipping
        {
            auto skip_result = buffer.skip(2);
            assert(skip_result.has_value());
            
            std::array<uint8_t, 5> read_buffer;
            auto result = buffer.read_raw_bytes(read_buffer);
            assert(result.has_value());
            assert(result.value() == 5);
            assert(std::string(read_buffer.begin(), read_buffer.end()) == "Third");
        }
        
        // Test reading beyond EOF
        {
            buffer.seek(test_content.size() - 5);
            std::array<uint8_t, 10> read_buffer;
            auto result = buffer.read_raw_bytes(read_buffer);
            assert(result.has_value());
            assert(result.value() == 5); // Only 5 bytes available
        }
        
        // Test error conditions
        {
            auto seek_result = buffer.seek(test_content.size() + 1);
            assert(!seek_result.has_value());
            assert(seek_result.error() == Error::InvalidPositionError);
        }
        
        std::filesystem::remove(temp_file);
        
    } catch (const std::exception& e) {
        std::filesystem::remove(temp_file);
        std::cerr << "FileBuffer test failed: " << e.what() << std::endl;
        return false;
    }
    
    // Test with non-existent file
    try {
        FileBuffer bad_buffer("non_existent_file.tmp");
        std::cerr << "Should have thrown exception for non-existent file" << std::endl;
        return false;
    } catch (const std::ios_base::failure&) {
        // Expected behavior
    }
    
    return true;
}

bool test_stream_reader() {
    std::string test_data = "Hello\nWorld\nString\0Delimited\nText";
    test_data += '\0'; // Add null terminator
    test_data += "More data after null";
    
    auto buffer = std::make_unique<MemoryBuffer>(test_data);
    StreamReader reader(std::move(buffer));
    
    // Test reading single byte
    {
        auto result = reader.read_byte();
        assert(result.has_value());
        assert(result.value() == 'H');
    }
    
    // Test reading multiple bytes
    {
        auto result = reader.read_bytes(4);
        assert(result.has_value());
        assert(result.value().size() == 4);
        std::string read_str(result.value().begin(), result.value().end());
        assert(read_str == "ello");
    }
    
    // Test reading until delimiter
    {
        auto result = reader.read_until('\n');
        assert(result.has_value());
        assert(result.value().size() == 0); // Should be empty since we're at '\n'
        
        auto result2 = reader.read_until('\n');
        assert(result2.has_value());
        std::string read_str(result2.value().begin(), result2.value().end());
        assert(read_str == "World");
    }
    
    // Test reading string (null-terminated)
    {
        auto result = reader.read_string();
        assert(result.has_value());
        assert(result.value() == "String");
    }
    
    // Test position and seeking
    {
        auto pos = reader.position();
        assert(pos.has_value());
        
        auto seek_result = reader.seek(0);
        assert(seek_result.has_value());
        
        auto new_pos = reader.position();
        assert(new_pos.has_value());
        assert(new_pos.value() == 0);
    }
    
    // Test skipping
    {
        auto skip_result = reader.skip(6);
        assert(skip_result.has_value());
        
        auto result = reader.read_bytes(5);
        assert(result.has_value());
        std::string read_str(result.value().begin(), result.value().end());
        assert(read_str == "World");
    }
    
    // Test peeking
    {
        reader.seek(0);
        
        auto peek_result = reader.peek_byte();
        assert(peek_result.has_value());
        assert(peek_result.value() == 'H');
        
        auto pos = reader.position();
        assert(pos.has_value());
        assert(pos.value() == 0); // Position should not change
        
        auto peek_bytes_result = reader.peek_bytes(5);
        assert(peek_bytes_result.has_value());
        std::string peeked_str(peek_bytes_result.value().begin(), peek_bytes_result.value().end());
        assert(peeked_str == "Hello");
        
        auto pos2 = reader.position();
        assert(pos2.has_value());
        assert(pos2.value() == 0); // Position should still not change
    }
    
    // Test reading all
    {
        auto result = reader.read_all();
        assert(result.has_value());
        assert(result.value().size() == test_data.size());
        
        std::string all_data(result.value().begin(), result.value().end());
        assert(all_data == test_data);
    }
    
    // Test reading until delimiter without consuming
    {
        reader.seek(0);
        auto result = reader.read_until('\n', false);
        assert(result.has_value());
        std::string read_str(result.value().begin(), result.value().end());
        assert(read_str == "Hello");
        
        // Next byte should be the delimiter
        auto next_byte = reader.read_byte();
        assert(next_byte.has_value());
        assert(next_byte.value() == '\n');
    }
    
    // Test error conditions
    {
        reader.seek(test_data.size());
        auto result = reader.read_byte();
        assert(!result.has_value());
        assert(result.error() == Error::EndOfData);
        
        auto bytes_result = reader.read_bytes(10);
        assert(!bytes_result.has_value());
        assert(bytes_result.error() == Error::EndOfData);
    }
    
    return true;
}

bool test_cached_reader() {
    // Create a larger test data set to test caching behavior
    std::vector<uint8_t> large_data = generate_test_data(10000);
    
    // Add some specific patterns for testing
    std::string pattern = "CACHE_TEST_PATTERN\n";
    for (size_t i = 0; i < pattern.size(); ++i) {
        large_data[i] = pattern[i];
    }
    
    auto buffer = std::make_unique<MemoryBuffer>(large_data);
    CachedReader reader(std::move(buffer), 1024); // Use 1KB cache
    
    // Test basic reading
    {
        auto result = reader.read_byte();
        assert(result.has_value());
        assert(result.value() == 'C');
    }
    
    // Test reading multiple bytes
    {
        auto result = reader.read_bytes(4);
        assert(result.has_value());
        assert(result.value().size() == 4);
        std::string read_str(result.value().begin(), result.value().end());
        assert(read_str == "ACHE");
    }
    
    // Test seeking within cache
    {
        auto seek_result = reader.seek(0);
        assert(seek_result.has_value());
        
        auto result = reader.read_bytes(5);
        assert(result.has_value());
        std::string read_str(result.value().begin(), result.value().end());
        assert(read_str == "CACHE");
    }
    
    // Test seeking outside cache (should trigger cache update)
    {
        auto seek_result = reader.seek(5000);
        assert(seek_result.has_value());
        
        auto result = reader.read_byte();
        assert(result.has_value());
        assert(result.value() == static_cast<uint8_t>(5000 % 256));
    }
    
    // Test reading until delimiter
    {
        reader.seek(0);
        auto result = reader.read_until('\n');
        assert(result.has_value());
        std::string read_str(result.value().begin(), result.value().end());
        assert(read_str == "CACHE_TEST_PATTERN");
    }
    
    // Test reading string
    {
        // Add a null-terminated string to the data
        reader.seek(100);
        auto buffer2 = std::make_unique<MemoryBuffer>(std::string("Test\0String", 11));
        CachedReader reader2(std::move(buffer2), 64);
        
        auto result = reader2.read_string();
        assert(result.has_value());
        assert(result.value() == "Test");
    }
    
    // Test peeking
    {
        reader.seek(0);
        
        auto peek_result = reader.peek_byte();
        assert(peek_result.has_value());
        assert(peek_result.value() == 'C');
        
        auto pos = reader.position();
        assert(pos.has_value());
        assert(pos.value() == 0);
        
        auto peek_bytes_result = reader.peek_bytes(10);
        assert(peek_bytes_result.has_value());
        std::string peeked_str(peek_bytes_result.value().begin(), peek_bytes_result.value().end());
        assert(peeked_str == "CACHE_TEST");
        
        auto pos2 = reader.position();
        assert(pos2.has_value());
        assert(pos2.value() == 0);
    }
    
    // Test reading all
    {
        auto result = reader.read_all();
        assert(result.has_value());
        assert(result.value().size() == large_data.size());
        
        for (size_t i = 0; i < pattern.size(); ++i) {
            assert(result.value()[i] == pattern[i]);
        }
    }
    
    // Test skipping
    {
        reader.seek(0);
        auto skip_result = reader.skip(6);
        assert(skip_result.has_value());
        
        auto result = reader.read_bytes(4);
        assert(result.has_value());
        std::string read_str(result.value().begin(), result.value().end());
        assert(read_str == "TEST");
    }
    
    // Test cross-cache boundary reading
    {
        reader.seek(1020); // Near cache boundary
        auto result = reader.read_bytes(10); // Read across boundary
        assert(result.has_value());
        assert(result.value().size() == 10);
        
        for (size_t i = 0; i < 10; ++i) {
            assert(result.value()[i] == static_cast<uint8_t>((1020 + i) % 256));
        }
    }
    
    // Test error conditions
    {
        auto seek_result = reader.seek(large_data.size() + 1);
        assert(!seek_result.has_value());
        assert(seek_result.error() == Error::InvalidPositionError);
        
        reader.seek(large_data.size() - 1);
        auto result = reader.read_bytes(10);
        assert(result.has_value());
        assert(result.value().size() == 1); // Only 1 byte available
    }
    
    return true;
}

static std::unordered_map<std::string, std::function<bool()>> test_dispatch {
    { "MemoryBuffer", test_memory_buffer },
    { "FileBuffer", test_file_buffer },
    { "StreamReader", test_stream_reader },
    { "CachedReader", test_cached_reader }
};

bool stream_test(std::optional<std::string> test_to_run) {
    if(test_to_run.has_value() && test_dispatch.contains(test_to_run.value())) {
        return !test_dispatch.at(test_to_run.value())(); 
    }

    return 1;
}
