#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <span>
#include <stdexcept>
#include <algorithm>

namespace dwhbll::collections::stream {

enum class Error {
    NoError = 0,        
    GenericError,       
    Unimplemented,      
    EndOfData,          
    DecompressionError, 
    FileOpenError,      
};

// Base class for error state management
class ErrorState {
protected:
    mutable Error last_error_ = Error::NoError; 
    mutable bool has_error_ = false;            

public:
    bool good() const { return !has_error_; }       
    bool fail() const { return has_error_; }        
    bool eof() const { return has_error_ && last_error_ == Error::EndOfData; } 
    Error last_error() const { return last_error_; } 

    void clear_error() const { 
        has_error_ = false;
        last_error_ = Error::NoError;
    }

    explicit operator bool() const { return !has_error_; } 

protected:
    // Always clears previous error state before setting new one
    void set_error(Error err) const {
        last_error_ = err;
        has_error_ = true;
    }

    // Clears error state on successful operations
    void clear_on_success() const {
        if (has_error_) {
            has_error_ = false;
            last_error_ = Error::NoError;
        }
    }

    // Propagate error from another ErrorState object
    void propagate_error(const ErrorState& other) const {
        if (other.has_error_) {
            set_error(other.last_error_);
        }
    }
};

class Buffer : public ErrorState {
public:
    virtual ~Buffer() = default; 

    // Reads either dest.size() or remaining() bytes, whichever is smaller
    virtual std::size_t read_raw_bytes(std::span<uint8_t> dest) = 0;
    virtual std::size_t peek_raw_bytes(std::span<uint8_t> dest, std::size_t offset = 0) const = 0;

    virtual bool seek(std::size_t pos) = 0;
    virtual bool skip(std::size_t count) = 0;

    virtual std::size_t position() const = 0;
    virtual std::size_t size() const = 0;
    virtual std::size_t remaining() const = 0;
};

class MemoryBuffer : public Buffer {
private:
    std::vector<uint8_t> data_;  
    mutable std::size_t pos_ = 0; 

public:
    explicit MemoryBuffer(std::vector<uint8_t> data)
        : data_(std::move(data)) {}
    
    explicit MemoryBuffer(std::span<const uint8_t> data)
        : data_(data.begin(), data.end()) {}
    
    explicit MemoryBuffer(const std::string& str)
        : data_(str.begin(), str.end()) {}
    
    std::size_t read_raw_bytes(std::span<uint8_t> dest) override {
        if (dest.empty()) {
            set_error(Error::GenericError);
            return 0;
        }
        
        std::size_t bytes_to_read = std::min(dest.size(), remaining());
        if (bytes_to_read == 0) {
            set_error(Error::EndOfData);
            return 0;
        }
        
        std::memcpy(dest.data(), data_.data() + pos_, bytes_to_read);
        pos_ += bytes_to_read;
        
        // Clear error only if we successfully read the requested amount
        if (bytes_to_read == dest.size()) {
            clear_on_success();
        } else {
            set_error(Error::EndOfData);
        }
        
        return bytes_to_read;
    }
    
    std::size_t peek_raw_bytes(std::span<uint8_t> dest, std::size_t offset = 0) const override {
        if (dest.empty()) {
            set_error(Error::GenericError);
            return 0;
        }
        
        if (pos_ + offset > data_.size()) { 
            set_error(Error::EndOfData);
            return 0;
        }
        
        std::size_t available_bytes = data_.size() - (pos_ + offset);
        std::size_t bytes_to_peek = std::min(dest.size(), available_bytes);
        
        if (bytes_to_peek == 0) {
            set_error(Error::EndOfData);
            return 0;
        }

        std::memcpy(dest.data(), data_.data() + pos_ + offset, bytes_to_peek);
        
        // Clear error only if we successfully peeked the requested amount
        if (bytes_to_peek == dest.size()) {
            clear_on_success();
        } else {
            set_error(Error::EndOfData);
        }
        
        return bytes_to_peek;
    }
    
    std::size_t position() const override {
        return pos_;
    }
    
    bool seek(std::size_t pos) override {
        if (pos > data_.size()) { 
            set_error(Error::EndOfData);
            return false;
        }
        pos_ = pos;
        clear_on_success();
        return true;
    }
    
    std::size_t size() const override {
        return data_.size();
    }
    
    std::size_t remaining() const override {
        return data_.size() - pos_;
    }
    
