include("${CMAKE_CURRENT_LIST_DIR}/Helpers.cmake")

if(NOT DEFINED GStreamer_FIND_VERSION)
    if(LINUX AND NOT ANDROID)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_MIN_VERSION})
    elseif(WIN32 AND NOT ANDROID)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_WIN_VERSION})
    elseif(ANDROID)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_ANDROID_VERSION})
    elseif(IOS)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_IOS_VERSION})
    elseif(MACOS)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_MACOS_VERSION})
    else()
        # Defense-in-depth: every supported platform is covered above; this
        # fires only if a new platform bool is added without a version branch.
        message(WARNING "GStreamer: unrecognized platform — using fallback version ${QGC_CONFIG_GSTREAMER_VERSION}")
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_VERSION})
    endif()
endif()

if(NOT GStreamer_FIND_VERSION)
    message(FATAL_ERROR "GStreamer version not configured. Ensure BuildConfig.cmake has been included "
        "and .github/build-config.json contains the appropriate gstreamer.version.<platform> entry.")
endif()

# GStreamer_ROOT_DIR is user input only (never written back), so a later configure can tell a
# user-chosen SDK from one discovery picked; see SdkState.cmake.
set(_qgc_gst_user_root "${GStreamer_ROOT_DIR}")
if(NOT _qgc_gst_user_root)
    if(GSTREAMER_ROOT)
        set(_qgc_gst_user_root "${GSTREAMER_ROOT}")
    elseif(GStreamer_ROOT)
        set(_qgc_gst_user_root "${GStreamer_ROOT}")
    endif()
endif()
if(_qgc_gst_user_root AND NOT EXISTS "${_qgc_gst_user_root}")
    message(FATAL_ERROR "GStreamer: User-provided directory does not exist: ${_qgc_gst_user_root}\n"
        "Correct the path or unset GStreamer_ROOT_DIR to allow auto-download.")
endif()

# CONTRACT: include() at directory scope, never inside a function — the layout
# flags below are directory-scope normal vars (if(NOT DEFINED)-guarded for override).
if(NOT DEFINED GStreamer_USE_STATIC_LIBS)
    if(ANDROID OR IOS)
        set(GStreamer_USE_STATIC_LIBS ON)
    else()
        set(GStreamer_USE_STATIC_LIBS OFF)
    endif()
endif()

if(NOT DEFINED GStreamer_USE_FRAMEWORK)
    if(APPLE)
        set(GStreamer_USE_FRAMEWORK ON)
    else()
        set(GStreamer_USE_FRAMEWORK OFF)
    endif()
endif()

# xcframework is the iOS-only layout; platforms override this to ON only when they
# confirm a GStreamer.xcframework path exists (see platform/IOS.cmake).
if(NOT DEFINED GStreamer_USE_XCFRAMEWORK)
    set(GStreamer_USE_XCFRAMEWORK OFF)
endif()

# The cached PKG_CONFIG_ARGN is user input; discovery derives its flags into a normal variable.
set(PKG_CONFIG_ARGN "" CACHE STRING "Extra arguments for pkg-config")
_qgc_gst_drop_legacy_pkg_config_args()
# Empty GStreamer_ROOT_DIR from here on means "discover or download"; platform discovery
# fills it in and sets GStreamer_AUTO_DOWNLOADED when it downloads.
_qgc_gst_select_sdk_root(GStreamer_ROOT_DIR GStreamer_AUTO_DOWNLOADED "${_qgc_gst_user_root}"
                         "${GStreamer_FIND_VERSION}"
)

# Per-platform discovery macros — extracted to keep this file focused on orchestration.
# _qgc_validate_expanded_pkg lives in platform/Apple.cmake (used only by Mac+iOS).
include("${CMAKE_CURRENT_LIST_DIR}/platform/Apple.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/platform/Windows.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/platform/Linux.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/platform/Android.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/platform/MacOS.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/platform/IOS.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/platform/PkgConfigTargets.cmake")

