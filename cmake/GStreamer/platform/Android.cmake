# Android GStreamer SDK discovery — invoked by Orchestrator.cmake.

if(_qgc_gstreamer_android_included)
    return()
endif()
set(_qgc_gstreamer_android_included TRUE)

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

        _qgc_android_abi_info("${CMAKE_ANDROID_ARCH_ABI}" GStreamer_ABI)
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
# Wrap each item of INPUT_LIST as MACRO_NAME(item), join, return via OUT_VAR.
function(_gst_generate_macro_list INPUT_LIST MACRO_NAME OUT_VAR)
    list(TRANSFORM ${INPUT_LIST} PREPEND "\n${MACRO_NAME}\(" OUTPUT_VARIABLE _result)
    list(TRANSFORM _result APPEND "\)")
    if(_result)
        set(_result "${_result};")
    endif()
    set(${OUT_VAR} "${_result}" PARENT_SCOPE)
endfunction()

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
    set(_gst_dropped_plugins)
    foreach(_p IN LISTS _gst_mobile_plugins)
        if(GStreamer_${_p}_FOUND)
            find_library(_gst_plugin_lib_${CMAKE_ANDROID_ARCH_ABI}_${_p} gst${_p}
                HINTS "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0"
                NO_DEFAULT_PATH
            )
            if(NOT _gst_plugin_lib_${CMAKE_ANDROID_ARCH_ABI}_${_p})
                message(WARNING "GStreamerMobile: Plugin '${_p}' has pkg-config but no static library, excluding from init")
                list(APPEND _gst_dropped_plugins ${_p})
                continue()
            endif()
            list(APPEND _gst_found_plugins ${_p})
        else()
            message(WARNING "GStreamerMobile: Plugin '${_p}' not found in SDK, excluding from init")
            list(APPEND _gst_dropped_plugins ${_p})
        endif()
    endforeach()
    _gst_restore_find_suffixes()

    # Backstop: fail configure if a runtime-required plugin was dropped (partial
    # per-ABI SDK). Alternate-aware: videoconvert+videoscale satisfies videoconvertscale.
    if(_gst_dropped_plugins)
        gstreamer_runtime_required_plugins(_gst_required)
        set(_gst_missing_required)
        foreach(_req IN LISTS _gst_required)
            gstreamer_plugin_satisfy_sets(PLUGIN "${_req}" OUT_VAR _req_sets)
            set(_req_ok FALSE)
            foreach(_set IN LISTS _req_sets)
                string(REPLACE "+" ";" _members "${_set}")
                set(_set_ok TRUE)
                foreach(_m IN LISTS _members)
                    if(NOT _m IN_LIST _gst_found_plugins)
                        set(_set_ok FALSE)
                        break()
                    endif()
                endforeach()
                if(_set_ok)
                    set(_req_ok TRUE)
                    break()
                endif()
            endforeach()
            if(NOT _req_ok)
                list(APPEND _gst_missing_required "${_req}")
            endif()
        endforeach()
        if(_gst_missing_required)
            list(JOIN _gst_missing_required ", " _gst_missing_required_str)
            message(FATAL_ERROR "GStreamerMobile: runtime-required plugins missing static libraries "
                "for ABI ${CMAKE_ANDROID_ARCH_ABI}: ${_gst_missing_required_str}. "
                "The GStreamer Android SDK is incomplete or corrupt — re-download it.")
        endif()
    endif()
    set(_gst_mobile_plugins ${_gst_found_plugins})

    if(GStreamer_DEBUG)
        message(STATUS "[GstMobile] Found plugins with .a: ${_gst_mobile_plugins}")
        foreach(_dbg_p IN LISTS _gst_mobile_plugins)
            message(STATUS "[GstMobile]   ${_dbg_p} -> ${_gst_plugin_lib_${CMAKE_ANDROID_ARCH_ABI}_${_dbg_p}}")
        endforeach()
    endif()

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
        if(GStreamer_${_gst_PLUGIN}_FOUND AND _gst_plugin_lib_${CMAKE_ANDROID_ARCH_ABI}_${_gst_PLUGIN})
            list(APPEND _gst_wa_libs "${_gst_plugin_lib_${CMAKE_ANDROID_ARCH_ABI}_${_gst_PLUGIN}}")
        endif()
    endforeach()

    set(${PLUGINS_OUT}         "${_gst_mobile_plugins}"    PARENT_SCOPE)
    set(${APIS_OUT}            "${_gst_mobile_apis}"       PARENT_SCOPE)
    set(${FOUND_COMPONENTS_OUT} "${_gst_validate_components}" PARENT_SCOPE)
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

