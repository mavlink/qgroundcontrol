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

if (NOT GStreamer_ROOT_DIR OR NOT EXISTS "${GStreamer_ROOT_DIR}")
    message(FATAL_ERROR "GStreamer_ROOT_DIR must be set to a valid directory before including FindGStreamerMobile "
        "(current value: '${GStreamer_ROOT_DIR}')")
endif()

if(NOT GSTREAMER_LIB_PATH)
    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
endif()

# _gst_IGNORED_SYSTEM_LIBRARIES and _gst_SRT_REGEX_PATCH are defined in GStreamerHelpers.cmake
if(NOT DEFINED _gst_IGNORED_SYSTEM_LIBRARIES OR NOT DEFINED _gst_SRT_REGEX_PATCH)
    include(GStreamerHelpers)
endif()

if(ANDROID)
    if(CMAKE_ANDROID_ARCH_ABI MATCHES "^armeabi" OR CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
        set(_GST_MOBILE_NEEDS_TEXTREL_ERROR TRUE)
        set(_GST_MOBILE_NEEDS_BSYMBOLIC_FIX TRUE)
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64" OR CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set(_GST_MOBILE_NEEDS_BSYMBOLIC_FIX TRUE)
    endif()
endif()

# Set up output variables for Android
if(ANDROID)
    if (NOT DEFINED GStreamer_JAVA_SRC_DIR AND DEFINED GSTREAMER_JAVA_SRC_DIR)
        set(GStreamer_JAVA_SRC_DIR "${GSTREAMER_JAVA_SRC_DIR}")
    elseif(NOT DEFINED GStreamer_JAVA_SRC_DIR)
        set(GStreamer_JAVA_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src/")
    elseif(NOT IS_ABSOLUTE "${GStreamer_JAVA_SRC_DIR}")
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
        set(GStreamer_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../${GStreamer_ASSETS_DIR}")
    endif()

elseif(IOS)
    if(NOT DEFINED GStreamer_ASSETS_DIR AND DEFINED GSTREAMER_ASSETS_DIR)
        set(GStreamer_ASSETS_DIR "${GSTREAMER_ASSETS_DIR}")
    elseif(NOT DEFINED GStreamer_ASSETS_DIR)
        set(GStreamer_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    elseif(NOT IS_ABSOLUTE "${GStreamer_ASSETS_DIR}")
        set(GStreamer_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../${GStreamer_ASSETS_DIR}")
    endif()
endif()

if (ANDROID)
    if(NOT EXISTS "${GStreamer_NDK_BUILD_PATH}/GStreamer.java")
        message(FATAL_ERROR "GStreamer.java not found at ${GStreamer_NDK_BUILD_PATH}. "
            "Verify GStreamer Android SDK installation.")
    endif()
endif()

if (ANDROID OR IOS)
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

set(_gst_plugins ${GStreamerMobile_FIND_COMPONENTS})
list(REMOVE_ITEM _gst_plugins fonts ca_certificates mobile)
list(REMOVE_DUPLICATES _gst_plugins)

set(_gst_mobile_plugins ${_gst_plugins})
list(FILTER _gst_mobile_plugins EXCLUDE REGEX "^api_")
set(_gst_mobile_apis ${_gst_plugins})
list(FILTER _gst_mobile_apis INCLUDE REGEX "^api_")

if(GStreamer_DEBUG)
    message(STATUS "[GstMobile] GStreamer_ROOT_DIR = ${GStreamer_ROOT_DIR}")
    message(STATUS "[GstMobile] GSTREAMER_LIB_PATH = ${GSTREAMER_LIB_PATH}")
    message(STATUS "[GstMobile] Requested plugins: ${_gst_mobile_plugins}")
    message(STATUS "[GstMobile] Requested APIs: ${_gst_mobile_apis}")
endif()

macro(_gst_generate_macro_list INPUT_LIST MACRO_NAME OUT_VAR)
    list(TRANSFORM ${INPUT_LIST} PREPEND "\n${MACRO_NAME}\(" OUTPUT_VARIABLE ${OUT_VAR})
    list(TRANSFORM ${OUT_VAR} APPEND "\)")
    if(${OUT_VAR})
        set(${OUT_VAR} "${${OUT_VAR}};")
    endif()
endmacro()

if (GSTREAMER_IS_MOBILE AND (NOT TARGET GStreamer::mobile))
    if (NOT G_IO_MODULES)
        set(G_IO_MODULES)
    endif()
    list(TRANSFORM G_IO_MODULES PREPEND "gio" OUTPUT_VARIABLE G_IO_MODULES_LIBS)
    _gst_generate_macro_list(G_IO_MODULES "GST_G_IO_MODULE_DECLARE" G_IO_MODULES_DECLARE)
    _gst_generate_macro_list(G_IO_MODULES "GST_G_IO_MODULE_LOAD" G_IO_MODULES_LOAD)

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

if (NOT GStreamer_FOUND)
    message(FATAL_ERROR "FindGStreamer must be called before FindGStreamerMobile. "
        "Ensure find_package(GStreamer) has completed successfully.")
endif()

if(GSTREAMER_IS_MOBILE AND TARGET GStreamerMobile)
    set(_gst_found_plugins)
    foreach(_p IN LISTS _gst_mobile_plugins)
        if(GStreamer_${_p}_FOUND)
            find_library(_gst_plugin_lib_${_p} gst${_p}
                HINTS "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0"
                NO_DEFAULT_PATH
            )
            if(NOT _gst_plugin_lib_${_p})
                message(STATUS "GStreamerMobile: Plugin '${_p}' has pkg-config but no static library, excluding from init")
                unset(_gst_plugin_lib_${_p} CACHE)
                continue()
            endif()
            set(_gst_resolved_plugin_lib_${_p} "${_gst_plugin_lib_${_p}}")
            unset(_gst_plugin_lib_${_p} CACHE)
            list(APPEND _gst_found_plugins ${_p})
        else()
            message(STATUS "GStreamerMobile: Plugin '${_p}' not found in SDK, excluding from init")
        endif()
    endforeach()
    set(_gst_mobile_plugins ${_gst_found_plugins})

    if(GStreamer_DEBUG)
        message(STATUS "[GstMobile] Found plugins with .a: ${_gst_mobile_plugins}")
        foreach(_dbg_p IN LISTS _gst_mobile_plugins)
            message(STATUS "[GstMobile]   ${_dbg_p} -> ${_gst_resolved_plugin_lib_${_dbg_p}}")
        endforeach()

        set(_dbg_plugins_with_pc)
        set(_dbg_plugins_without_pc)
        foreach(_dbg_p IN LISTS _gst_mobile_plugins)
            if(TARGET GStreamer::${_dbg_p})
                list(APPEND _dbg_plugins_with_pc ${_dbg_p})
            else()
                list(APPEND _dbg_plugins_without_pc ${_dbg_p})
            endif()
        endforeach()
        message(STATUS "[GstMobile] Plugins WITH GStreamer:: target: ${_dbg_plugins_with_pc}")
        message(STATUS "[GstMobile] Plugins WITHOUT GStreamer:: target: ${_dbg_plugins_without_pc}")
    endif()

    _gst_generate_macro_list(_gst_mobile_plugins "GST_PLUGIN_STATIC_DECLARE" PLUGINS_DECLARATION)
    _gst_generate_macro_list(_gst_mobile_plugins "GST_PLUGIN_STATIC_REGISTER" PLUGINS_REGISTRATION)

    if(ANDROID)
        configure_file("${CMAKE_CURRENT_LIST_DIR}/GStreamer/gstreamer_android-1.0.c.in" "${GStreamer_Mobile_MODULE_NAME}.c")
    else()
        configure_file("${CMAKE_CURRENT_LIST_DIR}/GStreamer/gst_ios_init.m.in" "${GStreamer_Mobile_MODULE_NAME}.m")
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
    set(GStreamerMobile_FIND_COMPONENTS ${_gst_validate_components})
endif()

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

    # Deduplicate to prevent --whole-archive duplicate symbol errors.
    # Also exclude plugin libs that will be linked separately at the end of this
    # file via _gst_resolved_plugin_lib — linking them here too causes duplicates.
    set(_gst_mobile_core_libs ${PC_GStreamer_LIBRARIES})
    if(GStreamer_DEBUG)
        message(STATUS "[GstMobile] PC_GStreamer_LIBRARIES (raw): ${PC_GStreamer_LIBRARIES}")
        message(STATUS "[GstMobile] PC_GStreamer_STATIC_LIBRARIES (raw): ${PC_GStreamer_STATIC_LIBRARIES}")
        message(STATUS "[GstMobile] PC_GStreamer_STATIC_LIBRARY_DIRS: ${PC_GStreamer_STATIC_LIBRARY_DIRS}")
    endif()
    list(REMOVE_DUPLICATES _gst_mobile_core_libs)
    foreach(_p IN LISTS _gst_mobile_plugins)
        list(REMOVE_ITEM _gst_mobile_core_libs "gst${_p}")
    endforeach()
    if(GStreamer_DEBUG)
        message(STATUS "[GstMobile] Core libs after plugin filtering: ${_gst_mobile_core_libs}")
    endif()

    set(_gst_mobile_core_hints ${GSTREAMER_LIB_PATH})
    _gst_resolve_and_link_libraries(GStreamerMobile PRIVATE _gst_mobile_core_libs _gst_mobile_core_hints WARN_MISSING)

    target_include_directories(
        GStreamerMobile
        PRIVATE
            $<TARGET_PROPERTY:GStreamer::GStreamer,INTERFACE_INCLUDE_DIRECTORIES>
    )

    if(DEFINED _GST_MOBILE_NEEDS_TEXTREL_ERROR)
        target_link_options(
            GStreamerMobile
            PRIVATE
                "-Wl,-z,text"
        )
    endif()

    if(DEFINED _GST_MOBILE_NEEDS_BSYMBOLIC_FIX)
        target_link_options(
            GStreamerMobile
            PRIVATE
                "-Wl,-Bsymbolic"
        )
    endif()

    if(ANDROID)
        target_link_options(
            GStreamerMobile
            PRIVATE
                "-Wl,--export-dynamic"
        )
    endif()

    if (ANDROID)
        set(GSTREAMER_PLUGINS_CLASSES)
        foreach(LOCAL_PLUGIN IN LISTS _gst_mobile_plugins)
            file(GLOB_RECURSE
                LOCAL_PLUGIN_CLASS
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

    if (G_IO_MODULES_LIBS)
        add_library(GStreamer::gio_modules INTERFACE IMPORTED)

        _gst_save_find_suffixes()
        foreach(_gio_lib IN LISTS G_IO_MODULES_LIBS)
            if(_gio_lib MATCHES "${_gst_SRT_REGEX_PATCH}")
                string(REGEX REPLACE "${_gst_SRT_REGEX_PATCH}" "\\1" _gio_lib "${_gio_lib}")
            endif()
            string(MAKE_C_IDENTIFIER "_gst_${_gio_lib}" _gio_cache_var)
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
            find_library(_gst_ssl_lib ssl
                HINTS "${GStreamer_ROOT_DIR}/lib" NO_DEFAULT_PATH)
            find_library(_gst_crypto_lib crypto
                HINTS "${GStreamer_ROOT_DIR}/lib" NO_DEFAULT_PATH)
            if(_gst_ssl_lib AND _gst_crypto_lib)
                target_link_libraries(GStreamer::gio_modules INTERFACE
                    "${_gst_ssl_lib}" "${_gst_crypto_lib}")
            else()
                target_link_libraries(GStreamer::gio_modules INTERFACE ssl crypto)
            endif()
            unset(_gst_ssl_lib CACHE)
            unset(_gst_crypto_lib CACHE)
        endif()

        target_link_libraries(
            GStreamerMobile
            PRIVATE
                GStreamer::gio_modules
        )
    endif()
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

if(ANDROID)
    if (TARGET GStreamerMobile AND TARGET copyjavasource_${CMAKE_ANDROID_ARCH_ABI})
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

include(FindPackageHandleStandardArgs)
foreach(_gst_PLUGIN IN LISTS _gst_plugins)
    set(GStreamerMobile_${_gst_PLUGIN}_FOUND "${GStreamer_${_gst_PLUGIN}_FOUND}")
endforeach()
if(GSTREAMER_IS_MOBILE AND TARGET GStreamerMobile)
    # Link plugin .a files with --whole-archive (needed for static registration symbols).
    # Plugin transitive dependencies (e.g. libavcodec for the libav plugin) are linked
    # separately via their GStreamer::<plugin> component targets. To avoid linking each
    # plugin .a twice (once under --whole-archive, once via the component target), we
    # extract only the transitive deps from the component target, excluding the plugin
    # .a itself.
    set(_gst_wa_libs)
    set(_dbg_plugins_linked_component)
    set(_dbg_plugins_no_component)
    foreach(_gst_PLUGIN IN LISTS _gst_mobile_plugins)
        if (GStreamer_${_gst_PLUGIN}_FOUND AND _gst_resolved_plugin_lib_${_gst_PLUGIN})
            list(APPEND _gst_wa_libs "${_gst_resolved_plugin_lib_${_gst_PLUGIN}}")
        endif()
        if(TARGET GStreamer::${_gst_PLUGIN})
            get_target_property(_plugin_iface_libs GStreamer::${_gst_PLUGIN} INTERFACE_LINK_LIBRARIES)
            if(_plugin_iface_libs)
                list(REMOVE_ITEM _plugin_iface_libs "${_gst_resolved_plugin_lib_${_gst_PLUGIN}}")
                if(_plugin_iface_libs)
                    target_link_libraries(GStreamerMobile PRIVATE ${_plugin_iface_libs})
                endif()
            endif()
            list(APPEND _dbg_plugins_linked_component ${_gst_PLUGIN})
            if(GStreamer_DEBUG)
                message(STATUS "[GstMobile] GStreamer::${_gst_PLUGIN} INTERFACE_LINK_LIBRARIES = ${_plugin_iface_libs}")
            endif()
        else()
            list(APPEND _dbg_plugins_no_component ${_gst_PLUGIN})
        endif()
    endforeach()

    if(GStreamer_DEBUG)
        message(STATUS "[GstMobile] WHOLE_ARCHIVE plugin libs: ${_gst_wa_libs}")
        message(STATUS "[GstMobile] Plugins linked via component target: ${_dbg_plugins_linked_component}")
        message(STATUS "[GstMobile] Plugins WITHOUT component target (deps may be missing): ${_dbg_plugins_no_component}")
    endif()

    if(_gst_wa_libs)
        target_link_options(GStreamerMobile PRIVATE
            "LINKER:--whole-archive"
            ${_gst_wa_libs}
            "LINKER:--no-whole-archive"
        )
    endif()

    if(GStreamer_DEBUG)
        get_target_property(_dbg_deps_libs GStreamer::deps INTERFACE_LINK_LIBRARIES)
        message(STATUS "[GstMobile] GStreamer::deps INTERFACE_LINK_LIBRARIES = ${_dbg_deps_libs}")
    endif()

    target_link_libraries(GStreamerMobile PRIVATE GStreamer::deps)
endif()
set(FPHSA_NAME_MISMATCHED TRUE)
find_package_handle_standard_args(GStreamerMobile
    HANDLE_COMPONENTS
)
unset(FPHSA_NAME_MISMATCHED)
