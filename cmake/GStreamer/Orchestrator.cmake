# GStreamer SDK orchestration, included once by src/VideoManager/VideoReceiver/GStreamer/CMakeLists.txt: picks the
# requested version, discovers or downloads the target platform's SDK (Platform.cmake, platform/*.cmake), creates the
# GStreamer:: targets, checks the plugin set and records the resolved SDK (SdkState.cmake, GStreamer::Layout).
#
# Input: QGC_GST_COMPONENTS (components beyond the mandatory ones, Components.cmake) and the cache options
# GStreamer_ROOT_DIR, QGC_GST_VERSION, QGC_GST_USE_HOMEBREW (macOS), GStreamer_USE_{STATIC_LIBS,FRAMEWORK} and
# PKG_CONFIG_ARGN.
# Sets, in the including directory, the discovery outputs (Platform.cmake; qgc_gst_restore_pkgconfig_env undoes their
# pkg-config environment) and GStreamer_VERSION, GStreamer_USE_MERGED_STATIC, GSTREAMER_APIS, GSTREAMER_PLUGINS,
# GSTREAMER_STATIC_PLUGINS (static layouts), G_IO_MODULES (mobile), GStreamer_<component>_FOUND (optional components)
# and QGC_GST_HAS_DMABUF (Linux).

