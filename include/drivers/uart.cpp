#include "drivers/uart.hpp"
#include "drivers/gpio.hpp"

namespace Drivers {

inline constexpr std::uintptr_t RCC_APB1ENR = 0x40023840UL;

void Uart::EnableClock(Instance instance) {
    if (instance == Instance::Uart2) {
        auto* apb1 = reinterpret_cast<volatile std::uint32_t*>(RCC_APB1ENR);
        *apb1 |= (1U << 17); // Enable USART2 clock
    }
}

Uart::Uart(Instance instance, std::uint32_t baudrate) 
    : regs_(reinterpret_cast<Registers*>(static_cast<std::uintptr_t>(instance))) {
    
    // Configure GPIO Pins (PA2 = TX, PA3 = RX) with AF7
    Gpio tx(Gpio::Port::A, 2);
    Gpio rx(Gpio::Port::A, 3);
    
    tx.Configure(Gpio::Mode::Alternate, Gpio::OutputType::PushPull, Gpio::Speed::High, Gpio::Pull::None);
    tx.SetAlternateFunction(7);

    rx.Configure(Gpio::Mode::Alternate, Gpio::OutputType::PushPull, Gpio::Speed::High, Gpio::Pull::None);
    rx.SetAlternateFunction(7);

    EnableClock(instance);

    // Compute Baud Rate Divider for 16MHz Peripheral Bus Clock
    // BaudRate = Fck / (16 * USARTDIV) -> USARTDIV = 16MHz / (16 * BaudRate) = 1MHz / BaudRate
    const std::uint32_t apb1_freq = 16000000UL;
    regs_->BRR = (apb1_freq + (baudrate / 2U)) / baudrate;

    // Enable Transmitter, Receiver, and USART peripheral
    regs_->CR1 = (1U << 13) | (1U << 3) | (1U << 2); // UE | TE | RE
}

void Uart::Write(char c) const {
    // Wait until Transmit Data Register is Empty (TXE bit 7)
    while (!(regs_->SR & (1U << 7))) {
        asm volatile("nop");
    }
    regs_->DR = static_cast<std::uint8_t>(c);
}

void Uart::Write(std::string_view message) const {
    for (char c : message) {
        Write(c);
    }
}

bool Uart::HasData() const {
    // Read Data Register Not Empty (RXNE bit 5)
    return (regs_->SR & (1U << 5)) != 0;
}

char Uart::Read() const {
    while (!HasData()) {
        asm volatile("nop");
    }
    return static_cast<char>(regs_->DR & 0xFF);
}

} // namespace Drivers