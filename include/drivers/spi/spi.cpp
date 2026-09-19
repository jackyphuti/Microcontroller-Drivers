#include "drivers/spi.hpp"
#include "drivers/gpio.hpp"

namespace Drivers {

inline constexpr std::uintptr_t RCC_APB2ENR = 0x40023844UL;

void Spi::EnableClock(Instance instance) {
    if (instance == Instance::Spi1) {
        auto* apb2 = reinterpret_cast<volatile std::uint32_t*>(RCC_APB2ENR);
        *apb2 |= (1U << 12); // Enable SPI1 clock
    }
}

Spi::Spi(Instance instance) 
    : regs_(reinterpret_cast<Registers*>(static_cast<std::uintptr_t>(instance))) {
    
    // Configure GPIO Pins for SPI1: PA5 (SCK), PA6 (MISO), PA7 (MOSI)
    // CS (Chip Select) is intentionally left out; it should be handled manually by the main loop 
    // as a standard GPIO output to support multi-slave configurations.
    Gpio sck(Gpio::Port::A, 5);
    Gpio miso(Gpio::Port::A, 6);
    Gpio mosi(Gpio::Port::A, 7);
    
    sck.Configure(Gpio::Mode::Alternate, Gpio::OutputType::PushPull, Gpio::Speed::High);
    miso.Configure(Gpio::Mode::Alternate, Gpio::OutputType::PushPull, Gpio::Speed::High);
    mosi.Configure(Gpio::Mode::Alternate, Gpio::OutputType::PushPull, Gpio::Speed::High);
    
    sck.SetAlternateFunction(5);
    miso.SetAlternateFunction(5);
    mosi.SetAlternateFunction(5);

    EnableClock(instance);

    // CR1 Configuration
    // Baud Rate Control: fPCLK/16 (Bit 3:5 = 011). At 16MHz, SPI runs at 1MHz.
    // CPOL=0, CPHA=0 (Mode 0), 8-bit frame, MSB first
    // Software Slave Management (SSM=1, SSI=1), Master Mode (MSTR=1)
    regs_->CR1 = (0b011 << 3) | (1U << 9) | (1U << 8) | (1U << 2);

    // Enable SPI
    regs_->CR1 |= (1U << 6); // SPE
}

std::uint8_t Spi::Transfer(std::uint8_t data) const {
    // Wait until Transmit Buffer is Empty (TXE)
    while (!(regs_->SR & (1U << 1))) { asm volatile("nop"); }
    
    // Write data to Data Register
    regs_->DR = data;
    
    // Wait until Receive Buffer is Not Empty (RXNE)
    while (!(regs_->SR & (1U << 0))) { asm volatile("nop"); }
    
    // Read and return the shifted-in byte
    return static_cast<std::uint8_t>(regs_->DR);
}

void Spi::Transfer(std::span<const std::uint8_t> tx_buffer, std::span<std::uint8_t> rx_buffer) const {
    const std::size_t len = tx_buffer.size();
    for (std::size_t i = 0; i < len; ++i) {
        std::uint8_t rx = Transfer(tx_buffer[i]);
        if (i < rx_buffer.size()) {
            rx_buffer[i] = rx;
        }
    }
}

} // namespace Drivers