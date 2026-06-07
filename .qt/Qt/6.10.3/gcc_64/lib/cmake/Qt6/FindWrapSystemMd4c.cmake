# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET WrapSystemMd4c::WrapSystemMd4c)
    set(WrapSystemMd4c_FOUND TRUE)
    return()
endif()
set(WrapSystemMd4c_REQUIRED_VARS __md4c_found)

find_package(md4c ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} QUIET)

set(__md4c_target_name "md4c::md4c")

if(md4c_FOUND)
    set(__md4c_found TRUE)

    # md4c provides a md4c::md4c target but
    # older versions create a md4c target without
    # namespace. If we find the old variant create
    # a namespaced target out of the md4c target.
    if(TARGET md4c AND NOT TARGET ${__md4c_target_name})
        add_library(${__md4c_target_name} INTERFACE IMPORTED)
        target_link_libraries(${__md4c_target_name} INTERFACE md4c)
    endif()

    if(md4c_VERSION)
        set(WrapSystemMd4c_VERSION "${md4c_VERSION}")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapSystemMd4c
                                  REQUIRED_VARS ${WrapSystemMd4c_REQUIRED_VARS}
                                  VERSION_VAR WrapSystemMd4c_VERSION)

if(WrapSystemMd4c_FOUND)
    add_library(WrapSystemMd4c::WrapSystemMd4c INTERFACE IMPORTED)
    target_link_libraries(WrapSystemMd4c::WrapSystemMd4c
                          INTERFACE "${__md4c_target_name}")
endif()

unset(__md4c_found)
unset(__md4c_target_name)
