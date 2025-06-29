#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <fstream>
#include <memory>
#include <span>
#include <expected>
#include <type_traits>

namespace dwhbll::collections::stream {

// TODO: add more errors (like failure to open file)
enum class Error {
    EndOfData,
    InvalidPosition,
};

template<typename T>
using Result = std::expected<T, Error>;

// Base Reader interface
class Reader {
public:
    virtual ~Reader() = default;
    
    virtual Result<std::uint8_t> read_byte() = 0;
    virtual Result<std::vector<std::uint8_t>> read_bytes(std::size_t count) = 0;
    virtual Result<std::string> read_string(std::size_t count) = 0;
    
    // Only for POD types
    template<typename T>
    requires std::is_trivially_copyable_v<T>
    Result<T> read() {
        auto bytes = read_bytes(sizeof(T));
        if (!bytes) return std::unexpected(bytes.error());
        
        if (bytes->size() != sizeof(T)) {
            return std::unexpected(Error::EndOfData);
        }
        
        T value;
        std::memcpy(&value, bytes->data(), sizeof(T));
        return value;
    }
    
    // Position operations
    virtual Result<std::size_t> position() const = 0;
    virtual Result<void> seek(std::size_t pos) = 0;
    virtual Result<std::size_t> size() const = 0;
    
    // Utility methods
    virtual bool at_end() const = 0;
    virtual Result<std::size_t> remaining() const = 0;
    
    // Read all remaining data
    virtual Result<std::vector<std::uint8_t>> read_all() = 0;
    
    // Skip bytes
    virtual Result<void> skip(std::size_t count) = 0;
    
    // Peek operations (read without advancing position)
    virtual Result<std::uint8_t> peek_byte() = 0;
    virtual Result<std::vector<std::uint8_t>> peek_bytes(std::size_t count) = 0;
};

// Reader that reads from a std::vector<std::uint8_t>
// TODO: maybe generalize to uint8_t*?
class MemoryReader : public Reader {
private:
    std::vector<std::uint8_t> data_;
    std::size_t pos_ = 0;
    
public:
    explicit MemoryReader(std::vector<std::uint8_t> data) 
        : data_(std::move(data)) {}
    
    explicit MemoryReader(std::span<const std::uint8_t> data) 
        : data_(data.begin(), data.end()) {}
    
    explicit MemoryReader(const std::string& str) 
        : data_(str.begin(), str.end()) {}
    
    Result<std::uint8_t> read_byte() override {
        if (pos_ >= data_.size()) {
            return std::unexpected(Error::EndOfData);
        }
        return data_[pos_++];
    }
    
    Result<std::vector<std::uint8_t>> read_bytes(std::size_t count) override {
        if (pos_ >= data_.size()) {
            return std::unexpected(Error::EndOfData);
        }
        
        std::size_t available = data_.size() - pos_;
        std::size_t to_read = std::min(count, available);
        
        std::vector<std::uint8_t> result(data_.begin() + pos_, data_.begin() + pos_ + to_read);
        pos_ += to_read;
        
        return result;
    }
    
    Result<std::string> read_string(std::size_t count) override {
        auto bytes = read_bytes(count);
        if (!bytes) return std::unexpected(bytes.error());
        
        return std::string(bytes->begin(), bytes->end());
    }
    
    Result<std::size_t> position() const override {
        return pos_;
    }
    
    Result<void> seek(std::size_t pos) override {
        if (pos > data_.size()) {
            return std::unexpected(Error::InvalidPosition);
        }
        pos_ = pos;
        return {};
    }
    
    Result<std::size_t> size() const override {
        return data_.size();
    }
    
    bool at_end() const override {
        return pos_ >= data_.size();
    }
    
    Result<std::size_t> remaining() const override {
        return data_.size() - pos_;
    }
    
    Result<std::vector<std::uint8_t>> read_all() override {
        if (pos_ >= data_.size()) {
            return std::vector<std::uint8_t>{};
        }
        
        std::vector<std::uint8_t> result(data_.begin() + pos_, data_.end());
        pos_ = data_.size();
        return result;
    }
    
