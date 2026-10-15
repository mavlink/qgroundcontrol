# SPDX-FileCopyrightText: 2024 L. E. Segovia <amy@centricular.com>
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Android GStreamer SDK discovery — invoked by Orchestrator.cmake.
#
# The mobile target (_qgc_create_android_mobile_target and its helpers) is derived from FindGStreamerMobile.cmake in
# cerbero (https://gitlab.freedesktop.org/gstreamer/cerbero, data/mobile/FindGStreamerMobile.cmake), which the Android
# SDK installs as share/cmake/FindGStreamerMobile.cmake. QGC changes:
#   - the plugin archives are linked into the application, which carries the only GStreamer core, instead of into a
#     separate libgstreamer_android.so, and the static-plugin registration source is generated on the application;
#   - plugins without a static library are left out, failing configure when a runtime-required one is missing;
#   - the plugin Java sources and the fonts/CA assets are staged in the build directory for Gradle, not copied into
#     the source tree;
#   - SDK discovery and extraction (above the mobile target) are QGC's own.

include_guard(GLOBAL)

# Single source of truth for Android ABI → cerbero dir + textrel/bsymbolic flags.
# Sets, in PARENT_SCOPE: <PFX>_DIR, <PFX>_NEEDS_TEXTREL_ERROR, <PFX>_NEEDS_BSYMBOLIC_FIX.
function(_qgc_android_abi_info ABI PFX)
    if(ABI STREQUAL "armeabi-v7a")
        set(_dir "armv7")
        set(_textrel TRUE)
        set(_bsym TRUE)
    elseif(ABI STREQUAL "arm64-v8a")
        set(_dir "arm64")
        set(_textrel FALSE)
        set(_bsym TRUE)  # Cerbero marks every ABI NEEDS_BSYMBOLIC_FIX
    elseif(ABI STREQUAL "x86")
        set(_dir "x86")
        set(_textrel TRUE)
        set(_bsym TRUE)
    elseif(ABI STREQUAL "x86_64")
        set(_dir "x86_64")
        set(_textrel FALSE)
        set(_bsym TRUE)
    else()
        message(FATAL_ERROR "Unsupported Android ABI: ${ABI}")
    endif()
    set(${PFX}_DIR "${_dir}" PARENT_SCOPE)
    set(${PFX}_NEEDS_TEXTREL_ERROR "${_textrel}" PARENT_SCOPE)
    set(${PFX}_NEEDS_BSYMBOLIC_FIX "${_bsym}" PARENT_SCOPE)
endfunction()

# _qgc_gst_android_extract(<staging> <archive> <abi_dir>)
# qgc_gst_acquire_sdk extractor: unpacks only the <abi_dir>/ tree of the universal SDK tarball into
# <staging>. Every ABI build (including Qt's multi-ABI sub-builds) acquires its own tree.
# cmakelang 0.6.13 does not recognize file(ARCHIVE_EXTRACT).
# cmake-lint: disable=E1126
function(_qgc_gst_android_extract staging archive abi_dir)
    file(
        ARCHIVE_EXTRACT
        INPUT
        "${archive}"
        DESTINATION
        "${staging}"
        PATTERNS
        "${abi_dir}/*"
    )
    if(NOT EXISTS "${staging}/${abi_dir}/lib/pkgconfig/gstreamer-1.0.pc")
        message(FATAL_ERROR "GStreamer: ${archive} has no ${abi_dir}/ SDK tree")
    endif()
endfunction()

