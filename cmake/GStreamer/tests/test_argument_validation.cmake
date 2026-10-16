cmake_minimum_required(VERSION 3.25)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")

if(TEST_CASE STREQUAL "acquire-extractor")
    include(Download)
    qgc_gst_acquire_sdk(
        PLATFORM
        macos
        VERSION
        1.28.4
        CACHE_SUBDIR
        gstreamer-test
        ARCHIVES
        runtime.pkg
        CONTENT
        sdk
        EXTRACT_FUNCTION
        _qgc_no_such_extractor
        CONTENT_DIR_OUT
        _content_dir
    )
elseif(TEST_CASE STREQUAL "layout-unknown")
    include(Layout)
    gstreamer_create_layout_target(SDK_ROOT "${CMAKE_CURRENT_BINARY_DIR}" UNKNOWN_ARGUMENT value)
elseif(TEST_CASE STREQUAL "pkgconfig-unknown")
    include(PkgConfig)
    gstreamer_apply_pkgconfig_env(UNKNOWN_ARGUMENT value MODE SDK LIBDIR "${CMAKE_CURRENT_BINARY_DIR}")
elseif(TEST_CASE STREQUAL "probe-unknown")
    include(Probe)
    qgc_check_gst_header(
        VAR
        _result
        HEADER
        gst/gst.h
        SYMBOL
        gst_init
        UNKNOWN_ARGUMENT
    )
elseif(DEFINED TEST_CASE)
    message(FATAL_ERROR "Unknown test case: ${TEST_CASE}")
endif()

if(DEFINED TEST_CASE)
    message(FATAL_ERROR "${TEST_CASE} unexpectedly succeeded")
endif()
