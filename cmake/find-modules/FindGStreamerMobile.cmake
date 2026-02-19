# SPDX-FileCopyrightText: 2024 L. E. Segovia <amy@centricular.com>
# SPDX-License-Identifier: LGPL-2.1-or-later

#[=======================================================================[.rst:
FindGStreamerMobile
-------

Creates additional mobile targets to install fonts and the CA certificate
bundle. Android and iOS only.

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`INTERFACE` targets:

``GStreamer::fonts``
  A target that will install GStreamer's default fonts into the app.

``GStreamer::ca_certificates``
  A target that will install the NSS CA certificate bundle into the app.

This module defines the following :prop_tgt:`SHARED` targets:

``GStreamer::mobile``
  A target that will build the shared library consisting of GStreamer plus all the selected plugin components. (Android/iOS only)

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``GStreamerMobile_FOUND``
  ON if the system has the GStreamer library.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``GStreamer_CA_BUNDLE``
  Path to /etc/ssl/certs/ca-certificates.crt.
``GStreamer_UBUNTU_R_TTF``
  Path to the TrueType font Ubuntu R.
``GStreamer_FONTS_CONF``
  Path to /etc/fonts.conf.

Configuration Variables
^^^^^^^^^^^^^^^

Like with the main GStreamer library, setting the following variables is
required, depending on the operating system:

``GStreamer_ROOT_DIR``
  Installation prefix of the GStreamer SDK.

``GStreamer_JAVA_SRC_DIR``
  Target directory for deploying the selected plugins' Java classfiles to. (Android only)

``GStreamer_Mobile_MODULE_NAME``
  Name for the GStreamer::mobile shared library. Default is ``gstreamer_android`` (Android) or ``gstreamer_mobile`` (iOS).

``GStreamer_ASSETS_DIR``
  Target directory for deploying assets to.

``G_IO_MODULES``
  Set this to the GIO modules you need, additional to any GStreamer plugins. (Usually set to ``gnutls`` or ``openssl``)

``G_IO_MODULES_PATH``
  Path for the static GIO modules.

#]=======================================================================]

if (GStreamerMobile_FOUND)
    return()
endif()

#####################
#  Setup variables  #
#####################

if (NOT DEFINED GStreamer_ROOT_DIR AND DEFINED GSTREAMER_ROOT)
    set(GStreamer_ROOT_DIR "${GSTREAMER_ROOT}")
endif()

if (NOT GStreamer_ROOT_DIR)
    message(FATAL_ERROR "GStreamer_ROOT_DIR is not set. Set it to the GStreamer SDK root or let FindGStreamerQGC auto-download.")
endif()

if (NOT EXISTS "${GStreamer_ROOT_DIR}")
    message(FATAL_ERROR "The directory GStreamer_ROOT_DIR=${GStreamer_ROOT_DIR} does not exist")
endif()

# These variables are defined in FindGStreamer.cmake (which runs before us in the
# orchestrated flow). Provide fallbacks for robustness / standalone use.
if(NOT DEFINED _gst_IGNORED_SYSTEM_LIBRARIES)
    set(_gst_IGNORED_SYSTEM_LIBRARIES c c++ unwind m dl atomic)
    if(ANDROID)
        list(APPEND _gst_IGNORED_SYSTEM_LIBRARIES log GLESv2 EGL OpenSLES android vulkan)
    elseif(APPLE)
        list(APPEND _gst_IGNORED_SYSTEM_LIBRARIES iconv resolv System)
    endif()
endif()
if(NOT DEFINED _gst_SRT_REGEX_PATCH)
    set(_gst_SRT_REGEX_PATCH "^:lib(.+)\\.(a|so|lib|dylib)$")
endif()

# Note: Extra deps (gio-2.0, zlib, gmodule-2.0) are added in FindGStreamerQGC.cmake
# before FindGStreamer runs, so they are resolved by pkg-config.

