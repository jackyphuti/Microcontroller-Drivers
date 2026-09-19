#include "core/system.hpp"
#include "drivers/gpio.hpp"
#include "drivers/uart.hpp"
#include "drivers/watchdog.hpp"

int main() {
    Drivers::Gpio led(Drivers::Gpio::Port::A, 5);
    led.Configure(Drivers::Gpio::Mode::Output, Drivers::Gpio::OutputType::PushPull);

    Drivers::Uart console(Drivers::Uart::Instance::Uart2, 115200);
    console.EnableInterrupts();
    Drivers::Watchdog::Enable(2000);

    console.Write("\r\n[BOOT] Ring Buffer and Interrupts Online.\r\n");

    while (true) {
        Drivers::Watchdog::ResetTimer();
        
        // Safely extract and process everything in the queue
        while (auto data = Drivers::g_rx_buffer.Pop()) {
            char c = *data;
            
            console.Write("\r\n[PROCESS] Pulled from buffer: ");
            console.Write(c);
            console.Write("\r\n");
            
            led.Toggle();
        }

        // Do heavy work here (e.g., rendering a display, calculating math)
        // The interrupt will continue catching new UART bytes in the background.
    }
    return 0;
}