    bool skip(std::size_t count) override {
        std::size_t new_pos = pos_ + count;
        if (new_pos > data_.size()) { 
            pos_ = data_.size();
            set_error(Error::EndOfData); 
            return false;
        }
        pos_ = new_pos;
        clear_on_success();
        return true;
    }
};

class FileBuffer : public Buffer {
private:
    mutable std::ifstream file_;              
    mutable std::size_t file_size_;           
    mutable std::size_t pos_ = 0; 

public:
    explicit FileBuffer(const std::string& filename) 
        : file_(filename, std::ios::binary | std::ios::ate) {
        if (!file_.is_open()) {
            set_error(Error::FileOpenError);
            return; // Don't throw, just set error state
        }
        file_size_ = file_.tellg(); 
        file_.seekg(0, std::ios::beg); 
        pos_ = 0;
        clear_on_success();
    }

    std::size_t read_raw_bytes(std::span<uint8_t> dest) override {
        if (dest.empty()) {
            set_error(Error::GenericError);
            return 0;
        }
        
        std::size_t bytes_to_read = std::min(dest.size(), remaining());
        if (bytes_to_read == 0) {
            set_error(Error::EndOfData);
            return 0;
        }

        file_.read(reinterpret_cast<char*>(dest.data()), bytes_to_read);
        std::size_t bytes_read = file_.gcount(); 
        pos_ += bytes_read;

        if (bytes_read == dest.size()) {
            clear_on_success();
        } else {
            set_error(Error::EndOfData);
        }
        
        return bytes_read;
    }

    std::size_t peek_raw_bytes(std::span<uint8_t> dest, std::size_t offset = 0) const override {
        if (dest.empty()) {
            set_error(Error::GenericError);
            return 0;
        }
        
        auto original_pos = file_.tellg(); 
        
        file_.seekg(pos_ + offset);
        if (file_.fail()) {
            set_error(Error::EndOfData);
            file_.seekg(original_pos); 
            return 0;
        }

        std::size_t bytes_to_peek = std::min(dest.size(), (file_size_ - (pos_ + offset)));
        if (bytes_to_peek == 0) { 
            set_error(Error::EndOfData);
            file_.seekg(original_pos); 
            return 0;
        }

        file_.read(reinterpret_cast<char*>(dest.data()), bytes_to_peek);
        std::size_t bytes_peeked = file_.gcount();
        file_.seekg(original_pos); 

        if (bytes_peeked == dest.size()) {
            clear_on_success();
        } else {
            set_error(Error::EndOfData);
        }
        
        return bytes_peeked;
    }

    std::size_t position() const override {
        return pos_;
    }
    
    bool seek(std::size_t pos) override {
        if (pos > file_size_) { 
            set_error(Error::EndOfData);
            return false;
        }
        
        file_.seekg(pos);
        if (file_.fail()) { 
            set_error(Error::GenericError);
            return false;
        }
        
        pos_ = pos;
        clear_on_success();
        return true;
    }
    
    std::size_t size() const override {
        return file_size_;
    }
    
    std::size_t remaining() const override {
        return file_size_ - pos_;
    }
    
    bool skip(std::size_t count) override {
        std::size_t new_pos = pos_ + count;
        if (new_pos > file_size_) { 
            pos_ = file_size_;
            set_error(Error::EndOfData);
            return false;
        }
        
        file_.seekg(count, std::ios::cur); 
        if (file_.fail()) { 
            set_error(Error::GenericError);
            return false;
        }
        
        pos_ = new_pos;
        clear_on_success();
        return true;
    }
};

class Reader : public ErrorState {
public:
    virtual ~Reader() = default;

    virtual Reader& read_byte(uint8_t& byte) = 0;
    virtual Reader& read_bytes(std::vector<uint8_t>& buf, std::size_t count) = 0;
    
    // Templates for char/uint8_t compatibility
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
    
    virtual Reader& read_until(std::vector<uint8_t>& data, uint8_t delimiter, bool consume_delimiter = true) = 0;
    virtual Reader& read_string(std::string& str) = 0;
    
    virtual Reader& seek(std::size_t pos) = 0;
    virtual Reader& skip(std::size_t count) = 0;
    
    virtual std::size_t position() const = 0;
    virtual std::size_t size() const = 0;
    virtual std::size_t remaining() const = 0;
    
    virtual Reader& read_all(std::vector<uint8_t>& data) = 0;
    
    virtual Reader& peek_byte(uint8_t& byte) = 0;
    virtual Reader& peek_bytes(std::vector<uint8_t>& data, std::size_t count) = 0;
};

class StreamReader : public Reader {
private:
    std::unique_ptr<Buffer> source_buffer_; 

public:
    explicit StreamReader(std::unique_ptr<Buffer> buffer)
        : source_buffer_(std::move(buffer)) {
        if (!source_buffer_) {
            set_error(Error::GenericError); 
        } else {
            // Propagate any initialization errors from the buffer
            propagate_error(*source_buffer_);
        }
    }
    
    Reader& read_byte(uint8_t& byte) override {
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return *this;
        }
        
