# SPDX-FileCopyrightText: 2024 L. E. Segovia <amy@centricular.com>
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Android GStreamer SDK discovery — invoked by Orchestrator.cmake.
#
# GStreamer::mobile (_qgc_gst_create_android_mobile_target and its helpers) is modelled on FindGStreamerMobile.cmake in
# cerbero (https://gitlab.freedesktop.org/gstreamer/cerbero, data/mobile/FindGStreamerMobile.cmake), which the Android
# SDK installs as share/cmake/FindGStreamerMobile.cmake: a SHARED libgstreamer_android.so holding GStreamer, the
# requested plugins and GIO modules and the generated static-plugin registration, which the app links instead of the
# SDK's archives. That module is not used as is because:
#   - it whole-archives libgstreamer-1.0.a, whose gstandroid.c.o brings a JNI_OnLoad that fails the library's load
#     unless the APK ships org.freedesktop.gstreamer.GStreamer, and whose JNI state is set only by GStreamer.init(),
#     which also runs gst_init() on the Java thread with its own environment and log handlers. QGC initializes
#     GStreamer itself (GStreamer.cc), so the core is linked like any archive and gst_android_jni.c
#     (src/VideoManager/GStreamer/Platform) provides the JNI state from the app instead;
#   - it registers every plugin it is given, while QGC registers only the allowlisted plugins that have a static
#     archive (failing configure when a bundle-required one is missing), with its own registration template
#     (Mobile.cmake);
#   - it copies the plugin Java sources and fonts/CA assets into the source tree, while QGC stages them in the build
#     directory for Gradle;
#   - its static-library and SDK discovery come from the SDK's FindGStreamer.cmake, while QGC's are its own
#     (Targets.cmake, Acquire.cmake).

include_guard(GLOBAL)

# Single source of truth for Android ABI → cerbero dir, textrel flag and 16 KB page alignment.
# Sets, in PARENT_SCOPE: <prefix>_DIR, <prefix>_NEEDS_TEXTREL_ERROR, <prefix>_16KB_PAGES.
# <prefix>_16KB_PAGES follows Qt's android_16kb_pages feature (the 64-bit ABIs), for which Qt::Platform links every
# Qt-linked target with -z max-page-size=16384.
# cmake-lint: disable=C0103
function(_qgc_gst_android_abi_info abi prefix)
    if(abi STREQUAL "armeabi-v7a")
        set(_dir "armv7")
        set(_textrel TRUE)
        set(_16kb_pages FALSE)
    elseif(abi STREQUAL "arm64-v8a")
        set(_dir "arm64")
        set(_textrel FALSE)
        set(_16kb_pages TRUE)
    elseif(abi STREQUAL "x86")
        set(_dir "x86")
        set(_textrel TRUE)
        set(_16kb_pages FALSE)
    elseif(abi STREQUAL "x86_64")
        set(_dir "x86_64")
        set(_textrel FALSE)
        set(_16kb_pages TRUE)
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
    set(${prefix}_16KB_PAGES
        "${_16kb_pages}"
        PARENT_SCOPE
    )
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
        _qgc_gst_space_free_download_cache()
        _qgc_gst_acquire_android_sdk(${_gst_android_abi_DIR} GStreamer_ROOT_DIR)
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    qgc_gst_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}")

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
    qgc_gst_apply_pkgconfig_env(
        MODE SDK ${_pkg_config_args} LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
                                            "${GSTREAMER_GIO_MODULE_PATH}/pkgconfig"
    )
    _qgc_gst_return_discovery()
endfunction()