# The helper modules, each described in its header, then each platform's discovery (platform/*.cmake).
foreach(
    _qgc_gst_module IN
    ITEMS Platform
          Components
          Layout
          Targets
          Probe
          PkgConfig
          Download
          PluginPolicy
          Install
          Notice
          StaticInit
          SdkState
          platform/Apple
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
        message(FATAL_ERROR "GStreamer: QGC_GST_VERSION='${QGC_GST_VERSION}' is not a GStreamer release version. Set "
                            "it to <major>.<minor>.<micro> (e.g. 1.28.4), or empty to use gstreamer.version in "
                            ".github/build-config.json."
        )
    endif()
    set(QGC_GST_REQUESTED_VERSION "${QGC_GST_VERSION}")
else()
    qgc_config_read(QGC_GST_REQUESTED_VERSION gstreamer version ${_qgc_gst_platform_VERSION_KEY})
endif()

if(GStreamer_ROOT_DIR AND NOT EXISTS "${GStreamer_ROOT_DIR}")
    message(FATAL_ERROR "GStreamer: User-provided directory does not exist: ${GStreamer_ROOT_DIR}\n"
                        "Correct the path or unset GStreamer_ROOT_DIR to allow auto-download."
    )
endif()

# Layout preferences; discovery overrides them with normal variables for the SDK it finds (a flat macOS SDK is not a
# framework, iOS links one merged archive). CONTRACT: include() this file at directory scope, never inside a function:
# these flags and the other outputs are directory-scope normal variables.
if(ANDROID OR IOS)
    set(_qgc_gst_static_default ON)
else()
    set(_qgc_gst_static_default OFF)
endif()
option(GStreamer_USE_STATIC_LIBS "Link the GStreamer SDK statically (required on Android and iOS)"
       ${_qgc_gst_static_default}
)
if(APPLE)
    set(_qgc_gst_framework_default ON)
else()
    set(_qgc_gst_framework_default OFF)
endif()
option(GStreamer_USE_FRAMEWORK "macOS: use the GStreamer SDK as GStreamer.framework" ${_qgc_gst_framework_default})

# iOS discovery switches to its single merged static archive (platform/IOS.cmake).
set(GStreamer_USE_MERGED_STATIC OFF)

# The cached PKG_CONFIG_ARGN is user input; discovery derives its flags into a normal variable.
set(PKG_CONFIG_ARGN
    ""
    CACHE STRING "Extra arguments for pkg-config"
)
_qgc_gst_drop_legacy_pkg_config_args()
_qgc_gst_select_sdk_root(GStreamer_ROOT_DIR GStreamer_AUTO_DOWNLOADED "${GStreamer_ROOT_DIR}"
                         "${QGC_GST_REQUESTED_VERSION}"
)

cmake_language(CALL _qgc_gst_discover_${_qgc_gst_platform}_sdk)

# Post-dispatch invariant: at most one of the three layout modes may be ON.
set(_qgc_gst_active_modes)
if(GStreamer_USE_MERGED_STATIC)
    list(APPEND _qgc_gst_active_modes GStreamer_USE_MERGED_STATIC)
endif()
if(GStreamer_USE_FRAMEWORK)
    list(APPEND _qgc_gst_active_modes GStreamer_USE_FRAMEWORK)
endif()
if(GStreamer_USE_STATIC_LIBS)
    list(APPEND _qgc_gst_active_modes GStreamer_USE_STATIC_LIBS)
endif()
list(LENGTH _qgc_gst_active_modes _qgc_gst_active_count)
if(_qgc_gst_active_count GREATER 1)
    message(FATAL_ERROR "GStreamer: conflicting layout modes — only one of GStreamer_USE_MERGED_STATIC, "
                        "GStreamer_USE_FRAMEWORK, GStreamer_USE_STATIC_LIBS may be ON simultaneously. "
                        "Active: ${_qgc_gst_active_modes}"
    )
endif()
unset(_qgc_gst_active_modes)
unset(_qgc_gst_active_count)

gstreamer_select_components(_qgc_gst_components ${QGC_GST_COMPONENTS})
set(GSTREAMER_APIS "")
foreach(_qgc_gst_component IN LISTS _qgc_gst_components)
    gstreamer_component_names(${_qgc_gst_component} _qgc_gst_api _)
    list(APPEND GSTREAMER_APIS ${_qgc_gst_api})
endforeach()

# The plugin list lives in .github/build-config.json; the alternate groups of PluginPolicy.cmake (e.g. the 1.22
# videoconvertscale fusion of videoconvert+videoscale) apply to the missing-plugin check below.
gstreamer_plugins_for("${_qgc_gst_platform_PLUGIN_KEY}" GSTREAMER_PLUGINS)

if((ANDROID OR IOS) AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND GSTREAMER_PLUGINS ${GSTREAMER_MOCK_SERVER_DEBUG_PLUGINS})
    list(REMOVE_DUPLICATES GSTREAMER_PLUGINS)
endif()
if(IOS)
    list(REMOVE_ITEM GSTREAMER_PLUGINS ${GSTREAMER_XCFRAMEWORK_SKIP_PLUGINS})
endif()

# Mobile apps register the OpenSSL GIO module (TLS) statically along with their plugins; on iOS the merged
# archive carries it.
if(ANDROID OR IOS)
    set(G_IO_MODULES openssl)
endif()

# Discovery derives its pkg-config flags from the root and the linkage, so only the user's cached flags join the
# fingerprint (_qgc_gst_refresh_sdk_cache).
set(_qgc_gst_fingerprint_pkg_config "")
if(NOT GStreamer_USE_MERGED_STATIC)
    find_package(PkgConfig REQUIRED QUIET)
    set(_qgc_gst_fingerprint_pkg_config "${PKG_CONFIG_EXECUTABLE}|$CACHE{PKG_CONFIG_ARGN}")
endif()
if(GStreamer_USE_STATIC_LIBS)
    set(_qgc_gst_linkage static)
else()
    set(_qgc_gst_linkage shared)
endif()
_qgc_gst_header_version("${GSTREAMER_INCLUDE_PATH}" _qgc_gst_sdk_version)
set(_qgc_gst_fingerprint "${GStreamer_ROOT_DIR}|${QGC_GST_REQUESTED_VERSION}|${_qgc_gst_sdk_version}")
string(APPEND _qgc_gst_fingerprint "|${_qgc_gst_linkage}|${_qgc_gst_fingerprint_pkg_config}")
_qgc_gst_refresh_sdk_cache("${_qgc_gst_fingerprint}")

if(GStreamer_USE_MERGED_STATIC)
    _qgc_gst_create_ios_targets()
else()
    _qgc_gst_create_sdk_targets()
endif()

# The sources include the mandatory components' headers unconditionally, so fail here rather than at compile time.
set(_qgc_gst_missing_mandatory "")
foreach(_qgc_gst_component IN LISTS GSTREAMER_MANDATORY_COMPONENTS)
    gstreamer_component_names(${_qgc_gst_component} _qgc_gst_api _qgc_gst_pc)
    if(NOT TARGET GStreamer::${_qgc_gst_api})
        list(APPEND _qgc_gst_missing_mandatory "${_qgc_gst_pc}")
    endif()
endforeach()
if(_qgc_gst_missing_mandatory)
    string(REPLACE ";" ", " _qgc_gst_missing_mandatory "${_qgc_gst_missing_mandatory}")
    message(FATAL_ERROR "GStreamer: mandatory pkg-config modules not found for the SDK at ${GStreamer_ROOT_DIR}: "
                        "${_qgc_gst_missing_mandatory}\nInstall the GStreamer development packages (gst-plugins-base) "
                        "or point GStreamer_ROOT_DIR at a complete SDK."
    )
endif()
# The requested components found; the optional ones' pkg-config probes are QUIET (Targets.cmake).
set(_qgc_gst_missing_components "")
foreach(_qgc_gst_component IN LISTS QGC_GST_COMPONENTS)
    gstreamer_component_names(${_qgc_gst_component} _qgc_gst_api _)
    if(TARGET GStreamer::${_qgc_gst_api})
        set(GStreamer_${_qgc_gst_component}_FOUND TRUE)
    else()
        list(APPEND _qgc_gst_missing_components "${_qgc_gst_component}")
    endif()
endforeach()
if(_qgc_gst_missing_components)
    list(JOIN _qgc_gst_missing_components ", " _qgc_gst_missing_components)
    message(STATUS "QGCGStreamer: optional components not found: ${_qgc_gst_missing_components}")
endif()

if(GStreamer_USE_STATIC_LIBS)
    target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_STATIC_BUILD)
    if(ANDROID)
        # Cerbero gstreamer-1.0.mk linker flags: --export-dynamic re-exports symbols for dlopen'd GIO
        # modules/plugins and -Bsymbolic keeps FFmpeg's page-relative relocations valid (both every ABI);
        # -z text hard-errors residual text relocations (armv7/x86, from platform/Android.cmake).
        _qgc_gst_android_abi_info("${CMAKE_ANDROID_ARCH_ABI}" _GST_ABI)
        set(_gst_android_link_opts "-Wl,--export-dynamic" "-Wl,-Bsymbolic")
        if(_GST_ABI_NEEDS_TEXTREL_ERROR)
            list(APPEND _gst_android_link_opts "-Wl,-z,text")
        endif()
        target_link_options(GStreamer::GStreamer INTERFACE ${_gst_android_link_opts})
    endif()
