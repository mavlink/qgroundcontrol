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
#   - functions instead of a find module: the requested plugins come from GSTREAMER_PLUGINS, and the component
#     bookkeeping (GStreamerMobile_FIND_COMPONENTS, *_FOUND) and the legacy GSTREAMER_* input variables are dropped;
#   - SDK discovery and extraction (above the mobile target) are QGC's own.

include_guard(GLOBAL)

# Single source of truth for Android ABI → cerbero dir + textrel flag.
# Sets, in PARENT_SCOPE: <prefix>_DIR, <prefix>_NEEDS_TEXTREL_ERROR.
# cmake-lint: disable=C0103
function(_qgc_android_abi_info abi prefix)
    if(abi STREQUAL "armeabi-v7a")
        set(_dir "armv7")
        set(_textrel TRUE)
    elseif(abi STREQUAL "arm64-v8a")
        set(_dir "arm64")
        set(_textrel FALSE)
    elseif(abi STREQUAL "x86")
        set(_dir "x86")
        set(_textrel TRUE)
    elseif(abi STREQUAL "x86_64")
        set(_dir "x86_64")
        set(_textrel FALSE)
    else()
        message(FATAL_ERROR "Unsupported Android ABI: ${abi}")
    endif()
    set(${prefix}_DIR
        "${_dir}"
        PARENT_SCOPE
    )
    set(${prefix}_NEEDS_TEXTREL_ERROR
        "${_textrel}"
        PARENT_SCOPE
    )
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