macro(_qgc_discover_android_sdk)
    if(NOT GStreamer_ROOT_DIR)
        _qgc_android_abi_info("${CMAKE_ANDROID_ARCH_ABI}" _gst_android_abi)
        qgc_gst_acquire_sdk(
            PLATFORM
            android
            VERSION
            ${GStreamer_FIND_VERSION}
            CACHE_SUBDIR
            "gstreamer-android-${GStreamer_FIND_VERSION}"
            ARCHIVES
            "gstreamer-android-${GStreamer_FIND_VERSION}.tar.xz"
            CONTENT
            ${_gst_android_abi_DIR}
            EXTRACT_FUNCTION
            _qgc_gst_android_extract
            EXTRACT_ARGS
            ${_gst_android_abi_DIR}
            CONTENT_DIR_OUT
            _gst_android_sdk
        )
        set(GStreamer_ROOT_DIR "${_gst_android_sdk}/${_gst_android_abi_DIR}")
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    gstreamer_create_layout_target(
        SDK_ROOT "${GStreamer_ROOT_DIR}"
        TYPE     STATIC_TARBALL
    )

    if(CMAKE_HOST_WIN32)
        gstreamer_apply_pkgconfig_env(
            MODE SDK
            PKG_CONFIG_EXE "${GStreamer_ROOT_DIR}/share/gst-android/ndk-build/tools/windows/pkg-config.exe"
            LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
            DONT_DEFINE_PREFIX
        )
    elseif(CMAKE_HOST_UNIX)
        if(CMAKE_HOST_APPLE)
            _qgc_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
        endif()
        gstreamer_apply_pkgconfig_env(
            MODE SDK
            LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    endif()
endmacro()

# ─────────────────────────────────────────────────────────────────────────────
# _qgc_create_android_mobile_target
# Creates GStreamer::mobile, which links the plugin archives, their helper libraries and the GIO modules into
# the application (the static-plugin registration source is generated on the application; see
# GSTREAMER_STATIC_PLUGINS), and copies the fonts/CA assets into the APK.
# Caller must set GStreamerMobile_FIND_COMPONENTS (the COMPONENTS list a
# find_package-style call would have set).
# Macro (not function): GStreamerMobile target / GStreamer_*_FOUND / cache
# variables must propagate to caller scope.
# ─────────────────────────────────────────────────────────────────────────────
# Pure-computation helper — no add_library / target_* calls.
# Reads GStreamer* globals set by the parent scope; returns results via OUT params.
function(_qgc_compute_android_mobile_target plugins_out apis_out found_components_out)
    set(_gst_plugins ${GStreamerMobile_FIND_COMPONENTS})
    list(REMOVE_ITEM _gst_plugins fonts ca_certificates mobile)
    list(REMOVE_DUPLICATES _gst_plugins)

    set(_gst_mobile_plugins ${_gst_plugins})
    list(FILTER _gst_mobile_plugins EXCLUDE REGEX "^api_")
    set(_gst_mobile_apis ${_gst_plugins})
    list(FILTER _gst_mobile_apis INCLUDE REGEX "^api_")

    if(GStreamer_DEBUG)
        message(STATUS "[GstMobile] Requested plugins: ${_gst_mobile_plugins}")
        message(STATUS "[GstMobile] Requested APIs: ${_gst_mobile_apis}")
    endif()

    # Resolve each plugin to a static .a; Orchestrator.cmake reports the requested plugins left out.
    _gst_save_find_suffixes()
    set(_gst_found_plugins)
    foreach(_p IN LISTS _gst_mobile_plugins)
        if(GStreamer_${_p}_FOUND)
            find_library(_gst_plugin_lib_${CMAKE_ANDROID_ARCH_ABI}_${_p} gst${_p}
                HINTS "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0"
                NO_DEFAULT_PATH
            )
            if(_gst_plugin_lib_${CMAKE_ANDROID_ARCH_ABI}_${_p})
                list(APPEND _gst_found_plugins ${_p})
            endif()
        endif()
    endforeach()
    _gst_restore_find_suffixes()
    set(_gst_mobile_plugins ${_gst_found_plugins})

    if(GStreamer_DEBUG)
        message(STATUS "[GstMobile] Found plugins with .a: ${_gst_mobile_plugins}")
        foreach(_dbg_p IN LISTS _gst_mobile_plugins)
            message(STATUS "[GstMobile]   ${_dbg_p} -> ${_gst_plugin_lib_${CMAKE_ANDROID_ARCH_ABI}_${_dbg_p}}")
        endforeach()
    endif()

    set(_gst_validate_components)
    foreach(_component IN ITEMS mobile ca_certificates fonts)
        if(_component IN_LIST GStreamerMobile_FIND_COMPONENTS)
            list(APPEND _gst_validate_components ${_component})
        endif()
    endforeach()
    foreach(_api IN LISTS _gst_mobile_apis)
        if(GStreamer_${_api}_FOUND)
            list(APPEND _gst_validate_components ${_api})
        endif()
    endforeach()
    list(APPEND _gst_validate_components ${_gst_mobile_plugins})
    list(REMOVE_DUPLICATES _gst_validate_components)

    set(${plugins_out}         "${_gst_mobile_plugins}"    PARENT_SCOPE)
    set(${apis_out}            "${_gst_mobile_apis}"       PARENT_SCOPE)
    set(${found_components_out} "${_gst_validate_components}" PARENT_SCOPE)
endfunction()

# _qgc_android_whole_archive_plugins(<plugin>...)
# Links each plugin's static archive (found by _qgc_compute_android_mobile_target) as
# $<LINK_LIBRARY:WHOLE_ARCHIVE,GStreamer::<plugin>_archive> wherever GStreamer::<plugin> is linked, so every
# gst_plugin_<plugin>_register() and its GType objects survive the linker's dead-strip and register into the
# app's single core. GStreamer::<plugin> no longer links the archive by path as well: a plain occurrence ahead of
# the whole-archive one would let the linker load some members twice.
function(_qgc_android_whole_archive_plugins)
    foreach(_plugin IN LISTS ARGN)
        set(_archive "${_gst_plugin_lib_${CMAKE_ANDROID_ARCH_ABI}_${_plugin}}")
        add_library(GStreamer::${_plugin}_archive STATIC IMPORTED GLOBAL)
        set_target_properties(GStreamer::${_plugin}_archive PROPERTIES IMPORTED_LOCATION "${_archive}")

        cmake_path(GET _archive FILENAME _archive_name)
        get_target_property(_libraries GStreamer::${_plugin} INTERFACE_LINK_LIBRARIES)
        set(_kept_libraries "$<LINK_LIBRARY:WHOLE_ARCHIVE,GStreamer::${_plugin}_archive>")
        foreach(_library IN LISTS _libraries)
            cmake_path(GET _library FILENAME _library_name)
            if(NOT _library_name STREQUAL _archive_name AND NOT _library MATCHES "-NOTFOUND$")
                list(APPEND _kept_libraries "${_library}")
            endif()
        endforeach()
        set_target_properties(GStreamer::${_plugin} PROPERTIES INTERFACE_LINK_LIBRARIES "${_kept_libraries}")
    endforeach()
endfunction()

# _qgc_android_stage_files(JAVA <source>|<relative path>... ASSETS <source>|<relative path>...)
# Copies the plugin Java sources under GStreamer_JAVA_SRC_DIR and the assets under GStreamer_ASSETS_DIR when the app
# builds; android/build.gradle adds both build-directory trees to the APK. Earlier builds copied these files into
# QGC_ANDROID_PACKAGE_SOURCE_DIR, where they would now duplicate the staged copies, so those copies are removed.
# cmake-lint: disable=C0113
function(_qgc_android_stage_files)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "" "JAVA;ASSETS")
    set(_outputs "")
    set(_legacy_copies "")
    foreach(_kind IN ITEMS JAVA ASSETS)
        if(_kind STREQUAL "JAVA")
            set(_root "${GStreamer_JAVA_SRC_DIR}")
            set(_legacy_root "${QGC_ANDROID_PACKAGE_SOURCE_DIR}/src")
        else()
            set(_root "${GStreamer_ASSETS_DIR}")
            set(_legacy_root "${QGC_ANDROID_PACKAGE_SOURCE_DIR}/assets")
        endif()
        foreach(_entry IN LISTS ARG_${_kind})
            string(REPLACE "|" ";" _entry "${_entry}")
            list(GET _entry 0 _source)
            list(GET _entry 1 _relative)
            set(_output "${_root}/${_relative}")
            cmake_path(GET _output PARENT_PATH _output_dir)
            add_custom_command(
                OUTPUT "${_output}"
                COMMAND "${CMAKE_COMMAND}" -E make_directory "${_output_dir}"
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_source}" "${_output}"
                DEPENDS "${_source}"
                COMMENT "Staging GStreamer ${_relative}"
                VERBATIM
            )
            list(APPEND _outputs "${_output}")
            if(QGC_ANDROID_PACKAGE_SOURCE_DIR AND EXISTS "${_legacy_root}/${_relative}")
                list(APPEND _legacy_copies "${_legacy_root}/${_relative}")
            endif()
        endforeach()
    endforeach()

    if(_legacy_copies)
        message(STATUS "GStreamer: removing files earlier builds copied into the Android package sources: "
                       "${_legacy_copies}"
        )
        file(REMOVE ${_legacy_copies})
    endif()
    if(NOT _outputs)
        return()
    endif()
    add_custom_target(gstreamer_android_files_${CMAKE_ANDROID_ARCH_ABI} DEPENDS ${_outputs})
    # INTERFACE GStreamer::mobile cannot carry build dependencies; attach the staging to the app.
    if(TARGET ${CMAKE_PROJECT_NAME})
        add_dependencies(${CMAKE_PROJECT_NAME} gstreamer_android_files_${CMAKE_ANDROID_ARCH_ABI})
    endif()
