#pragma once

#include "dwhbll/sanify/types.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <expected>
#include <filesystem>
#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <span>
#include <algorithm>
#include <format>

namespace dwhbll::collections::stream {

using namespace dwhbll::sanify;

enum class Error {
    NoError = 0,
    // Usually means that there was an error in the parameters passed
    GenericError,
    EndOfData,
    InvalidPositionError,
    DecompressionError,
    FileOpenError,
    Unimplemented
};

template <typename T>
using Result = std::expected<T, Error>;

class Buffer {
public:
    virtual ~Buffer() = default; 

    virtual Result<size_t> read_raw_bytes(std::span<u8> dest) = 0;
    virtual Result<size_t> peek_raw_bytes(std::span<u8> dest) = 0;

    virtual Result<void> seek(size_t pos) = 0;
    virtual Result<void> skip(size_t count) = 0;

    virtual Result<size_t> position() const = 0;
    virtual Result<size_t> size() const = 0;
    virtual Result<size_t> remaining() const = 0;
};

class MemoryBuffer : public Buffer {
private:
    std::vector<u8> data_;  
    size_t pos_ = 0; 

public:
    explicit MemoryBuffer(std::vector<u8> data)
        : data_(std::move(data)) {}
    
    explicit MemoryBuffer(std::span<const u8> data)
        : data_(data.begin(), data.end()) {}
    
    explicit MemoryBuffer(const std::string& str)
        : data_(str.begin(), str.end()) {}
    
    Result<size_t> read_raw_bytes(std::span<u8> dest) override {
        if (dest.empty()) {
            return std::unexpected(Error::GenericError);
        }

        auto rem = remaining();
        if (!rem)
            return std::unexpected(rem.error());
        
        size_t bytes_to_read = std::min(dest.size(), rem.value());
        
        std::copy(data_.begin() + pos_, data_.begin() + pos_ + bytes_to_read, dest.begin());
        pos_ += bytes_to_read;
        
        return bytes_to_read;
    }
    
    Result<size_t> peek_raw_bytes(std::span<u8> dest) override {
        if (dest.empty()) {
            return std::unexpected(Error::GenericError);
        }
        
        auto rem = remaining();
        if (!rem)
            return std::unexpected(rem.error());
        
        size_t bytes_to_peek = std::min(dest.size(), rem.value());

        std::copy(data_.begin() + pos_, data_.begin() + pos_ + bytes_to_peek, dest.begin());
        
        return bytes_to_peek;
    }

    Result<void> seek(size_t pos) override {
        if (pos > data_.size()) { 
            return std::unexpected(Error::InvalidPositionError);
        }

        pos_ = pos;
        return {};
    }

    Result<void> skip(size_t count) override {
        size_t new_pos = pos_ + count;
        if (new_pos > data_.size()) { 
            return std::unexpected(Error::InvalidPositionError);
        }

        pos_ = new_pos;
        return {};
    }

    Result<size_t> position() const override {
        return pos_;
    }
    
    Result<size_t> size() const override {
        return data_.size();
    }
    
    Result<size_t> remaining() const override {
        return data_.size() - pos_;
    }
};

class FileBuffer : public Buffer {
private:
    std::filesystem::path path_;
    std::ifstream file_;
    size_t pos_ = 0;

public:
    explicit FileBuffer(const std::filesystem::path& path) 
        : file_(path, std::ios::binary), 
          path_(path),
          pos_(0) {
        if (!file_.is_open()) {
            throw std::ios_base::failure(std::format("Failed to open {}", path.string()));
        }
    }

    ~FileBuffer() {
        if (file_.is_open()) {
            file_.close();
        }
    }

    Result<size_t> read_raw_bytes(std::span<u8> dest) override {
        if (dest.empty()) {
            return std::unexpected(Error::GenericError);
        }
        
        auto rem = remaining();
        if (!rem)
            return std::unexpected(rem.error());

        size_t bytes_to_read = std::min(rem.value(), dest.size());

        file_.read(reinterpret_cast<char*>(dest.data()), bytes_to_read);
        size_t bytes_read = file_.gcount(); 
        pos_ += bytes_read;
        
        return bytes_read;
    }

