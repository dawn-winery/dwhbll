#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <print>
#include <vector>
#include <string>
#include <fstream>
#include <span>

namespace dwhbll::collections::stream {

// TODO: add more errors (like failure to open file)
enum class Error {
    EndOfData,
    InvalidPosition,
};

// Base Reader interface
class Reader {
protected:
    Error last_error_ = static_cast<Error>(-1);
    bool has_error_ = false;

public:
    virtual ~Reader() = default;
    
    virtual Reader& read_byte(uint8_t& byte) = 0;
    virtual Reader& read_bytes(std::vector<uint8_t>& buf, std::size_t count) = 0;

    // Templated wrappers to also support chars
    template <typename T>
    requires std::is_same_v<T, uint8_t> || std::is_same_v<T, char>
    Reader& read_bytes(std::vector<T>& buf, std::size_t count) {
        static_assert(sizeof(T) == sizeof(uint8_t));
        std::vector<uint8_t> tmp;
        tmp.reserve(count);
        auto& r = read_bytes(tmp, count);
        buf.clear();
        buf.assign(tmp.begin(), tmp.end());
        return r;
    }

    template <typename T>
    requires std::is_same_v<T, uint8_t> || std::is_same_v<T, char>
    Reader& read_until(std::vector<T>& buf, T delimiter, bool consume_delimiter = true) {
        static_assert(sizeof(T) == sizeof(uint8_t));
        std::vector<uint8_t> tmp;
        auto& r = read_until(tmp, static_cast<uint8_t>(delimiter), consume_delimiter);
        buf.assign(tmp.begin(), tmp.end());
        return r;
    }

    
    // Read until a delimiter is found
    virtual Reader& read_until(std::vector<uint8_t>& data, uint8_t delimiter, bool consume_delimiter = true) = 0;
    
    // Read a null-terminated string
    virtual Reader& read_string(std::string& str) = 0;
    
    // Error functions
    bool good() const { return !has_error_; }
    bool fail() const { return has_error_; }
    bool eof() const { return has_error_ && last_error_ == Error::EndOfData; }

    void clear_error() {
        has_error_ = false;
        last_error_ = static_cast<Error>(-1);
    }

    explicit operator bool() const { return !has_error_; }
    
    // Position operations
    virtual std::size_t position() const = 0;
    virtual Reader& seek(std::size_t pos) = 0;
    virtual std::size_t size() const = 0;
    
    // Utility methods
    virtual std::size_t remaining() const = 0;
    
    // Read all remaining data
    virtual Reader& read_all(std::vector<uint8_t>& data) = 0;
    
    // Skip bytes
    virtual Reader& skip(std::size_t count) = 0;
    
    // Peek operations (read without advancing position)
    virtual Reader& peek_byte(uint8_t& byte) = 0;
    virtual Reader& peek_bytes(std::vector<uint8_t>& data, std::size_t count) = 0;

protected:
    void set_error(Error err) {
        last_error_ = err;
        has_error_ = true;
    }
};

// Reader that reads from a std::vector<uint8_t>
// TODO: maybe generalize to uint8_t*?
class MemoryReader : public Reader {
private:
    std::vector<uint8_t> data_;
    std::size_t pos_ = 0;
    
public:
    explicit MemoryReader(std::vector<uint8_t> data) 
        : data_(std::move(data)) {}
    
    explicit MemoryReader(std::span<const uint8_t> data) 
        : data_(data.begin(), data.end()) {}
    
    explicit MemoryReader(const std::string& str) 
        : data_(str.begin(), str.end()) {}
    
    Reader& read_byte(uint8_t& byte) override {
        if (pos_ >= data_.size()) {
            set_error(Error::EndOfData);
            return *this;
        }
        byte = data_[pos_++];
        return *this;
    }
    
    Reader& read_bytes(std::vector<uint8_t>& buf, std::size_t count) override {
        if (pos_ + count > data_.size()) {
            set_error(Error::EndOfData);
            return *this;
        }
        
        buf.assign(data_.begin() + pos_, data_.begin() + pos_ + count);
        pos_ += count;
        
        return *this;
    }
    
    Reader& read_until(std::vector<uint8_t>& data, uint8_t delimiter, bool consume_delimiter = true) override {
        data.clear();
        
        while (pos_ < data_.size()) {
            uint8_t byte = data_[pos_];
            
            if (byte == delimiter) {
                if (consume_delimiter) {
                    pos_++;
                }
                return *this;
            }
            
            data.push_back(byte);
            pos_++;
        }
        
        // Reached end of data without finding delimiter
        set_error(Error::EndOfData);
        return *this;
    }
    