endif()

# Plugins the app can use: static layouts register GSTREAMER_STATIC_PLUGINS in a generated gst_init_static_plugins()
# (the Android and iOS backends resolve it from the SDK's plugin archives while creating their targets); shared layouts
# load the plugin files at runtime, and Install.cmake verifies what desktop installs bundle.
if(GStreamer_USE_STATIC_LIBS OR GStreamer_USE_MERGED_STATIC)
    if(NOT ANDROID AND NOT IOS)
        # Desktop static layouts register every requested plugin that has a GStreamer::<plugin> target.
        set(GSTREAMER_STATIC_PLUGINS "")
        foreach(_gst_plugin IN LISTS GSTREAMER_PLUGINS)
            if(TARGET GStreamer::${_gst_plugin})
                list(APPEND GSTREAMER_STATIC_PLUGINS "${_gst_plugin}")
            endif()
        endforeach()
    endif()
    set(_gst_available_plugins ${GSTREAMER_STATIC_PLUGINS})
    set(_gst_plugin_source "the static SDK at ${GStreamer_ROOT_DIR}")
else()
    gstreamer_scan_plugin_basenames(_gst_available_plugins "${GSTREAMER_PLUGIN_PATH}")
    set(_gst_plugin_source "${GSTREAMER_PLUGIN_PATH}")
endif()
# Every plugin the SDK or system ships, used by this build or not: the plugin archives of static layouts (iOS: the
# plugins registered from its merged archive), else the plugin files.
if(GStreamer_USE_STATIC_LIBS)
    gstreamer_scan_plugin_basenames(_gst_shipped_plugins "${GSTREAMER_PLUGIN_PATH}" STATIC)
else()
    set(_gst_shipped_plugins ${_gst_available_plugins})
endif()
gstreamer_classify_missing_plugins(
    REQUESTED
    ${GSTREAMER_PLUGINS}
    AVAILABLE
    ${_gst_available_plugins}
    SHIPPED
    ${_gst_shipped_plugins}
    OUT_OPTIONAL
    _gst_missing_optional
    OUT_ABSENT
    _gst_absent_plugins
    OUT_UNUSABLE
    _gst_unusable_plugins
)
if(_gst_absent_plugins)
    list(JOIN _gst_absent_plugins ", " _gst_absent_plugins)
    message(
        WARNING "GStreamer: plugin allowlist names not found as plugins in ${_gst_plugin_source}: "
                "${_gst_absent_plugins}\nCheck them in gstreamer.plugins of .github/build-config.json (misspelled, or "
                "not built for this platform) or install the plugins; video features depending on them will not "
                "work at runtime."
    )
endif()
if(_gst_unusable_plugins)
    list(JOIN _gst_unusable_plugins ", " _gst_unusable_plugins)
    message(WARNING "GStreamer: requested plugins in ${_gst_plugin_source} that have no pkg-config module to link "
                    "them by: ${_gst_unusable_plugins}\nVideo features depending on these plugins will not work at "
                    "runtime."
    )