endfunction()

macro(_qgc_create_android_mobile_target)

if(NOT GSTREAMER_LIB_PATH)
    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
endif()

if(NOT DEFINED _gst_IGNORED_SYSTEM_LIBRARIES OR NOT DEFINED _gst_SRT_REGEX_PATCH)
    include("${CMAKE_CURRENT_LIST_DIR}/../Helpers.cmake")
endif()

if(ANDROID)
    if (NOT DEFINED GStreamer_JAVA_SRC_DIR AND DEFINED GSTREAMER_JAVA_SRC_DIR)
        set(GStreamer_JAVA_SRC_DIR "${GSTREAMER_JAVA_SRC_DIR}")
    elseif(NOT DEFINED GStreamer_JAVA_SRC_DIR)
        set(GStreamer_JAVA_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src/")
    endif()
    # Normalize once: the legacy-var branch copies its value verbatim, so a
    # relative path would otherwise stay un-anchored.
    if(NOT IS_ABSOLUTE "${GStreamer_JAVA_SRC_DIR}")
        set(GStreamer_JAVA_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../${GStreamer_JAVA_SRC_DIR}")
    endif()

    if(NOT DEFINED GStreamer_NDK_BUILD_PATH AND DEFINED GSTREAMER_NDK_BUILD_PATH)
        set(GStreamer_NDK_BUILD_PATH "${GSTREAMER_NDK_BUILD_PATH}")
    elseif(NOT DEFINED GStreamer_NDK_BUILD_PATH)
        set(GStreamer_NDK_BUILD_PATH  "${GStreamer_ROOT_DIR}/share/gst-android/ndk-build/")
    endif()
endif()

if(NOT DEFINED GStreamer_Mobile_MODULE_NAME)
    if(DEFINED GSTREAMER_ANDROID_MODULE_NAME)
        set(GStreamer_Mobile_MODULE_NAME "${GSTREAMER_ANDROID_MODULE_NAME}")
    else()
        set(GStreamer_Mobile_MODULE_NAME gstreamer_android)
    endif()
endif()

if(ANDROID)
    if(NOT DEFINED GStreamer_ASSETS_DIR AND DEFINED GSTREAMER_ASSETS_DIR)
        set(GStreamer_ASSETS_DIR "${GSTREAMER_ASSETS_DIR}")
    elseif(NOT DEFINED GStreamer_ASSETS_DIR)
        set(GStreamer_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src/assets/")
    elseif(NOT IS_ABSOLUTE "${GStreamer_ASSETS_DIR}")
        set(GStreamer_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../${GStreamer_ASSETS_DIR}")
    endif()

    # GStreamer.java is not consumed (init runs in C++); used only as a stable
    # SDK-layout anchor present regardless of plugin selection.
    if(NOT EXISTS "${GStreamer_NDK_BUILD_PATH}/GStreamer.java")
        message(FATAL_ERROR "GStreamer Android SDK not found at ${GStreamer_NDK_BUILD_PATH} "
            "(expected ndk-build layout anchor GStreamer.java is missing). "
            "Verify GStreamer Android SDK installation.")
    endif()
endif()

if(ANDROID)
    set(GSTREAMER_IS_MOBILE ON)
else()
    set(GSTREAMER_IS_MOBILE OFF)
endif()

if (GSTREAMER_IS_MOBILE)
    if (NOT DEFINED GStreamer_USE_STATIC_LIBS)
        set(GStreamer_USE_STATIC_LIBS ON)
    endif()
    if (NOT GStreamer_USE_STATIC_LIBS)
        message(FATAL_ERROR "Shared library GStreamer is not supported on mobile platforms")
    endif()
endif()

# Pre-condition: the pkg-config targets (and GStreamer_*_FOUND) must exist before
# any computation that reads them.
if(NOT TARGET GStreamer::GStreamer)
    message(FATAL_ERROR "qgc_gst_create_pkgconfig_targets() must run before _qgc_create_android_mobile_target().")
endif()

# ── Computation phase (function to avoid variable leakage) ───────────────────
_qgc_compute_android_mobile_target(_gst_mobile_plugins _gst_mobile_apis _gst_validate_components)
set(GSTREAMER_STATIC_PLUGINS ${_gst_mobile_plugins})
list(TRANSFORM G_IO_MODULES PREPEND "gio" OUTPUT_VARIABLE G_IO_MODULES_LIBS)

if(GStreamer_DEBUG)
    message(STATUS "[GstMobile] GStreamer_ROOT_DIR = ${GStreamer_ROOT_DIR}")
    message(STATUS "[GstMobile] GSTREAMER_LIB_PATH = ${GSTREAMER_LIB_PATH}")
endif()

# ── Side-effect phase: target creation ──────────────────────────────────────
if (GSTREAMER_IS_MOBILE AND (NOT TARGET GStreamer::mobile))
    if (NOT ANDROID)
        message(FATAL_ERROR "_qgc_create_android_mobile_target is Android-only; iOS uses the xcframework path.")
    endif()

    # Single-core model: GStreamer::mobile is an INTERFACE target that whole-archives the plugin .a's +
    # helpers + GIO modules into the app, which carries the one static core (GStreamer::GStreamer). A
    # separate .so would carry a duplicate GstElement core and fail g_type_is_a for every plugin.
    add_library(GStreamerMobile INTERFACE IMPORTED GLOBAL)
    add_library(GStreamer::mobile ALIAS GStreamerMobile)
endif()

if(GSTREAMER_IS_MOBILE AND TARGET GStreamerMobile)
    # INTERFACE-only: contributes plugin .a's + helpers + GIO modules to the app, which already carries
    # the single core. Do NOT re-link the core here — that produced the duplicate-GstElement-core failure.
    target_include_directories(
        GStreamerMobile
        INTERFACE
            $<TARGET_PROPERTY:GStreamer::GStreamer,INTERFACE_INCLUDE_DIRECTORIES>
    )

    set(_gst_staged_java)
    foreach(_gst_java_plugin IN LISTS _gst_mobile_plugins)
        file(GLOB_RECURSE
            _gst_java_files
            FOLLOW_SYMLINKS
            CONFIGURE_DEPENDS
            RELATIVE "${GStreamer_NDK_BUILD_PATH}"
            "${GStreamer_NDK_BUILD_PATH}/${_gst_java_plugin}/*.java"
        )
        foreach(_gst_java_file IN LISTS _gst_java_files)
            list(APPEND _gst_staged_java
                 "${GStreamer_NDK_BUILD_PATH}/${_gst_java_file}|org/freedesktop/gstreamer/${_gst_java_file}"
            )
        endforeach()
    endforeach()

    if (NOT G_IO_MODULES_PATH)
        pkg_get_variable(G_IO_MODULES_PATH gio-2.0 giomoduledir)
    endif()
    if (NOT G_IO_MODULES_PATH)
        set(G_IO_MODULES_PATH "${GStreamer_ROOT_DIR}/lib/gio/modules")
    endif()

    if (G_IO_MODULES_LIBS)
        add_library(GStreamer::gio_modules INTERFACE IMPORTED)

        _gst_save_find_suffixes()
        foreach(_gio_lib IN LISTS G_IO_MODULES_LIBS)
            if(_gio_lib MATCHES "${_gst_SRT_REGEX_PATCH}")
                string(REGEX REPLACE "${_gst_SRT_REGEX_PATCH}" "\\1" _gio_lib "${_gio_lib}")
            endif()
            string(MAKE_C_IDENTIFIER "_gst_${CMAKE_ANDROID_ARCH_ABI}_${_gio_lib}" _gio_cache_var)
            if(NOT ${_gio_cache_var})
                find_library(${_gio_cache_var}
                    NAMES ${_gio_lib}
                    HINTS ${G_IO_MODULES_PATH}
                    NO_DEFAULT_PATH
                    NO_CMAKE_FIND_ROOT_PATH
                )
            endif()
            if(${_gio_cache_var})
                target_link_libraries(GStreamer::gio_modules INTERFACE "${${_gio_cache_var}}")
            else()
                message(WARNING "GStreamerMobile: GIO module '${_gio_lib}' not found in ${G_IO_MODULES_PATH}")
            endif()
        endforeach()
        _gst_restore_find_suffixes()

        if ("openssl" IN_LIST G_IO_MODULES)
            target_link_directories(GStreamer::gio_modules INTERFACE
                "${GStreamer_ROOT_DIR}/lib"
                "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0"
            )
            find_library(_gst_ssl_lib_${CMAKE_ANDROID_ARCH_ABI} ssl
                HINTS "${GStreamer_ROOT_DIR}/lib" NO_DEFAULT_PATH)
            find_library(_gst_crypto_lib_${CMAKE_ANDROID_ARCH_ABI} crypto
                HINTS "${GStreamer_ROOT_DIR}/lib" NO_DEFAULT_PATH)
            if(_gst_ssl_lib_${CMAKE_ANDROID_ARCH_ABI} AND _gst_crypto_lib_${CMAKE_ANDROID_ARCH_ABI})
                target_link_libraries(GStreamer::gio_modules INTERFACE
                    "${_gst_ssl_lib_${CMAKE_ANDROID_ARCH_ABI}}" "${_gst_crypto_lib_${CMAKE_ANDROID_ARCH_ABI}}")
            else()
                message(WARNING "GStreamerMobile: ssl/crypto not found under ${GStreamer_ROOT_DIR}/lib; "
                    "falling back to NDK sysroot link names (may be wrong/absent)")
                target_link_libraries(GStreamer::gio_modules INTERFACE ssl crypto)
            endif()
        endif()

        target_link_libraries(GStreamerMobile INTERFACE GStreamer::gio_modules)
    endif()
    set(GStreamerMobile_mobile_FOUND TRUE)
endif()

set(GStreamerMobile_FIND_COMPONENTS ${_gst_validate_components})

set(_gst_staged_assets)
if(fonts IN_LIST GStreamerMobile_FIND_COMPONENTS)
    set(GStreamer_UBUNTU_R_TTF "${GStreamer_NDK_BUILD_PATH}/fontconfig/fonts/Ubuntu-R.ttf"
        CACHE FILEPATH "Path to Ubuntu-R.ttf")
    set(GStreamer_FONTS_CONF "${GStreamer_NDK_BUILD_PATH}/fontconfig/fonts.conf"
        CACHE FILEPATH "Path to fonts.conf")
    if(EXISTS "${GStreamer_UBUNTU_R_TTF}" AND EXISTS "${GStreamer_FONTS_CONF}")
        set(GStreamerMobile_fonts_FOUND ON)
        list(APPEND _gst_staged_assets
            "${GStreamer_UBUNTU_R_TTF}|fontconfig/fonts/truetype/Ubuntu-R.ttf"
            "${GStreamer_FONTS_CONF}|fontconfig/fonts.conf"
        )
    else()
        set(GStreamerMobile_fonts_FOUND OFF)
    endif()
endif()

if(ca_certificates IN_LIST GStreamerMobile_FIND_COMPONENTS)
    set(GStreamer_CA_BUNDLE "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt"
        CACHE FILEPATH "Path to ca-certificates bundle")
    if(EXISTS "${GStreamer_CA_BUNDLE}")
        set(GStreamerMobile_ca_certificates_FOUND ON)
        list(APPEND _gst_staged_assets "${GStreamer_CA_BUNDLE}|ssl/certs/ca-certificates.crt")
    else()
        set(GStreamerMobile_ca_certificates_FOUND OFF)
    endif()
endif()

_qgc_android_stage_files(JAVA ${_gst_staged_java} ASSETS ${_gst_staged_assets})

include(FindPackageHandleStandardArgs)
set(_gst_plugins ${GStreamerMobile_FIND_COMPONENTS})
list(REMOVE_ITEM _gst_plugins fonts ca_certificates mobile)
list(REMOVE_DUPLICATES _gst_plugins)
foreach(_gst_PLUGIN IN LISTS _gst_plugins)
    set(GStreamerMobile_${_gst_PLUGIN}_FOUND "${GStreamer_${_gst_PLUGIN}_FOUND}")
endforeach()
if(GSTREAMER_IS_MOBILE AND TARGET GStreamerMobile)
    _qgc_android_whole_archive_plugins(${_gst_mobile_plugins})

    # Each plugin target carries its whole-archived .a plus the GStreamer helper libraries the plugin references
    # (gstbase/rtp/rtpbase/app/video/audio/tag/pbutils/net/sdp/rtsp/controller), which are absent from
    # PC_GStreamer_LIBRARIES. The NDK's lld resolves archives regardless of their order, so no link group is needed.
    set(_gst_mobile_helper_targets)
    foreach(_gst_comp IN LISTS _gst_mobile_plugins)
        list(APPEND _gst_mobile_helper_targets GStreamer::${_gst_comp})
    endforeach()
    foreach(_gst_comp IN LISTS _gst_mobile_apis)
        if(GStreamer_${_gst_comp}_FOUND AND TARGET GStreamer::${_gst_comp})
            list(APPEND _gst_mobile_helper_targets GStreamer::${_gst_comp})
        endif()
    endforeach()
    target_link_libraries(GStreamerMobile INTERFACE ${_gst_mobile_helper_targets})

    # GIO core (g_tls_*) for the static-plugin TLS shim; gioopenssl is a GIO
    # module and does not pull gio core, so add it explicitly.
    _gst_resolve_and_link_libraries(GStreamerMobile INTERFACE "gio-2.0" "${GStreamer_ROOT_DIR}/lib" WARN_MISSING)

    if(GStreamer_DEBUG)
        get_target_property(_dbg_deps_libs GStreamer::deps INTERFACE_LINK_LIBRARIES)
        message(STATUS "[GstMobile] GStreamer::deps INTERFACE_LINK_LIBRARIES = ${_dbg_deps_libs}")
        message(STATUS "[GstMobile] Helper component targets: ${_gst_mobile_helper_targets}")
    endif()

    target_link_libraries(GStreamerMobile INTERFACE GStreamer::deps)
endif()
set(GStreamerMobile_FOUND TRUE)

endmacro()
