option(GStreamer_REQUIRE_CHECKSUM "Fail if SDK download checksum cannot be verified" OFF)
option(GStreamer_DEBUG "Print GStreamer CMake debug messages" OFF)

function(gstreamer_get_package_url PLATFORM VERSION OUTPUT_VAR)
    set(_base "https://gstreamer.freedesktop.org")
    set(_gl_base "https://gitlab.freedesktop.org/gstreamer/gstreamer/-/archive/${VERSION}/gstreamer-${VERSION}.tar.gz")

    if(PLATFORM STREQUAL "android")
        set(_url "${_base}/data/pkg/android/${VERSION}/gstreamer-1.0-android-universal-${VERSION}.tar.xz")
    elseif(PLATFORM STREQUAL "ios")
        set(_url "${_base}/data/pkg/ios/${VERSION}/gstreamer-1.0-devel-${VERSION}-ios-universal.pkg")
    elseif(PLATFORM STREQUAL "macos")
        set(_url "${_base}/data/pkg/macos/${VERSION}/gstreamer-1.0-${VERSION}-universal.pkg")
    elseif(PLATFORM STREQUAL "macos_devel")
        set(_url "${_base}/data/pkg/macos/${VERSION}/gstreamer-1.0-devel-${VERSION}-universal.pkg")
    elseif(PLATFORM MATCHES "^windows_msvc_(x64|arm64)$")
        if(VERSION VERSION_GREATER_EQUAL "1.28.0")
            set(_ext "exe")
        else()
            set(_ext "msi")
        endif()
        if(CMAKE_MATCH_1 STREQUAL "x64")
            set(_arch "x86_64")
        else()
            set(_arch "arm64")
        endif()
        set(_url "${_base}/data/pkg/windows/${VERSION}/msvc/gstreamer-1.0-msvc-${_arch}-${VERSION}.${_ext}")
    elseif(PLATFORM STREQUAL "good_plugins")
        set(_url "${_base}/src/gst-plugins-good/gst-plugins-good-${VERSION}.tar.xz")
    elseif(PLATFORM STREQUAL "good_plugins_qt6")
        set(_url "${_gl_base}?path=subprojects/gst-plugins-good/ext/qt6")
    elseif(PLATFORM STREQUAL "bad_plugins_qt6d3d11")
        set(_url "${_gl_base}?path=subprojects/gst-plugins-bad/ext/qt6d3d11")
    elseif(PLATFORM STREQUAL "monorepo")
        set(_url "${_gl_base}")
    else()
        message(FATAL_ERROR "gstreamer_get_package_url: Unknown platform '${PLATFORM}'")
    endif()

    set(${OUTPUT_VAR} "${_url}" PARENT_SCOPE)
endfunction()

function(gstreamer_get_s3_mirror_url PLATFORM VERSION OUTPUT_VAR)
    set(_s3_base "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer")

    if(PLATFORM STREQUAL "android")
        set(_dir "android")
    elseif(PLATFORM STREQUAL "ios")
        set(_dir "ios")
    elseif(PLATFORM STREQUAL "macos" OR PLATFORM STREQUAL "macos_devel")
        set(_dir "macos")
    elseif(PLATFORM STREQUAL "windows_msvc_x64" OR PLATFORM STREQUAL "windows_msvc_arm64")
        set(_dir "windows")
    else()
        set(${OUTPUT_VAR} "" PARENT_SCOPE)
        return()
    endif()

    gstreamer_get_package_url(${PLATFORM} ${VERSION} _primary_url)
    cmake_path(GET _primary_url FILENAME _filename)

    set(${OUTPUT_VAR} "${_s3_base}/${_dir}/${_filename}" PARENT_SCOPE)
endfunction()

