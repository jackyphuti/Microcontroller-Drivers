#pragma once

#include <cstddef>
#include <optional>

namespace Core {

template <typename T, std::size_t Size>
class RingBuffer {
public:
    RingBuffer() = default;

    // Pushes data into the buffer. Returns false if the buffer is full.
    bool Push(const T& item) {
        const std::size_t next_head = (head_ + 1) % Size;
        
        if (next_head == tail_) {
            return false; // Buffer overflow (Data loss occurs here if not handled)
        }
        
        buffer_[head_] = item;
        head_ = next_head;
        return true;
    }

    // Pops data from the buffer. Returns std::nullopt if empty.
    std::optional<T> Pop() {
        if (head_ == tail_) {
            return std::nullopt; // Buffer empty
        }
        
        T item = buffer_[tail_];
        tail_ = (tail_ + 1) % Size;
        return item;
    }

    [[nodiscard]] bool IsEmpty() const {
        return head_ == tail_;
    }

    [[nodiscard]] bool IsFull() const {
        return ((head_ + 1) % Size) == tail_;
    }

private:
    T buffer_[Size]{};
    
    // volatile ensures the compiler does not optimize away index reads, 
    // forcing it to fetch the actual value from RAM every time since the IRQ modifies it.
    volatile std::size_t head_{0}; 
    volatile std::size_t tail_{0}; 
};

} // namespace Core