# Dispatch to the appropriate platform discovery macro
if(WIN32 AND NOT ANDROID)
    _qgc_discover_windows_sdk()
elseif(LINUX AND NOT ANDROID)
    _qgc_discover_linux_sdk()
elseif(ANDROID)
    _qgc_discover_android_sdk()
elseif(MACOS AND NOT IOS)
    _qgc_discover_macos_sdk()
elseif(IOS)
    _qgc_discover_ios_sdk()
endif()

# Post-dispatch invariant: at most one of the three layout modes may be ON.
set(_qgc_gst_active_modes)
if(GStreamer_USE_XCFRAMEWORK)
    list(APPEND _qgc_gst_active_modes GStreamer_USE_XCFRAMEWORK)
endif()
if(GStreamer_USE_FRAMEWORK)
    list(APPEND _qgc_gst_active_modes GStreamer_USE_FRAMEWORK)
endif()
if(GStreamer_USE_STATIC_LIBS)
    list(APPEND _qgc_gst_active_modes GStreamer_USE_STATIC_LIBS)
endif()
list(LENGTH _qgc_gst_active_modes _qgc_gst_active_count)
if(_qgc_gst_active_count GREATER 1)
    message(FATAL_ERROR
        "GStreamer: conflicting layout modes — only one of GStreamer_USE_XCFRAMEWORK, "
        "GStreamer_USE_FRAMEWORK, GStreamer_USE_STATIC_LIBS may be ON simultaneously. "
        "Active: ${_qgc_gst_active_modes}")
endif()
unset(_qgc_gst_active_modes)
unset(_qgc_gst_active_count)

# xcframework sets GSTREAMER_LIB_PATH / GSTREAMER_PLUGIN_PATH to the slice dir
# (which has no lib/ subdir), so the existence checks still pass.
if(NOT GStreamer_USE_XCFRAMEWORK)
    set(_gst_missing_paths)
    if(NOT EXISTS "${GStreamer_ROOT_DIR}")
        list(APPEND _gst_missing_paths "GStreamer_ROOT_DIR=${GStreamer_ROOT_DIR}")
    endif()
    if(NOT EXISTS "${GSTREAMER_LIB_PATH}")
        list(APPEND _gst_missing_paths "GSTREAMER_LIB_PATH=${GSTREAMER_LIB_PATH}")
    endif()
    if(NOT EXISTS "${GSTREAMER_PLUGIN_PATH}")
        list(APPEND _gst_missing_paths "GSTREAMER_PLUGIN_PATH=${GSTREAMER_PLUGIN_PATH}")
    endif()
    if(NOT EXISTS "${GSTREAMER_INCLUDE_PATH}")
        list(APPEND _gst_missing_paths "GSTREAMER_INCLUDE_PATH=${GSTREAMER_INCLUDE_PATH}")
    endif()
    if(_gst_missing_paths)
        string(REPLACE ";" "\n  " _gst_missing_str "${_gst_missing_paths}")
        set(_gst_framework_detail "")
        if(DEFINED GSTREAMER_FRAMEWORK_PATH)
            set(_gst_framework_detail "GSTREAMER_FRAMEWORK_PATH=${GSTREAMER_FRAMEWORK_PATH}\n")
        endif()
        message(FATAL_ERROR
            "GStreamer: required directories do not exist on disk:\n  ${_gst_missing_str}\n"
            "${_gst_framework_detail}"
            "Check installation or set GStreamer_ROOT_DIR.")
    endif()

    if(GStreamer_USE_FRAMEWORK)
        if(NOT DEFINED GSTREAMER_FRAMEWORK_PATH)
            message(FATAL_ERROR "GStreamer: framework mode is active but GSTREAMER_FRAMEWORK_PATH is unset")
        elseif(NOT EXISTS "${GSTREAMER_FRAMEWORK_PATH}")
            message(FATAL_ERROR "GStreamer: Could not locate framework at ${GSTREAMER_FRAMEWORK_PATH}")
        endif()
    endif()
