# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapRt::WrapRt)
    set(WrapRt_FOUND ON)
    return()
endif()

set(WrapRt_FOUND OFF)

include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

find_library(LIBRT rt)

cmake_push_check_state()
if(LIBRT)
    list(APPEND CMAKE_REQUIRED_LIBRARIES "${LIBRT}")
endif()

check_cxx_source_compiles("
#include <time.h>
#include <unistd.h>

int main(int, char **) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return 0;
}
" HAVE_GETTIME)

check_cxx_source_compiles("
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

int main(int, char **) {
    shm_open(\"test\", O_RDWR | O_CREAT | O_EXCL, 0666);
    shm_unlink(\"test\");
    return 0;
}
" HAVE_SHM_OPEN_SHM_UNLINK)

cmake_pop_check_state()

if(HAVE_GETTIME OR HAVE_SHM_OPEN_SHM_UNLINK)
    set(WrapRt_FOUND ON)
    add_library(WrapRt::WrapRt INTERFACE IMPORTED)
    if (LIBRT)
        target_link_libraries(WrapRt::WrapRt INTERFACE "${LIBRT}")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapRt DEFAULT_MSG WrapRt_FOUND)