    Result<size_t> peek_raw_bytes(std::span<u8> dest) override {
        if (dest.empty()) {
            return std::unexpected(Error::GenericError);
        }
        
        assert(pos_ == file_.tellg());

        auto rem = remaining();
        if (!rem)
            return std::unexpected(rem.error());

        size_t bytes_to_peek = std::min(dest.size(), rem.value());

        file_.read(reinterpret_cast<char*>(dest.data()), bytes_to_peek);
        size_t bytes_peeked = static_cast<std::size_t>(file_.gcount());

        // Clear EOF and restore position
        file_.clear();
        file_.seekg(static_cast<std::streamoff>(pos_));
        
        return bytes_peeked;
    }

    Result<size_t> position() const override {
        return pos_;
    }
    
    Result<void> seek(size_t pos) override {
        auto sz = size();
        if (!sz)
            return std::unexpected(sz.error());

        if (pos > sz.value()) { 
            return std::unexpected(Error::InvalidPositionError);
        }
        
        file_.seekg(static_cast<std::streamoff>(pos));
        if (file_.fail()) { 
            return std::unexpected(Error::GenericError);
        }
        
        pos_ = pos;
        return {};
    }
    
    Result<size_t> size() const override {
        try {
            return std::filesystem::file_size(path_);
        }
        catch(const std::exception& e) {
            return std::unexpected(Error::GenericError);
        }
    }
    
    Result<size_t> remaining() const override {
        auto sz = size();
        if (!sz) 
            return std::unexpected(sz.error());

        if (pos_ > sz.value()) 
            return std::unexpected(Error::InvalidPositionError);

        return sz.value() - pos_;
    }
    
    Result<void> skip(size_t count) override {
        auto sz = size();
        if (!sz) 
            return std::unexpected(sz.error());

        size_t new_pos = pos_ + count;
        if (new_pos > sz.value()) { 
            return std::unexpected(Error::InvalidPositionError);
        }
        
        file_.seekg(count, std::ios::cur); 
        if (!file_)
            return std::unexpected(Error::GenericError);
        
        pos_ = new_pos;
        return {};
    }
};

class Reader {
public:
    virtual ~Reader() = default;

    virtual Result<u8> read_byte() = 0;
    virtual Result<std::vector<u8>> read_bytes(size_t count) = 0;
    
    // Templates for char/u8 compatibility
    template <typename T>
    requires (sizeof(T) == sizeof(u8))
    Result<std::vector<T>> read_bytes(size_t count) {
        auto result = read_bytes(count);
        if (!result)
            return std::unexpected(result.error());
        
        std::vector<T> converted;
        converted.reserve(result.value().size());
        for (auto byte : result.value()) {
            converted.push_back(static_cast<T>(byte));
        }
        return converted;
    }

    template <typename T>
    requires std::is_same_v<T, u8> || std::is_same_v<T, char>
    Result<std::vector<T>> read_until(T delimiter, bool consume_delimiter = true) {
        static_assert(sizeof(T) == sizeof(u8));
        auto result = read_until(static_cast<u8>(delimiter), consume_delimiter);
        if (!result)
            return std::unexpected(result.error());
        
        std::vector<T> converted;
        converted.reserve(result.value().size());
        for (auto byte : result.value()) {
            converted.push_back(static_cast<T>(byte));
        }
        return converted;
    }
    
    virtual Result<std::vector<u8>> read_until(uint8_t delimiter, bool consume_delimiter = true) = 0;
    virtual Result<std::string> read_string() = 0;
    
    virtual Result<void> seek(size_t pos) = 0;
    virtual Result<void> skip(size_t count) = 0;
    
    virtual Result<size_t> position() const = 0;
    virtual Result<size_t> size() const = 0;
    virtual Result<size_t> remaining() const = 0;
    
    virtual Result<std::vector<u8>> read_all() = 0;
    
    virtual Result<u8> peek_byte() = 0;
    virtual Result<std::vector<u8>> peek_bytes(size_t count) = 0;
};

class StreamReader : public Reader {
private:
    std::unique_ptr<Buffer> source_buffer_; 

public:
    explicit StreamReader(std::unique_ptr<Buffer> buffer)
        : source_buffer_(std::move(buffer)) {
        if (!source_buffer_) {
            throw std::invalid_argument("Buffer cannot be null");
        }
    }
    