else()
    if(NOT EXISTS "${GSTREAMER_XCFRAMEWORK_LIB}")
        message(FATAL_ERROR "GStreamer: xcframework library not found at ${GSTREAMER_XCFRAMEWORK_LIB}")
    endif()
endif()

# Always recompute from the current component set — a stale list from a prior
# configure would miss a newly-requested component.
gstreamer_build_apis_and_deps(GSTREAMER_APIS GSTREAMER_EXTRA_DEPS ${QGCGStreamer_FIND_COMPONENTS})

# Plugin list lives in .github/build-config.json. Alternate groups (e.g. the
# videoconvertscale↔videoconvert+videoscale 1.22 split) are owned by
# GStreamerPluginPolicy and applied below in the missing-plugin scan.
# Always recompute (see GSTREAMER_APIS above) — never carry a stale plugin list
# across reconfigures.
gstreamer_current_platform_key(_qgc_gst_plat_key)
gstreamer_plugins_for(PLATFORM "${_qgc_gst_plat_key}" OUT_VAR GSTREAMER_PLUGINS)
unset(_qgc_gst_plat_key)

# Mobile builds have no filesystem plugin scan — only statically registered plugins exist at
# runtime. Add the encode-side plugins the Debug-only MockLink video stream server
# needs (GSTREAMER_MOCK_SERVER_DEBUG_PLUGINS, PluginPolicy.cmake) in Debug builds.
if((ANDROID OR IOS) AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND GSTREAMER_PLUGINS ${GSTREAMER_MOCK_SERVER_DEBUG_PLUGINS})
    list(REMOVE_DUPLICATES GSTREAMER_PLUGINS)
endif()
# The iOS archive lacks the dependencies of these plugins, so they cannot be linked.
if(IOS)
    gstreamer_xcfw_skip(_gst_xcfw_skip)
    foreach(_gst_plugin IN LISTS _gst_xcfw_skip)
        list(REMOVE_ITEM GSTREAMER_PLUGINS "${_gst_plugin}")
    endforeach()
endif()

if(ANDROID)
    set(GStreamer_Mobile_MODULE_NAME gstreamer_android)
    set(G_IO_MODULES openssl)
    set(G_IO_MODULES_PATH "${GStreamer_ROOT_DIR}/lib/gio/modules")
    set(GStreamer_NDK_BUILD_PATH "${GStreamer_ROOT_DIR}/share/gst-android/ndk-build")
    if(QT_IS_ANDROID_MULTI_ABI_EXTERNAL_PROJECT AND DEFINED QT_INTERNAL_ANDROID_MULTI_ABI_BINARY_DIR)
        set(_gst_android_build_base "${QT_INTERNAL_ANDROID_MULTI_ABI_BINARY_DIR}")
    else()
        set(_gst_android_build_base "${CMAKE_BINARY_DIR}")
    endif()
    if(QT_USE_TARGET_ANDROID_BUILD_DIR)
        set(_gst_android_build_dir "${_gst_android_build_base}/android-build-${CMAKE_PROJECT_NAME}")
    else()
        set(_gst_android_build_dir "${_gst_android_build_base}/android-build")
    endif()
    set(GStreamer_JAVA_SRC_DIR "${_gst_android_build_dir}/src")
    set(GStreamer_ASSETS_DIR "${_gst_android_build_dir}/assets")
elseif(IOS)
    # The merged archive also carries the static GIO modules.
    set(GStreamer_Mobile_MODULE_NAME gstreamer_mobile)
    set(G_IO_MODULES openssl)
    set(GStreamer_ASSETS_DIR "${CMAKE_BINARY_DIR}/assets")
endif()

