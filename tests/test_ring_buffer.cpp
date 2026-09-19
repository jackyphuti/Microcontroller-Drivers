#include <cstddef>

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            return 1; \
        } \
    } while (false)

template <typename T, std::size_t Capacity>
class RingBuffer {
public:
    [[nodiscard]] constexpr std::size_t capacity() const {
        return Capacity;
    }
};

int test_ring_buffer_capacity() {
    RingBuffer<char, 256> buffer;
    ASSERT_TRUE(buffer.capacity() == 256);
    return 0;
}

int main() {
    int failures = 0;
    failures += test_ring_buffer_capacity();
    return failures;
}