function(gstreamer_get_fallback_checksum PLATFORM VERSION OUTPUT_VAR)
    set(_checksum "")

    if(DEFINED QGC_BUILD_CONFIG_CONTENT)
        string(JSON _hash ERROR_VARIABLE _err
            GET "${QGC_BUILD_CONFIG_CONTENT}" "gstreamer_checksums" "${VERSION}" "${PLATFORM}")
        if(NOT _err AND _hash)
            set(_checksum "SHA256=${_hash}")
        endif()
    endif()

    set(${OUTPUT_VAR} "${_checksum}" PARENT_SCOPE)
endfunction()

function(_gstreamer_fallback_or_warn PLATFORM VERSION OUTPUT_VAR MESSAGE)
    gstreamer_get_fallback_checksum(${PLATFORM} ${VERSION} _fallback_hash)
    if(_fallback_hash)
        message(WARNING "GStreamer: ${MESSAGE} for ${PLATFORM} ${VERSION}; using pinned fallback checksum.")
        set(${OUTPUT_VAR} "${_fallback_hash}" PARENT_SCOPE)
        return()
    endif()
    if(GStreamer_REQUIRE_CHECKSUM)
        message(FATAL_ERROR
            "GStreamer: ${MESSAGE} for ${PLATFORM} ${VERSION} "
            "and no pinned fallback exists. Set GStreamer_REQUIRE_CHECKSUM=OFF to bypass.")
    endif()
    message(WARNING "GStreamer: ${MESSAGE} for ${PLATFORM} ${VERSION}; continuing without verification.")
    set(${OUTPUT_VAR} "" PARENT_SCOPE)
endfunction()

function(_gstreamer_download_sidecar URL DEST_FILE)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_deps/checksums")
    set(_tmp "${DEST_FILE}.tmp")
    foreach(_attempt RANGE 1 3)
        file(DOWNLOAD "${URL}" "${_tmp}" STATUS _status TIMEOUT 30 TLS_VERIFY ON)
        list(GET _status 0 _code)
        if(_code EQUAL 0)
            file(RENAME "${_tmp}" "${DEST_FILE}")
            set(_dl_ok TRUE PARENT_SCOPE)
            return()
        endif()
        list(GET _status 1 _msg)
        message(STATUS "GStreamer: Checksum download attempt ${_attempt}/3 failed: ${_msg}")
        file(REMOVE "${_tmp}")
    endforeach()
    set(_dl_ok FALSE PARENT_SCOPE)
endfunction()

function(gstreamer_fetch_checksum PLATFORM VERSION OUTPUT_VAR)
    gstreamer_get_package_url(${PLATFORM} ${VERSION} _pkg_url)

    string(FIND "${_pkg_url}" "?" _qs_pos)
    if(NOT _qs_pos EQUAL -1)
        message(STATUS "GStreamer: Skipping checksum for ${PLATFORM} ${VERSION} (URL contains query string)")
        set(${OUTPUT_VAR} "" PARENT_SCOPE)
        return()
    endif()

    set(_checksum_url "${_pkg_url}.sha256sum")
    string(MD5 _url_hash "${_checksum_url}")
    set(_checksum_file "${CMAKE_BINARY_DIR}/_deps/checksums/${_url_hash}.sha256sum")

    foreach(_round RANGE 1 2)
        if(NOT EXISTS "${_checksum_file}")
            _gstreamer_download_sidecar("${_checksum_url}" "${_checksum_file}")
            if(NOT _dl_ok)
                _gstreamer_fallback_or_warn(${PLATFORM} ${VERSION} _result "Checksum sidecar unavailable")
                set(${OUTPUT_VAR} "${_result}" PARENT_SCOPE)
                return()
            endif()
        endif()

        file(READ "${_checksum_file}" _content)
        string(STRIP "${_content}" _content)
        string(REGEX MATCH "([0-9a-fA-F]{64})" _match "${_content}")
        if(_match)
            set(${OUTPUT_VAR} "SHA256=${CMAKE_MATCH_1}" PARENT_SCOPE)
            return()
        endif()

        message(STATUS "GStreamer: Could not parse checksum for ${PLATFORM} ${VERSION}")
        file(REMOVE "${_checksum_file}")
    endforeach()

    _gstreamer_fallback_or_warn(${PLATFORM} ${VERSION} _result "Checksum content invalid/unparseable")
    set(${OUTPUT_VAR} "${_result}" PARENT_SCOPE)