# Cached pkg-config, find_library and try-compile results describe one SDK: drop them when the
# root, its .pc version, the linkage or the pkg-config invocation changes.
set(_qgc_gst_fingerprint_pkg_config "")
if(NOT GStreamer_USE_XCFRAMEWORK)
    find_package(PkgConfig REQUIRED QUIET)
    # --static is appended later by _qgc_create_pkgconfig_targets (the linkage below covers it), and
    # discovery may re-derive the other flags in a different order on reconfigure.
    set(_qgc_gst_fingerprint_args ${PKG_CONFIG_ARGN})
    list(REMOVE_ITEM _qgc_gst_fingerprint_args --static)
    list(SORT _qgc_gst_fingerprint_args)
    set(_qgc_gst_fingerprint_pkg_config "${PKG_CONFIG_EXECUTABLE}|${_qgc_gst_fingerprint_args}")
endif()
if(GStreamer_USE_STATIC_LIBS)
    set(_qgc_gst_linkage static)
else()
    set(_qgc_gst_linkage shared)
endif()
_qgc_gst_sdk_pc_version("${GSTREAMER_LIB_PATH}" _qgc_gst_pc_version)
set(_qgc_gst_fingerprint "${GStreamer_ROOT_DIR}|${GStreamer_FIND_VERSION}|${_qgc_gst_pc_version}|${_qgc_gst_linkage}")
string(APPEND _qgc_gst_fingerprint "|${_qgc_gst_fingerprint_pkg_config}")
_qgc_gst_refresh_sdk_cache("${_qgc_gst_fingerprint}")

# Create GStreamer::* IMPORTED targets via the appropriate platform helper.
if(GStreamer_USE_XCFRAMEWORK)
    _qgc_create_xcframework_targets()
else()
    _qgc_create_pkgconfig_targets()
endif()

# Mandatory registry rows (GSTREAMER_COMPONENT_REGISTRY in cmake/GStreamer/Components.cmake)
# are hard requirements: the sources include their headers unconditionally, so fail here
# rather than at compile time when one was not found.
gstreamer_mandatory_components(_qgc_gst_mandatory _qgc_gst_mandatory_apis)
set(_qgc_gst_missing_mandatory "")
foreach(_api IN LISTS _qgc_gst_mandatory_apis)
    if(NOT TARGET GStreamer::${_api})
        gstreamer_resolve_component("${_api}" _ _ _qgc_gst_missing_pc _)
        list(APPEND _qgc_gst_missing_mandatory "${_qgc_gst_missing_pc}")
    endif()
endforeach()
if(_qgc_gst_missing_mandatory)
    string(REPLACE ";" ", " _qgc_gst_missing_mandatory "${_qgc_gst_missing_mandatory}")
    message(FATAL_ERROR "GStreamer: mandatory pkg-config modules not found for the SDK at ${GStreamer_ROOT_DIR}: "
                        "${_qgc_gst_missing_mandatory}\nInstall the GStreamer development packages (gst-plugins-base) "
                        "or point GStreamer_ROOT_DIR at a complete SDK."
    )
endif()
foreach(_comp IN LISTS _qgc_gst_mandatory)
    set(QGCGStreamer_${_comp}_FOUND TRUE)
    set(GStreamer_${_comp}_FOUND TRUE)
endforeach()
unset(_qgc_gst_missing_mandatory)
unset(_qgc_gst_mandatory)
unset(_qgc_gst_mandatory_apis)
# User-requested optional components: mark FOUND only if the corresponding
# api_ target was actually created by find_package(GStreamer).
foreach(_comp IN LISTS QGCGStreamer_FIND_COMPONENTS)
    gstreamer_resolve_component("${_comp}" _resolved_name _api _ _)
    # A registry match (non-empty resolved name) includes umbrella entries like
    # Core whose api/pc are intentionally empty; keying off _api alone mis-flags
    # Core as a typo. Derive a fallback api / flag unknown only when nothing matched.
    if(_resolved_name OR _api)
        set(_registry_known TRUE)
    else()
        set(_registry_known FALSE)
        gstreamer_component_to_api("${_comp}" _api)
    endif()
    # Require the api to be one we actually built, not just any same-named
    # imported target, before reporting the component FOUND.
    if(TARGET GStreamer::${_api} AND _api IN_LIST GSTREAMER_APIS)
        set(QGCGStreamer_${_comp}_FOUND TRUE)
        set(GStreamer_${_comp}_FOUND TRUE)
    elseif(NOT _registry_known AND NOT TARGET GStreamer::${_api})
        # Neither a registry entry nor a target — almost always a typo'd or
        # unsupported component name; surface it loudly.
        message(WARNING
            "QGCGStreamer: requested component '${_comp}' is not a known registry "
            "component and produced no GStreamer::${_api} target — check spelling "
            "(components are CamelCase, e.g. App, Audio, Pbutils) or platform support.")
    endif()
