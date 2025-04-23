# SPDX-FileCopyrightText: 2024 L. E. Segovia <amy@centricular.com>
# SPDX-License-Ref: LGPL-2.1-or-later

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

``GStreamer_Mobile_FOUND``
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

if (GStreamer_Mobile_FOUND)
    return()
endif()

#####################
#  Setup variables  #
#####################

if (NOT DEFINED GStreamer_ROOT_DIR AND DEFINED GSTREAMER_ROOT)
    set(GStreamer_ROOT_DIR ${GSTREAMER_ROOT})
endif()

if (NOT GStreamer_ROOT_DIR)
    set(GStreamer_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../")
endif()

if (NOT EXISTS "${GStreamer_ROOT_DIR}")
    message(FATAL_ERROR "The directory GStreamer_ROOT_DIR=${GStreamer_ROOT_DIR} does not exist")
endif()

set(_gst_required_vars)

if (ca_certificates IN_LIST GStreamerMobile_FIND_COMPONENTS)
    # for setting the default GTlsDatabase
    list(APPEND GStreamer_EXTRA_DEPS gio-2.0)
endif()

if (ANDROID)
    list(APPEND GStreamer_EXTRA_DEPS zlib)
endif()

# Prepare Android hotfixes for x264
if(ANDROID_ABI MATCHES "^armeabi")
    set(NEEDS_NOTEXT_FIX TRUE)
    set(NEEDS_BSYMBOLIC_FIX TRUE)
elseif(ANDROID_ABI STREQUAL "x86")
    set(NEEDS_NOTEXT_FIX TRUE)
    set(NEEDS_BSYMBOLIC_FIX TRUE)
# arm64: https://ffmpeg.org/pipermail/ffmpeg-devel/2022-July/298734.html 
elseif(ANDROID_ABI STREQUAL "x86_64" OR ANDROID_ABI STREQUAL "arm64-v8a")
    set(NEEDS_BSYMBOLIC_FIX TRUE)
endif()

# Set up output variables for Android
if(ANDROID)
    if (NOT DEFINED GStreamer_JAVA_SRC_DIR AND DEFINED GSTREAMER_JAVA_SRC_DIR)
        set(GStreamer_JAVA_SRC_DIR ${GSTREAMER_JAVA_SRC_DIR})
    elseif(NOT DEFINED GStreamer_JAVA_SRC_DIR)
        set(GStreamer_JAVA_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src/")
    else()
        # Gradle does not let us access the root of the subproject
        # so we implement the ndk-build assumption ourselves
        set(GStreamer_JAVA_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../${GStreamer_JAVA_SRC_DIR}")
    endif()

    if(NOT DEFINED GStreamer_NDK_BUILD_PATH AND DEFINED GSTREAMER_NDK_BUILD_PATH)
        set(GStreamer_NDK_BUILD_PATH "${GSTREAMER_NDK_BUILD_PATH}")
    elseif(NOT DEFINED GStreamer_NDK_BUILD_PATH)
        set(GStreamer_NDK_BUILD_PATH  "${GStreamer_ROOT}/share/gst-android/ndk-build/")
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
    else()
        # Same as above
        set(GStreamer_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../${GStreamer_ASSETS_DIR}")
    endif()

    if(NOT DEFINED GStreamer_NDK_BUILD_PATH AND DEFINED GSTREAMER_NDK_BUILD_PATH)
        set(GStreamer_NDK_BUILD_PATH "${GSTREAMER_NDK_BUILD_PATH}")
    elseif(NOT DEFINED GStreamer_NDK_BUILD_PATH)
        set(GStreamer_NDK_BUILD_PATH  "${GStreamer_ROOT}/share/gst-android/ndk-build/")
    endif()
elseif(IOS)
    if(NOT DEFINED GStreamer_ASSETS_DIR AND DEFINED GStreamer_ASSETS_DIR)
        set(GStreamer_ASSETS_DIR ${GStreamer_ASSETS_DIR})
    elseif(NOT DEFINED GStreamer_ASSETS_DIR)
        set(GStreamer_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    else()
        # Same as above
        set(GStreamer_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../${GStreamer_ASSETS_DIR}")
    endif()
endif()

if (ANDROID)
    file(READ "${GStreamer_NDK_BUILD_PATH}/GStreamer.java" JAVA_INPUT)
endif()

if (ANDROID OR APPLE)
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

# Now, let's set up targets for each of the components supplied
# These are the required plugins
set(_gst_plugins ${GStreamerMobile_FIND_COMPONENTS})
# These are custom handled targets, and must be skipped from the loop
list(REMOVE_ITEM _gst_plugins fonts ca_certificates mobile)
list(REMOVE_DUPLICATES _gst_plugins)

set(GSTREAMER_PLUGINS ${_gst_plugins})
# These are the API packages
set(GSTREAMER_APIS ${GSTREAMER_PLUGINS})
list(FILTER GSTREAMER_APIS INCLUDE REGEX "^api_")
# Filter them out, although they're handled the same
# they cannot be considered for the purposes of initialization
list(FILTER GSTREAMER_PLUGINS EXCLUDE REGEX "^api_")

if (GSTREAMER_IS_MOBILE AND (NOT TARGET GStreamer::mobile))
    # Generate the plugins' declaration strings
    # (don't append a semicolon, CMake does it as part of the list)
    list(TRANSFORM GSTREAMER_PLUGINS
        PREPEND "\nGST_PLUGIN_STATIC_DECLARE\("
        OUTPUT_VARIABLE PLUGINS_DECLARATION
    )
    list(TRANSFORM PLUGINS_DECLARATION
        APPEND "\)"
        OUTPUT_VARIABLE PLUGINS_DECLARATION
    )
    if(PLUGINS_DECLARATION)
        set(PLUGINS_DECLARATION "${PLUGINS_DECLARATION};")
    endif()

    # Generate the plugins' registration strings
    list(TRANSFORM GSTREAMER_PLUGINS
        PREPEND "\nGST_PLUGIN_STATIC_REGISTER\("
        OUTPUT_VARIABLE PLUGINS_REGISTRATION
    )
    list(TRANSFORM PLUGINS_REGISTRATION
        APPEND "\)"
        OUTPUT_VARIABLE PLUGINS_REGISTRATION
    )
    if(PLUGINS_REGISTRATION)
        set(PLUGINS_REGISTRATION "${PLUGINS_REGISTRATION};")
    endif()

    # Generate list of gio modules
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
        APPEND "\);"
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

    # Generates a source file that declares and registers all the required plugins
    if (ANDROID)
        configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/GStreamer/gstreamer_android-1.0.c.in"
            "${GStreamer_Mobile_MODULE_NAME}.c"
        )
    else()
        configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/GStreamer/gst_ios_init.m.in"
            "${GStreamer_Mobile_MODULE_NAME}.m"
        )
    endif()

    # Creates a shared library including gstreamer, its plugins and all the dependencies
    if (ANDROID)
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
        )
        find_library(Foundation_LIB Foundation REQUIRED)
        target_link_libraries(GStreamerMobile
            PRIVATE
                ${Foundation_LIB}
        )
    endif()
    add_library(GStreamer::mobile ALIAS GStreamerMobile)

    # Assume it's C++ for the sake of gstsoundtouch
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
find_package(GStreamer COMPONENTS ${_gst_plugins} REQUIRED)
else()
find_package(GStreamer COMPONENTS ${_gst_plugins})
endif()

# Path for the static GIO modules
pkg_get_variable(G_IO_MODULES_PATH gio-2.0 giomoduledir)
if (NOT G_IO_MODULES_PATH)
    set(G_IO_MODULES_PATH "${GStreamer_ROOT_DIR}/lib/gio/modules")
endif()

if (GSTREAMER_IS_MOBILE)
    set_target_properties(
        GStreamerMobile
        PROPERTIES
            VERSION ${PC_GStreamer_VERSION}
            SOVERSION ${PC_GStreamer_VERSION}
    )

    # Handle all libraries, even those specified with -l:libfoo.a (srt)
    # Due to the unavailability of pkgconf's `--maximum-traverse-depth`
    # on stock pkg-config, I attempt to simulate it through the shared
    # libraries listing.
    # If pkgconf is available, replace all PC_GStreamer_ entries with
    # PC_GStreamer_NoDeps and uncomment the code block above.
    foreach(LOCAL_LIB IN LISTS PC_GStreamer_LIBRARIES)
        # list(TRANSFORM REPLACE) is of no use here
        # https://gitlab.kitware.com/cmake/cmake/-/issues/16899
        if (LOCAL_LIB MATCHES "${_gst_SRT_REGEX_PATCH}")
            string(REGEX REPLACE "${_gst_SRT_REGEX_PATCH}" "\\1" LOCAL_LIB "${LOCAL_LIB}")
        endif()
        string(MAKE_C_IDENTIFIER "_gst_${LOCAL_LIB}" GST_LOCAL_LIB)
        # These have already been found by FindGStreamer
        if ("${${GST_LOCAL_LIB}}" IN_LIST _gst_IGNORED_SYSTEM_LIBRARIES)
            target_link_libraries(GStreamerMobile PRIVATE
                "${${GST_LOCAL_LIB}}"
            )
        elseif (MSVC)
            target_link_libraries(GStreamerMobile PRIVATE
                "/WHOLEARCHIVE:${${GST_LOCAL_LIB}}"
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

    target_link_libraries(
        GStreamerMobile
        PRIVATE
            GStreamer::deps
    )

    target_link_options(
        GStreamerMobile
        INTERFACE
            $<TARGET_PROPERTY:GStreamer::GStreamer,INTERFACE_LINK_OPTIONS>
    )

    target_include_directories(
        GStreamerMobile
        INTERFACE
            $<TARGET_PROPERTY:GStreamer::GStreamer,INTERFACE_INCLUDE_DIRECTORIES>
    )

    # Text relocations are required for all 32-bit objects. We
    # must disable the warning to allow linking with lld. Unlike gold, ld which
    # will silently allow text relocations, lld support must be explicit.
    #
    # See https://crbug.com/911658#c19 for more information. See also
    # https://trac.ffmpeg.org/ticket/7878
    if(DEFINED NEEDS_NOTEXT_FIX)
        target_link_options(
            GStreamerMobile
            PRIVATE
                "-Wl,-z,notext"
        )
    endif()

    # resolve textrels in the x86 asm
    if(DEFINED NEEDS_BSYMBOLIC_FIX)
        target_link_options(
            GStreamerMobile
            PRIVATE
                "-Wl,-Bsymbolic"
        )
    endif()

    if (ANDROID)
        # Collect all Java-based initializer classes
        set(GSTREAMER_PLUGINS_CLASSES)
        foreach(LOCAL_PLUGIN IN LISTS GSTREAMER_PLUGINS)
            file(GLOB_RECURSE
                LOCAL_PLUGIN_CLASS
                FOLLOW_SYMLINKS
                RELATIVE "${GStreamer_NDK_BUILD_PATH}"
                "${GStreamer_NDK_BUILD_PATH}/${LOCAL_PLUGIN}/*.java"
            )
            list(APPEND GSTREAMER_PLUGINS_CLASSES ${LOCAL_PLUGIN_CLASS})
        endforeach()

        # Same as above, but collect the plugins themselves
        set(GSTREAMER_PLUGINS_WITH_CLASSES)
        foreach(LOCAL_PLUGIN IN LISTS GSTREAMER_PLUGINS)
            if(EXISTS "${GStreamer_NDK_BUILD_PATH}/${LOCAL_PLUGIN}/")
                list(APPEND GSTREAMER_PLUGINS_WITH_CLASSES ${LOCAL_PLUGIN})
            endif()
        endforeach()

        add_custom_target(
            "copyjavasource_${ANDROID_ABI}"
        )

        foreach(LOCAL_FILE IN LISTS GSTREAMER_PLUGINS_CLASSES)
            string(MAKE_C_IDENTIFIER "cp_${LOCAL_FILE}" COPYJAVASOURCE_TGT)
            add_custom_target(
                ${COPYJAVASOURCE_TGT}
                COMMAND
                    "${CMAKE_COMMAND}" -E make_directory
                    "${GStreamer_JAVA_SRC_DIR}/org/freedesktop/gstreamer/"
                COMMAND
                    "${CMAKE_COMMAND}" -E copy
                    "${GStreamer_NDK_BUILD_PATH}/${LOCAL_FILE}"
                    "${GStreamer_JAVA_SRC_DIR}/org/freedesktop/gstreamer/"
                BYPRODUCTS
                    "${GStreamer_JAVA_SRC_DIR}/org/freedesktop/gstreamer/${LOCAL_FILE}"
            )
            add_dependencies(copyjavasource_${ANDROID_ABI} ${COPYJAVASOURCE_TGT})
        endforeach()
    endif()

    # And, finally, set the GIO modules up
    if (G_IO_MODULES_LIBS)
        add_library(GStreamer::gio_modules INTERFACE IMPORTED)

        _gst_apply_link_libraries(OFF G_IO_MODULES_LIBS G_IO_MODULES_PATH GStreamer::gio_modules)
        target_link_libraries(
            GStreamerMobile
            PRIVATE
                GStreamer::gio_modules
        )
    endif()
    set(GStreamer_mobile_FOUND TRUE)
endif()

if(fonts IN_LIST GStreamerMobile_FIND_COMPONENTS)
    set(GStreamer_UBUNTU_R_TTF "${GStreamer_NDK_BUILD_PATH}/fontconfig/fonts/Ubuntu-R.ttf"
        CACHE FILEPATH "Path to Ubuntu-R.ttf"
    )
    set(GStreamer_FONTS_CONF
        "${GStreamer_NDK_BUILD_PATH}/fontconfig/fonts.conf"
        CACHE FILEPATH "Path to fonts.conf"
    )
    if (EXISTS "${GStreamer_UBUNTU_R_TTF}" AND EXISTS "${GStreamer_FONTS_CONF}")
        set(GStreamerMobile_fonts_FOUND ON)
        set(_gst_required_vars "GStreamer_UBUNTU_R_TTF GStreamer_FONTS_CONF ${gst_required_vars}")

        if (ANDROID)
            string(REPLACE "//copyFonts" "copyFonts" JAVA_INPUT "${JAVA_INPUT}")
            add_custom_target(
                copyfontsres_${ANDROID_ABI}
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
                add_dependencies(GStreamerMobile copyfontsres_${ANDROID_ABI})
            endif()
        elseif(APPLE)
            set(GStreamerMobile_fonts_FOUND ON)
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
        string(REPLACE "//copyCaCertificates" "copyCaCertificates" JAVA_INPUT "${JAVA_INPUT}")
        set(_gst_required_vars "GStreamer_CA_BUNDLE ${_gst_required_vars}")

        if (ANDROID)
            add_custom_target(
                copycacertificatesres_${ANDROID_ABI}
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
                add_dependencies(GStreamerMobile copycacertificatesres_${ANDROID_ABI})
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
        add_dependencies(copyjavasource_${ANDROID_ABI} enable_includes_in_gstreamer_java)
        add_dependencies(GStreamerMobile copyjavasource_${ANDROID_ABI})
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

    if (GStreamer_${_gst_PLUGIN}_FOUND)
        target_link_libraries(
            GStreamerMobile
            PRIVATE
                GStreamer::${_gst_PLUGIN}
        )
    endif()
endforeach()
# FIXME: CMake does not tolerate interpolation of REQUIRED_VARS
find_package_handle_standard_args(GStreamerMobile
    HANDLE_COMPONENTS
)
