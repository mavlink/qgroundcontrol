# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET Flite::Flite)
    set(Flite_FOUND 1)
    return()
endif()

find_path(Flite_INCLUDE_DIR
    NAMES
        flite/flite.h
)
find_library(Flite_LIBRARY
    NAMES
        flite
)

if(NOT Flite_INCLUDE_DIR OR NOT Flite_LIBRARY)
    set(Flite_FOUND 0)
    return()
endif()

include(CMakePushCheckState)
include(CheckCXXSourceCompiles)

# Flite can be built with ALSA support,
# in which case we need to link ALSA as well
find_package(ALSA QUIET)

cmake_push_check_state(RESET)

set(CMAKE_REQUIRED_INCLUDES "${Flite_INCLUDE_DIR}")
set(CMAKE_REQUIRED_LIBRARIES "${Flite_LIBRARY}")

if(ALSA_FOUND)
    list(APPEND CMAKE_REQUIRED_LIBRARIES "${ALSA_LIBRARIES}")
endif()

check_cxx_source_compiles("
#include <flite/flite.h>

static int fliteAudioCb(const cst_wave *w, int start, int size,
    int last, cst_audio_streaming_info *asi)
{
    (void)w;
    (void)start;
    (void)size;
    (void)last;
    (void)asi;
    return CST_AUDIO_STREAM_STOP;
}

int main()
{
    cst_audio_streaming_info *asi = new_audio_streaming_info();
    asi->asc = fliteAudioCb; // This fails for old Flite
    new_audio_streaming_info();
    return 0;
}
" HAVE_FLITE)

cmake_pop_check_state()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Flite
    FOUND_VAR
        Flite_FOUND
    REQUIRED_VARS
        Flite_LIBRARY
        Flite_INCLUDE_DIR
        HAVE_FLITE
)

if(Flite_FOUND)
    add_library(Flite::Flite UNKNOWN IMPORTED)
    set_target_properties(Flite::Flite PROPERTIES
        IMPORTED_LOCATION "${Flite_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Flite_INCLUDE_DIR}"
    )
    if(ALSA_FOUND)
    set_target_properties(Flite::Flite PROPERTIES
        INTERFACE_LINK_LIBRARIES "${ALSA_LIBRARIES}"
    )
    endif()
endif()

mark_as_advanced(Flite_LIBRARY Flite_INCLUDE_DIR HAVE_FLITE)


if(HAVE_FLITE)
    set(Flite_FOUND 1)
else()
    message("Flite was found, but the version is too old (<2.0.0)")
    set(Flite_FOUND 0)
endif()