# _qgc_gst_android_link_archive(<target> <name> <archive>)
# Links <archive> whole-archived into <target>, with the libraries GStreamer::<name> (its pkg-config module) links
# besides it: a plain occurrence of the archive ahead of the whole-archive one would let the linker load some members
# twice. Whole-archiving keeps every registration function and GType of a plugin, or a GIO module's loader.
function(_qgc_gst_android_link_archive target name archive)
    add_library(GStreamer::${name}_archive STATIC IMPORTED GLOBAL)
    set_target_properties(
        GStreamer::${name}_archive
        PROPERTIES
            IMPORTED_LOCATION "${archive}"
    )
    target_link_libraries(${target} PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,GStreamer::${name}_archive>")
    if(NOT TARGET PkgConfig::PC_GStreamer_${name})
        return()
    endif()
    cmake_path(GET archive FILENAME _archive_name)
    cmake_path(GET archive STEM _archive_stem)
    string(REGEX REPLACE "^lib" "" _archive_lib "${_archive_stem}")
    get_target_property(_libraries PkgConfig::PC_GStreamer_${name} INTERFACE_LINK_LIBRARIES)
    set(_others "")
    foreach(_library IN LISTS _libraries)
        cmake_path(GET _library FILENAME _library_name)
        if(NOT _library_name STREQUAL _archive_name AND NOT _library STREQUAL _archive_lib)
            list(APPEND _others "${_library}")
        endif()
    endforeach()
    set_target_properties(
        PkgConfig::PC_GStreamer_${name}
        PROPERTIES
            INTERFACE_LINK_LIBRARIES "${_others}"
    )
    target_link_libraries(${target} PRIVATE GStreamer::${name})
endfunction()

# _qgc_gst_android_hide_archives(<target>)
# Keeps the symbols of the SDK archives <target> links out of its exports, except GStreamer's and GLib's libraries,
# whose API QGC calls: the app must not bind to the copies of zlib, OpenSSL, FFmpeg and the like inside the library.
function(_qgc_gst_android_hide_archives target)
    get_target_property(_libraries ${target} LINK_LIBRARIES)
    _qgc_gst_link_closure(_items ${_libraries})
    set(_archives "")
    foreach(_item IN LISTS _items)
        cmake_path(GET _item FILENAME _name)
        if(_name MATCHES "\\.a$" AND NOT _name MATCHES
                                     "^lib(gst[a-z0-9]+-1\\.0|glib-2\\.0|gobject-2\\.0|gio-2\\.0|gmodule-2\\.0)\\.a$"
        )
            list(APPEND _archives "${_name}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _archives)
    # lld reads a colon-separated list; a comma would end the LINKER: argument.
    list(JOIN _archives ":" _archives)
    if(_archives)
        target_link_options(${target} PRIVATE "LINKER:--exclude-libs=${_archives}")
    endif()
endfunction()

# _qgc_gst_android_stage_sdk_files(<ndk_build_dir> <plugin>...)
# Copies the <plugin>s' Java sources under QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR and the SDK's fonts and CA bundle under
# QGC_ANDROID_EXTRA_ASSETS_DIR at configure time, like src/Android/CMakeLists.txt does for its Java sources;
# android/build.gradle adds both build-directory trees to the APK (androiddeployqt deletes files written into
# android-build/ itself). Removes stale copies under QGC_ANDROID_PACKAGE_SOURCE_DIR, which Gradle would package twice.
# cmake-lint: disable=C0113,E1126
function(_qgc_gst_android_stage_sdk_files ndk_build)
    if(NOT QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR OR NOT QGC_ANDROID_EXTRA_ASSETS_DIR)
        message(
            FATAL_ERROR "GStreamer: QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR and QGC_ANDROID_EXTRA_ASSETS_DIR must be "
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
        message(
            STATUS "GStreamer: removing files earlier builds copied into the Android package sources: "
                   "${_legacy_copies}"
        )
        file(REMOVE ${_legacy_copies})
    endif()
endfunction()

# _qgc_gst_android_usage_target()
# Creates GStreamer::GStreamer with the include directories and compile options of GStreamer::Core and the components
# and GStreamer::mobile: the app compiles against the SDK's headers but links only the library.
function(_qgc_gst_android_usage_target)
    add_library(GStreamer::GStreamer INTERFACE IMPORTED GLOBAL)
    set(_includes "")
    set(_options "")
    foreach(_component IN ITEMS Core ${GSTREAMER_SELECTED_COMPONENTS})
        if(TARGET GStreamer::${_component})
            list(APPEND _includes ${PC_GStreamer_${_component}_INCLUDE_DIRS})
            list(APPEND _options ${PC_GStreamer_${_component}_CFLAGS_OTHER})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _includes)
    list(REMOVE_DUPLICATES _options)
    set_target_properties(
        GStreamer::GStreamer
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_includes}"
            INTERFACE_COMPILE_OPTIONS "${_options}"
    )
    target_link_libraries(GStreamer::GStreamer INTERFACE GStreamer::mobile)
endfunction()

# _qgc_gst_create_android_mobile_target()
# Creates GStreamer::mobile (GStreamerMobile), the SHARED libgstreamer_android.so that links GStreamer::Static, the
# requested plugins that have a static archive and the G_IO_MODULES, registers those plugins and modules in its
# generated gst_init_static_plugins() and carries the JNI state GStreamer's Android plugins need (gst_android_jni.c).
# GStreamer::GStreamer carries the SDK's headers and links GStreamer::mobile (_qgc_gst_android_usage_target): the app
# takes every GStreamer function from that library, which keeps QGC_GST_APP_CORE_SYMBOLS for it. Also stages the plugin
# Java sources and the fonts/CA assets for the APK. Sets GSTREAMER_STATIC_PLUGINS, the plugins registered, in the
# caller's scope.
function(_qgc_gst_create_android_mobile_target)
    set(_ndk_build "${GStreamer_ROOT_DIR}/share/gst-android/ndk-build")
    # GStreamer.java (not used, see this file's header) anchors the ndk-build layout whatever the plugin selection.
    if(NOT EXISTS "${_ndk_build}/GStreamer.java")
        message(
            FATAL_ERROR
                "GStreamer Android SDK not found at ${_ndk_build} "
                "(expected ndk-build layout anchor GStreamer.java is missing). "
                "Verify GStreamer Android SDK installation."
        )
    endif()

    set(_plugins "")
    foreach(_plugin IN LISTS GSTREAMER_PLUGINS)
        if(NOT EXISTS "${GSTREAMER_PLUGIN_PATH}/libgst${_plugin}.a" OR _plugin IN_LIST _plugins)
            continue()
        endif()
        _qgc_gst_pkg_target(plugin_${_plugin} "gst${_plugin}" QUIET)
        if(TARGET GStreamer::plugin_${_plugin})
            list(APPEND _plugins ${_plugin})
        endif()
    endforeach()

    cmake_path(
        SET _jni_state NORMALIZE
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../../src/VideoManager/GStreamer/Platform/gst_android_jni.c"
    )
    qgc_gst_add_mobile_library(SOURCES "${_jni_state}" PLUGINS ${_plugins})
    set_target_properties(
        GStreamerMobile
        PROPERTIES
            LIBRARY_OUTPUT_NAME gstreamer_android
    )
    target_link_libraries(GStreamerMobile PRIVATE GStreamer::Static)
    foreach(_plugin IN LISTS _plugins)
        _qgc_gst_android_link_archive(GStreamerMobile plugin_${_plugin} "${GSTREAMER_PLUGIN_PATH}/libgst${_plugin}.a")
    endforeach()
    # A GIO module's .pc file names the directory the SDK was built in as its -L, so its archive is linked by path.
    foreach(_module IN LISTS G_IO_MODULES)
        set(_archive "${GSTREAMER_GIO_MODULE_PATH}/libgio${_module}.a")
        if(NOT EXISTS "${_archive}")
            message(FATAL_ERROR "GStreamer::mobile: GIO module archive ${_archive} not found")
        endif()
        _qgc_gst_pkg_target(gio_${_module} "gio${_module}" QUIET)
        _qgc_gst_android_link_archive(GStreamerMobile gio_${_module} "${_archive}")
    endforeach()

    # Cerbero's gstreamer-1.0.mk link flags: -Bsymbolic keeps FFmpeg's page-relative relocations valid (every ABI), -z
    # text hard-errors residual text relocations (armv7/x86).
    _qgc_gst_android_abi_info("${CMAKE_ANDROID_ARCH_ABI}" _abi)
    target_link_options(GStreamerMobile PRIVATE "LINKER:-Bsymbolic")
    if(_abi_NEEDS_TEXTREL_ERROR)
        target_link_options(GStreamerMobile PRIVATE "LINKER:-z,text")
    endif()
    # The library links no Qt target, so it does not get Qt::Platform's 16 KB alignment; a 4 KB-aligned library does
    # not load on a device with 16 KB pages, and Google Play rejects it.
    if(_abi_16KB_PAGES)
        target_link_options(GStreamerMobile PRIVATE "LINKER:-z,max-page-size=16384")
    endif()
    _qgc_gst_android_hide_archives(GStreamerMobile)

    _qgc_gst_android_usage_target()
    _qgc_gst_android_stage_sdk_files("${_ndk_build}" ${_plugins})
    set(GSTREAMER_STATIC_PLUGINS
        "${_plugins}"
        PARENT_SCOPE
    )
endfunction()
