# GStreamer SDK orchestration, included once by src/VideoManager/GStreamer/CMakeLists.txt: picks the
# requested version, discovers or downloads the target platform's SDK (Platform.cmake, platform/*.cmake), creates the
# GStreamer:: targets, checks the plugin set and records the resolved SDK (SdkState.cmake, GStreamer::Layout).
#
# Input: QGC_GST_COMPONENTS (components beyond the mandatory ones, Components.cmake) and the cache options
# GStreamer_ROOT_DIR, QGC_GST_VERSION, QGC_GST_USE_HOMEBREW (macOS) and PKG_CONFIG_ARGN.
# Sets, in the including directory, the discovery outputs (Platform.cmake; qgc_gst_restore_pkgconfig_env undoes their
# pkg-config environment) and GStreamer_VERSION, GSTREAMER_SELECTED_COMPONENTS, GSTREAMER_PLUGINS,
# GSTREAMER_STATIC_PLUGINS and G_IO_MODULES (mobile), GStreamer_<component>_FOUND (optional components) and
# QGC_GST_HAS_DMABUF (Linux).

# The helper modules, each described in its header, then each platform's discovery (platform/*.cmake).
foreach(
    _qgc_gst_module IN
    ITEMS
        Platform
        Components
        Layout
        Targets
        Probe
        PkgConfig
        Packages
        Acquire
        PluginPolicy
        Install
        Notice
        Mobile
        SdkState
        platform/Windows
        platform/Linux
        platform/Android
        platform/MacOS
        platform/IOS
)
    include("${CMAKE_CURRENT_LIST_DIR}/${_qgc_gst_module}.cmake")
endforeach()

qgc_gst_platform(_qgc_gst_platform)