    Result<void> skip(std::size_t count) override {
        std::size_t new_pos = pos_ + count;
        if (new_pos > data_.size()) {
            pos_ = data_.size();
        } else {
            pos_ = new_pos;
        }
        return {};
    }
    
    Result<std::uint8_t> peek_byte() override {
        if (pos_ >= data_.size()) {
            return std::unexpected(Error::EndOfData);
        }
        return data_[pos_];
    }
    
    Result<std::vector<std::uint8_t>> peek_bytes(std::size_t count) override {
        if (pos_ >= data_.size()) {
            return std::unexpected(Error::EndOfData);
        }
        
        std::size_t available = data_.size() - pos_;
        std::size_t to_read = std::min(count, available);
        
        return std::vector<std::uint8_t>(data_.begin() + pos_, data_.begin() + pos_ + to_read);
    }
};

// Reader that reads from a file
class FileReader : public Reader {
private:
    mutable std::ifstream file_;
    mutable std::size_t file_size_;
    
    void ensure_file_size() const {
        if (file_size_ == 0) {
            auto current_pos = file_.tellg();
            file_.seekg(0, std::ios::end);
            file_size_ = file_.tellg();
            file_.seekg(current_pos);
        }
    }
    
public:
    explicit FileReader(const std::string& filename) : file_(filename, std::ios::binary) {
        if (!file_.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        file_.seekg(0, std::ios::end);
        file_size_ = file_.tellg();
        file_.seekg(0, std::ios::beg);
    }
    
    Result<std::uint8_t> read_byte() override {
        char byte;
        if (!file_.read(&byte, 1)) {
            return std::unexpected(Error::EndOfData);
        }
        return static_cast<std::uint8_t>(byte);
    }
    
    Result<std::vector<std::uint8_t>> read_bytes(std::size_t count) override {
        std::vector<std::uint8_t> buffer(count);
        auto bytes_read = file_.read(reinterpret_cast<char*>(buffer.data()), count).gcount();
        
        if (bytes_read == 0) {
            return std::unexpected(Error::EndOfData);
        }
        
        buffer.resize(bytes_read);
        return buffer;
    }
    
    Result<std::string> read_string(std::size_t count) override {
        std::string str(count, '\0');
        auto bytes_read = file_.read(str.data(), count).gcount();
        
        if (bytes_read == 0) {
            return std::unexpected(Error::EndOfData);
        }
        
        str.resize(bytes_read);
        return str;
    }
    
    Result<std::size_t> position() const override {
        return file_.tellg();
    }
    
    Result<void> seek(std::size_t pos) override {
        file_.seekg(pos);
        if (file_.fail()) {
            return std::unexpected(Error::InvalidPosition);
        }
        return {};
    }
    
    Result<std::size_t> size() const override {
        ensure_file_size();
        return file_size_;
    }
    
    bool at_end() const override {
        return file_.eof() || file_.peek() == EOF;
    }
    
    Result<std::size_t> remaining() const override {
        auto pos = position();
        if (!pos) return std::unexpected(pos.error());
        
        auto sz = size();
        if (!sz) return std::unexpected(sz.error());
        
        return *sz - *pos;
    }
    
    Result<std::vector<std::uint8_t>> read_all() override {
        auto rem = remaining();
        if (!rem) return std::unexpected(rem.error());
        
        if (*rem == 0) {
            return std::vector<std::uint8_t>{};
        }
        
        return read_bytes(*rem);
    }
    
    Result<void> skip(std::size_t count) override {
        file_.seekg(count, std::ios::cur);
        if (file_.fail()) {
            return std::unexpected(Error::InvalidPosition);
        }
        return {};
    }
    
    Result<std::uint8_t> peek_byte() override {
        int byte = file_.peek();
        if (byte == EOF) {
            return std::unexpected(Error::EndOfData);
        }
        return static_cast<std::uint8_t>(byte);
    }
    
    Result<std::vector<std::uint8_t>> peek_bytes(std::size_t count) override {
        auto current_pos = file_.tellg();
        auto bytes = read_bytes(count);
        file_.seekg(current_pos);
        return bytes;
    }
};

// Wrapper around a Reader with caching
class CachedReader : public Reader {
private:
    std::unique_ptr<Reader> source_;
    mutable std::vector<std::uint8_t> cache_;
    mutable std::size_t cache_start_pos_ = 0;
    mutable std::size_t cache_pos_ = 0;
    std::size_t cache_size_;
    mutable std::size_t idx_ = 0;
    