endforeach()

if(GStreamer_USE_STATIC_LIBS)
    target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_STATIC_BUILD)
    if(ANDROID)
        # Cerbero gstreamer-1.0.mk linker flags, per-ABI from platform/Android.cmake:
        # -Bsymbolic (all ABIs) keeps FFmpeg's page-relative relocations valid,
        # -z text (armv7/x86) hard-errors residual text relocations, --export-dynamic
        # (all ABIs) re-exports symbols for dlopen'd GIO modules/plugins. Global-property
        # guard because link options aren't deduped and this reruns on reconfigure.
        get_property(_gst_android_linkopts_done GLOBAL PROPERTY _qgc_gst_android_linkopts_applied)
        if(NOT _gst_android_linkopts_done)
            _qgc_android_abi_info("${CMAKE_ANDROID_ARCH_ABI}" _GST_ABI)
            set(_gst_android_link_opts "-Wl,--export-dynamic")
            if(_GST_ABI_NEEDS_BSYMBOLIC_FIX)
                list(APPEND _gst_android_link_opts "-Wl,-Bsymbolic")
            endif()
            if(_GST_ABI_NEEDS_TEXTREL_ERROR)
                list(APPEND _gst_android_link_opts "-Wl,-z,text")
            endif()
            target_link_options(GStreamer::GStreamer INTERFACE ${_gst_android_link_opts})
            set_property(GLOBAL PROPERTY _qgc_gst_android_linkopts_applied TRUE)
        endif()
    endif()
endif()

# Plugins the app can use: the plugin files of a shared layout (Android/iOS bake plugins into the static
# link, and Install.cmake verifies desktop installs), plus the GStreamer::<plugin> targets of static layouts.
set(_gst_available_plugins "")
set(_gst_plugins_scanned FALSE)
if(NOT GStreamer_USE_STATIC_LIBS AND NOT GStreamer_USE_XCFRAMEWORK AND EXISTS "${GSTREAMER_PLUGIN_PATH}")
    gstreamer_scan_plugin_basenames(_gst_available_plugins "${GSTREAMER_PLUGIN_PATH}")
    set(_gst_plugins_scanned TRUE)
endif()
foreach(_gst_plugin IN LISTS GSTREAMER_PLUGINS)
    if(TARGET GStreamer::${_gst_plugin})
        list(APPEND _gst_available_plugins "${_gst_plugin}")
    endif()
endforeach()

# Alternate-aware: e.g. videoconvert+videoscale satisfy videoconvertscale.
foreach(_gst_plugin IN LISTS GSTREAMER_PLUGINS)
    gstreamer_plugin_satisfied(PLUGIN "${_gst_plugin}" AVAILABLE ${_gst_available_plugins} OUT_VAR
                               GST_PLUGIN_${_gst_plugin}_FOUND
    )
endforeach()

