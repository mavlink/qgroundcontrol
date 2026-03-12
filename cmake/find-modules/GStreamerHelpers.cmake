# gstreamer_get_package_url(<PLATFORM> <VERSION> <OUTPUT_VAR>)
#   PLATFORM: android, ios, macos, macos_devel, windows_msvc_x64, windows_msvc_arm64,
#             good_plugins, good_plugins_qt6, monorepo
function(gstreamer_get_package_url PLATFORM VERSION OUTPUT_VAR)
    if(PLATFORM STREQUAL "android")
        set(_url "https://gstreamer.freedesktop.org/data/pkg/android/${VERSION}/gstreamer-1.0-android-universal-${VERSION}.tar.xz")
    elseif(PLATFORM STREQUAL "ios")
        set(_url "https://gstreamer.freedesktop.org/data/pkg/ios/${VERSION}/gstreamer-1.0-devel-${VERSION}-ios-universal.pkg")
    elseif(PLATFORM STREQUAL "macos")
        set(_url "https://gstreamer.freedesktop.org/data/pkg/macos/${VERSION}/gstreamer-1.0-${VERSION}-universal.pkg")
    elseif(PLATFORM STREQUAL "macos_devel")
        set(_url "https://gstreamer.freedesktop.org/data/pkg/macos/${VERSION}/gstreamer-1.0-devel-${VERSION}-universal.pkg")
    elseif(PLATFORM STREQUAL "windows_msvc_x64")
        if(VERSION VERSION_GREATER_EQUAL "1.28.0")
            set(_ext "exe")
        else()
            set(_ext "msi")
        endif()
        set(_url "https://gstreamer.freedesktop.org/data/pkg/windows/${VERSION}/msvc/gstreamer-1.0-msvc-x86_64-${VERSION}.${_ext}")
    elseif(PLATFORM STREQUAL "windows_msvc_arm64")
        if(VERSION VERSION_GREATER_EQUAL "1.28.0")
            set(_ext "exe")
        else()
            set(_ext "msi")
        endif()
        set(_url "https://gstreamer.freedesktop.org/data/pkg/windows/${VERSION}/msvc/gstreamer-1.0-msvc-arm64-${VERSION}.${_ext}")
    elseif(PLATFORM STREQUAL "good_plugins")
        set(_url "https://gstreamer.freedesktop.org/src/gst-plugins-good/gst-plugins-good-${VERSION}.tar.xz")
    elseif(PLATFORM STREQUAL "good_plugins_qt6")
        set(_url "https://gitlab.freedesktop.org/gstreamer/gstreamer/-/archive/${VERSION}/gstreamer-${VERSION}.tar.gz?path=subprojects/gst-plugins-good/ext/qt6")
    elseif(PLATFORM STREQUAL "monorepo")
        set(_url "https://gitlab.freedesktop.org/gstreamer/gstreamer/-/archive/${VERSION}/gstreamer-${VERSION}.tar.gz")
    else()
        message(FATAL_ERROR "gstreamer_get_package_url: Unknown platform '${PLATFORM}'")
    endif()

    set(${OUTPUT_VAR} "${_url}" PARENT_SCOPE)
endfunction()

# gstreamer_get_s3_mirror_url(<PLATFORM> <VERSION> <OUTPUT_VAR>)
# Derives an S3 mirror URL from the primary URL's filename.
# Returns empty string for platforms without an S3 mirror.
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
    # Works for simple URLs without query strings (those platforms are excluded above)
    cmake_path(GET _primary_url FILENAME _filename)

    set(${OUTPUT_VAR} "${_s3_base}/${_dir}/${_filename}" PARENT_SCOPE)
endfunction()

