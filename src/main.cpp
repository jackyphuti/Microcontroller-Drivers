#include "core/system.hpp"
#include "drivers/gpio.hpp"
#include "drivers/uart.hpp"
#include "drivers/watchdog.hpp"
#include "drivers/i2c.hpp"
#include "drivers/spi.hpp"

// Utility to convert an 8-bit hex value to a string for the console
void PrintHex8(const Drivers::Uart& console, std::uint8_t val) {
    const char hex_chars[] = "0123456789ABCDEF";
    console.Write(hex_chars[(val >> 4) & 0x0F]);
    console.Write(hex_chars[val & 0x0F]);
}

int main() {
    // ---------------------------------------------------------
    // 1. Hardware Initialization Phase
    // ---------------------------------------------------------
    
    // Status LED (PA5 on Nucleo)
    Drivers::Gpio led(Drivers::Gpio::Port::A, 5);
    led.Configure(Drivers::Gpio::Mode::Output, Drivers::Gpio::OutputType::PushPull);

    // Telemetry Console (Interrupt-driven via Ring Buffer)
    Drivers::Uart console(Drivers::Uart::Instance::Uart2, 115200);
    console.EnableInterrupts();

    // Communication Buses
    Drivers::I2c i2c(Drivers::I2c::Instance::I2c1);
    Drivers::Spi spi(Drivers::Spi::Instance::Spi1);

    // Arm the hardware watchdog (2000 ms timeout)
    Drivers::Watchdog::Enable(2000);

    // ---------------------------------------------------------
    // 2. Boot Sequence
    // ---------------------------------------------------------
    console.Write("\r\n========================================\r\n");
    console.Write("[BOOT] Production Firmware Baseline Online\r\n");
    console.Write("[INFO] Target: STM32F411 (Cortex-M4)\r\n");
    console.Write("[INFO] Watchdog: Armed\r\n");
    console.Write("========================================\r\n");
    console.Write("Type 'help' for commands.\r\n> ");

    // ---------------------------------------------------------
    // 3. Non-Blocking Task Scheduler State
    // ---------------------------------------------------------
    std::uint64_t last_heartbeat = Core::GetTick();
    std::uint32_t uptime_seconds = 0;
    
    char cmd_buffer[16];
    std::uint8_t cmd_idx = 0;

    // ---------------------------------------------------------
    // 4. The Super-Loop
    // ---------------------------------------------------------
    while (true) {
        // [TASK 1] Pet the watchdog. If this loop ever freezes, the MCU hardware resets.
        Drivers::Watchdog::ResetTimer();

        // [TASK 2] Process incoming UART commands asynchronously
        while (auto data = Drivers::g_rx_buffer.Pop()) {
            char c = *data;
            
            // Echo character back to terminal
            console.Write(c); 

            if (c == '\r' || c == '\n') {
                cmd_buffer[cmd_idx] = '\0'; // Null-terminate
                console.Write("\r\n");

                // Command Parser
                if (cmd_idx > 0) {
                    if (cmd_buffer[0] == 'h') {
                        console.Write("Commands:\r\n");
                        console.Write("  h - Help\r\n");
                        console.Write("  i - Scan I2C bus for devices\r\n");
                        console.Write("  f - Force a crash (Test Watchdog)\r\n");
                    } 
                    else if (cmd_buffer[0] == 'i') {
                        console.Write("[I2C] Scanning bus...\r\n");
                        int devices_found = 0;
                        for (std::uint8_t addr = 1; addr < 128; ++addr) {
                            std::uint8_t dummy_buf[1];
                            // Attempt a 1-byte read to see if the device ACKs
                            if (i2c.Read(addr, dummy_buf)) {
                                console.Write("  Device found at 0x");
                                PrintHex8(console, addr);
                                console.Write("\r\n");
                                devices_found++;
                            }
                        }
                        if (devices_found == 0) {
                            console.Write("  No devices found.\r\n");
                        }
                    }
                    else if (cmd_buffer[0] == 'f') {
                        console.Write("[FAULT] Hanging the CPU. Watchdog will reset system in 2s...\r\n");
                        while (true) {
                            // Infinite loop. We stop petting the watchdog here.
                            asm volatile("nop"); 
                        }
                    }
                    else {
                        console.Write("Unknown command.\r\n");
                    }
                }
                
                cmd_idx = 0; // Reset buffer
                console.Write("> ");
            } 
            else if (cmd_idx < sizeof(cmd_buffer) - 1) {
                cmd_buffer[cmd_idx++] = c;
            }
        }

        // [TASK 3] 1-Hertz Heartbeat & Telemetry (Runs exactly every 1000ms without blocking)
        if ((Core::GetTick() - last_heartbeat) >= 1000) {
            last_heartbeat += 1000;
            uptime_seconds++;
            
            led.Toggle();

            // Print system telemetry every 10 seconds without disrupting the CLI prompt
            if (uptime_seconds % 10 == 0) {
                console.Write("\r\n[SYS] Uptime: ");
                
                // Simple integer to string
                std::uint32_t temp = uptime_seconds;
                char buf[12];
                int idx = 0;
                char rev[12];
                int r_idx = 0;
                while (temp > 0) {
                    rev[r_idx++] = static_cast<char>('0' + (temp % 10));
                    temp /= 10;
                }
                while (r_idx > 0) buf[idx++] = rev[--r_idx];
                buf[idx] = '\0';
                
                console.Write(buf);
                console.Write(" seconds\r\n> ");
            }
        }
    }

    return 0;
}