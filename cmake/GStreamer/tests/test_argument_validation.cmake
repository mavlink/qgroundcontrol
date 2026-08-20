cmake_minimum_required(VERSION 3.22)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")

if(TEST_CASE STREQUAL "download-delegates-required")
    include(Download)
    gstreamer_resilient_download(DESTINATION_DIR "${CMAKE_CURRENT_BINARY_DIR}" RESULT_VAR _result URLS
                                 "https://invalid.example.invalid/archive"
    )
elseif(TEST_CASE STREQUAL "download-result-required")
    include(Download)
    gstreamer_resilient_download(FILENAME archive DESTINATION_DIR "${CMAKE_CURRENT_BINARY_DIR}" URLS
                                 "https://invalid.example.invalid/archive"
    )
elseif(TEST_CASE STREQUAL "download-unknown")
    include(Download)
    gstreamer_resilient_download(UNKNOWN_ARGUMENT value)
elseif(TEST_CASE STREQUAL "secondary-archive-pair")
    include(Download)
    set(GStreamer_FIND_VERSION 1.28.4)
    gstreamer_resolve_or_download_sdk(
        PLATFORM
        macos
        CACHE_SUBDIR
        gstreamer-test
        FILENAME_PRIMARY
        runtime.pkg
        FILENAME_SECONDARY
        devel.pkg
        CACHE_DIR_OUT
        _cache_dir
        ARCHIVE_OUT
        _archive
    )
elseif(TEST_CASE STREQUAL "install-unknown")
    include(Install)
    gstreamer_install_plugins(
        SOURCE_DIR
        "${CMAKE_CURRENT_BINARY_DIR}"
        DEST_DIR
        lib
        EXTENSION
        so
        PREFIX
        libgst
        UNKNOWN_ARGUMENT
    )
elseif(TEST_CASE STREQUAL "layout-unknown")
    include(Layout)
    gstreamer_create_layout_target(SDK_ROOT "${CMAKE_CURRENT_BINARY_DIR}" TYPE FLAT UNKNOWN_ARGUMENT value)
elseif(TEST_CASE STREQUAL "pkgconfig-unknown")
    include(PkgConfig)
    gstreamer_apply_pkgconfig_env(UNKNOWN_ARGUMENT value MODE SDK LIBDIR "${CMAKE_CURRENT_BINARY_DIR}")
elseif(TEST_CASE STREQUAL "plugin-policy-unknown")
    include(PluginPolicy)
    gstreamer_plugin_satisfy_sets(PLUGIN coreelements OUT_VAR _sets UNKNOWN_ARGUMENT)
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