endfunction()

macro(_gstreamer_parse_expected_hash _HASH_SPEC _OUT_ALGO _OUT_EXPECTED)
    set(_gph_valid MD5 SHA1 SHA224 SHA256 SHA384 SHA512 SHA3_224 SHA3_256 SHA3_384 SHA3_512)
    string(REGEX MATCH "^([A-Za-z0-9_]+)=(.+)$" _gph_match "${${_HASH_SPEC}}")
    if(NOT _gph_match)
        message(FATAL_ERROR "gstreamer_resilient_download: Invalid EXPECTED_HASH format '${${_HASH_SPEC}}' (expected ALGO=hex)")
    endif()
    set(${_OUT_ALGO} "${CMAKE_MATCH_1}")
    set(${_OUT_EXPECTED} "${CMAKE_MATCH_2}")
    list(FIND _gph_valid "${${_OUT_ALGO}}" _gph_idx)
    if(_gph_idx EQUAL -1)
        message(FATAL_ERROR "gstreamer_resilient_download: Unsupported hash algorithm '${${_OUT_ALGO}}'")
    endif()
endmacro()

function(gstreamer_resilient_download)
    cmake_parse_arguments(ARG "ALLOW_FAILURE" "FILENAME;DESTINATION_DIR;RESULT_VAR;TIMEOUT;INACTIVITY_TIMEOUT;EXPECTED_HASH" "URLS" ${ARGN})

    foreach(_required FILENAME DESTINATION_DIR RESULT_VAR)
        if(NOT ARG_${_required})
            message(FATAL_ERROR "gstreamer_resilient_download: ${_required} is required")
        endif()
    endforeach()
    if(NOT ARG_URLS)
        message(FATAL_ERROR "gstreamer_resilient_download: at least one URL is required")
    endif()

    if(NOT ARG_TIMEOUT)
        set(ARG_TIMEOUT 120)
    endif()
    if(NOT ARG_INACTIVITY_TIMEOUT)
        set(ARG_INACTIVITY_TIMEOUT 60)
    endif()

    set(_dest "${ARG_DESTINATION_DIR}/${ARG_FILENAME}")

    if(EXISTS "${_dest}")
        if(ARG_EXPECTED_HASH)
            _gstreamer_parse_expected_hash(ARG_EXPECTED_HASH _hash_algo _expected)
            file(${_hash_algo} "${_dest}" _cached_hash)
            if(NOT _cached_hash STREQUAL "${_expected}")
                message(STATUS "GStreamer: Cached ${ARG_FILENAME} failed ${_hash_algo} check, re-downloading")
                file(REMOVE "${_dest}")
            else()
                set(${ARG_RESULT_VAR} "${_dest}" PARENT_SCOPE)
                return()
            endif()
        else()
            message(STATUS "GStreamer: Using cached ${ARG_FILENAME} (no checksum verification)")
            set(${ARG_RESULT_VAR} "${_dest}" PARENT_SCOPE)
            return()
        endif()
    endif()

    file(MAKE_DIRECTORY "${ARG_DESTINATION_DIR}")
    set(_tmp "${_dest}.tmp")
    set(_tried_urls "")

    foreach(_url IN LISTS ARG_URLS)
        if(NOT _url)
            continue()
        endif()
        message(STATUS "GStreamer: Downloading ${ARG_FILENAME} from ${_url}")
        file(DOWNLOAD "${_url}" "${_tmp}"
            STATUS _status
            SHOW_PROGRESS
            TIMEOUT ${ARG_TIMEOUT}
            INACTIVITY_TIMEOUT ${ARG_INACTIVITY_TIMEOUT}
            TLS_VERIFY ON
        )
        list(GET _status 0 _code)
        if(_code EQUAL 0)
            if(ARG_EXPECTED_HASH)
                _gstreamer_parse_expected_hash(ARG_EXPECTED_HASH _hash_algo _expected)
                file(${_hash_algo} "${_tmp}" _actual_hash)
                if(NOT _actual_hash STREQUAL "${_expected}")
                    message(WARNING "GStreamer: ${_hash_algo} mismatch for ${ARG_FILENAME} from ${_url}")
                    file(REMOVE "${_tmp}")
                    list(APPEND _tried_urls "${_url}")
                    continue()
                endif()
            endif()
            file(RENAME "${_tmp}" "${_dest}")
            set(${ARG_RESULT_VAR} "${_dest}" PARENT_SCOPE)
            return()
        endif()
        list(GET _status 1 _error)
        message(WARNING "GStreamer: Failed to download from ${_url}: ${_error}")
        file(REMOVE "${_tmp}")
        list(APPEND _tried_urls "${_url}")
    endforeach()

    if(ARG_ALLOW_FAILURE)
        message(STATUS "GStreamer: All download URLs failed for ${ARG_FILENAME} (allowing failure). Tried: ${_tried_urls}")
        set(${ARG_RESULT_VAR} "" PARENT_SCOPE)
        return()
    endif()

    message(FATAL_ERROR "GStreamer: All download URLs failed for ${ARG_FILENAME}.\n"
        "Tried: ${_tried_urls}\n"
        "Install manually from https://gstreamer.freedesktop.org/download/ or set GStreamer_ROOT_DIR.")
