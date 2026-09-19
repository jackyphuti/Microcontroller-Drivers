#include "drivers/i2c.hpp"
#include "drivers/gpio.hpp"

namespace Drivers {

inline constexpr std::uintptr_t RCC_APB1ENR = 0x40023840UL;

void I2c::EnableClock(Instance instance) {
    if (instance == Instance::I2c1) {
        auto* apb1 = reinterpret_cast<volatile std::uint32_t*>(RCC_APB1ENR);
        *apb1 |= (1U << 21); // Enable I2C1 clock
    }
}

I2c::I2c(Instance instance) 
    : regs_(reinterpret_cast<Registers*>(static_cast<std::uintptr_t>(instance))) {
    
    // Configure GPIO Pins for I2C1: PB6 (SCL), PB7 (SDA)
    // I2C strictly requires Open-Drain output type to allow external pull-up resistors to pull the line high.
    Gpio scl(Gpio::Port::B, 6);
    Gpio sda(Gpio::Port::B, 7);
    
    scl.Configure(Gpio::Mode::Alternate, Gpio::OutputType::OpenDrain, Gpio::Speed::High, Gpio::Pull::PullUp);
    sda.Configure(Gpio::Mode::Alternate, Gpio::OutputType::OpenDrain, Gpio::Speed::High, Gpio::Pull::PullUp);
    
    scl.SetAlternateFunction(4);
    sda.SetAlternateFunction(4);

    EnableClock(instance);

    // Software Reset the I2C Block
    regs_->CR1 |= (1U << 15);
    regs_->CR1 &= ~(1U << 15);

    // Set Peripheral Clock Frequency (16 MHz APB1)
    regs_->CR2 = 16;

    // Configure Clock Control Register (CCR) for Standard Mode (100 kHz)
    // T_high = CCR * T_PCLK1 = 80 * (1 / 16MHz) = 5 us. (Period = 10 us -> 100 kHz)
    regs_->CCR = 80;

    // Maximum rise time in Standard Mode is 1000ns. 
    // TRISE = (1000ns / 62.5ns) + 1 = 17
    regs_->TRISE = 17;

    // Enable I2C Peripheral
    regs_->CR1 |= (1U << 0); // PE
}

bool I2c::Write(std::uint8_t device_address, std::span<const std::uint8_t> data) const {
    // Generate START condition
    regs_->CR1 |= (1U << 8); 
    while (!(regs_->SR1 & (1U << 0))); // Wait for SB (Start Bit)

    // Send Slave Address with Write bit (0)
    regs_->DR = static_cast<std::uint32_t>(device_address << 1);
    
    // Wait for ADDR flag
    while (!(regs_->SR1 & (1U << 1))) {
        if (regs_->SR1 & (1U << 10)) { // AF (Acknowledge Failure)
            regs_->CR1 |= (1U << 9); // Generate STOP
            regs_->SR1 &= ~(1U << 10); // Clear AF
            return false;
        }
    }
    
    // Hardware Quirk: ADDR flag is cleared by reading SR1 followed by SR2
    (void)regs_->SR1;
    (void)regs_->SR2;

    for (std::uint8_t byte : data) {
        while (!(regs_->SR1 & (1U << 7))); // Wait for TXE (Transmit Empty)
        regs_->DR = byte;
    }

    while (!(regs_->SR1 & (1U << 2))); // Wait for BTF (Byte Transfer Finished)
    
    // Generate STOP condition
    regs_->CR1 |= (1U << 9); 
    return true;
}

bool I2c::Read(std::uint8_t device_address, std::span<std::uint8_t> buffer) const {
    if (buffer.empty()) return true;

    // Enable ACK generation
    regs_->CR1 |= (1U << 10);

    // Generate START
    regs_->CR1 |= (1U << 8); 
    while (!(regs_->SR1 & (1U << 0)));

    // Send Slave Address with Read bit (1)
    regs_->DR = static_cast<std::uint32_t>((device_address << 1) | 1);
    
    while (!(regs_->SR1 & (1U << 1))) {
        if (regs_->SR1 & (1U << 10)) { 
            regs_->CR1 |= (1U << 9); 
            regs_->SR1 &= ~(1U << 10); 
            return false;
        }
    }

    // 1-byte read requires special clearing procedure
    if (buffer.size() == 1) {
        regs_->CR1 &= ~(1U << 10); // Clear ACK
        (void)regs_->SR1; (void)regs_->SR2; // Clear ADDR
        regs_->CR1 |= (1U << 9); // Generate STOP
        
        while (!(regs_->SR1 & (1U << 6))); // Wait for RXNE
        buffer[0] = static_cast<std::uint8_t>(regs_->DR);
        return true;
    }

    // N-byte read
    (void)regs_->SR1; (void)regs_->SR2; // Clear ADDR

    for (std::size_t i = 0; i < buffer.size(); ++i) {
        if (i == buffer.size() - 1) {
            regs_->CR1 &= ~(1U << 10); // Clear ACK for the last byte
            regs_->CR1 |= (1U << 9);   // Generate STOP before reading the last byte
        }

        while (!(regs_->SR1 & (1U << 6))); // Wait for RXNE
        buffer[i] = static_cast<std::uint8_t>(regs_->DR);
    }

    return true;
}

} // namespace Drivers