        std::array<uint8_t, 1> temp_byte_array;
        std::size_t bytes_read = source_buffer_->read_raw_bytes(temp_byte_array);
        
        if (bytes_read == 1) {
            byte = temp_byte_array[0];
            clear_on_success();
        } else {
            propagate_error(*source_buffer_);
        }
        
        return *this;
    }
    
    Reader& read_bytes(std::vector<uint8_t>& buf, std::size_t count) override {
        if (!source_buffer_) {
            set_error(Error::GenericError);
            buf.clear();
            return *this;
        }

        buf.resize(count); 
        std::size_t bytes_read = source_buffer_->read_raw_bytes(buf);
        
        if (bytes_read == count) {
            clear_on_success();
        } else {
            buf.resize(bytes_read); 
            propagate_error(*source_buffer_);
        }
        
        return *this;
    }
    
    Reader& read_until(std::vector<uint8_t>& data, uint8_t delimiter, bool consume_delimiter = true) override {
        data.clear();
        
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return *this;
        }

        uint8_t byte;
        bool found_delimiter = false;
        
        while (true) {
            std::array<uint8_t, 1> temp_array;
            std::size_t bytes_read = source_buffer_->read_raw_bytes(temp_array);
            
            if (bytes_read == 0) {
                propagate_error(*source_buffer_);
                break;
            }
            
            byte = temp_array[0];
            
            if (byte == delimiter) {
                found_delimiter = true;
                if (!consume_delimiter) {
                    source_buffer_->seek(source_buffer_->position() - 1);
                    if (source_buffer_->fail()) {
                        propagate_error(*source_buffer_);
                    }
                }
                break;
            }
            
            data.push_back(byte);
        }
        
        if (found_delimiter) {
            clear_on_success();
        }
        // If we didn't find delimiter, error is already propagated from buffer
        
        return *this;
    }
    
    Reader& read_string(std::string& str) override {
        str.clear();
        
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return *this;
        }

        uint8_t byte;
        bool found_null = false;
        
        while (true) {
            std::array<uint8_t, 1> temp_array;
            std::size_t bytes_read = source_buffer_->read_raw_bytes(temp_array);
            
            if (bytes_read == 0) {
                propagate_error(*source_buffer_);
                break;
            }
            
            byte = temp_array[0];
            
            if (byte == 0) {
                found_null = true;
                break;
            }
            
            str.push_back(static_cast<char>(byte));
        }
        
        if (found_null) {
            clear_on_success();
        }
        // If we hit EOF without null terminator, error is already propagated
        
        return *this;
    }
    
    std::size_t position() const override {
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return 0;
        }
        return source_buffer_->position();
    }
    
    Reader& seek(std::size_t pos) override {
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return *this;
        }
        
        bool success = source_buffer_->seek(pos);
        if (success) {
            clear_on_success();
        } else {
            propagate_error(*source_buffer_);
        }
        
        return *this;
    }
    
    std::size_t size() const override {
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return 0;
        }
        return source_buffer_->size();
    }
    
    std::size_t remaining() const override {
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return 0;
        }
        return source_buffer_->remaining();
    }

    Reader& read_all(std::vector<uint8_t>& data) override {
        data.clear();
        
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return *this;
        }
        
        std::size_t remaining_bytes = source_buffer_->remaining();
        if (remaining_bytes == 0) {
            clear_on_success(); // Successfully read 0 bytes
            return *this;
        }
        
        data.resize(remaining_bytes);
        std::size_t bytes_read = source_buffer_->read_raw_bytes(data);
        
        if (bytes_read == remaining_bytes) {
            clear_on_success();
        } else {
            data.resize(bytes_read);
            propagate_error(*source_buffer_);
        }
        
        return *this;
    }

    Reader& skip(std::size_t count) override {
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return *this;
        }
        
        bool success = source_buffer_->skip(count);
        if (success) {
            clear_on_success();
        } else {
            propagate_error(*source_buffer_);
        }
        
        return *this;
    }

    Reader& peek_byte(uint8_t& byte) override {
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return *this;
        }
        
        std::array<uint8_t, 1> temp_byte_array;
        std::size_t bytes_peeked = source_buffer_->peek_raw_bytes(temp_byte_array);
        
        if (bytes_peeked == 1) {
            byte = temp_byte_array[0];
            clear_on_success();
        } else {
            propagate_error(*source_buffer_);
        }
        
        return *this;
    }
    
    Reader& peek_bytes(std::vector<uint8_t>& data, std::size_t count) override {
        if (!source_buffer_) {
            set_error(Error::GenericError);
            data.clear();
            return *this;
        }
        
        data.resize(count);
        std::size_t bytes_peeked = source_buffer_->peek_raw_bytes(data);
        
        if (bytes_peeked == count) {
            clear_on_success();
        } else {
            data.resize(bytes_peeked);
            propagate_error(*source_buffer_);
        }
        
        return *this;
    }
};