# gstreamer_get_fallback_checksum(<PLATFORM> <VERSION> <OUTPUT_VAR>)
# Returns a pinned checksum for known artifacts when sidecar checksum fetch fails.
function(gstreamer_get_fallback_checksum PLATFORM VERSION OUTPUT_VAR)
    set(_checksum "")

    if(VERSION STREQUAL "1.28.1")
        if(PLATFORM STREQUAL "android")
            set(_checksum "SHA256=7b3f7cfd289aa1ac237899220b3d8fbfa722337c23175c047e06ec881c505481")
        elseif(PLATFORM STREQUAL "windows_msvc_x64")
            set(_checksum "SHA256=2ec50356d2d0937a9ead0f99d322f81d8413b9514c9d58ed41ca58fbcf25bfde")
        elseif(PLATFORM STREQUAL "windows_msvc_arm64")
            set(_checksum "SHA256=0a1938b7a8568ee5695c4c1755743cacc4a1643538cacdfc5be3c82426c0e193")
        elseif(PLATFORM STREQUAL "macos")
            set(_checksum "SHA256=02803f73435daabe8fb12b79c38c6775d0efb83af001474558ba25c4f874d305")
        elseif(PLATFORM STREQUAL "macos_devel")
            set(_checksum "SHA256=df167b41559afbcd743276c6b068cba2ada8f5b69eb68095415a7a5a7515e52c")
        elseif(PLATFORM STREQUAL "ios")
            set(_checksum "SHA256=3255cd01f8c4d92322be0c5825a192b998fef05989a161dcae3cef22c517ae71")
        elseif(PLATFORM STREQUAL "good_plugins")
            set(_checksum "SHA256=738e26aee41b7a62050e40b81adc017a110a7f32d1ec49fa6a0300846c44368d")
        endif()
    elseif(VERSION STREQUAL "1.22.12")
        if(PLATFORM STREQUAL "android")
            set(_checksum "SHA256=be92cf477d140c270b480bd8ba0e26b1e01c8db042c46b9e234d87352112e485")
        elseif(PLATFORM STREQUAL "windows_msvc_x64")
            set(_checksum "SHA256=e5cbc6fb9f40fc2850806163df4b9d92012f967c842dc000a2b254cbcd7901d6")
        endif()
    endif()

    set(${OUTPUT_VAR} "${_checksum}" PARENT_SCOPE)
endfunction()

# gstreamer_fetch_checksum(<PLATFORM> <VERSION> <OUTPUT_VAR>)
# Downloads the .sha256sum sidecar from freedesktop.org and returns "SHA256=<hash>".
# Returns empty string on failure (checksum unavailable or unparseable).
function(gstreamer_fetch_checksum PLATFORM VERSION OUTPUT_VAR)
    gstreamer_get_package_url(${PLATFORM} ${VERSION} _pkg_url)

    # URLs with query strings (e.g. good_plugins_qt6) have no sidecar checksum
    string(FIND "${_pkg_url}" "?" _qs_pos)
    if(NOT _qs_pos EQUAL -1)
        message(STATUS "GStreamer: Skipping checksum for ${PLATFORM} ${VERSION} (URL contains query string)")
        set(${OUTPUT_VAR} "" PARENT_SCOPE)
        return()
    endif()

    set(_checksum_url "${_pkg_url}.sha256sum")

    string(MD5 _url_hash "${_checksum_url}")
    set(_checksum_dir "${CMAKE_BINARY_DIR}/_deps/checksums")
    set(_checksum_file "${_checksum_dir}/${_url_hash}.sha256sum")

    foreach(_round RANGE 1 2)
        if(NOT EXISTS "${_checksum_file}")
            file(MAKE_DIRECTORY "${_checksum_dir}")
            set(_checksum_tmp "${_checksum_file}.tmp")
            set(_code 1)
            foreach(_attempt RANGE 1 3)
                file(DOWNLOAD "${_checksum_url}" "${_checksum_tmp}"
                    STATUS _status
                    TIMEOUT 30
                    TLS_VERIFY ON
                )
                list(GET _status 0 _code)
                if(_code EQUAL 0)
                    break()
                endif()
                list(GET _status 1 _msg)
                message(STATUS "GStreamer: Checksum download attempt ${_attempt}/3 failed: ${_msg}")
                file(REMOVE "${_checksum_tmp}")
            endforeach()
            if(NOT _code EQUAL 0)
                gstreamer_get_fallback_checksum(${PLATFORM} ${VERSION} _fallback_hash)
                if(_fallback_hash)
                    message(WARNING
                        "GStreamer: Checksum sidecar unavailable for ${PLATFORM} ${VERSION}; "
                        "using pinned fallback checksum.")
                    set(${OUTPUT_VAR} "${_fallback_hash}" PARENT_SCOPE)
                    return()
                endif()
                message(WARNING
                    "GStreamer: Checksum sidecar unavailable for ${PLATFORM} ${VERSION}; "
                    "continuing without integrity verification.")
                set(${OUTPUT_VAR} "" PARENT_SCOPE)
                return()
            endif()
            file(RENAME "${_checksum_tmp}" "${_checksum_file}")
        endif()

        file(READ "${_checksum_file}" _content)
        string(STRIP "${_content}" _content)
        string(REGEX MATCH "([0-9a-fA-F]+)" _match "${_content}")
        if(_match)
            string(LENGTH "${CMAKE_MATCH_1}" _hash_len)
            if(_hash_len EQUAL 64)
                set(${OUTPUT_VAR} "SHA256=${CMAKE_MATCH_1}" PARENT_SCOPE)
                return()
            endif()
        endif()

        # Parse failed — purge cached file and retry once
        file(SIZE "${_checksum_file}" _file_size)
        string(SUBSTRING "${_content}" 0 60 _content_preview)
        string(REGEX REPLACE "[^ -~]" "?" _content_preview "${_content_preview}")
        message(STATUS "GStreamer: Could not parse checksum for ${PLATFORM} ${VERSION} "
            "(file size: ${_file_size}, preview: [${_content_preview}])")
        file(REMOVE "${_checksum_file}")
    endforeach()

    gstreamer_get_fallback_checksum(${PLATFORM} ${VERSION} _fallback_hash)
    if(_fallback_hash)
        message(WARNING
            "GStreamer: Sidecar checksum content invalid/unparseable for ${PLATFORM} ${VERSION}; "
            "using pinned fallback checksum.")
        set(${OUTPUT_VAR} "${_fallback_hash}" PARENT_SCOPE)
        return()
    endif()

    message(WARNING "GStreamer: Could not parse checksum for ${PLATFORM} ${VERSION}; continuing without verification.")
    set(${OUTPUT_VAR} "" PARENT_SCOPE)