# Pre-condition: find_package(GStreamer) must have run before any computation
# that reads GStreamer_*_FOUND or creates targets.
if (NOT GStreamer_FOUND)
    message(FATAL_ERROR "find_package(GStreamer) must complete before _qgc_create_android_mobile_target(). "
        "Ensure find_package(GStreamer) has completed successfully.")
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

    if (ANDROID)
        set(GSTREAMER_PLUGINS_CLASSES)
        foreach(LOCAL_PLUGIN IN LISTS _gst_mobile_plugins)
            file(GLOB_RECURSE
                LOCAL_PLUGIN_CLASS
                FOLLOW_SYMLINKS
                CONFIGURE_DEPENDS
                RELATIVE "${GStreamer_NDK_BUILD_PATH}"
                "${GStreamer_NDK_BUILD_PATH}/${LOCAL_PLUGIN}/*.java"
            )
            list(APPEND GSTREAMER_PLUGINS_CLASSES ${LOCAL_PLUGIN_CLASS})
        endforeach()

        # androiddeployqt mirrors the package source dir into android-build/src at deploy, deleting
        # anything else there — so plugin java must land in the package source dir, not the build dir.
        if(QGC_ANDROID_PACKAGE_SOURCE_DIR)
            set(_gst_java_dest_dir "${QGC_ANDROID_PACKAGE_SOURCE_DIR}/src")
        else()
            set(_gst_java_dest_dir "${GStreamer_JAVA_SRC_DIR}")
        endif()

        add_custom_target("copyjavasource_${CMAKE_ANDROID_ARCH_ABI}")

        foreach(LOCAL_FILE IN LISTS GSTREAMER_PLUGINS_CLASSES)
            cmake_path(GET LOCAL_FILE FILENAME _java_filename)
            cmake_path(GET LOCAL_FILE PARENT_PATH _java_subdir)
            string(MAKE_C_IDENTIFIER "cp_${LOCAL_FILE}" COPYJAVASOURCE_TGT)
            add_custom_target(
                ${COPYJAVASOURCE_TGT}
                COMMAND
                    "${CMAKE_COMMAND}" -E make_directory
                    "${_gst_java_dest_dir}/org/freedesktop/gstreamer/${_java_subdir}"
                COMMAND
                    "${CMAKE_COMMAND}" -E copy_if_different
                    "${GStreamer_NDK_BUILD_PATH}/${LOCAL_FILE}"
                    "${_gst_java_dest_dir}/org/freedesktop/gstreamer/${_java_subdir}/"
                BYPRODUCTS
                    "${_gst_java_dest_dir}/org/freedesktop/gstreamer/${_java_subdir}/${_java_filename}"
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
    # The static-plugin registration shim (gst_init_static_plugins) is generated on the
    # APP target (single core) by the app CMakeLists, not as a separate .so source here.
endif()

set(GStreamerMobile_FIND_COMPONENTS ${_gst_validate_components})

# Same clobber rule as copyjavasource: androiddeployqt mirrors the package source dir into
# android-build/assets at deploy, deleting anything else there — so fonts/CA assets must land in
# the package source dir's assets/, not the build dir, or they never reach the APK.
if(QGC_ANDROID_PACKAGE_SOURCE_DIR)
    set(_gst_assets_dest_dir "${QGC_ANDROID_PACKAGE_SOURCE_DIR}/assets")
else()
    set(_gst_assets_dest_dir "${GStreamer_ASSETS_DIR}")
endif()

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
                "${_gst_assets_dest_dir}/fontconfig/fonts/truetype/"
            COMMAND
                "${CMAKE_COMMAND}" -E copy_if_different
                "${GStreamer_UBUNTU_R_TTF}"
                "${_gst_assets_dest_dir}/fontconfig/fonts/truetype/"
            COMMAND
                "${CMAKE_COMMAND}" -E copy_if_different
                "${GStreamer_FONTS_CONF}"
                "${_gst_assets_dest_dir}/fontconfig/"
            BYPRODUCTS
                "${_gst_assets_dest_dir}/fontconfig/fonts/truetype/Ubuntu-R.ttf"
                "${_gst_assets_dest_dir}/fontconfig/fonts.conf"
        )
        # INTERFACE GStreamer::mobile can't carry build deps; attach asset copy to the app.
        if(TARGET ${CMAKE_PROJECT_NAME})
            add_dependencies(${CMAKE_PROJECT_NAME} copyfontsres_${CMAKE_ANDROID_ARCH_ABI})
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
                "${_gst_assets_dest_dir}/ssl/certs/"
            COMMAND
                "${CMAKE_COMMAND}" -E copy_if_different
                "${GStreamer_CA_BUNDLE}"
                "${_gst_assets_dest_dir}/ssl/certs/"
            BYPRODUCTS "${_gst_assets_dest_dir}/ssl/certs/ca-certificates.crt"
        )
        if(TARGET ${CMAKE_PROJECT_NAME})
            add_dependencies(${CMAKE_PROJECT_NAME} copycacertificatesres_${CMAKE_ANDROID_ARCH_ABI})
        endif()
    else()
        set(GStreamerMobile_ca_certificates_FOUND OFF)
    endif()