endfunction()

function(gstreamer_download_sdk PLATFORM VERSION FILENAME DESTINATION_DIR RESULT_VAR)
    cmake_parse_arguments(_DL "ALLOW_FAILURE" "" "" ${ARGN})

    gstreamer_get_package_url(${PLATFORM} ${VERSION} _url)
    gstreamer_get_s3_mirror_url(${PLATFORM} ${VERSION} _s3_url)
    gstreamer_fetch_checksum(${PLATFORM} ${VERSION} _hash)

    set(_urls "${_url}")
    if(_s3_url)
        list(APPEND _urls "${_s3_url}")
    endif()

    set(_args
        URLS ${_urls}
        FILENAME "${FILENAME}"
        DESTINATION_DIR "${DESTINATION_DIR}"
        RESULT_VAR _result
    )
    if(_hash)
        list(APPEND _args EXPECTED_HASH "${_hash}")
    endif()
    if(_DL_ALLOW_FAILURE)
        list(APPEND _args ALLOW_FAILURE)
    endif()

    gstreamer_resilient_download(${_args})
    set(${RESULT_VAR} "${_result}" PARENT_SCOPE)
endfunction()

# gstreamer_get_recommended_version(<VERSION_STRING> <OUTPUT_VAR>)
# Parses major.minor from VERSION_STRING, looks up the patch from
# build-config.json (QGC_GSTREAMER_PATCH_<M>_<N>), and returns M.N.P.
function(gstreamer_get_recommended_version VERSION_STRING OUTPUT_VAR)
    string(REPLACE "." ";" _ver_list "${VERSION_STRING}")
    list(GET _ver_list 0 _major)
    list(GET _ver_list 1 _minor)
    set(_detected_patch "")
    if("${VERSION_STRING}" MATCHES "^${_major}\\.${_minor}\\.([0-9]+)")
        set(_detected_patch "${CMAKE_MATCH_1}")
    endif()

    set(_var_name "QGC_GSTREAMER_PATCH_${_major}_${_minor}")
    if(DEFINED ${_var_name})
        set(_patch "${${_var_name}}")
    elseif(_detected_patch)
        set(_patch "${_detected_patch}")
        message(STATUS
            "No patch mapping for GStreamer ${_major}.${_minor}; using detected patch ${_patch}.")
    else()
        set(_patch 0)
        message(WARNING "No patch mapping for GStreamer ${_major}.${_minor} — defaulting to .0. "
                        "This version is not used by any platform target in .github/build-config.json.")
    endif()

    set(${OUTPUT_VAR} "${_major}.${_minor}.${_patch}" PARENT_SCOPE)