# _qgc_discover_android_sdk()
# Uses the SDK under GStreamer_ROOT_DIR, else extracts the CMAKE_ANDROID_ARCH_ABI tree of the downloaded universal SDK,
# and points pkg-config at it. Sets GStreamer_ROOT_DIR, GStreamer_AUTO_DOWNLOADED and
# GSTREAMER_{LIB,PLUGIN,INCLUDE}_PATH; on a Windows host also the SDK's PKG_CONFIG_EXECUTABLE and PKG_CONFIG_ARGN,
# while a macOS host uses Homebrew's pkg-config (cached PKG_CONFIG_EXECUTABLE).
# cmakelang 0.6.13 predates return(PROPAGATE).
# cmake-lint: disable=C0103,E1121
function(_qgc_discover_android_sdk)
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

    gstreamer_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}" TYPE STATIC_TARBALL)

    set(_outputs GStreamer_ROOT_DIR GStreamer_AUTO_DOWNLOADED GSTREAMER_LIB_PATH GSTREAMER_PLUGIN_PATH
                 GSTREAMER_INCLUDE_PATH
    )
    if(CMAKE_HOST_WIN32)
        gstreamer_apply_pkgconfig_env(
            MODE
            SDK
            PKG_CONFIG_EXE
            "${GStreamer_ROOT_DIR}/share/gst-android/ndk-build/tools/windows/pkg-config.exe"
            LIBDIR
            "${GSTREAMER_LIB_PATH}/pkgconfig"
            "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
            DONT_DEFINE_PREFIX
        )
        list(APPEND _outputs PKG_CONFIG_EXECUTABLE PKG_CONFIG_ARGN)
    elseif(CMAKE_HOST_UNIX)
        if(CMAKE_HOST_APPLE)
            _qgc_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
        endif()
        gstreamer_apply_pkgconfig_env(MODE SDK LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig"
                                      "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    endif()
    return(PROPAGATE ${_outputs})
endfunction()

# _qgc_android_find_plugin_archives(<out_var>)
# Sets <out_var> to the GSTREAMER_PLUGINS, in order, that have a pkg-config target and a static archive in the SDK; the
# archive of each is cached as _gst_plugin_lib_<abi>_<plugin>. Orchestrator.cmake reports the requested plugins left
# out.
function(_qgc_android_find_plugin_archives out_var)
    set(_plugins ${GSTREAMER_PLUGINS})
    list(REMOVE_DUPLICATES _plugins)
    if(GStreamer_DEBUG)
        message(STATUS "[GstMobile] Requested plugins: ${_plugins}")
    endif()

    _gst_save_find_suffixes()
    set(_found "")
    foreach(_p IN LISTS _plugins)
        if(GStreamer_${_p}_FOUND)
            find_library(
                _gst_plugin_lib_${CMAKE_ANDROID_ARCH_ABI}_${_p} gst${_p}
                HINTS "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0"
                NO_DEFAULT_PATH
            )
            if(_gst_plugin_lib_${CMAKE_ANDROID_ARCH_ABI}_${_p})
                list(APPEND _found ${_p})
            endif()
        endif()
    endforeach()
    _gst_restore_find_suffixes()

    if(GStreamer_DEBUG)
        message(STATUS "[GstMobile] Found plugins with .a: ${_found}")
        foreach(_dbg_p IN LISTS _found)
            message(STATUS "[GstMobile]   ${_dbg_p} -> ${_gst_plugin_lib_${CMAKE_ANDROID_ARCH_ABI}_${_dbg_p}}")
        endforeach()
    endif()
    set(${out_var}
        "${_found}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_android_whole_archive_plugins(<plugin>...)
# Links each plugin's static archive (found by _qgc_android_find_plugin_archives) as
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
# Copies the plugin Java sources under QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR and the assets under
# QGC_ANDROID_EXTRA_ASSETS_DIR when the app builds; android/build.gradle adds both build-directory trees to the APK
# (androiddeployqt deletes files written into android-build/ itself). Earlier builds copied these files into
# QGC_ANDROID_PACKAGE_SOURCE_DIR, where they would now duplicate the staged copies, so those copies are removed.
# cmake-lint: disable=C0113
function(_qgc_android_stage_files)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "" "JAVA;ASSETS")
    if(NOT QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR OR NOT QGC_ANDROID_EXTRA_ASSETS_DIR)
        message(FATAL_ERROR "GStreamer: QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR and QGC_ANDROID_EXTRA_ASSETS_DIR must be "
                            "set (cmake/platform/Android.cmake) before GStreamer is configured."
        )
    endif()
    set(_outputs "")
    set(_legacy_copies "")
    foreach(_kind IN ITEMS JAVA ASSETS)
        if(_kind STREQUAL "JAVA")
            set(_root "${QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR}")
            set(_legacy_root "${QGC_ANDROID_PACKAGE_SOURCE_DIR}/src")
        else()
            set(_root "${QGC_ANDROID_EXTRA_ASSETS_DIR}")
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

# _qgc_android_link_gio_modules(<target>)
# Links the static GIO modules named in G_IO_MODULES, plus OpenSSL for gioopenssl, into <target> through
# GStreamer::gio_modules.
function(_qgc_android_link_gio_modules target)
    list(TRANSFORM G_IO_MODULES PREPEND "gio" OUTPUT_VARIABLE _gio_module_libs)
    if(NOT _gio_module_libs)
        return()
    endif()
    set(_gio_modules_path "${GStreamer_ROOT_DIR}/lib/gio/modules")
    add_library(GStreamer::gio_modules INTERFACE IMPORTED)

    _gst_save_find_suffixes()
    foreach(_gio_lib IN LISTS _gio_module_libs)
        string(MAKE_C_IDENTIFIER "_gst_${CMAKE_ANDROID_ARCH_ABI}_${_gio_lib}" _gio_cache_var)
        if(NOT ${_gio_cache_var})
            find_library(
                ${_gio_cache_var}
                NAMES ${_gio_lib}
                HINTS ${_gio_modules_path}
                NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
            )
        endif()
        if(${_gio_cache_var})
            target_link_libraries(GStreamer::gio_modules INTERFACE "${${_gio_cache_var}}")
        else()
            message(WARNING "GStreamerMobile: GIO module '${_gio_lib}' not found in ${_gio_modules_path}")
        endif()
    endforeach()
    _gst_restore_find_suffixes()

    if("openssl" IN_LIST G_IO_MODULES)
        target_link_directories(GStreamer::gio_modules INTERFACE "${GStreamer_ROOT_DIR}/lib"
                                "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0"
        )
        set(_ssl _gst_ssl_lib_${CMAKE_ANDROID_ARCH_ABI})
        set(_crypto _gst_crypto_lib_${CMAKE_ANDROID_ARCH_ABI})
        find_library(
            ${_ssl} ssl
            HINTS "${GStreamer_ROOT_DIR}/lib"
            NO_DEFAULT_PATH
        )
        find_library(
            ${_crypto} crypto
            HINTS "${GStreamer_ROOT_DIR}/lib"
            NO_DEFAULT_PATH
        )
        if(${_ssl} AND ${_crypto})
            target_link_libraries(GStreamer::gio_modules INTERFACE "${${_ssl}}" "${${_crypto}}")
        else()
            message(WARNING "GStreamerMobile: ssl/crypto not found under ${GStreamer_ROOT_DIR}/lib; "
                            "falling back to NDK sysroot link names (may be wrong/absent)"
            )
            target_link_libraries(GStreamer::gio_modules INTERFACE ssl crypto)
        endif()
    endif()

    target_link_libraries(${target} INTERFACE GStreamer::gio_modules)
endfunction()

# _qgc_android_stage_sdk_files(<ndk_build_dir> <plugin>...)
# Stages the Java sources of the <plugin>s and the SDK's fonts and CA bundle (_qgc_android_stage_files).
function(_qgc_android_stage_sdk_files ndk_build)
    set(_gst_staged_java)
    foreach(_gst_java_plugin IN LISTS ARGN)
        file(
            GLOB_RECURSE _gst_java_files FOLLOW_SYMLINKS CONFIGURE_DEPENDS
            RELATIVE "${ndk_build}"
            "${ndk_build}/${_gst_java_plugin}/*.java"
        )
        foreach(_gst_java_file IN LISTS _gst_java_files)
            list(APPEND _gst_staged_java "${ndk_build}/${_gst_java_file}|org/freedesktop/gstreamer/${_gst_java_file}")
        endforeach()
    endforeach()

    set(_gst_staged_assets)
    if(EXISTS "${ndk_build}/fontconfig")
        set(GStreamer_UBUNTU_R_TTF
            "${ndk_build}/fontconfig/fonts/Ubuntu-R.ttf"
            CACHE FILEPATH "Path to Ubuntu-R.ttf"
        )
        set(GStreamer_FONTS_CONF
            "${ndk_build}/fontconfig/fonts.conf"
            CACHE FILEPATH "Path to fonts.conf"
        )
        if(EXISTS "${GStreamer_UBUNTU_R_TTF}" AND EXISTS "${GStreamer_FONTS_CONF}")
            list(APPEND _gst_staged_assets "${GStreamer_UBUNTU_R_TTF}|fontconfig/fonts/truetype/Ubuntu-R.ttf"
                 "${GStreamer_FONTS_CONF}|fontconfig/fonts.conf"
            )
        endif()
    endif()

    if(EXISTS "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt")
        set(GStreamer_CA_BUNDLE
            "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt"
            CACHE FILEPATH "Path to ca-certificates bundle"
        )
        if(EXISTS "${GStreamer_CA_BUNDLE}")
            list(APPEND _gst_staged_assets "${GStreamer_CA_BUNDLE}|ssl/certs/ca-certificates.crt")
        endif()
    endif()

    _qgc_android_stage_files(JAVA ${_gst_staged_java} ASSETS ${_gst_staged_assets})
endfunction()

# _qgc_create_android_mobile_target()
# Creates GStreamer::mobile, an INTERFACE target that links the plugin archives, their helper libraries and the GIO
# modules into the application, and stages the plugin Java sources and the fonts/CA assets for the APK. Sets
# GSTREAMER_STATIC_PLUGINS, the plugins the application registers (qgc_gst_generate_static_init), in the caller's scope.
function(_qgc_create_android_mobile_target)
    if(NOT GStreamer_USE_STATIC_LIBS)
        message(FATAL_ERROR "Shared library GStreamer is not supported on mobile platforms")
    endif()
    if(NOT TARGET GStreamer::GStreamer)
        message(FATAL_ERROR "qgc_gst_create_pkgconfig_targets() must run before _qgc_create_android_mobile_target().")
    endif()
    set(_ndk_build "${GStreamer_ROOT_DIR}/share/gst-android/ndk-build")
    # GStreamer.java is not consumed (init runs in C++); used only as a stable
    # SDK-layout anchor present regardless of plugin selection.
    if(NOT EXISTS "${_ndk_build}/GStreamer.java")
        message(FATAL_ERROR "GStreamer Android SDK not found at ${_ndk_build} "
                            "(expected ndk-build layout anchor GStreamer.java is missing). "
                            "Verify GStreamer Android SDK installation."
        )
    endif()

    _qgc_android_find_plugin_archives(_plugins)
    if(GStreamer_DEBUG)
        message(STATUS "[GstMobile] GStreamer_ROOT_DIR = ${GStreamer_ROOT_DIR}")
        message(STATUS "[GstMobile] GSTREAMER_LIB_PATH = ${GSTREAMER_LIB_PATH}")
    endif()

    # Single-core model: GStreamer::mobile is an INTERFACE target that whole-archives the plugin .a's +
    # helpers + GIO modules into the app, which carries the one static core (GStreamer::GStreamer). A
    # separate .so would carry a duplicate GstElement core and fail g_type_is_a for every plugin.
    # Do NOT re-link the core here — that produced the duplicate-GstElement-core failure.
    add_library(GStreamerMobile INTERFACE IMPORTED GLOBAL)
    add_library(GStreamer::mobile ALIAS GStreamerMobile)
    target_include_directories(GStreamerMobile
                               INTERFACE $<TARGET_PROPERTY:GStreamer::GStreamer,INTERFACE_INCLUDE_DIRECTORIES>
    )
    _qgc_android_link_gio_modules(GStreamerMobile)
    _qgc_android_stage_sdk_files("${_ndk_build}" ${_plugins})
    _qgc_android_whole_archive_plugins(${_plugins})

    # Each plugin target carries its whole-archived .a plus the GStreamer helper libraries the plugin references
    # (gstbase/rtp/rtpbase/app/video/audio/tag/pbutils/net/sdp/rtsp/controller), which are absent from
    # PC_GStreamer_LIBRARIES. The NDK's lld resolves archives regardless of their order, so no link group is needed.
    list(TRANSFORM _plugins PREPEND "GStreamer::" OUTPUT_VARIABLE _plugin_targets)
    target_link_libraries(GStreamerMobile INTERFACE ${_plugin_targets})

    # GIO core (g_tls_*) for the static-plugin TLS shim; gioopenssl is a GIO
    # module and does not pull gio core, so add it explicitly.
    _gst_resolve_and_link_libraries(GStreamerMobile INTERFACE "gio-2.0" "${GStreamer_ROOT_DIR}/lib" WARN_MISSING)

    if(GStreamer_DEBUG)
        get_target_property(_dbg_deps_libs GStreamer::deps INTERFACE_LINK_LIBRARIES)
        message(STATUS "[GstMobile] GStreamer::deps INTERFACE_LINK_LIBRARIES = ${_dbg_deps_libs}")
        message(STATUS "[GstMobile] Helper component targets: ${_plugin_targets}")
    endif()

    target_link_libraries(GStreamerMobile INTERFACE GStreamer::deps)
    set(GSTREAMER_STATIC_PLUGINS
        "${_plugins}"
        PARENT_SCOPE
    )
endfunction()