# Static layouts register GSTREAMER_STATIC_PLUGINS in a generated gst_init_static_plugins(); the Android and iOS
# backends resolve it from the SDK's plugin archives while creating their targets.
if(GStreamer_USE_STATIC_LIBS OR GStreamer_USE_XCFRAMEWORK)
    if(NOT ANDROID AND NOT IOS)
        # Desktop static layouts register every requested plugin that has a GStreamer::<plugin> target.
        set(GSTREAMER_STATIC_PLUGINS "")
        foreach(_gst_plugin IN LISTS GSTREAMER_PLUGINS)
            if(TARGET GStreamer::${_gst_plugin})
                list(APPEND GSTREAMER_STATIC_PLUGINS "${_gst_plugin}")
            endif()
        endforeach()
    endif()
    gstreamer_missing_required(REQUIRED ${GSTREAMER_PLUGINS} AVAILABLE ${GSTREAMER_STATIC_PLUGINS} OUT_VAR
                               _gst_missing_plugins
    )
    if(_gst_missing_plugins)
        message(WARNING "GStreamer: requested plugins missing from the static SDK at ${GStreamer_ROOT_DIR} are not "
                        "registered: ${_gst_missing_plugins}"
        )
    endif()
    # Mobile apps have no other way to load a plugin at runtime.
    if(ANDROID OR IOS)
        gstreamer_runtime_required_plugins(_gst_required_plugins)
        gstreamer_missing_required(REQUIRED ${_gst_required_plugins} AVAILABLE ${GSTREAMER_STATIC_PLUGINS} OUT_VAR
                                   _gst_missing_required
        )
        if(_gst_missing_required)
            list(JOIN _gst_missing_required ", " _gst_missing_required)
            message(FATAL_ERROR "GStreamer: runtime-required plugins are missing from the static SDK at "
                                "${GStreamer_ROOT_DIR}: ${_gst_missing_required}. The SDK is incomplete or "
                                "corrupt — re-download it."
            )
        endif()
    endif()
endif()

if(_gst_plugins_scanned)
    gstreamer_missing_required(REQUIRED ${GSTREAMER_PLUGINS} AVAILABLE ${_gst_available_plugins} OUT_VAR
                               _gst_missing_plugins
    )
    if(_gst_missing_plugins)
        message(WARNING "GStreamer: The following plugins are listed in GSTREAMER_PLUGINS "
            "but not found in ${GSTREAMER_PLUGIN_PATH}: ${_gst_missing_plugins}\n"
            "Video features depending on these plugins will not work at runtime.")
    endif()
endif()

# Resolves ALIAS targets and applies INTERFACE compile definitions — the cycle
# below (and the feature probe further down) need this twice.
# target_compile_definitions can't be called on ALIAS targets, so unwrap first.
function(_qgc_gst_apply_def GST_TARGET)
    if(NOT TARGET ${GST_TARGET})
        return()
    endif()
    get_target_property(_aliased ${GST_TARGET} ALIASED_TARGET)
    if(_aliased)
        target_compile_definitions(${_aliased} INTERFACE ${ARGN})
    else()
        target_compile_definitions(${GST_TARGET} INTERFACE ${ARGN})
    endif()
endfunction()

# Mobile (iOS/Android) builds consume GStreamerMobile (alias GStreamer::mobile) instead
# of GStreamer::GStreamer, so propagate version defines and feature-test results to both.
if(GStreamer_VERSION)
    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" _gst_ver_match "${GStreamer_VERSION}")
    if(_gst_ver_match)
        foreach(_gst_target IN ITEMS GStreamer::GStreamer GStreamerMobile)
            _qgc_gst_apply_def(${_gst_target}
                QGC_GST_BUILD_VERSION_MAJOR=${CMAKE_MATCH_1}
                QGC_GST_BUILD_VERSION_MINOR=${CMAKE_MATCH_2}
            )
        endforeach()
    endif()
endif()

# GstVideoOrientationMeta is officially in 1.26+, but bundled iOS/Android SDKs sometimes
# strip it. Feature-test the actual header instead of trusting the version number.
foreach(_gst_target IN ITEMS GStreamer::GStreamer GStreamerMobile)
    if(TARGET ${_gst_target})
        qgc_check_gst_header(
            VAR    QGC_GST_HAS_VIDEO_ORIENTATION_META_${_gst_target}
            HEADER gst/video/gstvideometa.h
            SYMBOL "sizeof(GstVideoOrientationMeta)"
            TARGET ${_gst_target}
        )
        if(QGC_GST_HAS_VIDEO_ORIENTATION_META_${_gst_target})
            _qgc_gst_apply_def(${_gst_target} QGC_HAS_GST_VIDEO_ORIENTATION_META=1)
        endif()
    endif()