endfunction()

function(gstreamer_install_gio_modules)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;DEST_DIR;EXTENSION" "" ${ARGN})

    if(NOT EXISTS "${ARG_SOURCE_DIR}")
        message(WARNING "gstreamer_install_gio_modules: SOURCE_DIR does not exist: ${ARG_SOURCE_DIR}")
        return()
    endif()

    file(GLOB _modules "${ARG_SOURCE_DIR}/*.${ARG_EXTENSION}")

    if(_modules)
        install(FILES ${_modules} DESTINATION "${ARG_DEST_DIR}")
    endif()
endfunction()

function(gstreamer_install_plugins)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;DEST_DIR;EXTENSION;PREFIX" "" ${ARGN})

    if(NOT EXISTS "${ARG_SOURCE_DIR}")
        message(WARNING "gstreamer_install_plugins: SOURCE_DIR does not exist: ${ARG_SOURCE_DIR}")
        return()
    endif()

    file(GLOB _all_plugins "${ARG_SOURCE_DIR}/${ARG_PREFIX}*.${ARG_EXTENSION}")

    set(_plugins_to_install "")
    foreach(_plugin_path IN LISTS _all_plugins)
        get_filename_component(_plugin_name "${_plugin_path}" NAME)
        foreach(_allowed IN LISTS GSTREAMER_PLUGINS)
            if(_plugin_name MATCHES "^${ARG_PREFIX}${_allowed}([^a-zA-Z0-9]|$)")
                list(APPEND _plugins_to_install "${_plugin_path}")
                break()
            endif()
        endforeach()
    endforeach()

    if(_plugins_to_install)
        install(FILES ${_plugins_to_install} DESTINATION "${ARG_DEST_DIR}")
    endif()
endfunction()

macro(_gst_configure_pkg_config)
    cmake_parse_arguments(_GPC "DONT_DEFINE_PREFIX" "PKG_CONFIG_EXE" "LIBDIR" ${ARGN})
    if(_GPC_PKG_CONFIG_EXE)
        set(ENV{PKG_CONFIG} "${_GPC_PKG_CONFIG_EXE}")
        set(PKG_CONFIG_EXECUTABLE "$ENV{PKG_CONFIG}" CACHE FILEPATH "pkg-config executable" FORCE)
    endif()
    if(_GPC_LIBDIR)
        set(ENV{PKG_CONFIG_PATH} "")
        if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
            set(ENV{PKG_CONFIG_LIBDIR} "${_GPC_LIBDIR}")
        else()
            string(REPLACE ";" ":" _gpc_libdir "${_GPC_LIBDIR}")
            set(ENV{PKG_CONFIG_LIBDIR} "${_gpc_libdir}")
            unset(_gpc_libdir)
        endif()
    endif()
    if(_GPC_DONT_DEFINE_PREFIX AND NOT "--dont-define-prefix" IN_LIST PKG_CONFIG_ARGN)
        list(APPEND PKG_CONFIG_ARGN --dont-define-prefix)
    endif()
    unset(_GPC_DONT_DEFINE_PREFIX)
    unset(_GPC_PKG_CONFIG_EXE)
    unset(_GPC_LIBDIR)
    unset(_GPC_UNPARSED_ARGUMENTS)
    unset(_GPC_KEYWORDS_MISSING_VALUES)
endmacro()

macro(_gst_set_standard_paths)
    cmake_parse_arguments(_GSP "" "INCLUDE_PATH" "" ${ARGN})
    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    if(_GSP_INCLUDE_PATH)
        set(GSTREAMER_INCLUDE_PATH "${_GSP_INCLUDE_PATH}")
    else()
        set(GSTREAMER_INCLUDE_PATH "${GStreamer_ROOT_DIR}/include")
    endif()
    # Remove any prior --define-variable entries before appending fresh ones
    list(FILTER PKG_CONFIG_ARGN EXCLUDE REGEX "^--define-variable=")
    list(APPEND PKG_CONFIG_ARGN
        --define-variable=prefix=${GStreamer_ROOT_DIR}
        --define-variable=libdir=${GSTREAMER_LIB_PATH}
        --define-variable=includedir=${GSTREAMER_INCLUDE_PATH}
    )
    unset(_GSP_INCLUDE_PATH)
    unset(_GSP_UNPARSED_ARGUMENTS)
    unset(_GSP_KEYWORDS_MISSING_VALUES)
