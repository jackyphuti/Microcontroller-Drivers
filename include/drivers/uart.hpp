#pragma once

#include <cstdint>
#include <string_view>
#include <optional>

namespace Drivers {

class Uart {
public:
    enum class Instance : std::uintptr_t {
        Uart2 = 0x40004400UL
    };

    explicit Uart(Instance instance, std::uint32_t baudrate);

    void Write(char c) const;
    void Write(std::string_view message) const;
    
    // Interrupt Control
    void EnableInterrupts() const;

private:
    struct Registers {
        volatile std::uint32_t SR;
        volatile std::uint32_t DR;
        volatile std::uint32_t BRR;
        volatile std::uint32_t CR1;
        volatile std::uint32_t CR2;
        volatile std::uint32_t CR3;
        volatile std::uint32_t GTPR;
    };

    Registers* const regs_;
    static void EnableClock(Instance instance);
};

// Global flags for the IRQ handler to communicate with main()
extern volatile char g_async_rx_char;
extern volatile bool g_async_rx_ready;

} // namespace Drivers