endfunction()

# gstreamer_resilient_download(URLS <url>... FILENAME <name> DESTINATION_DIR <dir>
#     RESULT_VAR <var> [TIMEOUT <sec>] [INACTIVITY_TIMEOUT <sec>] [EXPECTED_HASH <ALGO=hash>]
#     [ALLOW_FAILURE])
# Downloads a file, trying each URL in order. Uses atomic temp-file rename to
# prevent partial-download ghosts. Skips if the destination file already exists.
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

# gstreamer_download_sdk(<PLATFORM> <VERSION> <FILENAME> <DESTINATION_DIR> <RESULT_VAR>
#     [ALLOW_FAILURE])
function(gstreamer_download_sdk PLATFORM VERSION FILENAME DESTINATION_DIR RESULT_VAR)
    cmake_parse_arguments(_DL "ALLOW_FAILURE" "" "" ${ARGN})

    gstreamer_get_package_url(${PLATFORM} ${VERSION} _url)
    gstreamer_get_s3_mirror_url(${PLATFORM} ${VERSION} _s3_url)
    gstreamer_fetch_checksum(${PLATFORM} ${VERSION} _hash)

    set(_args
        URLS "${_url}" "${_s3_url}"
        FILENAME "${FILENAME}"
        DESTINATION_DIR "${DESTINATION_DIR}"
        RESULT_VAR _result
        EXPECTED_HASH "${_hash}"
    )
    if(_DL_ALLOW_FAILURE)
        list(APPEND _args ALLOW_FAILURE)
    endif()

    gstreamer_resilient_download(${_args})
    set(${RESULT_VAR} "${_result}" PARENT_SCOPE)
endfunction()

# gstreamer_get_recommended_version(<MAJOR> <MINOR> <OUTPUT_VAR> [<PATCH_FALLBACK>])
# Patch versions are auto-extracted from platform version strings in build-config.json
# via BuildConfig.cmake (e.g., gstreamer_android_version "1.22.12" -> QGC_GSTREAMER_PATCH_1_22=12).
function(gstreamer_get_recommended_version MAJOR MINOR OUTPUT_VAR)
    set(_var_name "QGC_GSTREAMER_PATCH_${MAJOR}_${MINOR}")
    if(DEFINED ${_var_name})
        set(_patch "${${_var_name}}")
    elseif(ARGC GREATER 3 AND NOT "${ARGV3}" STREQUAL "")
        set(_patch "${ARGV3}")
        message(STATUS
            "No patch mapping for GStreamer ${MAJOR}.${MINOR}; using detected patch ${_patch}.")
    else()
        set(_patch 0)
        message(WARNING "No patch mapping for GStreamer ${MAJOR}.${MINOR} — defaulting to .0. "
                        "This version is not used by any platform target in .github/build-config.json.")
    endif()

    set(${OUTPUT_VAR} "${MAJOR}.${MINOR}.${_patch}" PARENT_SCOPE)
endfunction()

