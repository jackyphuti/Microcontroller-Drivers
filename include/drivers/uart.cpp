#include "drivers/uart.hpp"
#include "drivers/gpio.hpp"

namespace Drivers {

volatile char g_async_rx_char = '\0';
volatile bool g_async_rx_ready = false;

// ... (Keep existing EnableClock, Constructor, and Write methods) ...

void Uart::EnableInterrupts() const {
    // 1. Enable the RX Not Empty Interrupt on the UART peripheral
    regs_->CR1 |= (1U << 5); // RXNEIE

    // 2. Enable IRQ 38 in the CPU's Nested Vector Interrupt Controller (NVIC)
    // The NVIC ISER (Interrupt Set-Enable Registers) are 32-bits wide.
    // IRQ 38 is in ISER[1] (which handles IRQs 32-63) at bit 6 (38 - 32 = 6).
    inline constexpr std::uintptr_t NVIC_ISER1 = 0xE000E104UL;
    auto* iser1 = reinterpret_cast<volatile std::uint32_t*>(NVIC_ISER1);
    
    *iser1 |= (1U << 6);
}

} // namespace Drivers

// ---------------------------------------------------------
// Hardware Interrupt Request (IRQ) Handler
// ---------------------------------------------------------
// This function instantly interrupts the CPU the microsecond a byte arrives.
extern "C" void USART2_IRQHandler() {
    auto* sr = reinterpret_cast<volatile std::uint32_t*>(0x40004400UL); // USART2 SR
    auto* dr = reinterpret_cast<volatile std::uint32_t*>(0x40004404UL); // USART2 DR

    // Check if the interrupt was caused by the RX Not Empty (RXNE) flag
    if (*sr & (1U << 5)) { 
        // Reading the Data Register (DR) automatically clears the interrupt flag
        Drivers::g_async_rx_char = static_cast<char>(*dr & 0xFF);
        Drivers::g_async_rx_ready = true;
    }
}