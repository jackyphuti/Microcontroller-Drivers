#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace Drivers {

class Uart {
public:
    enum class Instance : std::uintptr_t {
        Uart2 = 0x40004400UL
    };

    explicit Uart(Instance instance, std::uint32_t baudrate);

    [[nodiscard]] bool Write(char c, std::uint32_t timeout_ms) const;
    [[nodiscard]] bool Write(const char* message, std::uint32_t timeout_ms) const;
    [[nodiscard]] bool Write(std::string_view message, std::uint32_t timeout_ms) const;
    [[nodiscard]] std::optional<char> Read(std::uint32_t timeout_ms) const;
    [[nodiscard]] bool HasData() const;

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

} // namespace Drivers