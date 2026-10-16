# SPDX-FileCopyrightText: 2024 L. E. Segovia <amy@centricular.com>
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Android GStreamer SDK discovery — invoked by Orchestrator.cmake.
#
# The mobile target (_qgc_gst_create_android_mobile_target and its helpers) is derived from FindGStreamerMobile.cmake in
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
function(_qgc_gst_android_abi_info abi prefix)
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

# _qgc_gst_android_sdk_pkg_config(<root> <host> <out_var>)
# Sets <out_var> to the pkg-config the Android SDK at <root> bundles for the build <host> (windows or macos; cerbero's
# pkg-config-dist recipe puts them in share/gst-android/ndk-build/tools/<host>/), else to "": a Linux host uses its
# own, and a macOS host Homebrew's when the SDK has none for it.
function(_qgc_gst_android_sdk_pkg_config root host out_var)
    set(_tools "${root}/share/gst-android/ndk-build/tools")
    set(_pkg_config "")
    if(host STREQUAL "windows")
        set(_pkg_config "${_tools}/windows/pkg-config.exe")
    elseif(host STREQUAL "macos" AND EXISTS "${_tools}/macos/pkg-config")
        set(_pkg_config "${_tools}/macos/pkg-config")
    endif()
    set(${out_var}
        "${_pkg_config}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_discover_android_sdk()
# Uses the SDK under GStreamer_ROOT_DIR, else extracts the CMAKE_ANDROID_ARCH_ABI tree of the downloaded universal SDK.
# Windows and macOS hosts use the pkg-config the SDK bundles for them (_qgc_gst_android_sdk_pkg_config).
# cmake-lint: disable=C0103
function(_qgc_gst_discover_android_sdk)
    if(NOT GStreamer_ROOT_DIR)
        _qgc_gst_android_abi_info("${CMAKE_ANDROID_ARCH_ABI}" _gst_android_abi)
        qgc_gst_acquire_sdk(
            PLATFORM
            android
            VERSION
            ${QGC_GST_REQUESTED_VERSION}
            CACHE_SUBDIR
            "gstreamer-android-${QGC_GST_REQUESTED_VERSION}"
            ARCHIVES
            "gstreamer-android-${QGC_GST_REQUESTED_VERSION}.tar.xz"
            CONTENT
            ${_gst_android_abi_DIR}
            EXTRACT_FUNCTION
            _qgc_gst_android_extract
            CONTENT_DIR_OUT
            _gst_android_sdk
        )
        set(GStreamer_ROOT_DIR "${_gst_android_sdk}/${_gst_android_abi_DIR}")
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    gstreamer_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}")

    if(CMAKE_HOST_WIN32)
        set(_host windows)
    elseif(CMAKE_HOST_APPLE)
        set(_host macos)
    else()
        set(_host linux)
    endif()
    _qgc_gst_android_sdk_pkg_config("${GStreamer_ROOT_DIR}" ${_host} _sdk_pkg_config)
    set(_pkg_config_args "")
    if(_sdk_pkg_config)
        list(APPEND _pkg_config_args PKG_CONFIG_EXE "${_sdk_pkg_config}")
    elseif(CMAKE_HOST_APPLE)
        _qgc_gst_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
    endif()
    # The Windows pkg-config redefines prefix from each .pc file's location by default.
    if(CMAKE_HOST_WIN32)
        list(APPEND _pkg_config_args DONT_DEFINE_PREFIX)
    endif()
    gstreamer_apply_pkgconfig_env(MODE SDK ${_pkg_config_args} LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig"
                                  "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
    )
    _qgc_gst_return_discovery()
endfunction()