# Prepare Android linker fixes for x264/ffmpeg
if(CMAKE_ANDROID_ARCH_ABI MATCHES "^armeabi")
    set(_GST_MOBILE_NEEDS_TEXTREL_ERROR TRUE)
    set(_GST_MOBILE_NEEDS_BSYMBOLIC_FIX TRUE)
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
    set(_GST_MOBILE_NEEDS_TEXTREL_ERROR TRUE)
    set(_GST_MOBILE_NEEDS_BSYMBOLIC_FIX TRUE)
# arm64: https://ffmpeg.org/pipermail/ffmpeg-devel/2022-July/298734.html
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64" OR CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    set(_GST_MOBILE_NEEDS_BSYMBOLIC_FIX TRUE)
endif()

# Set up output variables for Android
if(ANDROID)
    if (NOT DEFINED GStreamer_JAVA_SRC_DIR AND DEFINED GSTREAMER_JAVA_SRC_DIR)
        set(GStreamer_JAVA_SRC_DIR "${GSTREAMER_JAVA_SRC_DIR}")
    elseif(NOT DEFINED GStreamer_JAVA_SRC_DIR)
        set(GStreamer_JAVA_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src/")
    elseif(NOT IS_ABSOLUTE "${GStreamer_JAVA_SRC_DIR}")
        # Gradle does not let us access the root of the subproject
        # so we implement the ndk-build assumption ourselves
        # Only prepend relative path if GStreamer_JAVA_SRC_DIR is not absolute
        set(GStreamer_JAVA_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../${GStreamer_JAVA_SRC_DIR}")
    endif()

    if(NOT DEFINED GStreamer_NDK_BUILD_PATH AND DEFINED GSTREAMER_NDK_BUILD_PATH)
        set(GStreamer_NDK_BUILD_PATH "${GSTREAMER_NDK_BUILD_PATH}")
    elseif(NOT DEFINED GStreamer_NDK_BUILD_PATH)
        set(GStreamer_NDK_BUILD_PATH  "${GStreamer_ROOT_DIR}/share/gst-android/ndk-build/")
    endif()
endif()

if(NOT DEFINED GStreamer_Mobile_MODULE_NAME)
    if (DEFINED GSTREAMER_ANDROID_MODULE_NAME)
        set(GStreamer_Mobile_MODULE_NAME "${GSTREAMER_ANDROID_MODULE_NAME}")
    elseif(ANDROID)
        set(GStreamer_Mobile_MODULE_NAME gstreamer_android)
    else()
        set(GStreamer_Mobile_MODULE_NAME gstreamer_mobile)
    endif()
endif()

if(ANDROID)
    if(NOT DEFINED GStreamer_ASSETS_DIR AND DEFINED GSTREAMER_ASSETS_DIR)
        set(GStreamer_ASSETS_DIR "${GSTREAMER_ASSETS_DIR}")
    elseif(NOT DEFINED GStreamer_ASSETS_DIR)
        set(GStreamer_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src/assets/")
    elseif(NOT IS_ABSOLUTE "${GStreamer_ASSETS_DIR}")
        # Only prepend relative path if GStreamer_ASSETS_DIR is not absolute
        set(GStreamer_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../${GStreamer_ASSETS_DIR}")
    endif()

elseif(IOS)
    if(NOT DEFINED GStreamer_ASSETS_DIR AND DEFINED GSTREAMER_ASSETS_DIR)
        set(GStreamer_ASSETS_DIR "${GSTREAMER_ASSETS_DIR}")
    elseif(NOT DEFINED GStreamer_ASSETS_DIR)
        set(GStreamer_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    elseif(NOT IS_ABSOLUTE "${GStreamer_ASSETS_DIR}")
        # Only prepend relative path if GStreamer_ASSETS_DIR is not absolute
        set(GStreamer_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../${GStreamer_ASSETS_DIR}")
    endif()
endif()

if (ANDROID)
    if(NOT EXISTS "${GStreamer_NDK_BUILD_PATH}/GStreamer.java")
        message(FATAL_ERROR "GStreamer.java not found at ${GStreamer_NDK_BUILD_PATH}. "
            "Verify GStreamer Android SDK installation.")
    endif()
    file(READ "${GStreamer_NDK_BUILD_PATH}/GStreamer.java" JAVA_INPUT)
endif()

if (ANDROID OR IOS)
    set(GSTREAMER_IS_MOBILE ON)
else()
    set(GSTREAMER_IS_MOBILE OFF)
endif()

# Block shared GStreamer on mobile
if (GSTREAMER_IS_MOBILE)
    if (NOT DEFINED GStreamer_USE_STATIC_LIBS)
        set(GStreamer_USE_STATIC_LIBS ON)
    endif()
    if (NOT GStreamer_USE_STATIC_LIBS)
        message(FATAL_ERROR "Shared library GStreamer is not supported on mobile platforms")
    endif()
endif()

# Extract the plugin subset from the mobile component list.
# Use a local variable (_gst_mobile_plugins) to avoid overwriting the caller's
# GSTREAMER_PLUGINS, which may be needed after this file returns (e.g. for
# install-time plugin filtering in GStreamerHelpers).
set(_gst_plugins ${GStreamerMobile_FIND_COMPONENTS})
list(REMOVE_ITEM _gst_plugins fonts ca_certificates mobile)
list(REMOVE_DUPLICATES _gst_plugins)

set(_gst_mobile_plugins ${_gst_plugins})
list(FILTER _gst_mobile_plugins EXCLUDE REGEX "^api_")
set(_gst_mobile_apis ${_gst_plugins})
list(FILTER _gst_mobile_apis INCLUDE REGEX "^api_")

if (GSTREAMER_IS_MOBILE AND (NOT TARGET GStreamer::mobile))
    # Generate list of gio modules (independent of plugin discovery)
    if (NOT G_IO_MODULES)
        set(G_IO_MODULES)
    endif()
    list(TRANSFORM G_IO_MODULES
        PREPEND "gio"
        OUTPUT_VARIABLE G_IO_MODULES_LIBS
    )
    list(TRANSFORM G_IO_MODULES
        PREPEND "\nGST_G_IO_MODULE_DECLARE\("
        OUTPUT_VARIABLE G_IO_MODULES_DECLARE
    )
    list(TRANSFORM G_IO_MODULES_DECLARE
        APPEND "\)"
        OUTPUT_VARIABLE G_IO_MODULES_DECLARE
    )
    if(G_IO_MODULES_DECLARE)
        set(G_IO_MODULES_DECLARE "${G_IO_MODULES_DECLARE};")
    endif()
    list(TRANSFORM G_IO_MODULES
        PREPEND "\nGST_G_IO_MODULE_LOAD\("
        OUTPUT_VARIABLE G_IO_MODULES_LOAD
    )
    list(TRANSFORM G_IO_MODULES_LOAD
        APPEND "\)"
        OUTPUT_VARIABLE G_IO_MODULES_LOAD
    )
    if(G_IO_MODULES_LOAD)
        set(G_IO_MODULES_LOAD "${G_IO_MODULES_LOAD};")
    endif()

    # Creates a shared library including gstreamer, its plugins and all the dependencies.
    # The init source file is generated by configure_file after find_package(GStreamer)
    # determines which plugins are actually available in the SDK.
    if (ANDROID)
        set_source_files_properties("${GStreamer_Mobile_MODULE_NAME}.c" PROPERTIES GENERATED TRUE)
        add_library(GStreamerMobile
            SHARED
                "${GStreamer_Mobile_MODULE_NAME}.c"
        )

    else()
        add_library(GStreamerMobile SHARED)
        enable_language(OBJC OBJCXX)
        target_sources(GStreamerMobile
            PRIVATE
                "${GStreamer_Mobile_MODULE_NAME}.m"
        )
        set_source_files_properties("${GStreamer_Mobile_MODULE_NAME}.m"
            PROPERTIES
                LANGUAGE OBJC
                GENERATED TRUE
        )
        find_library(Foundation_LIB Foundation REQUIRED)
        target_link_libraries(GStreamerMobile
            PRIVATE
                ${Foundation_LIB}
        )
    endif()
    add_library(GStreamer::mobile ALIAS GStreamerMobile)

    # Set CXX linker language for plugins that contain C++ code
    set_target_properties(
        GStreamerMobile
        PROPERTIES
            LIBRARY_OUTPUT_NAME ${GStreamer_Mobile_MODULE_NAME}
    )
    if (APPLE)
        set_target_properties(
            GStreamerMobile
            PROPERTIES
                LINKER_LANGUAGE OBJCXX
                FRAMEWORK TRUE
                FRAMEWORK_VERSION A
                MACOSX_FRAMEWORK_IDENTIFIER org.gstreamer.GStreamerMobile
        )
    else()
        set_target_properties(
            GStreamerMobile
            PROPERTIES
                LINKER_LANGUAGE CXX
        )
    endif()
endif()

if (GStreamerMobile_FIND_REQUIRED)
    find_package(GStreamer REQUIRED)
else()
    find_package(GStreamer)
endif()

# Filter plugins to only those actually found, then regenerate the init template.
# Without this, the template declares symbols for plugins missing from the SDK
# (e.g. gst_plugin_dav1d_register) causing linker errors.
if(GSTREAMER_IS_MOBILE AND TARGET GStreamerMobile)
    set(_gst_found_plugins)
    foreach(_p IN LISTS _gst_mobile_plugins)
        if(GStreamer_${_p}_FOUND)
            # On Android, also verify the static library exists. Some plugins have
            # .pc files in the SDK but no compiled static library (e.g. dav1d).
            if(ANDROID)
                if(NOT EXISTS "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0/libgst${_p}.a")
                    message(STATUS "GStreamerMobile: Plugin '${_p}' has pkg-config but no static library, excluding from init")
                    continue()
                endif()
            endif()
            list(APPEND _gst_found_plugins ${_p})
        else()
            message(STATUS "GStreamerMobile: Plugin '${_p}' not found in SDK, excluding from init")
        endif()
    endforeach()
    set(_gst_mobile_plugins ${_gst_found_plugins})

    list(TRANSFORM _gst_mobile_plugins PREPEND "\nGST_PLUGIN_STATIC_DECLARE\(" OUTPUT_VARIABLE PLUGINS_DECLARATION)
    list(TRANSFORM PLUGINS_DECLARATION APPEND "\)" OUTPUT_VARIABLE PLUGINS_DECLARATION)
    if(PLUGINS_DECLARATION)
        set(PLUGINS_DECLARATION "${PLUGINS_DECLARATION};")
    endif()
    list(TRANSFORM _gst_mobile_plugins PREPEND "\nGST_PLUGIN_STATIC_REGISTER\(" OUTPUT_VARIABLE PLUGINS_REGISTRATION)
    list(TRANSFORM PLUGINS_REGISTRATION APPEND "\)" OUTPUT_VARIABLE PLUGINS_REGISTRATION)
    if(PLUGINS_REGISTRATION)
        set(PLUGINS_REGISTRATION "${PLUGINS_REGISTRATION};")
    endif()

    if(ANDROID)
        configure_file("${CMAKE_CURRENT_LIST_DIR}/GStreamer/gstreamer_android-1.0.c.in" "${GStreamer_Mobile_MODULE_NAME}.c")
    else()
        configure_file("${CMAKE_CURRENT_LIST_DIR}/GStreamer/gst_ios_init.m.in" "${GStreamer_Mobile_MODULE_NAME}.m")
    endif()

    # Validate only components that are available (plus explicitly requested non-plugin components).
    # This keeps missing optional plugins from failing HANDLE_COMPONENTS after filtering.
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
    set(GStreamerMobile_FIND_COMPONENTS ${_gst_validate_components})
endif()

# Path for the static GIO modules (FindGStreamerQGC may have already set this)
if (NOT G_IO_MODULES_PATH)
    pkg_get_variable(G_IO_MODULES_PATH gio-2.0 giomoduledir)
endif()
if (NOT G_IO_MODULES_PATH)
    set(G_IO_MODULES_PATH "${GStreamer_ROOT_DIR}/lib/gio/modules")
endif()

if (GSTREAMER_IS_MOBILE AND TARGET GStreamerMobile)
    if(PC_GStreamer_VERSION)
        set_target_properties(
            GStreamerMobile
            PROPERTIES
                VERSION ${PC_GStreamer_VERSION}
                SOVERSION ${PC_GStreamer_VERSION}
        )
    endif()

    # Handle all libraries, even those specified with -l:libfoo.a (srt)
    # Due to the unavailability of pkgconf's `--maximum-traverse-depth`
    # on stock pkg-config, I attempt to simulate it through the shared
    # libraries listing.
    # If pkgconf is available, replace all PC_GStreamer_ entries with
    # PC_GStreamer_NoDeps and uncomment the code block above.
    # Deduplicate to prevent --whole-archive from including the same archive
    # twice (causes duplicate symbol errors with ld.lld). pkg-config returns
    # duplicates when multiple packages share transitive deps like glib-2.0.
    # Work on a copy to avoid mutating the pkg-config output in parent scope.
    set(_gst_mobile_core_libs ${PC_GStreamer_LIBRARIES})
    list(REMOVE_DUPLICATES _gst_mobile_core_libs)
    foreach(LOCAL_LIB IN LISTS _gst_mobile_core_libs)
        # list(TRANSFORM REPLACE) is of no use here
        # https://gitlab.kitware.com/cmake/cmake/-/issues/16899
        if (LOCAL_LIB MATCHES "${_gst_SRT_REGEX_PATCH}")
            string(REGEX REPLACE "${_gst_SRT_REGEX_PATCH}" "\\1" LOCAL_LIB "${LOCAL_LIB}")
        endif()
        string(MAKE_C_IDENTIFIER "_gst_${LOCAL_LIB}" GST_LOCAL_LIB)
        if(NOT DEFINED ${GST_LOCAL_LIB} OR "${${GST_LOCAL_LIB}}" STREQUAL "")
            message(WARNING "GStreamerMobile: Library '${LOCAL_LIB}' not resolved by FindGStreamer, skipping")
            continue()
        endif()
        # These have already been found by FindGStreamer
        if ("${${GST_LOCAL_LIB}}" IN_LIST _gst_IGNORED_SYSTEM_LIBRARIES)
            target_link_libraries(GStreamerMobile PRIVATE
                "${${GST_LOCAL_LIB}}"
            )
        elseif(APPLE)
            target_link_libraries(GStreamerMobile PRIVATE
                "-Wl,-force_load,${${GST_LOCAL_LIB}}"
            )
        else()
            target_link_libraries(GStreamerMobile PRIVATE
                "-Wl,--whole-archive,${${GST_LOCAL_LIB}},--no-whole-archive"
            )
        endif()
    endforeach()

    # GStreamer::deps is linked after plugin archives (see end of file) so that
    # transitive deps like iconv, tag, codecparsers satisfy plugin references.

    target_link_options(
        GStreamerMobile
        INTERFACE
            $<TARGET_PROPERTY:GStreamer::GStreamer,INTERFACE_LINK_OPTIONS>
    )

    target_include_directories(
        GStreamerMobile
        SYSTEM INTERFACE
            $<TARGET_PROPERTY:GStreamer::GStreamer,INTERFACE_INCLUDE_DIRECTORIES>
    )

    # Text relocations are strictly forbidden, error out if we encounter any
    if(DEFINED _GST_MOBILE_NEEDS_TEXTREL_ERROR)
        target_link_options(
            GStreamerMobile
            PRIVATE
                "-Wl,-z,text"
        )
    endif()

    # resolve textrels in the x86 asm
    if(DEFINED _GST_MOBILE_NEEDS_BSYMBOLIC_FIX)
        target_link_options(
            GStreamerMobile
            PRIVATE
                "-Wl,-Bsymbolic"
        )
    endif()

    # Android-specific linker options for static GStreamer plugins
    if(ANDROID)
        target_link_options(
            GStreamerMobile
            PRIVATE
                "-Wl,--export-dynamic"
        )
    endif()

    if (ANDROID)
        # Collect all Java-based initializer classes
        set(GSTREAMER_PLUGINS_CLASSES)
        foreach(LOCAL_PLUGIN IN LISTS _gst_mobile_plugins)
            file(GLOB_RECURSE
                LOCAL_PLUGIN_CLASS
                CONFIGURE_DEPENDS
                FOLLOW_SYMLINKS
                RELATIVE "${GStreamer_NDK_BUILD_PATH}"
                "${GStreamer_NDK_BUILD_PATH}/${LOCAL_PLUGIN}/*.java"
            )
            list(APPEND GSTREAMER_PLUGINS_CLASSES ${LOCAL_PLUGIN_CLASS})
        endforeach()

        add_custom_target(
            "copyjavasource_${CMAKE_ANDROID_ARCH_ABI}"
        )

        foreach(LOCAL_FILE IN LISTS GSTREAMER_PLUGINS_CLASSES)
            cmake_path(GET LOCAL_FILE FILENAME _java_filename)
            cmake_path(GET LOCAL_FILE PARENT_PATH _java_subdir)
            string(MAKE_C_IDENTIFIER "cp_${LOCAL_FILE}" COPYJAVASOURCE_TGT)
            add_custom_target(
                ${COPYJAVASOURCE_TGT}
                COMMAND
                    "${CMAKE_COMMAND}" -E make_directory
                    "${GStreamer_JAVA_SRC_DIR}/org/freedesktop/gstreamer/${_java_subdir}"
                COMMAND
                    "${CMAKE_COMMAND}" -E copy
                    "${GStreamer_NDK_BUILD_PATH}/${LOCAL_FILE}"
                    "${GStreamer_JAVA_SRC_DIR}/org/freedesktop/gstreamer/${_java_subdir}/"
                BYPRODUCTS
                    "${GStreamer_JAVA_SRC_DIR}/org/freedesktop/gstreamer/${_java_subdir}/${_java_filename}"
            )
            add_dependencies(copyjavasource_${CMAKE_ANDROID_ARCH_ABI} ${COPYJAVASOURCE_TGT})
        endforeach()
    endif()

    # And, finally, set the GIO modules up
    if (G_IO_MODULES_LIBS)
        add_library(GStreamer::gio_modules INTERFACE IMPORTED)

        _gst_apply_link_libraries(OFF G_IO_MODULES_LIBS G_IO_MODULES_PATH GStreamer::gio_modules)

        # If using openssl GIO module, we need to link OpenSSL libraries
        # The GIO openssl module depends on libssl and libcrypto
        if ("openssl" IN_LIST G_IO_MODULES)
            # Add GStreamer lib directories to search path (following reference implementation pattern)
            target_link_directories(GStreamer::gio_modules INTERFACE
                "${GStreamer_ROOT_DIR}/lib"
                "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0"
            )
            # Link OpenSSL libraries by name (will be found via link directories)
            target_link_libraries(GStreamer::gio_modules INTERFACE
                ssl
                crypto
            )
        endif()

        target_link_libraries(
            GStreamerMobile
            PRIVATE
                GStreamer::gio_modules
        )
    endif()
    # Mark the 'mobile' component as found (matches find_package_handle_standard_args naming)
    set(GStreamerMobile_mobile_FOUND TRUE)
endif()

set(GSTREAMER_RESOURCES)

if(fonts IN_LIST GStreamerMobile_FIND_COMPONENTS)
    if(ANDROID)
        set(GStreamer_UBUNTU_R_TTF "${GStreamer_NDK_BUILD_PATH}/fontconfig/fonts/Ubuntu-R.ttf"
            CACHE FILEPATH "Path to Ubuntu-R.ttf")
        set(GStreamer_FONTS_CONF "${GStreamer_NDK_BUILD_PATH}/fontconfig/fonts.conf"
            CACHE FILEPATH "Path to fonts.conf")
    elseif(IOS)
        set(GStreamer_UBUNTU_R_TTF "${GStreamer_ROOT_DIR}/share/fontconfig/fonts/Ubuntu-R.ttf"
            CACHE FILEPATH "Path to Ubuntu-R.ttf")
        set(GStreamer_FONTS_CONF "${GStreamer_ROOT_DIR}/etc/fonts/fonts.conf"
            CACHE FILEPATH "Path to fonts.conf")
    endif()
    if (EXISTS "${GStreamer_UBUNTU_R_TTF}" AND EXISTS "${GStreamer_FONTS_CONF}")
        set(GStreamerMobile_fonts_FOUND ON)

        if (ANDROID)
            string(REPLACE "//copyFonts" "copyFonts" JAVA_INPUT "${JAVA_INPUT}")
            add_custom_target(
                copyfontsres_${CMAKE_ANDROID_ARCH_ABI}
                COMMAND
                    "${CMAKE_COMMAND}" -E make_directory
                    "${GStreamer_ASSETS_DIR}/fontconfig/fonts/truetype/"
                COMMAND
                    "${CMAKE_COMMAND}" -E copy
                    "${GStreamer_UBUNTU_R_TTF}"
                    "${GStreamer_ASSETS_DIR}/fontconfig/fonts/truetype/"
                COMMAND
                    "${CMAKE_COMMAND}" -E copy
                    "${GStreamer_FONTS_CONF}"
                    "${GStreamer_ASSETS_DIR}/fontconfig/"
                BYPRODUCTS
                    "${GStreamer_ASSETS_DIR}/fontconfig/fonts/truetype/Ubuntu-R.ttf"
                    "${GStreamer_ASSETS_DIR}/fontconfig/fonts.conf"
            )

            if (TARGET GStreamerMobile)
                add_dependencies(GStreamerMobile copyfontsres_${CMAKE_ANDROID_ARCH_ABI})
            endif()
        elseif(APPLE)
            list(APPEND GSTREAMER_RESOURCES "${GStreamer_FONTS_CONF}" "${GStreamer_UBUNTU_R_TTF}")
        else()
            message(FATAL_ERROR "No fonts assets available for this operating system.")
        endif()
    else()
        set(GStreamerMobile_fonts_FOUND OFF)
    endif()
endif()

if(ca_certificates IN_LIST GStreamerMobile_FIND_COMPONENTS)
    set(GStreamer_CA_BUNDLE "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt"
        CACHE FILEPATH "Path to ca-certificates bundle"
    )
    if (EXISTS "${GStreamer_CA_BUNDLE}")
        set(GStreamerMobile_ca_certificates_FOUND ON)

        if (ANDROID)
            string(REPLACE "//copyCaCertificates" "copyCaCertificates" JAVA_INPUT "${JAVA_INPUT}")
            add_custom_target(
                copycacertificatesres_${CMAKE_ANDROID_ARCH_ABI}
                COMMAND
                    "${CMAKE_COMMAND}" -E make_directory
                    "${GStreamer_ASSETS_DIR}/ssl/certs/"
                COMMAND
                    "${CMAKE_COMMAND}" -E copy
                    "${GStreamer_CA_BUNDLE}"
                    "${GStreamer_ASSETS_DIR}/ssl/certs/"
                BYPRODUCTS "${GStreamer_ASSETS_DIR}/ssl/certs/ca-certificates.crt"
            )

            if (TARGET GStreamerMobile)
                add_dependencies(GStreamerMobile copycacertificatesres_${CMAKE_ANDROID_ARCH_ABI})
                target_compile_definitions(GStreamerMobile
                    PRIVATE
                        GSTREAMER_INCLUDE_CA_CERTIFICATES
                )
            endif()
        elseif (APPLE)
            list(APPEND GSTREAMER_RESOURCES "${GStreamer_CA_BUNDLE}")
        else()
            message(FATAL_ERROR "No certificate bundle available for this operating system.")
        endif()
    else()
        set(GStreamerMobile_ca_certificates_FOUND OFF)
    endif()
endif()

if (ANDROID)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/GStreamer.java" "${JAVA_INPUT}")
    add_custom_target(
        enable_includes_in_gstreamer_java
        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${GStreamer_JAVA_SRC_DIR}/org/freedesktop/gstreamer/"
        COMMAND
            "${CMAKE_COMMAND}" -E copy
            "${CMAKE_CURRENT_BINARY_DIR}/GStreamer.java"
            "${GStreamer_JAVA_SRC_DIR}/org/freedesktop/gstreamer/GStreamer.java"
        BYPRODUCTS
            "${GStreamer_JAVA_SRC_DIR}/org/freedesktop/gstreamer/GStreamer.java"
    )
    if (TARGET GStreamerMobile)
        add_dependencies(copyjavasource_${CMAKE_ANDROID_ARCH_ABI} enable_includes_in_gstreamer_java)
        add_dependencies(GStreamerMobile copyjavasource_${CMAKE_ANDROID_ARCH_ABI})
    endif()
endif()

if (TARGET GStreamerMobile AND GSTREAMER_RESOURCES)
    set_target_properties(
        GStreamerMobile
        PROPERTIES
            RESOURCE "${GSTREAMER_RESOURCES}"
    )
endif()

# Perform final validation
include(FindPackageHandleStandardArgs)
foreach(_gst_PLUGIN IN LISTS _gst_plugins)
    set(GStreamerMobile_${_gst_PLUGIN}_FOUND "${GStreamer_${_gst_PLUGIN}_FOUND}")
endforeach()
if(GSTREAMER_IS_MOBILE AND TARGET GStreamerMobile)
    foreach(_gst_PLUGIN IN LISTS _gst_mobile_plugins)
        if (GStreamer_${_gst_PLUGIN}_FOUND)
            if(ANDROID)
                # Link the plugin .a with --whole-archive to ensure static plugin
                # init functions (gst_plugin_<name>_register) are not discarded.
                set(_plugin_lib_path "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0/libgst${_gst_PLUGIN}.a")
                if(EXISTS "${_plugin_lib_path}")
                    target_link_libraries(GStreamerMobile PRIVATE
                        "-Wl,--whole-archive,${_plugin_lib_path},--no-whole-archive"
                    )
                endif()
                # Link the plugin target for its transitive deps (resolved from
                # the plugin's .pc file by FindGStreamer). The plugin .a itself is
                # already force-linked above; this pulls in sub-libraries like
                # pbutils, net, sdp, etc.
                if(TARGET GStreamer::${_gst_PLUGIN})
                    target_link_libraries(GStreamerMobile PRIVATE GStreamer::${_gst_PLUGIN})
                endif()
            else()
                target_link_libraries(
                    GStreamerMobile
                    PRIVATE
                        GStreamer::${_gst_PLUGIN}
                )
            endif()
        endif()
    endforeach()
    # Link dependency libs last so they satisfy references from both core
    # archives and plugin archives linked above.
    target_link_libraries(GStreamerMobile PRIVATE GStreamer::deps)
endif()
# Suppress "package name mismatch" warning when included from FindGStreamerQGC
set(FPHSA_NAME_MISMATCHED TRUE)
find_package_handle_standard_args(GStreamerMobile
    HANDLE_COMPONENTS
)
unset(FPHSA_NAME_MISMATCHED)