# The version asked of the SDK: QGC_GST_VERSION when set, else the platform's gstreamer.version entry in
# .github/build-config.json (the minimum on Linux; the SDK to download or accept elsewhere).
if(QGC_GST_VERSION)
    # The SDK packages and checksum pins are named by the full release version.
    if(NOT QGC_GST_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
        message(
            FATAL_ERROR
                "GStreamer: QGC_GST_VERSION='${QGC_GST_VERSION}' is not a GStreamer release version. Set "
                "it to <major>.<minor>.<micro> (e.g. 1.28.4), or empty to use gstreamer.version in "
                ".github/build-config.json."
        )
    endif()
    set(QGC_GST_REQUESTED_VERSION "${QGC_GST_VERSION}")
else()
    qgc_config_read(QGC_GST_REQUESTED_VERSION gstreamer version ${_qgc_gst_platform_VERSION_KEY})
endif()
qgc_config_read(_qgc_gst_bundled_minimum gstreamer version bundled_minimum)
_qgc_gst_require_bundled_version(${_qgc_gst_platform} "${QGC_GST_REQUESTED_VERSION}" "${_qgc_gst_bundled_minimum}")

if(GStreamer_ROOT_DIR AND NOT EXISTS "${GStreamer_ROOT_DIR}")
    message(
        FATAL_ERROR "GStreamer: User-provided directory does not exist: ${GStreamer_ROOT_DIR}\n"
                    "Correct the path or unset GStreamer_ROOT_DIR to allow auto-download."
    )
endif()
if(GStreamer_ROOT_DIR)
    _qgc_gst_require_no_spaces(GStreamer_ROOT_DIR "${GStreamer_ROOT_DIR}")
endif()

# The SDK's linkage: the desktop SDKs are shared, Android's are static archives linked into GStreamer::mobile
# (platform/Android.cmake), and iOS links one merged static archive (platform/IOS.cmake). Discovery sets
# GStreamer_USE_FRAMEWORK for a macOS GStreamer.framework. CONTRACT: include() this file at directory scope, never
# inside a function: that flag and the other outputs are directory-scope normal variables.
set(GStreamer_USE_FRAMEWORK OFF)

# The cached PKG_CONFIG_ARGN is user input; discovery derives its flags into a normal variable.
set(PKG_CONFIG_ARGN
    ""
    CACHE STRING "Extra arguments for pkg-config"
)
_qgc_gst_drop_legacy_pkg_config_args()
_qgc_gst_select_sdk_root(
    GStreamer_ROOT_DIR GStreamer_AUTO_DOWNLOADED "${GStreamer_ROOT_DIR}" "${QGC_GST_REQUESTED_VERSION}"
)

cmake_language(CALL _qgc_gst_discover_${_qgc_gst_platform}_sdk)
# Discovery downloads only to a CPM_SOURCE_CACHE without spaces (_qgc_gst_space_free_download_cache); check the SDK it
# settled on.
_qgc_gst_require_no_spaces("The GStreamer SDK" "${GStreamer_ROOT_DIR}")

qgc_gst_select_components(GSTREAMER_SELECTED_COMPONENTS ${QGC_GST_COMPONENTS})

# The plugin list lives in .github/build-config.json; the alternate groups of PluginPolicy.cmake (e.g. the 1.22
# videoconvertscale fusion of videoconvert+videoscale) apply to the missing-plugin check below.
qgc_gst_plugins_for("${_qgc_gst_platform_PLUGIN_KEY}" GSTREAMER_PLUGINS)

if(_qgc_gst_platform_STATIC_PLUGINS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND GSTREAMER_PLUGINS ${GSTREAMER_MOCK_SERVER_DEBUG_PLUGINS})
    list(REMOVE_DUPLICATES GSTREAMER_PLUGINS)
endif()

# Mobile apps register the OpenSSL GIO module (TLS) statically along with their plugins; on iOS the merged
# archive carries it.
if(_qgc_gst_platform_STATIC_PLUGINS)
    set(G_IO_MODULES openssl)
endif()

# Discovery derives its pkg-config flags from the root and the linkage, so only the user's cached flags join the
# fingerprint (_qgc_gst_refresh_sdk_cache).
set(_qgc_gst_fingerprint_pkg_config "")
if(_qgc_gst_platform_PKG_CONFIG)
    find_package(PkgConfig REQUIRED QUIET)
    set(_qgc_gst_fingerprint_pkg_config "${PKG_CONFIG_EXECUTABLE}|$CACHE{PKG_CONFIG_ARGN}")
endif()
set(_qgc_gst_linkage ${_qgc_gst_platform_LINKAGE})
# A normal variable: the GStreamer subtree's later pkg-config probes (HwBuffers) need the same flags.
if(_qgc_gst_linkage STREQUAL "static" AND NOT "--static" IN_LIST PKG_CONFIG_ARGN)
    list(APPEND PKG_CONFIG_ARGN --static)
endif()
_qgc_gst_header_version("${GSTREAMER_INCLUDE_PATH}" _qgc_gst_sdk_version)
set(_qgc_gst_fingerprint "${GStreamer_ROOT_DIR}|${QGC_GST_REQUESTED_VERSION}|${_qgc_gst_sdk_version}")
string(APPEND _qgc_gst_fingerprint "|${_qgc_gst_linkage}|${_qgc_gst_fingerprint_pkg_config}")
_qgc_gst_refresh_sdk_cache("${_qgc_gst_fingerprint}")

cmake_language(CALL ${_qgc_gst_platform_TARGETS})

# The sources include the mandatory components' headers unconditionally, so fail here rather than at compile time.
set(_qgc_gst_missing_mandatory "")
foreach(_qgc_gst_component IN LISTS GSTREAMER_MANDATORY_COMPONENTS)
    if(NOT TARGET GStreamer::${_qgc_gst_component})
        qgc_gst_component_module(${_qgc_gst_component} _qgc_gst_module)
        list(APPEND _qgc_gst_missing_mandatory "${_qgc_gst_module}")
    endif()
endforeach()
if(_qgc_gst_missing_mandatory)
    string(REPLACE ";" ", " _qgc_gst_missing_mandatory "${_qgc_gst_missing_mandatory}")
    message(
        FATAL_ERROR
            "GStreamer: mandatory pkg-config modules not found for the SDK at ${GStreamer_ROOT_DIR}: "
            "${_qgc_gst_missing_mandatory}\nInstall the GStreamer development packages (gst-plugins-base) "
            "or point GStreamer_ROOT_DIR at a complete SDK."
    )
endif()
# The requested components found; the optional ones' pkg-config probes are QUIET (Targets.cmake).
set(_qgc_gst_missing_components "")
foreach(_qgc_gst_component IN LISTS QGC_GST_COMPONENTS)
    if(TARGET GStreamer::${_qgc_gst_component})
        set(GStreamer_${_qgc_gst_component}_FOUND TRUE)
    else()
        list(APPEND _qgc_gst_missing_components "${_qgc_gst_component}")
    endif()
endforeach()
if(_qgc_gst_missing_components)
    list(JOIN _qgc_gst_missing_components ", " _qgc_gst_missing_components)
    message(STATUS "QGCGStreamer: optional components not found: ${_qgc_gst_missing_components}")
endif()

# Mobile apps register their plugins in GStreamer::mobile's generated gst_init_static_plugins() (Mobile.cmake).
if(_qgc_gst_platform_STATIC_PLUGINS)
    target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_STATIC_BUILD)
endif()

# Plugins the app can use: mobile layouts register GSTREAMER_STATIC_PLUGINS, which their backends resolve from the
# SDK's plugin archives while creating GStreamer::mobile; desktop layouts load the plugin files at runtime, and
# Install.cmake verifies what desktop installs bundle.
set(_gst_static_args "")
if(_qgc_gst_platform_STATIC_PLUGINS)
    set(_gst_available_plugins ${GSTREAMER_STATIC_PLUGINS})
    set(_gst_plugin_source "the static SDK at ${GStreamer_ROOT_DIR}")
    set(_gst_static_args STATIC)