endif()
if(_gst_missing_optional)
    list(JOIN _gst_missing_optional ", " _gst_missing_optional)
    message(STATUS "GStreamer: optional decoder plugins not in ${_gst_plugin_source}: ${_gst_missing_optional} "
                   "(hardware-specific or alternatives; decoding uses those present)"
    )
endif()
# Mobile apps have no other way to load a plugin at runtime.
if(ANDROID OR IOS)
    gstreamer_missing_required(REQUIRED ${GSTREAMER_RUNTIME_REQUIRED_PLUGINS} AVAILABLE ${_gst_available_plugins}
                               OUT_VAR _gst_missing_required
    )
    if(_gst_missing_required)
        list(JOIN _gst_missing_required ", " _gst_missing_required)
        message(FATAL_ERROR "GStreamer: runtime-required plugins are missing from the static SDK at "
                            "${GStreamer_ROOT_DIR}: ${_gst_missing_required}. The SDK is incomplete or "
                            "corrupt — re-download it."
        )
    endif()
endif()

# GstVideoOrientationMeta is officially in 1.26+, but bundled iOS/Android SDKs sometimes strip it, so test the header.
# Only GStreamer::GStreamer is probed: on iOS GStreamer::mobile is a framework this project builds, which try_compile
# cannot link, and the sizeof() probe needs no archive.
qgc_check_gst_header(
    VAR
    QGC_GST_HAS_VIDEO_ORIENTATION_META
    HEADER
    gst/video/gstvideometa.h
    SYMBOL
    "sizeof(GstVideoOrientationMeta)"
    TARGET
    GStreamer::GStreamer
)
if(QGC_GST_HAS_VIDEO_ORIENTATION_META)
    target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_HAS_GST_VIDEO_ORIENTATION_META=1)
endif()

# Zero-copy DMABuf GPU path probe: HwBuffers/CMakeLists.txt builds the DMABuf path when QGC_GST_HAS_DMABUF is set. It
# needs gst-allocators, which the Linux branch of src/.../GStreamer/CMakeLists.txt requests (the Allocators component).
if(LINUX)
    qgc_check_gst_header(VAR QGC_GST_HAS_DMABUF HEADER gst/allocators/gstdmabuf.h SYMBOL gst_is_dmabuf_memory)
endif()

if(GStreamer_USE_MERGED_STATIC)
    set(_qgc_gst_path "merged-static")
elseif(GStreamer_USE_FRAMEWORK)
    set(_qgc_gst_path "framework")
elseif(GStreamer_USE_STATIC_LIBS)
    set(_qgc_gst_path "static")
else()
    set(_qgc_gst_path "shared")
endif()
list(LENGTH GSTREAMER_PLUGINS _qgc_gst_plugin_total)
set(_qgc_gst_plugin_count 0)
foreach(_p IN LISTS GSTREAMER_PLUGINS)
    if(_p IN_LIST _gst_available_plugins)
        math(EXPR _qgc_gst_plugin_count "${_qgc_gst_plugin_count}+1")
    endif()
endforeach()
message(STATUS "QGCGStreamer: version=${GStreamer_VERSION} path=${_qgc_gst_path} root=${GStreamer_ROOT_DIR} "
               "plugins=${_qgc_gst_plugin_count}/${_qgc_gst_plugin_total} auto_downloaded=${GStreamer_AUTO_DOWNLOADED}"
)

# Post-configure tooling (.github/scripts/verify_executable.py, generate_cpm_sbom.py) and PrintSummary read the
# QGC_GST_RESOLVED_* entries from the cache.
_qgc_gst_record_resolved_sdk("${GStreamer_ROOT_DIR}" "${QGC_GST_REQUESTED_VERSION}" "${GStreamer_AUTO_DOWNLOADED}"
                             "${GStreamer_VERSION}"
)

# The app ships the LGPL text and a notice naming this GStreamer and its sources: Android stages them as APK assets,
# iOS as bundle resources (src/VideoManager/VideoReceiver/GStreamer/CMakeLists.txt) and desktop installs copy them from
# GStreamer::Layout's NOTICE_DIR (Install.cmake).
if(ANDROID)
    set(_qgc_gst_notice_dir "${QGC_ANDROID_EXTRA_ASSETS_DIR}/licenses/gstreamer")
else()
    set(_qgc_gst_notice_dir "${CMAKE_BINARY_DIR}/gstreamer-notice")
endif()
qgc_gst_write_notice("${_qgc_gst_notice_dir}")
gstreamer_layout_set(NOTICE_DIR "${_qgc_gst_notice_dir}")
