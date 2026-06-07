# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#.rst:
# PlatformGraphics
# ---------

if(CMAKE_SYSTEM_NAME STREQUAL "Integrity")
    set(platform Integrity)
elseif(CMAKE_SYSTEM_NAME STREQUAL "VxWorks")
    set(platform VxWorks)
else()
    set(PlatformGraphics_FOUND FALSE)
    return()
endif()

find_package(${platform}PlatformGraphics)

set(platform_target ${platform}PlatformGraphics::${platform}PlatformGraphics)
if(NOT ${platform}PlatformGraphics_FOUND OR
    NOT TARGET ${platform_target})
    set(PlatformGraphics_FOUND FALSE)
    return()
endif()

if(NOT TARGET PlatformGraphics::PlatformGraphics)
    add_library(PlatformGraphics::PlatformGraphics INTERFACE IMPORTED)
    target_link_libraries(PlatformGraphics::PlatformGraphics INTERFACE ${platform_target})

    # The list of libraries that are required to pass the EGL/OpenGL/GLESv2
    # compile checks. The list might or might not be provided by platforms or
    # toolchain files.
    foreach(known_var LIBRARIES INCLUDES DEFINITIONS)
        string(TOLOWER "${known_var}" known_var_lc)
        if(${platform}PlatformGraphics_REQUIRED_${known_var})
            set_property(TARGET PlatformGraphics::PlatformGraphics PROPERTY
                _qt_internal_platform_graphics_required_${known_var_lc}
                "${${platform}PlatformGraphics_REQUIRED_${known_var}}"
            )
        endif()
    endforeach()
    unset(known_var)
    unset(known_var_lc)
endif()

function(platform_graphics_extend_check_cxx_source_required_variables)
    foreach(known_var LIBRARIES INCLUDES DEFINITIONS)
        string(TOLOWER "${known_var}" known_var_lc)
        get_target_property(platform_graphics_required_${known_var_lc}
            PlatformGraphics::PlatformGraphics
            _qt_internal_platform_graphics_required_${known_var_lc}
        )
        if(platform_graphics_required_${known_var_lc})
            list(APPEND CMAKE_REQUIRED_${known_var} ${platform_graphics_required_${known_var_lc}})
            set(CMAKE_REQUIRED_${known_var} "${CMAKE_REQUIRED_${known_var}}" PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

unset(platform)
unset(platform_target)

set(PlatformGraphics_FOUND TRUE)