class CachedReader : public Reader {
private:
    std::unique_ptr<Buffer> source_buffer_;
    std::vector<uint8_t> cache_;
    const std::size_t cache_chunk_size_;
    std::size_t pos_ = 0;
    std::size_t cache_start_pos_ = 0;

    void update_cache() {
        if(pos_ >= cache_start_pos_ && pos_ < cache_start_pos_ + cache_.size()) {
            return;
        }

        source_buffer_->seek(pos_);
        if(source_buffer_->fail()) {
            set_error(source_buffer_->last_error());
            return;
        }

        cache_.resize(cache_chunk_size_);
        size_t bytes_read = source_buffer_->read_raw_bytes(cache_);
        if(bytes_read == 0) {
            set_error(source_buffer_->last_error());
            return;
        }

        cache_.resize(bytes_read);
        cache_start_pos_ = pos_;
    }

public:
    explicit CachedReader(std::unique_ptr<Buffer> source_buffer, std::size_t cache_chunk_size = 4096)
        : source_buffer_(std::move(source_buffer)), cache_chunk_size_(cache_chunk_size) {
        if (!source_buffer_) {
            set_error(Error::GenericError);
        }

        update_cache();
    }

    Reader& read_byte(uint8_t& byte) override {
        clear_error();
        if (!source_buffer_ || source_buffer_->fail()) {
            set_error(source_buffer_ ? source_buffer_->last_error() : Error::GenericError);
            return *this;
        }
        
        update_cache();
        if(fail())
            return *this;

        size_t cache_offset = pos_ - cache_start_pos_;
        
        byte = cache_[cache_offset];
        pos_++;
        return *this;
    }

    Reader& read_bytes(std::vector<uint8_t>& buf, std::size_t count) override {
        clear_error();
        buf.clear();
        if (!source_buffer_ || source_buffer_->fail()) {
            set_error(source_buffer_ ? source_buffer_->last_error() : Error::GenericError);
            return *this;
        }
        
        buf.reserve(count);
        while (count > 0) {
            update_cache();
            if(fail())
                break;

            size_t offset = pos_ - cache_start_pos_;
            size_t available = cache_.size() - offset;
            if(available == 0) {
                set_error(Error::EndOfData);
                break;
            }

            size_t to_copy = std::min(available, count);

            buf.insert(buf.end(), cache_.begin() + offset,
                       cache_.begin() + offset + to_copy);
            pos_ += to_copy;
            count -= to_copy;
        }
        
        if (count > 0) {
            set_error(Error::EndOfData);
        }

        return *this;
    }

    Reader& read_until(std::vector<uint8_t>& data, uint8_t delimiter, bool consume_delimiter = true) override {
        clear_error();
        data.clear();
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return *this;
        }

        uint8_t byte;
        while(read_byte(byte).good()) {
            if (byte == delimiter) {
                if (!consume_delimiter) {
                    seek(position() - 1);
                }
                return *this;
            }
            data.push_back(byte);
        }

        return *this;
    }
    
    Reader& read_string(std::string& str) override {
        clear_error();
        str.clear();
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return *this;
        }

        uint8_t byte;
        while(read_byte(byte).good()) {
            if (byte == 0) {
                return *this;
            }
            str.push_back(static_cast<char>(byte));
        }

        return *this;
    }

    Reader& seek(std::size_t pos) override {
        clear_error();
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return *this;
        }
        if (pos > size()) {
            set_error(Error::EndOfData);
            return *this;
        }
        pos_ = pos;
        return *this;
    }

    Reader& skip(std::size_t count) override {
        clear_error();
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return *this;
        }
        if (pos_ + count > size()) {
            pos_ = size();
            set_error(Error::EndOfData);
            return *this;
        }
        pos_ += count;
        return *this;
    }

    std::size_t position() const override {
        return pos_;
    }

    std::size_t size() const override {
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return 0;
        }
        return source_buffer_->size();
    }

    std::size_t remaining() const override {
        if (!source_buffer_) {
            set_error(Error::GenericError);
            return 0;
        }
        return size() - position();
    }

    Reader& read_all(std::vector<uint8_t>& data) override {
        clear_error();
        seek(0);
        if(fail()) return *this;
        return read_bytes(data, remaining());
    }

    Reader& peek_byte(uint8_t& byte) override {
        clear_error();
        auto original_pos = position();
        read_byte(byte);
        pos_ = original_pos;
        return *this;
    }

    Reader& peek_bytes(std::vector<uint8_t>& data, std::size_t count) override {
        clear_error();
        auto original_pos = position();
        read_bytes(data, count);
        pos_ = original_pos;
        return *this;
    }
};

} 
