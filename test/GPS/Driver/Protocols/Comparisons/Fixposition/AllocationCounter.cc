#include "AllocationCounter.h"

#include <algorithm>
#include <cstdlib>
#include <new>

namespace {
thread_local bool tracking = false;
thread_local AllocationCounter::Counts counts;

void account(std::size_t size)
{
    if (tracking) {
        ++counts.calls;
        counts.bytes += size;
        counts.largest = std::max(counts.largest, size);
    }
}

void* allocate(std::size_t size)
{
    if (void* memory = std::malloc(size ? size : 1)) {
        account(size);
        return memory;
    }
    throw std::bad_alloc();
}

void* allocateAligned(std::size_t size, std::align_val_t alignment)
{
    void* memory = nullptr;
    if (posix_memalign(&memory, static_cast<std::size_t>(alignment), size ? size : 1) == 0) {
        account(size);
        return memory;
    }
    throw std::bad_alloc();
}
}  // namespace

void AllocationCounter::start()
{
    counts = {};
    tracking = true;
}

AllocationCounter::Counts AllocationCounter::stop()
{
    tracking = false;
    return counts;
}

void* operator new(std::size_t size)
{
    return allocate(size);
}

void* operator new[](std::size_t size)
{
    return allocate(size);
}

void* operator new(std::size_t size, std::align_val_t alignment)
{
    return allocateAligned(size, alignment);
}

void* operator new[](std::size_t size, std::align_val_t alignment)
{
    return allocateAligned(size, alignment);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    try {
        return allocate(size);
    } catch (...) {
        return nullptr;
    }
}

void* operator new[](std::size_t size, const std::nothrow_t& tag) noexcept
{
    return ::operator new(size, tag);
}

void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    try {
        return allocateAligned(size, alignment);
    } catch (...) {
        return nullptr;
    }
}

void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t& tag) noexcept
{
    return ::operator new(size, alignment, tag);
}

void operator delete(void* memory) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, std::size_t) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::align_val_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, std::align_val_t) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t, std::align_val_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, std::size_t, std::align_val_t) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, const std::nothrow_t&) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, const std::nothrow_t&) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::align_val_t, const std::nothrow_t&) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, std::align_val_t, const std::nothrow_t&) noexcept
{
    std::free(memory);
}
