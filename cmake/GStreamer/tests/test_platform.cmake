cmake_minimum_required(VERSION 3.25)
# The Orchestrator and the install dispatcher call each platform's helpers by name, and the platform's version variable
# and plugin key must exist: check every platform, including the ones this host cannot build.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Platform)
include(Install)
foreach(_file IN ITEMS Apple Windows Linux Android MacOS IOS)
    include("${CMAKE_CURRENT_LIST_DIR}/../platform/${_file}.cmake")
endforeach()
file(READ "${CMAKE_CURRENT_LIST_DIR}/../../BuildConfig.cmake" _build_config_cmake)
file(READ "${CMAKE_CURRENT_LIST_DIR}/../../../.github/build-config.json" _build_config_json)

# platform variable|platform|installs an SDK
foreach(_case IN ITEMS "ANDROID|android|FALSE" "IOS|ios|FALSE" "WIN32|windows|TRUE" "MACOS|macos|TRUE"
                       "LINUX|linux|TRUE"
)
    string(REPLACE "|" ";" _parts "${_case}")
    list(GET _parts 0 _variable)
    list(GET _parts 1 _expected)
    list(GET _parts 2 _installs)
    foreach(_platform_variable IN ITEMS ANDROID IOS WIN32 MACOS LINUX)
        set(${_platform_variable} FALSE)
    endforeach()
    set(${_variable} TRUE)

    qgc_gst_platform(_platform)
    qgc_test_assert_streq("${_variable} platform" "${_expected}" "${_platform}")
    if(NOT COMMAND _qgc_gst_discover_${_platform}_sdk)
        message(FATAL_ERROR "ASSERT [${_platform} discovery]: _qgc_gst_discover_${_platform}_sdk is not defined")
    endif()
    if(_installs AND NOT COMMAND gstreamer_install_${_platform}_sdk)
        message(FATAL_ERROR "ASSERT [${_platform} install]: gstreamer_install_${_platform}_sdk is not defined")
    endif()
    if(NOT _build_config_cmake MATCHES "qgc_config_get_value\\(${_platform_VERSION_VAR} \"gstreamer\\.version\\.")
        message(FATAL_ERROR "ASSERT [${_platform} version]: BuildConfig.cmake sets no ${_platform_VERSION_VAR}")
    endif()
    string(
        JSON
        _plugins
        ERROR_VARIABLE
        _error
        GET
        "${_build_config_json}"
        gstreamer
        plugins
        ${_platform_PLUGIN_KEY}
    )
    qgc_test_assert_streq("${_platform} plugin key ${_platform_PLUGIN_KEY}" NOTFOUND "${_error}")
endforeach()
qgc_test_pass("every platform's helpers, version variable and plugin key exist")