    void update_cache() const {
        if (idx_ >= cache_start_pos_ && idx_ < cache_start_pos_ + cache_.size()) {
            return; // Already in buffer
        }
        
        // Need to refresh buffer
        auto seek_result = source_->seek(idx_);
        if (!seek_result) return;
        
        auto new_data = source_->read_bytes(cache_size_);
        if (!new_data) {
            cache_.clear();
            return;
        }
        
        cache_ = std::move(*new_data);
        cache_start_pos_ = idx_;
    }
    
public:
    CachedReader(std::unique_ptr<Reader> underlying, std::size_t buffer_size = 4096)
        : source_(std::move(underlying)), cache_size_(buffer_size) {}
    
    Result<std::uint8_t> read_byte() override {
        update_cache();
        
        if (cache_.empty() || idx_ >= cache_start_pos_ + cache_.size()) {
            return std::unexpected(Error::EndOfData);
        }
        
        std::size_t buffer_offset = idx_ - cache_start_pos_;
        std::uint8_t byte = cache_[buffer_offset];
        idx_++;
        
        return byte;
    }
    
    Result<std::vector<std::uint8_t>> read_bytes(std::size_t count) override {
        std::vector<std::uint8_t> result;
        result.reserve(count);
        
        while(count > 0) {
            update_cache();

            std::uint64_t offset = idx_ - cache_start_pos_;
            std::uint64_t available = cache_size_ - offset;

            if(available == 0) {
                return std::unexpected(Error::EndOfData);
            }

            std::size_t to_copy = std::min(count, available);

            result.insert(result.end(), 
                          cache_.begin() + offset, 
                          cache_.begin() + offset + to_copy);

            idx_ += to_copy;
            count -= to_copy;
        }
        
        return result;
    }
    
    Result<std::string> read_string(std::size_t count) override {
        auto bytes = read_bytes(count);
        if (!bytes) return std::unexpected(bytes.error());
        
        return std::string(bytes->begin(), bytes->end());
    }
    
    Result<std::size_t> position() const override {
        return idx_;
    }
    
    Result<void> seek(std::size_t pos) override {
        idx_ = pos;
        return {};
    }
    
    Result<std::size_t> size() const override {
        return source_->size();
    }
    
    bool at_end() const override {
        auto sz = size();
        return sz && idx_ >= *sz;
    }
    
    Result<std::size_t> remaining() const override {
        auto sz = size();
        if (!sz) return std::unexpected(sz.error());
        
        return *sz - idx_;
    }
    
    Result<std::vector<std::uint8_t>> read_all() override {
        auto rem = remaining();
        if (!rem) return std::unexpected(rem.error());
        
        return read_bytes(*rem);
    }
    
    Result<void> skip(std::size_t count) override {
        idx_ += count;
        auto sz = size();
        if (sz && idx_ > *sz) {
            idx_ = *sz;
        }
        return {};
    }
    
    Result<std::uint8_t> peek_byte() override {
        update_cache();
        
        if (cache_.empty() || idx_ >= cache_start_pos_ + cache_.size()) {
            return std::unexpected(Error::EndOfData);
        }
        
        std::size_t buffer_offset = idx_ - cache_start_pos_;
        return cache_[buffer_offset];
    }
    
    Result<std::vector<std::uint8_t>> peek_bytes(std::size_t count) override {
        std::size_t saved_pos = idx_;
        auto result = read_bytes(count);
        idx_ = saved_pos;
        return result;
    }
};

}