    Reader& read_string(std::string& str) override {
        str.clear();
        
        while (pos_ < data_.size()) {
            uint8_t byte = data_[pos_++];
            
            if (byte == 0) {
                return *this;
            }
            
            str.push_back(static_cast<char>(byte));
        }
        
        // Reached end of data without finding null terminator
        set_error(Error::EndOfData);
        return *this;
    }
    
    std::size_t position() const override {
        return pos_;
    }
    
    Reader& seek(std::size_t pos) override {
        if (pos > data_.size()) {
            set_error(Error::InvalidPosition);
            return *this;
        }
        pos_ = pos;
        return *this;
    }
    
    std::size_t size() const override {
        return data_.size();
    }
    
    std::size_t remaining() const override {
        return data_.size() - pos_;
    }
    
    Reader& read_all(std::vector<uint8_t>& data) override {
        if (pos_ >= data_.size()) {
            data.clear();
            return *this;
        }
        
        data.assign(data_.begin() + pos_, data_.end());
        pos_ = data_.size();
        return *this;
    }
    
    Reader& skip(std::size_t count) override {
        std::size_t new_pos = pos_ + count;
        if (new_pos > data_.size()) {
            pos_ = data_.size();
        } else {
            pos_ = new_pos;
        }
        return *this;
    }
    
    Reader& peek_byte(uint8_t& byte) override {
        if (pos_ >= data_.size()) {
            set_error(Error::EndOfData);
            return *this;
        }
        byte = data_[pos_];
        return *this;
    }
    
    Reader& peek_bytes(std::vector<uint8_t>& data, std::size_t count) override {
        if (pos_ >= data_.size()) {
            set_error(Error::EndOfData);
            return *this;
        }
        
        std::size_t available = data_.size() - pos_;
        std::size_t to_read = std::min(count, available);
        
        data.assign(data_.begin() + pos_, data_.begin() + pos_ + to_read);
        return *this;
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
    
    Reader& read_byte(uint8_t& byte) override {
        char ch;
        if (!file_.read(&ch, 1)) {
            set_error(Error::EndOfData);
            return *this;
        }
        byte = static_cast<uint8_t>(ch);
        return *this;
    }
    
    Reader& read_bytes(std::vector<uint8_t>& buf, std::size_t count) override {
        buf.resize(count);
        auto bytes_read = file_.read(reinterpret_cast<char*>(buf.data()), count).gcount();
        
        if (bytes_read == 0) {
            set_error(Error::EndOfData);
        }
        
        return *this;
    }
    
    Reader& read_until(std::vector<uint8_t>& data, uint8_t delimiter, bool consume_delimiter = true) override {
        data.clear();
        
        char ch;
        while (file_.read(&ch, 1)) {
            uint8_t byte = static_cast<uint8_t>(ch);
            
            if (byte == delimiter) {
                if (!consume_delimiter) {
                    file_.seekg(-1, std::ios::cur);
                }
                return *this;
            }
            
            data.push_back(byte);
        }
        
        // Reached end of file without finding delimiter
        set_error(Error::EndOfData);
        return *this;
    }
    
    Reader& read_string(std::string& str) override {
        str.clear();
        
        char ch;
        while (file_.read(&ch, 1)) {
            if (ch == 0) {
                return *this;
            }
            
            str.push_back(ch);
        }
        
        // Reached end of file without finding null terminator
        set_error(Error::EndOfData);
        return *this;
    }
    
    std::size_t position() const override {
        return file_.tellg();
    }
    
    Reader& seek(std::size_t pos) override {
        file_.seekg(pos);
        if (file_.fail()) {
            set_error(Error::InvalidPosition);
        }
        return *this;
    }
    
    std::size_t size() const override {
        ensure_file_size();
        return file_size_;
    }
    
    std::size_t remaining() const override {
        auto pos = position();
        return file_size_ - pos;
    }
    
    Reader& read_all(std::vector<uint8_t>& data) override {
        auto rem = remaining();
        
        if (rem == 0) {
            data.clear();
            return *this;
        }
        
        data.resize(rem);
        read_bytes(data, rem);
        return *this;
    }
    
    Reader& skip(std::size_t count) override {
        file_.seekg(count, std::ios::cur);
        if (file_.fail()) {
            set_error(Error::InvalidPosition);
        }
        return *this;
    }
    
    Reader& peek_byte(uint8_t& byte) override {
        int ch = file_.peek();
        if (ch == EOF) {
            set_error(Error::EndOfData);
            return *this;
        }
        byte = static_cast<uint8_t>(ch);
        return *this;
    }
    
    Reader& peek_bytes(std::vector<uint8_t>& data, std::size_t count) override {
        auto current_pos = file_.tellg();
        data.resize(count);
        if(!read_bytes(data, count))
            return *this;
        file_.seekg(current_pos);
        return *this;
    }
};

// Wrapper around a Reader with caching
class CachedReader : public Reader {
private:
    std::unique_ptr<Reader> source_;
    mutable std::vector<uint8_t> cache_;
    mutable std::size_t cache_start_pos_ = 0;
    std::size_t cache_size_;
    mutable std::size_t idx_ = 0;
    
