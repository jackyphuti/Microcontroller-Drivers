#pragma once

#include <cstdint>
#include <span>

namespace Drivers {

class Spi {
public:
    enum class Instance : std::uintptr_t {
        Spi1 = 0x40013000UL // APB2 Peripheral
    };

    explicit Spi(Instance instance);

    // SPI is strictly full-duplex. Every transmitted byte shifts in a received byte.
    std::uint8_t Transfer(std::uint8_t data) const;
    
    // Transfer bulk data arrays safely
    void Transfer(std::span<const std::uint8_t> tx_buffer, std::span<std::uint8_t> rx_buffer) const;

private:
    struct Registers {
        volatile std::uint32_t CR1;
        volatile std::uint32_t CR2;
        volatile std::uint32_t SR;
        volatile std::uint32_t DR;
        volatile std::uint32_t CRCPR;
        volatile std::uint32_t RXCRCR;
        volatile std::uint32_t TXCRCR;
        volatile std::uint32_t I2SCFGR;
        volatile std::uint32_t I2SPR;
    };

    Registers* const regs_;
    static void EnableClock(Instance instance);
};

} // namespace Drivers