endforeach()

# Zero-copy DMABuf GPU path probe — Linux-only, defined in platform/Linux.cmake.
if(LINUX)
    _qgc_detect_dmabuf()
endif()

# One-line diagnostic — surfaces which discovery path won when CI logs are the
# only forensics available.
if(GStreamer_USE_XCFRAMEWORK)
    set(_qgc_gst_path "xcframework")
elseif(GStreamer_USE_FRAMEWORK)
    set(_qgc_gst_path "framework")
elseif(GStreamer_USE_STATIC_LIBS)
    set(_qgc_gst_path "static")
else()
    set(_qgc_gst_path "shared")
endif()
list(LENGTH GSTREAMER_PLUGINS _qgc_gst_plugin_total)
if(GStreamer_USE_STATIC_LIBS OR GStreamer_USE_XCFRAMEWORK)
    list(LENGTH GSTREAMER_STATIC_PLUGINS _qgc_gst_plugin_count)
else()
    set(_qgc_gst_plugin_count 0)
    foreach(_p IN LISTS GSTREAMER_PLUGINS)
        if(_p IN_LIST _gst_available_plugins)
            math(EXPR _qgc_gst_plugin_count "${_qgc_gst_plugin_count}+1")
        endif()
    endforeach()
endif()
message(STATUS "QGCGStreamer: version=${GStreamer_VERSION} path=${_qgc_gst_path} root=${GStreamer_ROOT_DIR} plugins=${_qgc_gst_plugin_count}/${_qgc_gst_plugin_total} auto_downloaded=${GStreamer_AUTO_DOWNLOADED}")

# Name the discovery macro that should have set these, for a clear failure.
if(WIN32 AND NOT ANDROID)
    set(_qgc_disco_macro "_qgc_discover_windows_sdk (platform/Windows.cmake)")
elseif(LINUX AND NOT ANDROID)
    set(_qgc_disco_macro "_qgc_discover_linux_sdk (platform/Linux.cmake)")
elseif(ANDROID)
    set(_qgc_disco_macro "_qgc_discover_android_sdk (platform/Android.cmake)")
elseif(IOS)
    set(_qgc_disco_macro "_qgc_discover_ios_sdk (platform/IOS.cmake)")
elseif(MACOS)
    set(_qgc_disco_macro "_qgc_discover_macos_sdk (platform/MacOS.cmake)")
else()
    set(_qgc_disco_macro "the platform discovery dispatch")
endif()
set(_qgc_required_vars GStreamer_ROOT_DIR)
if(NOT GStreamer_USE_XCFRAMEWORK)
    list(APPEND _qgc_required_vars GSTREAMER_LIB_PATH GSTREAMER_PLUGIN_PATH)
endif()
foreach(_qgc_req IN LISTS _qgc_required_vars)
    if(NOT ${_qgc_req})
        message(FATAL_ERROR "QGCGStreamer: required variable ${_qgc_req} is not set after "
            "platform discovery — ${_qgc_disco_macro} failed to set it.")
    endif()
endforeach()
unset(_qgc_required_vars)
unset(_qgc_disco_macro)

# Post-configure tooling (.github/scripts/verify_executable.py, the Windows workflow) reads
# QGC_GST_RESOLVED_ROOT from CMakeCache.txt; PrintSummary reads the GStreamer::Layout properties.
_qgc_gst_record_resolved_sdk("${GStreamer_ROOT_DIR}" "${GStreamer_FIND_VERSION}" "${GStreamer_AUTO_DOWNLOADED}")
gstreamer_layout_set(VERSION "${GStreamer_VERSION}")
gstreamer_layout_set(AUTO_DOWNLOADED "${GStreamer_AUTO_DOWNLOADED}")

set(QGCGStreamer_FOUND TRUE)