else()
    qgc_gst_scan_plugin_basenames(_gst_available_plugins "${GSTREAMER_PLUGIN_PATH}")
    set(_gst_plugin_source "${GSTREAMER_PLUGIN_PATH}")
endif()
# Every plugin the SDK or system ships, used by this build or not: the plugin archives of static layouts (iOS: the
# plugins registered from its merged archive), else the plugin files.
if(_qgc_gst_platform_PLUGIN_ARCHIVES)
    qgc_gst_scan_plugin_basenames(_gst_shipped_plugins "${GSTREAMER_PLUGIN_PATH}" STATIC)
else()
    set(_gst_shipped_plugins ${_gst_available_plugins})
endif()
qgc_gst_check_plugins(
    REQUESTED ${GSTREAMER_PLUGINS}
    AVAILABLE ${_gst_available_plugins}
    SHIPPED ${_gst_shipped_plugins}
    SOURCE "${_gst_plugin_source}" ${_gst_static_args}
)

# GstVideoOrientationMeta is officially in 1.26+, but bundled iOS/Android SDKs sometimes strip it, so test the header.
# A sizeof() needs no archive, so iOS can probe GStreamer::GStreamer, which carries headers only.
qgc_gst_check_header(
    VAR QGC_GST_HAS_VIDEO_ORIENTATION_META
    HEADER gst/video/gstvideometa.h
    SYMBOL "sizeof(GstVideoOrientationMeta)"
)
if(QGC_GST_HAS_VIDEO_ORIENTATION_META)
    target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_HAS_GST_VIDEO_ORIENTATION_META=1)
endif()

# Zero-copy DMABuf GPU path probe: HwBuffers/CMakeLists.txt builds the DMABuf path when QGC_GST_HAS_DMABUF is set. It
# needs gst-allocators, which the Linux branch of src/.../GStreamer/CMakeLists.txt requests (the Allocators component).
if(LINUX)
    qgc_gst_check_header(
        VAR QGC_GST_HAS_DMABUF
        HEADER gst/allocators/gstdmabuf.h
        SYMBOL gst_is_dmabuf_memory
    )
endif()

set(_qgc_gst_path "${_qgc_gst_platform_PATH_LABEL}")
if(GStreamer_USE_FRAMEWORK)
    set(_qgc_gst_path "framework")
endif()
# The requested plugins this build ships, as the status line counts and the notice lists them.
set(_qgc_gst_shipped_plugins "")
foreach(_p IN LISTS GSTREAMER_PLUGINS)
    if(_p IN_LIST _gst_available_plugins)
        list(APPEND _qgc_gst_shipped_plugins "${_p}")
    endif()
endforeach()
# The plugin facts of this configure, which the install rules (Install.cmake) read instead of scanning the plugin
# directory again.
qgc_gst_layout_set(AVAILABLE_PLUGINS "${_gst_available_plugins}")
qgc_gst_layout_set(BUNDLED_PLUGINS "${_qgc_gst_shipped_plugins}")
list(LENGTH GSTREAMER_PLUGINS _qgc_gst_plugin_total)
list(LENGTH _qgc_gst_shipped_plugins _qgc_gst_plugin_count)
message(
    STATUS "QGCGStreamer: version=${GStreamer_VERSION} path=${_qgc_gst_path} root=${GStreamer_ROOT_DIR} "
           "plugins=${_qgc_gst_plugin_count}/${_qgc_gst_plugin_total} auto_downloaded=${GStreamer_AUTO_DOWNLOADED}"
)

# Post-configure tooling (tools/ci/packaging/verify_executable.py, generate_cpm_sbom.py) and PrintSummary read the
# QGC_GST_RESOLVED_* entries from the cache.
_qgc_gst_record_resolved_sdk(
    "${GStreamer_ROOT_DIR}" "${QGC_GST_REQUESTED_VERSION}" "${GStreamer_AUTO_DOWNLOADED}" "${GStreamer_VERSION}"
)

# The app ships the LGPL text and a notice naming this GStreamer and its sources: Android stages them as APK assets,
# iOS as bundle resources (src/VideoManager/GStreamer/CMakeLists.txt) and desktop installs copy them from
# GStreamer::Layout's NOTICE_DIR (Install.cmake).
if(_qgc_gst_platform_NOTICE_ASSETS)
    set(_qgc_gst_notice_dir "${QGC_ANDROID_EXTRA_ASSETS_DIR}/licenses/gstreamer")
else()
    set(_qgc_gst_notice_dir "${CMAKE_BINARY_DIR}/gstreamer-notice")
endif()
qgc_gst_write_notice("${_qgc_gst_notice_dir}" PLUGINS ${_qgc_gst_shipped_plugins})
qgc_gst_layout_set(NOTICE_DIR "${_qgc_gst_notice_dir}")
