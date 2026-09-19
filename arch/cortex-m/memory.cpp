#include "core/memory.hpp"

// Provided by the flash.ld linker script
extern "C" std::uint32_t _ebss;

namespace {
    // The heap starts immediately after the uninitialized variables (.bss section)
    std::uint8_t* g_heap_start = reinterpret_cast<std::uint8_t*>(&_ebss);
    std::uint8_t* g_heap_current = nullptr;
    
    // STM32F401 has 96KB of SRAM ending at 0x20018000
    inline constexpr std::uintptr_t SRAM_END = 0x20018000UL;
}

namespace Core {
namespace Memory {

void Init() {
    g_heap_current = g_heap_start;
}

void* Allocate(std::size_t size) {
    // 8-byte alignment for ARM AAPCS compliance
    const std::size_t aligned_size = (size + 7U) & ~7U;

    if (g_heap_current + aligned_size > reinterpret_cast<std::uint8_t*>(SRAM_END)) {
        return nullptr; // Out of memory
    }

    void* ptr = g_heap_current;
    g_heap_current += aligned_size;
    return ptr;
}

void Free(void* ptr) {
    (void)ptr; // Bump allocators cannot free fragmented memory
}

} // namespace Memory
} // namespace Core

void* operator new(std::size_t size) { return Core::Memory::Allocate(size); }
void* operator new[](std::size_t size) { return Core::Memory::Allocate(size); }
void operator delete(void* ptr) noexcept { Core::Memory::Free(ptr); }
void operator delete[](void* ptr) noexcept { Core::Memory::Free(ptr); }