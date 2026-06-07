# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapAtomic::WrapAtomic)
    set(WrapAtomic_FOUND ON)
    return()
endif()

include(CheckCXXSourceCompiles)

set (atomic_test_sources "#include <atomic>
#include <cstdint>

int main(int, char **)
{
    volatile std::atomic<char>    size_1;
    volatile std::atomic<short>   size_2;
    volatile std::atomic<int>     size_4;
    volatile std::atomic<int64_t> size_8;

    ++size_1;
    ++size_2;
    ++size_4;
    ++size_8;

    (void)size_1.load(std::memory_order_relaxed);
    (void)size_2.load(std::memory_order_relaxed);
    (void)size_4.load(std::memory_order_relaxed);
    (void)size_8.load(std::memory_order_relaxed);

    return 0;
}")

check_cxx_source_compiles("${atomic_test_sources}" HAVE_STDATOMIC)
if(NOT HAVE_STDATOMIC)
    set(_req_libraries "${CMAKE_REQUIRED_LIBRARIES}")
    set(atomic_LIB "-latomic")
    set(CMAKE_REQUIRED_LIBRARIES ${atomic_LIB})
    check_cxx_source_compiles("${atomic_test_sources}" HAVE_STDATOMIC_WITH_LIB)
    set(CMAKE_REQUIRED_LIBRARIES "${_req_libraries}")
endif()

add_library(WrapAtomic::WrapAtomic INTERFACE IMPORTED)
if(HAVE_STDATOMIC_WITH_LIB)
    # atomic_LIB is already found above.
    target_link_libraries(WrapAtomic::WrapAtomic INTERFACE ${atomic_LIB})
endif()

set(WrapAtomic_FOUND 1)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapAtomic DEFAULT_MSG WrapAtomic_FOUND)
