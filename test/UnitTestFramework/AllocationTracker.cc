#include "AllocationTracker.h"

#include <algorithm>
#include <cstdlib>
#include <new>

#ifdef _WIN32
#include <malloc.h>
#endif

#ifndef QGC_UNITTEST_BUILD
#error Allocation instrumentation must only be linked into unit-test builds.
#endif

namespace {
thread_local QGCTest::AllocationTracker* activeTracker = nullptr;
thread_local QGCTest::AllocationFailureScope* activeFailure = nullptr;

void* allocate(std::size_t size, std::size_t alignment = 0)
{
    QGCTest::detail::recordAllocation(size);
    for (;;) {
        void* memory = nullptr;
        if (!QGCTest::detail::failAllocation()) {
            if (alignment == 0) {
                memory = std::malloc(size ? size : 1);
            } else {
                alignment = (std::max) (alignment, sizeof(void*));
#ifdef _WIN32
                memory = _aligned_malloc(size ? size : 1, alignment);
#else
                if (posix_memalign(&memory, alignment, size ? size : 1) != 0) {
                    memory = nullptr;
                }
#endif
            }
        }
        if (memory) {
            return memory;
        }
        const auto handler = std::get_new_handler();
        if (!handler) {
            throw std::bad_alloc();
        }
        handler();
    }
}

void releaseAligned(void* memory) noexcept
{
#ifdef _WIN32
    _aligned_free(memory);
#else
    std::free(memory);
#endif
}
}  // namespace

QGCTest::AllocationTracker::AllocationTracker() noexcept
    : _previous(activeTracker)
{
    activeTracker = this;
}

QGCTest::AllocationTracker::~AllocationTracker()
{
    activeTracker = _previous;
}

void QGCTest::detail::recordAllocation(std::size_t size) noexcept
{
    for (auto* tracker = activeTracker; tracker; tracker = tracker->_previous) {
        ++tracker->_counts.calls;
        tracker->_counts.bytes += size;
        tracker->_counts.largest = (std::max) (tracker->_counts.largest, size);
    }
}

QGCTest::AllocationFailureScope::AllocationFailureScope(std::size_t failures) noexcept
    : _previous(activeFailure)
    , _remaining(failures)
{
    activeFailure = this;
}

QGCTest::AllocationFailureScope::~AllocationFailureScope()
{
    activeFailure = _previous;
}

bool QGCTest::detail::failAllocation() noexcept
{
    if (!activeFailure || activeFailure->_remaining == 0) {
        return false;
    }
    --activeFailure->_remaining;
    return true;
}

void* operator new(std::size_t size)
{
    return allocate(size);
}

void* operator new[](std::size_t size)
{
    return ::operator new(size);
}

void* operator new(std::size_t size, std::align_val_t alignment)
{
    return allocate(size, static_cast<std::size_t>(alignment));
}

void* operator new[](std::size_t size, std::align_val_t alignment)
{
    return ::operator new(size, alignment);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    try {
        return ::operator new(size);
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
        return ::operator new(size, alignment);
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
    releaseAligned(memory);
}

void operator delete[](void* memory, std::align_val_t) noexcept
{
    releaseAligned(memory);
}

void operator delete(void* memory, std::size_t, std::align_val_t) noexcept
{
    releaseAligned(memory);
}

void operator delete[](void* memory, std::size_t, std::align_val_t) noexcept
{
    releaseAligned(memory);
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
    releaseAligned(memory);
}

void operator delete[](void* memory, std::align_val_t, const std::nothrow_t&) noexcept
{
    releaseAligned(memory);
}
