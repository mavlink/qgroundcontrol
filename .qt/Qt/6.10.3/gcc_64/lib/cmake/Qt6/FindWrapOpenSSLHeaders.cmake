# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapOpenSSLHeaders::WrapOpenSSLHeaders)
    set(WrapOpenSSLHeaders_FOUND ON)
    return()
endif()

set(WrapOpenSSLHeaders_FOUND OFF)

# When cross-compiling (to Android for example), we need to add the OPENSSL_ROOT_DIR as a root path,
# otherwise the value would just be appended to the sysroot, which is wrong.
if(OPENSSL_ROOT_DIR)
    set(__find_wrap_openssl_headers_backup_root_dir "${CMAKE_FIND_ROOT_PATH}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${OPENSSL_ROOT_DIR}")
endif()

find_package(OpenSSL ${WrapOpenSSLHeaders_FIND_VERSION})

if(OPENSSL_ROOT_DIR)
    set(CMAKE_FIND_ROOT_PATH "${__find_wrap_openssl_headers_backup_root_dir}")
endif()

# We are interested only in include headers. The libraries might be missing, so we can't check the
# _FOUND variable.
if(OPENSSL_INCLUDE_DIR)
    set(WrapOpenSSLHeaders_FOUND ON)

    add_library(WrapOpenSSLHeaders::WrapOpenSSLHeaders INTERFACE IMPORTED)
    target_include_directories(WrapOpenSSLHeaders::WrapOpenSSLHeaders INTERFACE
        ${OPENSSL_INCLUDE_DIR})

    set_target_properties(WrapOpenSSLHeaders::WrapOpenSSLHeaders PROPERTIES
        _qt_is_nolink_target TRUE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapOpenSSLHeaders
    REQUIRED_VARS
        OPENSSL_INCLUDE_DIR
    VERSION_VAR
        OPENSSL_VERSION
)
