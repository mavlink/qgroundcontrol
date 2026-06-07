# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Computes which sanitizer options should be set based on features evaluated in qtbase.
# Sets ECM_ENABLE_SANITIZERS with those options in the function calling scope.
function(qt_internal_set_up_sanitizer_options)
    set(ECM_ENABLE_SANITIZERS "" CACHE STRING "Enable sanitizers")
    set_property(CACHE ECM_ENABLE_SANITIZERS PROPERTY STRINGS
        "address;memory;thread;undefined;fuzzer;fuzzer-no-link")

    # If QT_FEATURE_sanitize_foo was enabled, make sure to set the appropriate
    # ECM_ENABLE_SANITIZERS value.
    set(enabled_sanitizer_features "")
    foreach(sanitizer_type address memory thread undefined)
        if(QT_FEATURE_sanitize_${sanitizer_type})
            list(APPEND enabled_sanitizer_features "${sanitizer_type}")
        endif()
    endforeach()

    # There's a mismatch between fuzzer-no-link ECM option and fuzzer_no_link Qt feature.
    if(QT_FEATURE_sanitize_fuzzer_no_link)
        list(APPEND enabled_sanitizer_features "fuzzer-no-link")
    endif()

    if(enabled_sanitizer_features)
        set(ECM_ENABLE_SANITIZERS "${enabled_sanitizer_features}" PARENT_SCOPE)
    endif()
endfunction()

# This function clears the previously set sanitizer flags from CMAKE_<C|CXX>_FLAGS
function(qt_internal_skip_sanitizer)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
        CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
        CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        foreach(sanitizer ${ECM_ENABLE_SANITIZERS})
            string(TOLOWER "${sanitizer}" sanitizer)
            enable_sanitizer_flags("${sanitizer}")
            qt_internal_remove_compiler_flags(${XSAN_COMPILE_FLAGS})
        endforeach()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)
    endif()
endfunction()

# This function disables the sanitizer library linking to all targets created in a subdirectory
# where the function is called. Note that the function should be called after all involved targets
# are created, to make sure they are collected by the function.
function(qt_internal_skip_linking_sanitizer)
    _qt_internal_collect_buildsystem_targets(all_targets "${CMAKE_CURRENT_SOURCE_DIR}"
        INCLUDE
            STATIC_LIBRARY
            MODULE_LIBRARY
            SHARED_LIBRARY
            OBJECT_LIBRARY
            EXECUTABLE
    )
    foreach(t IN LISTS all_targets)
        set_property(TARGET ${t} PROPERTY SKIP_SANITIZER TRUE)
    endforeach()
endfunction()
