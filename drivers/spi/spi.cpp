#include "drivers/spi.hpp"
#include "drivers/gpio.hpp"

namespace Drivers {

namespace {

constexpr std::uintptr_t RCC_APB2ENR = 0x40023844UL;

} // namespace

void Spi::EnableClock(Instance instance) {
    if (instance == Instance::Spi1) {
        auto* apb2 = reinterpret_cast<volatile std::uint32_t*>(RCC_APB2ENR);
        *apb2 |= (1U << 12); // Enable SPI1 clock
    }
}

Spi::Spi(Instance instance) 
    : regs_(reinterpret_cast<Registers*>(static_cast<std::uintptr_t>(instance))) {
    
    // Configure GPIO Pins for SPI1: PB3 (SCK), PB4 (MISO), PB5 (MOSI)
    // Using Port B keeps PA5 available for the status LED without hardware pin conflict.
    Gpio sck(Gpio::Port::B, 3);
    Gpio miso(Gpio::Port::B, 4);
    Gpio mosi(Gpio::Port::B, 5);
    
    sck.Configure(Gpio::Mode::Alternate, Gpio::OutputType::PushPull, Gpio::Speed::High, Gpio::Pull::None);
    miso.Configure(Gpio::Mode::Alternate, Gpio::OutputType::PushPull, Gpio::Speed::High, Gpio::Pull::PullUp);
    mosi.Configure(Gpio::Mode::Alternate, Gpio::OutputType::PushPull, Gpio::Speed::High, Gpio::Pull::None);
    
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
    // Flush any stale data in the receive buffer
    if (regs_->SR & (1U << 0)) {
        (void)regs_->DR;
    }

    // Wait until Transmit Buffer is Empty (TXE)
    std::uint32_t timeout = 50000;
    while (!(regs_->SR & (1U << 1))) {
        if (--timeout == 0U) {
            return 0xFF;
        }
    }
    
    // Write data to Data Register
    regs_->DR = data;
    
    // Wait until Receive Buffer is Not Empty (RXNE)
    timeout = 50000;
    while (!(regs_->SR & (1U << 0))) {
        if (--timeout == 0U) {
            return 0xFF;
        }
    }
    
    // Read and return the shifted-in byte
    return static_cast<std::uint8_t>(regs_->DR & 0xFFU);
}

void Spi::Transfer(std::span<const std::uint8_t> tx_buffer, std::span<std::uint8_t> rx_buffer) const {
    const std::size_t max_len = (tx_buffer.size() > rx_buffer.size()) ? tx_buffer.size() : rx_buffer.size();
    for (std::size_t i = 0; i < max_len; ++i) {
        const std::uint8_t tx_byte = (i < tx_buffer.size()) ? tx_buffer[i] : 0xFFU;
        const std::uint8_t rx_byte = Transfer(tx_byte);
        if (i < rx_buffer.size()) {
            rx_buffer[i] = rx_byte;
        }
    }

    // Wait until transmission is complete and SPI is no longer busy (BSY flag, bit 7)
    std::uint32_t timeout = 50000;
    while ((regs_->SR & (1U << 7)) && --timeout) {
        asm volatile("nop");
    }
}

} // namespace Drivers
