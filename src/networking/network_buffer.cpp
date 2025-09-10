#include "../../include/ecscope/networking/network_types.hpp"
#include <algorithm>
#include <cstring>

namespace ecscope::networking {

NetworkBuffer::NetworkBuffer(size_t capacity) 
    : buffer_(capacity), size_(0) {
}

void NetworkBuffer::resize(size_t new_size) {
    if (new_size > buffer_.size()) {
        buffer_.resize(new_size);
    }
    size_ = new_size;
}

void NetworkBuffer::reserve(size_t new_capacity) {
    if (new_capacity > buffer_.size()) {
        buffer_.reserve(new_capacity);
    }
}

void NetworkBuffer::append(const void* data, size_t length) {
    if (size_ + length > buffer_.size()) {
        buffer_.resize(size_ + length);
    }
    
    std::memcpy(buffer_.data() + size_, data, length);
    size_ += length;
}

void NetworkBuffer::append(const std::vector<uint8_t>& data) {
    append(data.data(), data.size());
}

void NetworkBuffer::prepend(const void* data, size_t length) {
    if (size_ + length > buffer_.size()) {
        buffer_.resize(size_ + length);
    }
    
    // Move existing data to make room at the beginning
    if (size_ > 0) {
        std::memmove(buffer_.data() + length, buffer_.data(), size_);
    }
    
    // Copy new data to the beginning
    std::memcpy(buffer_.data(), data, length);
    size_ += length;
}

} // namespace ecscope::networking