    Result<u8> read_byte() override {
        std::array<u8, 1> temp_byte_array;
        auto result = source_buffer_->read_raw_bytes(temp_byte_array);
        
        if (!result)
            return std::unexpected(result.error());
        
        if (result.value() == 0) {
            return std::unexpected(Error::EndOfData);
        }
        
        return temp_byte_array[0];
    }
    
    Result<std::vector<u8>> read_bytes(size_t count) override {
        if (count == 0) {
            return std::vector<u8>{};
        }
        
        std::vector<u8> buf(count);
        auto result = source_buffer_->read_raw_bytes(buf);
        
        if (!result)
            return std::unexpected(result.error());
        
        buf.resize(result.value());
        return buf;
    }
    
    Result<std::vector<u8>> read_until(uint8_t delimiter, bool consume_delimiter = true) override {
        std::vector<u8> data;
        
        while (true) {
            auto byte_result = read_byte();
            if (!byte_result) {
                if (byte_result.error() == Error::EndOfData && !data.empty()) {
                    // Found some data before EOF
                    return data;
                }
                return std::unexpected(byte_result.error());
            }
            
            u8 byte = byte_result.value();
            
            if (byte == delimiter) {
                if (!consume_delimiter) {
                    auto pos = source_buffer_->position();
                    if (!pos)
                        return std::unexpected(pos.error());
                    
                    auto seek_result = source_buffer_->seek(pos.value() - 1);
                    if (!seek_result)
                        return std::unexpected(seek_result.error());
                }
                return data;
            }
            
            data.push_back(byte);
        }
    }
    
    Result<std::string> read_string() override {
        std::string str;
        
        while (true) {
            auto byte_result = read_byte();
            if (!byte_result) {
                if (byte_result.error() == Error::EndOfData && !str.empty()) {
                    // Found some data before EOF
                    return str;
                }
                return std::unexpected(byte_result.error());
            }
            
            u8 byte = byte_result.value();
            
            if (byte == 0) {
                return str;
            }
            
            str.push_back(static_cast<char>(byte));
        }
    }
    
    Result<size_t> position() const override {
        return source_buffer_->position();
    }
    
    Result<void> seek(size_t pos) override {
        return source_buffer_->seek(pos);
    }
    
    Result<size_t> size() const override {
        return source_buffer_->size();
    }
    
    Result<size_t> remaining() const override {
        return source_buffer_->remaining();
    }

    Result<std::vector<u8>> read_all() override {
        auto rem = source_buffer_->remaining();
        if (!rem)
            return std::unexpected(rem.error());
        
        if (rem.value() == 0) {
            return std::vector<u8>{};
        }
        
        return read_bytes(rem.value());
    }

    Result<void> skip(size_t count) override {
        return source_buffer_->skip(count);
    }

    Result<u8> peek_byte() override {
        std::array<u8, 1> temp_byte_array;
        auto result = source_buffer_->peek_raw_bytes(temp_byte_array);
        
        if (!result)
            return std::unexpected(result.error());
        
        if (result.value() == 0) {
            return std::unexpected(Error::EndOfData);
        }
        
        return temp_byte_array[0];
    }
    
    Result<std::vector<u8>> peek_bytes(size_t count) override {
        if (count == 0) {
            return std::vector<u8>{};
        }
        
        std::vector<u8> data(count);
        auto result = source_buffer_->peek_raw_bytes(data);
        
        if (!result)
            return std::unexpected(result.error());
        
        data.resize(result.value());
        return data;
    }
};

class CachedReader : public Reader {
private:
    std::unique_ptr<Buffer> source_buffer_;
    std::vector<u8> cache_;
    const size_t cache_chunk_size_;
    size_t pos_ = 0;
    size_t cache_start_pos_ = 0;

    Result<void> update_cache() {
        if (pos_ >= cache_start_pos_ && pos_ < cache_start_pos_ + cache_.size()) {
            return {};
        }

        auto seek_result = source_buffer_->seek(pos_);
        if (!seek_result)
            return std::unexpected(seek_result.error());

        cache_.resize(cache_chunk_size_);
        auto read_result = source_buffer_->read_raw_bytes(cache_);
        if (!read_result)
            return std::unexpected(read_result.error());

        if (read_result.value() == 0) {
            return std::unexpected(Error::EndOfData);
        }

        cache_.resize(read_result.value());
        cache_start_pos_ = pos_;
        return {};
    }

public:
    explicit CachedReader(std::unique_ptr<Buffer> source_buffer, size_t cache_chunk_size = 4096)
        : source_buffer_(std::move(source_buffer)), cache_chunk_size_(cache_chunk_size) {
        if (!source_buffer_) {
            throw std::invalid_argument("Buffer cannot be null");
        }
    }

