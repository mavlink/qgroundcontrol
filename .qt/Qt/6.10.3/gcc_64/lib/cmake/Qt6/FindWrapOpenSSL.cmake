# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapOpenSSL::WrapOpenSSL)
    set(WrapOpenSSL_FOUND ON)
    return()
endif()

set(WrapOpenSSL_FOUND OFF)

# Reuse logic from the headers find script.
find_package(WrapOpenSSLHeaders ${WrapOpenSSL_FIND_VERSION} MODULE)

if(TARGET OpenSSL::SSL)
    if(WIN32)
        get_target_property(libType OpenSSL::Crypto TYPE)
        if(libType STREQUAL "ALIAS")
            get_target_property(writableLib OpenSSL::Crypto ALIASED_TARGET)
        else()
            set(writableLib OpenSSL::Crypto)
        endif()
        set_property(TARGET ${writableLib} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ws2_32 crypt32)
        unset(libType)
        unset(writableLib)
    endif()

    set(WrapOpenSSL_FOUND ON)

    add_library(WrapOpenSSL::WrapOpenSSL INTERFACE IMPORTED)
    target_link_libraries(WrapOpenSSL::WrapOpenSSL INTERFACE OpenSSL::SSL)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapOpenSSL
    REQUIRED_VARS
        OPENSSL_CRYPTO_LIBRARY
        OPENSSL_INCLUDE_DIR
    VERSION_VAR
        OPENSSL_VERSION
)