function(gstreamer_install_gio_modules)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;DEST_DIR;EXTENSION" "" ${ARGN})

    if(NOT EXISTS "${ARG_SOURCE_DIR}")
        message(WARNING "gstreamer_install_gio_modules: SOURCE_DIR does not exist: ${ARG_SOURCE_DIR}")
        return()
    endif()

    # Ship the full GIO module set from the selected runtime. Filtering can
    # remove TLS/proxy helpers required by some deployments.
    file(GLOB modules_to_install "${ARG_SOURCE_DIR}/*.${ARG_EXTENSION}")

    if(modules_to_install)
        install(FILES ${modules_to_install} DESTINATION "${ARG_DEST_DIR}")
    endif()
endfunction()

function(gstreamer_install_plugins)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;DEST_DIR;EXTENSION;PREFIX" "" ${ARGN})

    if(NOT EXISTS "${ARG_SOURCE_DIR}")
        message(WARNING "gstreamer_install_plugins: SOURCE_DIR does not exist: ${ARG_SOURCE_DIR}")
        return()
    endif()

    file(GLOB all_plugins "${ARG_SOURCE_DIR}/${ARG_PREFIX}*.${ARG_EXTENSION}")

    # Install only plugins listed in GSTREAMER_PLUGINS (set by FindQGCGStreamer).
    set(plugins_to_install "")
    foreach(plugin_path IN LISTS all_plugins)
        get_filename_component(plugin_name "${plugin_path}" NAME)
        foreach(allowed IN LISTS GSTREAMER_PLUGINS)
            if(plugin_name MATCHES "^${ARG_PREFIX}${allowed}([^a-zA-Z0-9]|$)")
                list(APPEND plugins_to_install "${plugin_path}")
                break()
            endif()
        endforeach()
    endforeach()

    if(plugins_to_install)
        install(FILES ${plugins_to_install} DESTINATION "${ARG_DEST_DIR}")
    endif()
endfunction()

# _gst_set_standard_paths()
# Sets GSTREAMER_LIB_PATH, GSTREAMER_PLUGIN_PATH, GSTREAMER_INCLUDE_PATH from
# GStreamer_ROOT_DIR and appends the standard --define-variable args to
# PKG_CONFIG_ARGN.  Must be a macro so variables propagate to caller scope.
# Optional: pass INCLUDE_PATH to override the default include directory.
macro(_gst_set_standard_paths)
    cmake_parse_arguments(_GSP "" "INCLUDE_PATH" "" ${ARGN})
    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    if(_GSP_INCLUDE_PATH)
        set(GSTREAMER_INCLUDE_PATH "${_GSP_INCLUDE_PATH}")
    else()
        set(GSTREAMER_INCLUDE_PATH "${GStreamer_ROOT_DIR}/include")
    endif()
    list(APPEND PKG_CONFIG_ARGN
        --define-variable=prefix=${GStreamer_ROOT_DIR}
        --define-variable=libdir=${GSTREAMER_LIB_PATH}
        --define-variable=includedir=${GSTREAMER_INCLUDE_PATH}
    )
endmacro()

# _gst_normalize_and_validate_root()
# Normalize GStreamer_ROOT_DIR path separators and verify the directory exists.
# Must be called as a macro (not function) so it modifies the caller's scope.
macro(_gst_normalize_and_validate_root)
    cmake_path(CONVERT "${GStreamer_ROOT_DIR}" TO_CMAKE_PATH_LIST GStreamer_ROOT_DIR NORMALIZE)
    if(NOT EXISTS "${GStreamer_ROOT_DIR}")
        message(FATAL_ERROR "GStreamer: SDK not found at '${GStreamer_ROOT_DIR}' — "
            "check installation or set GStreamer_ROOT_DIR")
    endif()
endmacro()

# WARNING: This function copies ALL shared libraries from SOURCE_DIR without
# filtering. Only call it for auto-downloaded SDKs or known-clean directories.
# Do NOT point it at a shared system prefix (e.g. /usr/lib, Homebrew lib/).
function(gstreamer_install_libs)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;DEST_DIR;EXTENSION" "" ${ARGN})

    if(NOT EXISTS "${ARG_SOURCE_DIR}")
        message(WARNING "gstreamer_install_libs: SOURCE_DIR does not exist: ${ARG_SOURCE_DIR}")
        return()
    endif()

    file(GLOB all_libs "${ARG_SOURCE_DIR}/*.${ARG_EXTENSION}")
    if(all_libs)
        install(FILES ${all_libs} DESTINATION "${ARG_DEST_DIR}")
    endif()
endfunction()
