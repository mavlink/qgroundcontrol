# Target platform of the GStreamer modules. A platform's SDK discovery is _qgc_gst_discover_<platform>_sdk
# (platform/*.cmake) and, on desktop, its installer qgc_gst_install_<platform>_sdk (platform/*.cmake too, run by
# Install.cmake).
#
# A discovery function resolves the SDK into GStreamer_ROOT_DIR (and GStreamer_AUTO_DOWNLOADED when it downloaded it),
# its GSTREAMER_{LIB,PLUGIN,INCLUDE,PLUGIN_SCANNER,GIO_MODULE}_PATH layout (qgc_gst_create_layout_target) and its
# pkg-config view (qgc_gst_apply_pkgconfig_env), sets GStreamer_USE_FRAMEWORK for a macOS GStreamer.framework, and
# returns with _qgc_gst_return_discovery().

include_guard(GLOBAL)

# _qgc_gst_check_args(<function> [<keyword>...] [ALLOW_EMPTY <keyword>...])
# Fails configure, naming <function>, when the caller's cmake_parse_arguments(... ARG ...) result has unknown arguments,
# a keyword given without a value (unless listed after ALLOW_EMPTY), or an empty value for one of the <keyword>s.
function(_qgc_gst_check_args fn)
    cmake_parse_arguments(PARSE_ARGV 1 _check "" "" "ALLOW_EMPTY")
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "${fn}: unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    set(_missing ${ARG_KEYWORDS_MISSING_VALUES})
    if(_missing AND _check_ALLOW_EMPTY)
        list(REMOVE_ITEM _missing ${_check_ALLOW_EMPTY})
    endif()
    if(_missing)
        message(FATAL_ERROR "${fn}: missing values for: ${_missing}")
    endif()
    foreach(_keyword IN LISTS _check_UNPARSED_ARGUMENTS)
        if("${ARG_${_keyword}}" STREQUAL "")
            message(FATAL_ERROR "${fn}: ${_keyword} is required")
        endif()
    endforeach()
endfunction()

# qgc_gst_platform(<prefix>)
# Sets, in the caller's scope:
#   <prefix>              the target platform: windows, linux, android, macos or ios
#   <prefix>_VERSION_KEY  gstreamer.version.<key> in build-config.json: the SDK version, or on Linux the floor
#   <prefix>_PLUGIN_KEY   gstreamer.plugins.<key> list in build-config.json added to the common plugins
# Fails configure on any other platform.
# cmake-lint: disable=C0103,E1121
function(qgc_gst_platform prefix)
    if(ANDROID)
        set(${prefix} android)
        set(${prefix}_VERSION_KEY android)
        set(${prefix}_PLUGIN_KEY android)
    elseif(IOS)
        set(${prefix} ios)
        set(${prefix}_VERSION_KEY ios)
        set(${prefix}_PLUGIN_KEY apple)
    elseif(WIN32)
        set(${prefix} windows)
        set(${prefix}_VERSION_KEY windows)
        set(${prefix}_PLUGIN_KEY windows)
    elseif(MACOS)
        set(${prefix} macos)
        set(${prefix}_VERSION_KEY macos)
        set(${prefix}_PLUGIN_KEY apple)
    elseif(LINUX)
        set(${prefix} linux)
        set(${prefix}_VERSION_KEY minimum)
        set(${prefix}_PLUGIN_KEY linux)
    else()
        message(FATAL_ERROR "GStreamer: unsupported target platform (CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME})")
    endif()
    return(PROPAGATE ${prefix} ${prefix}_VERSION_KEY ${prefix}_PLUGIN_KEY)
endfunction()

# _qgc_gst_require_bundled_version(<platform> <version> <floor>)
# Fails configure when <platform> uses an official SDK (every platform but Linux, whose system GStreamer has its own
# floor) and <version> is older than <floor>, gstreamer.version.bundled_minimum: discovery reads the SDK layouts from
# that release on (the Windows installers' names, iOS's merged framework).
function(_qgc_gst_require_bundled_version platform version floor)
    if(platform STREQUAL "linux" OR version VERSION_GREATER_EQUAL floor)
        return()
    endif()
    message(
        FATAL_ERROR
            "GStreamer: the ${platform} SDK must be ${floor} or later (gstreamer.version.bundled_minimum), but "
            "${version} was requested. Set QGC_GST_VERSION, or gstreamer.version.${platform} in "
            ".github/build-config.json, to ${floor} or later."
    )
endfunction()

