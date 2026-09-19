#include "core/system.hpp"
#include "drivers/gpio.hpp"
#include "drivers/uart.hpp"
#include "drivers/watchdog.hpp"

int main() {
    Drivers::Gpio led(Drivers::Gpio::Port::A, 5);
    led.Configure(Drivers::Gpio::Mode::Output, Drivers::Gpio::OutputType::PushPull);

    Drivers::Uart console(Drivers::Uart::Instance::Uart2, 115200);
    
    // Arm the hardware interrupt pipeline
    console.EnableInterrupts();
    Drivers::Watchdog::Enable(2000);

    console.Write("\r\n[BOOT] Interrupt-Driven UART Online.\r\n");

    while (true) {
        Drivers::Watchdog::ResetTimer();
        
        // Non-blocking check for async data injected by the IRQ handler
        if (Drivers::g_async_rx_ready) {
            char incoming = Drivers::g_async_rx_char;
            Drivers::g_async_rx_ready = false; // Clear the flag

            console.Write("\r\n[IRQ] CPU was interrupted! Received: ");
            console.Write(incoming);
            console.Write("\r\n");
            
            // Toggle LED instantly upon receiving a keystroke
            led.Toggle(); 
        }

        // The CPU is now free to do heavy processing here without missing data
        // ...
    }
    return 0;
}