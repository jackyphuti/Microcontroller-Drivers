#include "../include/core/ring_buffer.hpp"
#include <iostream>

#define ASSERT_TRUE(condition, msg) \
    if (!(condition)) { std::cerr << "FAIL: " << msg << "\n"; return 1; }

int test_ring_buffer_fifo() {
    Core::RingBuffer<int, 4> buffer; // Size 4 means it holds 3 items max (N-1 rule)

    ASSERT_TRUE(buffer.IsEmpty(), "Buffer should be empty initially");

    buffer.Push(10);
    buffer.Push(20);
    buffer.Push(30);

    ASSERT_TRUE(buffer.IsFull(), "Buffer should be full at N-1 capacity");
    ASSERT_TRUE(buffer.Push(40) == false, "Pushing to a full buffer should fail safely");

    auto val1 = buffer.Pop();
    ASSERT_TRUE(val1.has_value() && *val1 == 10, "First pop should be 10 (FIFO)");

    buffer.Push(50); // Now that we popped one, we can push again (wrap-around)
    
    auto val2 = buffer.Pop();
    ASSERT_TRUE(val2.has_value() && *val2 == 20, "Second pop should be 20");

    return 0;
}

int main() {
    int failures = 0;
    failures += test_ring_buffer_fifo();
    
    if (failures == 0) {
        std::cout << "All Ring Buffer tests passed.\n";
    }
    return failures;
}