    Result<u8> read_byte() override {
        auto update_result = update_cache();
        if (!update_result)
            return std::unexpected(update_result.error());

        size_t cache_offset = pos_ - cache_start_pos_;
        if (cache_offset >= cache_.size()) {
            return std::unexpected(Error::EndOfData);
        }
        
        u8 byte = cache_[cache_offset];
        pos_++;
        return byte;
    }

    Result<std::vector<u8>> read_bytes(size_t count) override {
        if (count == 0) {
            return std::vector<u8>{};
        }
        
        std::vector<u8> buf;
        buf.reserve(count);
        
        while (count > 0) {
            auto update_result = update_cache();
            if (!update_result) {
                if (update_result.error() == Error::EndOfData && !buf.empty()) {
                    // Return partial data
                    return buf;
                }
                return std::unexpected(update_result.error());
            }

            size_t offset = pos_ - cache_start_pos_;
            size_t available = cache_.size() - offset;
            if (available == 0) {
                if (!buf.empty()) {
                    return buf; // Return partial data
                }
                return std::unexpected(Error::EndOfData);
            }

            size_t to_copy = std::min(available, count);

            buf.insert(buf.end(), cache_.begin() + offset,
                       cache_.begin() + offset + to_copy);
            pos_ += to_copy;
            count -= to_copy;
        }

        return buf;
    }

    Result<std::vector<u8>> read_until(uint8_t delimiter, bool consume_delimiter = true) override {
        std::vector<u8> data;
        
        while (true) {
            auto byte_result = read_byte();
            if (!byte_result) {
                if (byte_result.error() == Error::EndOfData && !data.empty()) {
                    return data;
                }
                return std::unexpected(byte_result.error());
            }
            
            u8 byte = byte_result.value();
            
            if (byte == delimiter) {
                if (!consume_delimiter) {
                    pos_--; // Move back one position
                }
                return data;
            }
            
            data.push_back(byte);
        }
    }
    
    Result<std::string> read_string() override {
        std::string str;
        
        while (true) {
            auto byte_result = read_byte();
            if (!byte_result) {
                if (byte_result.error() == Error::EndOfData && !str.empty()) {
                    return str;
                }
                return std::unexpected(byte_result.error());
            }
            
            u8 byte = byte_result.value();
            
            if (byte == 0) {
                return str;
            }
            
            str.push_back(static_cast<char>(byte));
        }
    }

    Result<void> seek(size_t pos) override {
        auto sz = size();
        if (!sz)
            return std::unexpected(sz.error());
        
        if (pos > sz.value()) {
            return std::unexpected(Error::InvalidPositionError);
        }
        
        pos_ = pos;
        return {};
    }

    Result<void> skip(size_t count) override {
        auto sz = size();
        if (!sz)
            return std::unexpected(sz.error());
        
        if (pos_ + count > sz.value()) {
            return std::unexpected(Error::InvalidPositionError);
        }
        
        pos_ += count;
        return {};
    }

    Result<size_t> position() const override {
        return pos_;
    }

    Result<size_t> size() const override {
        return source_buffer_->size();
    }

    Result<size_t> remaining() const override {
        auto sz = size();
        if (!sz)
            return std::unexpected(sz.error());
        
        return sz.value() - pos_;
    }

    Result<std::vector<u8>> read_all() override {
        auto seek_result = seek(0);
        if (!seek_result)
            return std::unexpected(seek_result.error());
        
        auto rem = remaining();
        if (!rem)
            return std::unexpected(rem.error());
        
        return read_bytes(rem.value());
    }

    Result<u8> peek_byte() override {
        size_t original_pos = pos_;
        auto result = read_byte();
        pos_ = original_pos;
        return result;
    }

    Result<std::vector<u8>> peek_bytes(size_t count) override {
        size_t original_pos = pos_;
        auto result = read_bytes(count);
        pos_ = original_pos;
        return result;
    }
};

}