endmacro()

macro(_gst_normalize_and_validate_root)
    cmake_path(CONVERT "${GStreamer_ROOT_DIR}" TO_CMAKE_PATH_LIST GStreamer_ROOT_DIR NORMALIZE)
    if(NOT EXISTS "${GStreamer_ROOT_DIR}")
        message(FATAL_ERROR "GStreamer: SDK not found at '${GStreamer_ROOT_DIR}' — "
            "check installation or set GStreamer_ROOT_DIR")
    endif()
endmacro()

function(gstreamer_install_libs)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;DEST_DIR;EXTENSION" "" ${ARGN})

    if(NOT EXISTS "${ARG_SOURCE_DIR}")
        message(WARNING "gstreamer_install_libs: SOURCE_DIR does not exist: ${ARG_SOURCE_DIR}")
        return()
    endif()

    set(_blocked_prefixes
        /usr/lib /usr/local/lib /opt/homebrew/lib /opt/homebrew/opt
        "C:/Windows" "C:/Program Files" "C:/Program Files (x86)"
    )
    foreach(_prefix IN LISTS _blocked_prefixes)
        cmake_path(IS_PREFIX _prefix "${ARG_SOURCE_DIR}" NORMALIZE _is_system)
        if(_is_system)
            message(FATAL_ERROR
                "gstreamer_install_libs: refusing to copy from system/shared prefix '${ARG_SOURCE_DIR}'.\n"
                "This function copies ALL shared libraries unfiltered. Use an auto-downloaded SDK "
                "or set GStreamer_ROOT_DIR to an isolated installation.")
        endif()
    endforeach()

    file(GLOB _all_libs "${ARG_SOURCE_DIR}/*.${ARG_EXTENSION}")
    if(_all_libs)
        install(FILES ${_all_libs} DESTINATION "${ARG_DEST_DIR}")
    endif()
endfunction()

# Shared list of system libraries that should be linked by name, not resolved via find_library.
# Used by both FindGStreamer.cmake and FindGStreamerMobile.cmake.
if(NOT DEFINED _gst_IGNORED_SYSTEM_LIBRARIES)
    set(_gst_IGNORED_SYSTEM_LIBRARIES c c++ unwind m dl atomic)
    if(ANDROID)
        list(APPEND _gst_IGNORED_SYSTEM_LIBRARIES log GLESv2 EGL OpenSLES android vulkan)
    elseif(APPLE)
        list(APPEND _gst_IGNORED_SYSTEM_LIBRARIES iconv resolv System)
    elseif(WIN32)
        list(APPEND _gst_IGNORED_SYSTEM_LIBRARIES
            ws2_32 ole32 oleaut32 winmm shlwapi secur32 iphlpapi dnsapi
            userenv bcrypt crypt32 advapi32 kernel32 shell32 uuid)
    endif()
endif()
if(NOT DEFINED _gst_SRT_REGEX_PATCH)
    set(_gst_SRT_REGEX_PATCH "^:lib(.+)\\.(a|so|lib|dylib)$")
endif()

# Save/restore macros for CMAKE_FIND_LIBRARY_SUFFIXES/PREFIXES.
# Used by FindGStreamer.cmake and FindGStreamerMobile.cmake when resolving static libs.
macro(_gst_save_find_suffixes)
    set(_gst_saved_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(_gst_saved_prefixes ${CMAKE_FIND_LIBRARY_PREFIXES})
    set(CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
    if(GStreamer_USE_STATIC_LIBS)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    elseif(APPLE)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".dylib" ".so" ".tbd")
    elseif(UNIX)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".so")
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")
    endif()