endif()

if(TARGET ${CMAKE_PROJECT_NAME} AND TARGET copyjavasource_${CMAKE_ANDROID_ARCH_ABI})
    add_dependencies(${CMAKE_PROJECT_NAME} copyjavasource_${CMAKE_ANDROID_ARCH_ABI})
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
        # Whole-archive the plugin .a's into the app so every gst_plugin_<name>_register() and its
        # GType objects survive the linker's dead-strip and register into the app's single core.
        target_link_options(GStreamerMobile INTERFACE
            "LINKER:--whole-archive"
            ${_gst_wa_libs}
            "LINKER:--no-whole-archive"
        )
    endif()

    # The whole-archived plugin .a's reference GStreamer helper libraries
    # (gstbase/rtp/rtpbase/app/video/audio/tag/pbutils/net/sdp/rtsp/controller)
    # absent from PC_GStreamer_LIBRARIES; each found GStreamer::<comp> target
    # carries them as absolute .a paths via PC_GStreamer_<comp>_STATIC_LIBRARIES.
    set(_gst_mobile_helper_targets)
    foreach(_gst_comp IN LISTS _gst_mobile_plugins)
        if(TARGET GStreamer::${_gst_comp})
            list(APPEND _gst_mobile_helper_targets GStreamer::${_gst_comp})
        endif()
    endforeach()
    foreach(_gst_comp IN LISTS _gst_mobile_apis)
        if(GStreamer_${_gst_comp}_FOUND AND TARGET GStreamer::${_gst_comp})
            list(APPEND _gst_mobile_helper_targets GStreamer::${_gst_comp})
        endif()
    endforeach()
    if(_gst_mobile_helper_targets)
        target_link_options(GStreamerMobile INTERFACE "LINKER:--start-group")
        target_link_libraries(GStreamerMobile INTERFACE ${_gst_mobile_helper_targets})
        target_link_options(GStreamerMobile INTERFACE "LINKER:--end-group")
    endif()

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