# _qgc_gst_android_find_plugin_archives(<out_var>)
# Sets <out_var> to the GSTREAMER_PLUGINS, in order, that have a pkg-config target and a static archive in the SDK.
# Orchestrator.cmake reports the requested plugins left out.
function(_qgc_gst_android_find_plugin_archives out_var)
    set(_found "")
    foreach(_plugin IN LISTS GSTREAMER_PLUGINS)
        if(TARGET GStreamer::${_plugin} AND EXISTS "${GSTREAMER_PLUGIN_PATH}/libgst${_plugin}.a")
            list(APPEND _found ${_plugin})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _found)
    if(GStreamer_DEBUG)
        message(STATUS "GStreamer::mobile: requested plugins ${GSTREAMER_PLUGINS}, with an archive: ${_found}")
    endif()
    set(${out_var}
        "${_found}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_android_whole_archive_plugins(<plugin>...)
# Links each plugin's static archive in GSTREAMER_PLUGIN_PATH as
# $<LINK_LIBRARY:WHOLE_ARCHIVE,GStreamer::<plugin>_archive> wherever GStreamer::<plugin> is linked, so every
# gst_plugin_<plugin>_register() and its GType objects survive the linker's dead-strip and register into the
# app's single core. GStreamer::<plugin> must not also link the archive by path: a plain occurrence ahead of the
# whole-archive one would let the linker load some members twice.
function(_qgc_gst_android_whole_archive_plugins)
    foreach(_plugin IN LISTS ARGN)
        set(_archive "${GSTREAMER_PLUGIN_PATH}/libgst${_plugin}.a")
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

# _qgc_gst_android_link_gio_modules(<target>)
# Links the static GIO modules named in G_IO_MODULES, plus OpenSSL for gioopenssl, into <target> through
# GStreamer::gio_modules.
function(_qgc_gst_android_link_gio_modules target)
    if(NOT G_IO_MODULES)
        return()
    endif()
    add_library(GStreamer::gio_modules INTERFACE IMPORTED)
    foreach(_module IN LISTS G_IO_MODULES)
        set(_archive "${GStreamer_ROOT_DIR}/lib/gio/modules/libgio${_module}.a")
        if(EXISTS "${_archive}")
            target_link_libraries(GStreamer::gio_modules INTERFACE "${_archive}")
        else()
            message(WARNING "GStreamer::mobile: GIO module archive ${_archive} not found")
        endif()
    endforeach()

    if("openssl" IN_LIST G_IO_MODULES)
        target_link_directories(GStreamer::gio_modules INTERFACE "${GStreamer_ROOT_DIR}/lib"
                                "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0"
        )
        if(EXISTS "${GStreamer_ROOT_DIR}/lib/libssl.a" AND EXISTS "${GStreamer_ROOT_DIR}/lib/libcrypto.a")
            target_link_libraries(GStreamer::gio_modules INTERFACE "${GStreamer_ROOT_DIR}/lib/libssl.a"
                                                                   "${GStreamer_ROOT_DIR}/lib/libcrypto.a"
            )
        else()
            message(WARNING "GStreamer::mobile: ssl/crypto not found under ${GStreamer_ROOT_DIR}/lib; "
                            "falling back to NDK sysroot link names (may be wrong/absent)"
            )
            target_link_libraries(GStreamer::gio_modules INTERFACE ssl crypto)
        endif()
    endif()

    target_link_libraries(${target} INTERFACE GStreamer::gio_modules)
endfunction()

# _qgc_gst_android_stage_sdk_files(<ndk_build_dir> <plugin>...)
# Copies the <plugin>s' Java sources under QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR and the SDK's fonts and CA bundle under
# QGC_ANDROID_EXTRA_ASSETS_DIR at configure time, like src/Android/CMakeLists.txt does for its Java sources;
# android/build.gradle adds both build-directory trees to the APK (androiddeployqt deletes files written into
# android-build/ itself). Removes stale copies under QGC_ANDROID_PACKAGE_SOURCE_DIR, which Gradle would package twice.
# cmake-lint: disable=C0113
function(_qgc_gst_android_stage_sdk_files ndk_build)
    if(NOT QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR OR NOT QGC_ANDROID_EXTRA_ASSETS_DIR)
        message(FATAL_ERROR "GStreamer: QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR and QGC_ANDROID_EXTRA_ASSETS_DIR must be "
                            "set (cmake/platform/Android.cmake) before GStreamer is configured."
        )
    endif()

    # <source>|<staging directory>|<directory under QGC_ANDROID_PACKAGE_SOURCE_DIR>|<path under both>
    set(_java_dirs "${QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR}|src")
    set(_asset_dirs "${QGC_ANDROID_EXTRA_ASSETS_DIR}|assets")
    set(_files "")
    foreach(_plugin IN LISTS ARGN)
        file(
            GLOB_RECURSE _java_files FOLLOW_SYMLINKS CONFIGURE_DEPENDS
            RELATIVE "${ndk_build}"
            "${ndk_build}/${_plugin}/*.java"
        )
        foreach(_java_file IN LISTS _java_files)
            list(APPEND _files "${ndk_build}/${_java_file}|${_java_dirs}|org/freedesktop/gstreamer/${_java_file}")
        endforeach()
    endforeach()
    set(_fonts "${ndk_build}/fontconfig")
    if(EXISTS "${_fonts}/fonts/Ubuntu-R.ttf" AND EXISTS "${_fonts}/fonts.conf")
        list(APPEND _files "${_fonts}/fonts/Ubuntu-R.ttf|${_asset_dirs}|fontconfig/fonts/truetype/Ubuntu-R.ttf"
             "${_fonts}/fonts.conf|${_asset_dirs}|fontconfig/fonts.conf"
        )
    endif()
    set(_ca_bundle "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt")
    if(EXISTS "${_ca_bundle}")
        list(APPEND _files "${_ca_bundle}|${_asset_dirs}|ssl/certs/ca-certificates.crt")
    endif()

    set(_legacy_copies "")
    foreach(_file IN LISTS _files)
        string(REPLACE "|" ";" _file "${_file}")
        list(GET _file 0 _source)
        list(GET _file 1 _staging_dir)
        list(GET _file 2 _package_dir)
        list(GET _file 3 _relative)
        set(_output "${_staging_dir}/${_relative}")
        cmake_path(GET _output PARENT_PATH _output_dir)
        file(MAKE_DIRECTORY "${_output_dir}")
        file(COPY_FILE "${_source}" "${_output}" ONLY_IF_DIFFERENT)
        set(_legacy_copy "${QGC_ANDROID_PACKAGE_SOURCE_DIR}/${_package_dir}/${_relative}")
        if(QGC_ANDROID_PACKAGE_SOURCE_DIR AND EXISTS "${_legacy_copy}")
            list(APPEND _legacy_copies "${_legacy_copy}")
        endif()
    endforeach()

    if(_legacy_copies)
        message(STATUS "GStreamer: removing files earlier builds copied into the Android package sources: "
                       "${_legacy_copies}"
        )
        file(REMOVE ${_legacy_copies})
    endif()
endfunction()

# _qgc_gst_create_android_mobile_target()
# Creates GStreamer::mobile, an INTERFACE target that links the plugin archives, their helper libraries and the GIO
# modules into the application, and stages the plugin Java sources and the fonts/CA assets for the APK. Sets
# GSTREAMER_STATIC_PLUGINS, the plugins the application registers (qgc_gst_generate_static_init), in the caller's scope.
function(_qgc_gst_create_android_mobile_target)
    if(NOT GStreamer_USE_STATIC_LIBS)
        message(FATAL_ERROR "Shared library GStreamer is not supported on mobile platforms")
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

    _qgc_gst_android_find_plugin_archives(_plugins)
    if(GStreamer_DEBUG)
        message(STATUS "GStreamer::mobile: GStreamer_ROOT_DIR = ${GStreamer_ROOT_DIR}")
        message(STATUS "GStreamer::mobile: GSTREAMER_LIB_PATH = ${GSTREAMER_LIB_PATH}")
    endif()

    # Single-core model: GStreamer::mobile is an INTERFACE target that whole-archives the plugin .a's +
    # helpers + GIO modules into the app, which carries the one static core (GStreamer::GStreamer). A
    # separate .so would carry a duplicate GstElement core and fail g_type_is_a for every plugin.
    add_library(GStreamerMobile INTERFACE IMPORTED GLOBAL)
    add_library(GStreamer::mobile ALIAS GStreamerMobile)
    target_include_directories(GStreamerMobile
                               INTERFACE $<TARGET_PROPERTY:GStreamer::GStreamer,INTERFACE_INCLUDE_DIRECTORIES>
    )
    _qgc_gst_android_link_gio_modules(GStreamerMobile)
    _qgc_gst_android_stage_sdk_files("${_ndk_build}" ${_plugins})
    _qgc_gst_android_whole_archive_plugins(${_plugins})

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
        message(STATUS "GStreamer::mobile: GStreamer::deps INTERFACE_LINK_LIBRARIES = ${_dbg_deps_libs}")
        message(STATUS "GStreamer::mobile: Helper component targets: ${_plugin_targets}")
    endif()

    target_link_libraries(GStreamerMobile INTERFACE GStreamer::deps)
    set(GSTREAMER_STATIC_PLUGINS
        "${_plugins}"
        PARENT_SCOPE
    )
endfunction()