# _qgc_gst_return_discovery()
# Returns from a discovery function, setting every discovery output in the caller's scope as the function left it.
macro(_qgc_gst_return_discovery)
    return(
        PROPAGATE
        GStreamer_ROOT_DIR
        GStreamer_AUTO_DOWNLOADED
        GStreamer_USE_FRAMEWORK
        GSTREAMER_LIB_PATH
        GSTREAMER_PLUGIN_PATH
        GSTREAMER_INCLUDE_PATH
        GSTREAMER_PLUGIN_SCANNER_PATH
        GSTREAMER_GIO_MODULE_PATH
        GSTREAMER_FRAMEWORK_PATH
        GSTREAMER_XCFRAMEWORK_LIB
        GStreamer_IOS_CA_BUNDLE
        PKG_CONFIG_EXECUTABLE
        PKG_CONFIG_ARGN
        PKG_CONFIG_USE_CMAKE_PREFIX_PATH
    )
endmacro()

# _qgc_gst_require_no_spaces(<what> <path>)
# Fails configure when <path> contains a space: pkg-config prints paths unquoted, so FindPkgConfig would split them. iOS
# reads its SDK without pkg-config, so any path does there.
function(_qgc_gst_require_no_spaces what path)
    if(IOS OR NOT path MATCHES " ")
        return()
    endif()
    message(
        FATAL_ERROR
            "GStreamer: ${what} '${path}' contains a space, which pkg-config cannot pass on: it prints paths "
            "unquoted. Use an SDK in a path without spaces (e.g. C:/gstreamer/1.0/msvc_x86_64, where "
            "tools/setup/install_dependencies installs it) and point GStreamer_ROOT_DIR at it, or unset "
            "GStreamer_ROOT_DIR to download one."
    )
endfunction()

# _qgc_gst_space_free_download_cache()
# Discovery calls this just before it downloads an SDK to CPM_SOURCE_CACHE, which defaults to the source tree.
# pkg-config needs the SDK's path without spaces (_qgc_gst_require_no_spaces), so when CPM_SOURCE_CACHE has one, a
# Windows host sets CPM_SOURCE_CACHE in the caller's scope to its 8.3 short path. Configure fails on other hosts, or
# when that short path still has a space.
function(_qgc_gst_space_free_download_cache)
    if(NOT CPM_SOURCE_CACHE MATCHES " ")
        return()
    endif()
    set(_reason "")
    if(CMAKE_HOST_WIN32)
        file(MAKE_DIRECTORY "${CPM_SOURCE_CACHE}")
        _qgc_gst_windows_short_path("${CPM_SOURCE_CACHE}" _short)
        if(_short
           AND NOT _short MATCHES " "
           AND IS_DIRECTORY "${_short}"
        )
            message(STATUS "GStreamer: downloading the SDK to ${_short}, the short path of ${CPM_SOURCE_CACHE}")
            set(CPM_SOURCE_CACHE
                "${_short}"
                PARENT_SCOPE
            )
            return()
        endif()
        set(_reason " and no 8.3 short path without one (its volume may not create short names)")
    endif()
    message(
        FATAL_ERROR
            "GStreamer: CPM_SOURCE_CACHE '${CPM_SOURCE_CACHE}', where the GStreamer SDK is downloaded, contains a "
            "space${_reason}, which pkg-config cannot pass on: it prints paths unquoted. Set CPM_SOURCE_CACHE (as a "
            "CMake or environment variable) to a directory without spaces, e.g. C:/cpm-cache or ~/.cache/CPM, or "
            "point GStreamer_ROOT_DIR at an SDK in such a path."
    )
endfunction()

# _qgc_gst_windows_short_path(<path> <out_var>)
# Sets <out_var> to the 8.3 short form of the existing <path>, with forward slashes, as cmd's %~s modifier prints it: a
# component without a short name keeps its long one. Sets "" when cmd fails.
function(_qgc_gst_windows_short_path path out_var)
    cmake_path(NATIVE_PATH path _native)
    # execute_process quotes arguments for the C runtime, whose escaped quotes cmd does not understand. cmd expands the
    # command from the environment instead, so the path keeps its plain quotes.
    set(ENV{QGC_GST_SHORT_PATH_COMMAND} "for %I in (\"${_native}\") do @echo %~sI")
    execute_process(
        COMMAND cmd /d /v:off /c %QGC_GST_SHORT_PATH_COMMAND%
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _short
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    unset(ENV{QGC_GST_SHORT_PATH_COMMAND})
    if(_result EQUAL 0 AND NOT _short STREQUAL "")
        cmake_path(SET _short "${_short}")
    else()
        set(_short "")
    endif()
    set(${out_var}
        "${_short}"
        PARENT_SCOPE
    )
endfunction()
