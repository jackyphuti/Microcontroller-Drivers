#pragma once

#include <cstddef>
#include <cstdint>

namespace Core {
namespace Memory {

    void Init();
    void* Allocate(std::size_t size);
    void Free(void* ptr);

} // namespace Memory
} // namespace Core

void* operator new(std::size_t size);
void* operator new[](std::size_t size);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;