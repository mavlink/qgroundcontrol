# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


find_package(PkgConfig QUIET)

function(find_component component prefix header library)
    if(NOT TARGET VAAPI::${component})
        string(TOUPPER ${component} upper)
        pkg_check_modules(PC_VAAPI_${upper} ${prefix} IMPORTED_TARGET)
        if(TARGET PkgConfig::PC_VAAPI_${upper})
            add_library(VAAPI::${component} INTERFACE IMPORTED)
            target_link_libraries(VAAPI::${component} INTERFACE PkgConfig::PC_VAAPI_${upper})
        else()
            find_path(VAAPI_${component}_INCLUDE_DIR
                NAMES ${header}
                PATH_SUFFIXES VAAPI-1.0
            )
            find_library(VAAPI_${component}_LIBRARY
                NAMES ${library}
            )
            if(VAAPI_${component}_LIBRARY AND VAAPI_${component}_INCLUDE_DIR)
                add_library(VAAPI::${component} INTERFACE IMPORTED)
                target_include_directories(VAAPI::${component} INTERFACE ${VAAPI_${component}_INCLUDE_DIR})
                target_link_libraries(VAAPI::${component} INTERFACE ${VAAPI_${component}_LIBRARY})
            endif()
            mark_as_advanced(VAAPI_${component}_INCLUDE_DIR VAAPI_${component}_LIBRARY)
        endif()
    endif()

    if(TARGET VAAPI::${component})
        set(VAAPI_${component}_FOUND TRUE PARENT_SCOPE)
    endif()
endfunction()

find_component(VA libva va/va.h libva)
find_component(DRM libva-drm va/va-drm.h libva-drm)

if(TARGET VAAPI::VA)
    target_link_libraries(VAAPI::VA)
endif()
if(TARGET VAAPI::VA AND TARGET VAAPI::DRM)
    target_link_libraries(VAAPI::DRM INTERFACE VAAPI::VA)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VAAPI
                                REQUIRED_VARS
                                VAAPI_VA_FOUND
                                VAAPI_DRM_FOUND
                                HANDLE_COMPONENTS
)

if(VAAPI_FOUND AND NOT TARGET VAAPI::VAAPI)
    add_library(VAAPI::VAAPI INTERFACE IMPORTED)
    target_link_libraries(VAAPI::VAAPI INTERFACE
                            VAAPI::VA
                            VAAPI::DRM
    )
endif()
