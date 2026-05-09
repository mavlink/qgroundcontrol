# Android GStreamer SDK discovery — invoked by Orchestrator.cmake.

if(_qgc_gstreamer_android_included)
    return()
endif()
set(_qgc_gstreamer_android_included TRUE)

macro(_qgc_discover_android_sdk)
    if(NOT DEFINED GStreamer_ROOT_DIR)
        if(CPM_SOURCE_CACHE)
            set(_gst_android_cache "${CPM_SOURCE_CACHE}/gstreamer-android")
        else()
            set(_gst_android_cache "${CMAKE_BINARY_DIR}/_deps/gstreamer-android")
        endif()
        gstreamer_download_sdk(android ${GStreamer_FIND_VERSION}
            "gstreamer-android-${GStreamer_FIND_VERSION}.tar.xz" "${_gst_android_cache}" _gst_android_archive)

        CPMAddPackage(
            NAME gstreamer
            VERSION ${GStreamer_FIND_VERSION}
            URL "file://${_gst_android_archive}"
        )

        if(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
            set(GStreamer_ABI_DIR "armv7")
        elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
            set(GStreamer_ABI_DIR "arm64")
        elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
            set(GStreamer_ABI_DIR "x86")
        elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
            set(GStreamer_ABI_DIR "x86_64")
        else()
            message(FATAL_ERROR "Unsupported Android ABI: ${CMAKE_ANDROID_ARCH_ABI}")
        endif()
        set(GStreamer_ROOT_DIR "${gstreamer_SOURCE_DIR}/${GStreamer_ABI_DIR}")
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
# Build the libgstreamer_android.so wrapper, register static plugins via the
# shared gst_static_plugins.c.in template, copy fonts/CA assets into the APK.
# Caller must set GStreamerMobile_FIND_COMPONENTS (the COMPONENTS list a
# find_package-style call would have set).
# Macro (not function): GStreamerMobile target / GStreamer_*_FOUND / cache
# variables must propagate to caller scope.
# ─────────────────────────────────────────────────────────────────────────────
# Variable indirection (${${INPUT_LIST}}) requires macro semantics.
macro(_gst_generate_macro_list INPUT_LIST MACRO_NAME OUT_VAR)
    list(TRANSFORM ${INPUT_LIST} PREPEND "\n${MACRO_NAME}\(" OUTPUT_VARIABLE ${OUT_VAR})
    list(TRANSFORM ${OUT_VAR} APPEND "\)")
    if(${OUT_VAR})
        set(${OUT_VAR} "${${OUT_VAR}};")
    endif()
endmacro()

# Pure-computation helper — no add_library / target_* calls.
# Reads GStreamer* globals set by the parent scope; returns results via OUT params.
function(_qgc_compute_android_mobile_target
    PLUGINS_OUT APIS_OUT FOUND_COMPONENTS_OUT
    WA_LIBS_OUT PLUGINS_DECL_OUT PLUGINS_REG_OUT
    GIO_DECL_OUT GIO_LOAD_OUT
)
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

    # Resolve each plugin to a static .a; drop those without one.
    _gst_save_find_suffixes()
    set(_gst_found_plugins)
    foreach(_p IN LISTS _gst_mobile_plugins)
        if(GStreamer_${_p}_FOUND)
            find_library(_gst_plugin_lib_${_p} gst${_p}
                HINTS "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0"
                NO_DEFAULT_PATH
            )
            if(NOT _gst_plugin_lib_${_p})
                message(STATUS "GStreamerMobile: Plugin '${_p}' has pkg-config but no static library, excluding from init")
                continue()
            endif()
            list(APPEND _gst_found_plugins ${_p})
        else()
            message(STATUS "GStreamerMobile: Plugin '${_p}' not found in SDK, excluding from init")
        endif()
    endforeach()
    _gst_restore_find_suffixes()
    set(_gst_mobile_plugins ${_gst_found_plugins})

    if(GStreamer_DEBUG)
        message(STATUS "[GstMobile] Found plugins with .a: ${_gst_mobile_plugins}")
        foreach(_dbg_p IN LISTS _gst_mobile_plugins)
            message(STATUS "[GstMobile]   ${_dbg_p} -> ${_gst_plugin_lib_${_dbg_p}}")
        endforeach()
    endif()

    list(TRANSFORM G_IO_MODULES PREPEND "gio" OUTPUT_VARIABLE _gio_libs)
    _gst_generate_macro_list(_gst_mobile_plugins "GST_PLUGIN_STATIC_DECLARE" _decl)
    _gst_generate_macro_list(_gst_mobile_plugins "QGC_REGISTER_STATIC_PLUGIN" _reg)
    _gst_generate_macro_list(G_IO_MODULES "GST_G_IO_MODULE_DECLARE" _gio_decl)
    _gst_generate_macro_list(G_IO_MODULES "GST_G_IO_MODULE_LOAD" _gio_load)

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

    # Build whole-archive list from resolved paths.
    set(_gst_wa_libs)
    foreach(_gst_PLUGIN IN LISTS _gst_mobile_plugins)
        if(GStreamer_${_gst_PLUGIN}_FOUND AND _gst_plugin_lib_${_gst_PLUGIN})
            list(APPEND _gst_wa_libs "${_gst_plugin_lib_${_gst_PLUGIN}}")
        endif()
    endforeach()

    set(${PLUGINS_OUT}         "${_gst_mobile_plugins}"    PARENT_SCOPE)
    set(${APIS_OUT}            "${_gst_mobile_apis}"       PARENT_SCOPE)
    set(${FOUND_COMPONENTS_OUT}"${_gst_validate_components}" PARENT_SCOPE)
    set(${WA_LIBS_OUT}         "${_gst_wa_libs}"           PARENT_SCOPE)
    set(${PLUGINS_DECL_OUT}    "${_decl}"                  PARENT_SCOPE)
    set(${PLUGINS_REG_OUT}     "${_reg}"                   PARENT_SCOPE)
    set(${GIO_DECL_OUT}        "${_gio_decl}"              PARENT_SCOPE)
    set(${GIO_LOAD_OUT}        "${_gio_load}"              PARENT_SCOPE)
endfunction()

macro(_qgc_create_android_mobile_target)

if(NOT GSTREAMER_LIB_PATH)
    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
endif()

if(NOT DEFINED _gst_IGNORED_SYSTEM_LIBRARIES OR NOT DEFINED _gst_SRT_REGEX_PATCH)
    include("${CMAKE_CURRENT_LIST_DIR}/../Helpers.cmake")
endif()

if(ANDROID)
    # Match Cerbero gstreamer-1.0.mk:NEEDS_TEXTREL_ERROR / NEEDS_BSYMBOLIC_FIX —
    # arm64-v8a needs neither (no asm textrels, default binding semantics).
    if(CMAKE_ANDROID_ARCH_ABI MATCHES "^armeabi" OR CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
        set(_GST_MOBILE_NEEDS_TEXTREL_ERROR TRUE)
        set(_GST_MOBILE_NEEDS_BSYMBOLIC_FIX TRUE)
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
        set(_GST_MOBILE_NEEDS_BSYMBOLIC_FIX TRUE)
    endif()
endif()

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

    if(NOT EXISTS "${GStreamer_NDK_BUILD_PATH}/GStreamer.java")
        message(FATAL_ERROR "GStreamer.java not found at ${GStreamer_NDK_BUILD_PATH}. "
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

# ── Computation phase (function to avoid variable leakage) ───────────────────
_qgc_compute_android_mobile_target(
    _gst_mobile_plugins _gst_mobile_apis _gst_validate_components
    _gst_wa_libs PLUGINS_DECLARATION PLUGINS_REGISTRATION
    G_IO_MODULES_DECLARE G_IO_MODULES_LOAD
)
list(TRANSFORM G_IO_MODULES PREPEND "gio" OUTPUT_VARIABLE G_IO_MODULES_LIBS)

if(GStreamer_DEBUG)
    message(STATUS "[GstMobile] GStreamer_ROOT_DIR = ${GStreamer_ROOT_DIR}")
    message(STATUS "[GstMobile] GSTREAMER_LIB_PATH = ${GSTREAMER_LIB_PATH}")
endif()

# ── Side-effect phase: target creation ──────────────────────────────────────
# Pre-condition: find_package(GStreamer) must have run; check before any
# add_library so a misordered call doesn't leave a half-built target behind.
if (NOT GStreamer_FOUND)
    message(FATAL_ERROR "find_package(GStreamer) must complete before _qgc_create_android_mobile_target(). "
        "Ensure find_package(GStreamer) has completed successfully.")
endif()

if (GSTREAMER_IS_MOBILE AND (NOT TARGET GStreamer::mobile))
    if (NOT ANDROID)
        message(FATAL_ERROR "_qgc_create_android_mobile_target is Android-only; iOS uses the xcframework path.")
    endif()

    set_source_files_properties("${GStreamer_Mobile_MODULE_NAME}.c" PROPERTIES GENERATED TRUE)
    add_library(GStreamerMobile SHARED "${GStreamer_Mobile_MODULE_NAME}.c")
    add_library(GStreamer::mobile ALIAS GStreamerMobile)

    set_target_properties(
        GStreamerMobile
        PROPERTIES
            LIBRARY_OUTPUT_NAME ${GStreamer_Mobile_MODULE_NAME}
            LINKER_LANGUAGE CXX
    )
endif()

if(GSTREAMER_IS_MOBILE AND TARGET GStreamerMobile)
    if(PC_GStreamer_VERSION)
        set_target_properties(
            GStreamerMobile
            PROPERTIES
                VERSION ${PC_GStreamer_VERSION}
                SOVERSION ${PC_GStreamer_VERSION}
        )
    endif()

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
    _gst_resolve_and_link_libraries(GStreamerMobile PRIVATE "${_gst_mobile_core_libs}" "${_gst_mobile_core_hints}" WARN_MISSING)

    target_include_directories(
        GStreamerMobile
        PRIVATE
            $<TARGET_PROPERTY:GStreamer::GStreamer,INTERFACE_INCLUDE_DIRECTORIES>
    )

    if(DEFINED _GST_MOBILE_NEEDS_TEXTREL_ERROR)
        target_link_options(GStreamerMobile PRIVATE "-Wl,-z,text")
    endif()

    if(DEFINED _GST_MOBILE_NEEDS_BSYMBOLIC_FIX)
        target_link_options(GStreamerMobile PRIVATE "-Wl,-Bsymbolic")
    endif()

    if(ANDROID)
        target_link_options(GStreamerMobile PRIVATE "-Wl,--export-dynamic")
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

        add_custom_target("copyjavasource_${CMAKE_ANDROID_ARCH_ABI}")

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
        endif()

        target_link_libraries(GStreamerMobile PRIVATE GStreamer::gio_modules)
    endif()
    set(GStreamerMobile_mobile_FOUND TRUE)

    configure_file("${CMAKE_SOURCE_DIR}/src/VideoManager/VideoReceiver/GStreamer/gst_static_plugins.c.in"
        "${GStreamer_Mobile_MODULE_NAME}.c")
endif()

set(GStreamerMobile_FIND_COMPONENTS ${_gst_validate_components})

if(fonts IN_LIST GStreamerMobile_FIND_COMPONENTS)
    set(GStreamer_UBUNTU_R_TTF "${GStreamer_NDK_BUILD_PATH}/fontconfig/fonts/Ubuntu-R.ttf"
        CACHE FILEPATH "Path to Ubuntu-R.ttf")
    set(GStreamer_FONTS_CONF "${GStreamer_NDK_BUILD_PATH}/fontconfig/fonts.conf"
        CACHE FILEPATH "Path to fonts.conf")
    if(EXISTS "${GStreamer_UBUNTU_R_TTF}" AND EXISTS "${GStreamer_FONTS_CONF}")
        set(GStreamerMobile_fonts_FOUND ON)
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
        if(TARGET GStreamerMobile)
            add_dependencies(GStreamerMobile copyfontsres_${CMAKE_ANDROID_ARCH_ABI})
        endif()
    else()
        set(GStreamerMobile_fonts_FOUND OFF)
    endif()
endif()

if(ca_certificates IN_LIST GStreamerMobile_FIND_COMPONENTS)
    set(GStreamer_CA_BUNDLE "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt"
        CACHE FILEPATH "Path to ca-certificates bundle")
    if(EXISTS "${GStreamer_CA_BUNDLE}")
        set(GStreamerMobile_ca_certificates_FOUND ON)
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
        if(TARGET GStreamerMobile)
            add_dependencies(GStreamerMobile copycacertificatesres_${CMAKE_ANDROID_ARCH_ABI})
        endif()
    else()
        set(GStreamerMobile_ca_certificates_FOUND OFF)
    endif()
endif()

if(TARGET GStreamerMobile AND TARGET copyjavasource_${CMAKE_ANDROID_ARCH_ABI})
    add_dependencies(GStreamerMobile copyjavasource_${CMAKE_ANDROID_ARCH_ABI})
endif()

include(FindPackageHandleStandardArgs)
set(_gst_plugins ${GStreamerMobile_FIND_COMPONENTS})
list(REMOVE_ITEM _gst_plugins fonts ca_certificates mobile)
list(REMOVE_DUPLICATES _gst_plugins)
foreach(_gst_PLUGIN IN LISTS _gst_plugins)
    set(GStreamerMobile_${_gst_PLUGIN}_FOUND "${GStreamer_${_gst_PLUGIN}_FOUND}")
endforeach()
if(GSTREAMER_IS_MOBILE AND TARGET GStreamerMobile)
    if(GStreamer_DEBUG)
        message(STATUS "[GstMobile] WHOLE_ARCHIVE plugin libs: ${_gst_wa_libs}")
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
set(GStreamerMobile_FOUND TRUE)

endmacro()