endmacro()

macro(_gst_restore_find_suffixes)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${_gst_saved_suffixes})
    set(CMAKE_FIND_LIBRARY_PREFIXES ${_gst_saved_prefixes})
endmacro()

# _gst_resolve_and_link_libraries(<TARGET> <SCOPE> <LIBS_VAR> <HINTS_VAR> [HIDE] [WARN_MISSING])
#
# Resolves a list of library names via find_library and links them to TARGET with the given SCOPE
# (PRIVATE, INTERFACE, or PUBLIC). Handles SRT regex patching and system library passthrough.
#
#   HIDE         - Use -hidden-l (Apple) or --exclude-libs (Unix) to hide symbols
#   WARN_MISSING - Warn and skip missing libraries instead of failing
macro(_gst_resolve_and_link_libraries _grll_TARGET _grll_SCOPE _grll_LIBS_VAR _grll_HINTS_VAR)
    cmake_parse_arguments(_grll "HIDE;WARN_MISSING" "" "" ${ARGN})

    if(_grll_HIDE AND APPLE)
        target_link_directories(${_grll_TARGET} ${_grll_SCOPE} ${${_grll_HINTS_VAR}})
    endif()

    _gst_save_find_suffixes()

    foreach(_grll_LIB IN LISTS ${_grll_LIBS_VAR})
        if(_grll_LIB MATCHES "${_gst_SRT_REGEX_PATCH}")
            string(REGEX REPLACE "${_gst_SRT_REGEX_PATCH}" "\\1" _grll_LIB "${_grll_LIB}")
        endif()

        if("${_grll_LIB}" IN_LIST _gst_IGNORED_SYSTEM_LIBRARIES)
            target_link_libraries(${_grll_TARGET} ${_grll_SCOPE} "${_grll_LIB}")
            continue()
        endif()

        string(MAKE_C_IDENTIFIER "_gst_${_grll_LIB}" _grll_CACHE_VAR)
        if(DEFINED ${_grll_CACHE_VAR} AND "${${_grll_CACHE_VAR}}" MATCHES "NOTFOUND$")
            unset(${_grll_CACHE_VAR} CACHE)
        endif()
        if(NOT DEFINED ${_grll_CACHE_VAR} OR "${${_grll_CACHE_VAR}}" MATCHES "NOTFOUND$")
            if(_grll_WARN_MISSING)
                find_library(${_grll_CACHE_VAR} NAMES ${_grll_LIB} HINTS ${${_grll_HINTS_VAR}} NO_DEFAULT_PATH)
            else()
                find_library(${_grll_CACHE_VAR} NAMES ${_grll_LIB} HINTS ${${_grll_HINTS_VAR}}
                    NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)
            endif()
        endif()

        if(NOT ${_grll_CACHE_VAR})
            if(_grll_WARN_MISSING)
                message(WARNING "GStreamer: Library '${_grll_LIB}' not found in ${${_grll_HINTS_VAR}}, skipping")
            endif()
            continue()
        endif()

        if(_grll_HIDE AND APPLE)
            get_filename_component(_grll_NAME_WE "${${_grll_CACHE_VAR}}" NAME_WE)
            string(REGEX REPLACE "^lib" "" _grll_NAME_WE "${_grll_NAME_WE}")
            target_link_libraries(${_grll_TARGET} ${_grll_SCOPE} "-hidden-l${_grll_NAME_WE}")
        elseif(_grll_HIDE AND (UNIX OR ANDROID))
            target_link_libraries(${_grll_TARGET} ${_grll_SCOPE} -Wl,--exclude-libs,${${_grll_CACHE_VAR}})
        else()
            target_link_libraries(${_grll_TARGET} ${_grll_SCOPE} "${${_grll_CACHE_VAR}}")
        endif()
    endforeach()

    _gst_restore_find_suffixes()
endmacro()
