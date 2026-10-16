# Target platform of the GStreamer modules. A platform's SDK discovery is _qgc_gst_discover_<platform>_sdk
# (platform/*.cmake) and, on desktop, its installer gstreamer_install_<platform>_sdk (Install.cmake).
#
# A discovery function resolves the SDK into GStreamer_ROOT_DIR (and GStreamer_AUTO_DOWNLOADED when it downloaded it),
# its GSTREAMER_{LIB,PLUGIN,INCLUDE}_PATH layout (gstreamer_create_layout_target) and its pkg-config view
# (gstreamer_apply_pkgconfig_env), overrides the GStreamer_USE_* layout preferences when the SDK it found differs, and
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

# _qgc_gst_return_discovery()
# Returns from a discovery function, setting every discovery output in the caller's scope as the function left it.
macro(_qgc_gst_return_discovery)
    return(
        PROPAGATE
        GStreamer_ROOT_DIR
        GStreamer_AUTO_DOWNLOADED
        GStreamer_USE_FRAMEWORK
        GStreamer_USE_STATIC_LIBS
        GStreamer_USE_MERGED_STATIC
        GSTREAMER_LIB_PATH
        GSTREAMER_PLUGIN_PATH
        GSTREAMER_INCLUDE_PATH
        GSTREAMER_FRAMEWORK_PATH
        GSTREAMER_XCFRAMEWORK_LIB
        GStreamer_IOS_CA_BUNDLE
        PKG_CONFIG_EXECUTABLE
        PKG_CONFIG_ARGN
        PKG_CONFIG_USE_CMAKE_PREFIX_PATH
    )
endmacro()
