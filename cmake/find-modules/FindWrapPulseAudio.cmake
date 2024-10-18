# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# FindWrapPulseAudio
# ---------
#
# Try to locate the pulseaudio library.
# If found, this will define the following variables:
#
# ``WrapPulseAudio_FOUND``
#     True if the pulseaudio library is available
#
# If ``WrapPulseAduio_FOUND`` is TRUE, it will also define the following
# imported target:
#
# ``WrapPulseAudio::WrapPulseAudio``
#     The pulseaudio library to link to

if(TARGET WrapPulseAudio::WrapPulseAudio)
    set(WrapPulseAudio_FOUND ON)
    return()
endif()
find_package(PulseAudio QUIET)
if(PulseAudio_FOUND)
    set(WrapPulseAudio_FOUND 1)
endif()
if(WrapPulseAudio_FOUND AND NOT TARGET WrapPulseAudio::WrapPulseAudio)
    add_library(WrapPulseAudio::WrapPulseAudio INTERFACE IMPORTED)
    target_include_directories(WrapPulseAudio::WrapPulseAudio INTERFACE "${PULSEAUDIO_INCLUDE_DIR}")
    target_link_libraries(WrapPulseAudio::WrapPulseAudio
                        INTERFACE "${PULSEAUDIO_LIBRARY}")
endif()
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapPulseAudio REQUIRED_VARS
                                PULSEAUDIO_LIBRARY PULSEAUDIO_INCLUDE_DIR WrapPulseAudio_FOUND)

mark_as_advanced(PULSEAUDIO_LIBRARY PULSEAUDIO_INCLUDE_DIR)