    void update_cache() const {
        if (idx_ >= cache_start_pos_ && idx_ < cache_start_pos_ + cache_.size()) {
            return; // Already in cache
        }

        source_->seek(idx_);
        if (source_->fail()) {
            cache_.clear();
            return;
        }

        cache_.clear();
        std::size_t available = source_->remaining();
        std::size_t to_read = std::min(cache_size_, available);

        if (to_read == 0) {
            return; // Nothing left to read
        }

        source_->read_bytes(cache_, to_read);

        if (source_->fail() && cache_.empty()) {
            // Only set error if absolutely nothing was read
            cache_.clear();
            return;
        }

        cache_start_pos_ = idx_;
    }
public:
    CachedReader(std::unique_ptr<Reader> underlying, std::size_t buffer_size = 4096)
        : source_(std::move(underlying)), cache_size_(buffer_size) {}
    
    Reader& read_byte(uint8_t& byte) override {
        update_cache();
        
        if (cache_.empty() || idx_ >= cache_start_pos_ + cache_.size()) {
            set_error(Error::EndOfData);
            return *this;
        }
        
        std::size_t buffer_offset = idx_ - cache_start_pos_;
        byte = cache_[buffer_offset];
        idx_++;
        
        return *this;
    }
    
    Reader& read_bytes(std::vector<uint8_t>& buf, std::size_t count) override {
        buf.clear();

        while(count > 0) {
            update_cache();

            std::uint64_t offset = idx_ - cache_start_pos_;
            std::uint64_t available = cache_.size() - offset;

            if(available == 0) {
                set_error(Error::EndOfData);
                return *this;
            }

            std::size_t to_copy = std::min(count, available);

            buf.insert(buf.end(), cache_.begin() + offset, cache_.begin() + offset + to_copy);

            idx_ += to_copy;
            count -= to_copy;
        }
        
        return *this;
    }
    
    Reader& read_until(std::vector<uint8_t>& data, uint8_t delimiter, bool consume_delimiter = true) override {
        data.clear();
        
        uint8_t byte;
        while (read_byte(byte)) {
            if (byte == delimiter) {
                if (!consume_delimiter) {
                    idx_--; // Move back one position
                }
                return *this;
            }
            
            data.push_back(byte);
        }
        
        // read_byte already set the error
        return *this;
    }
    
    Reader& read_string(std::string& str) override {
        str.clear();
        
        uint8_t byte;
        while (read_byte(byte)) {
            if (byte == 0) {
                return *this;
            }
            
            str.push_back(static_cast<char>(byte));
        }
        
        // read_byte already set the error
        return *this;
    }
    
    std::size_t position() const override {
        return idx_;
    }
    
    Reader& seek(std::size_t pos) override {
        idx_ = pos;
        return *this;
    }
    
    std::size_t size() const override {
        return source_->size();
    }
    
    std::size_t remaining() const override {
        auto sz = size();
        return sz - idx_;
    }
    
    Reader& read_all(std::vector<uint8_t>& data) override {
        auto rem = remaining();
        data.resize(rem);
        read_bytes(data, rem);
        return *this;
    }
    
    Reader& skip(std::size_t count) override {
        idx_ += count;
        auto sz = size();
        if (idx_ > sz) {
            idx_ = sz;
        }
        return *this;
    }
    
    Reader& peek_byte(uint8_t& byte) override {
        update_cache();
        
        if (cache_.empty() || idx_ >= cache_start_pos_ + cache_.size()) {
            set_error(Error::EndOfData);
            return *this;
        }
        
        std::size_t buffer_offset = idx_ - cache_start_pos_;
        byte = cache_[buffer_offset];
        return *this;
    }
    
    Reader& peek_bytes(std::vector<uint8_t>& data, std::size_t count) override {
        std::size_t saved_pos = idx_;
        data.resize(count);
        read_bytes(data, count);
        idx_ = saved_pos;
        return *this;
    }
};

}
