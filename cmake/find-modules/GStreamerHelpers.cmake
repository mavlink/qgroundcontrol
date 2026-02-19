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
        set(_url "https://gstreamer.freedesktop.org/data/pkg/windows/${VERSION}/msvc/gstreamer-1.0-msvc-x86_64-${VERSION}.exe")
    elseif(PLATFORM STREQUAL "windows_msvc_arm64")
        set(_url "https://gstreamer.freedesktop.org/data/pkg/windows/${VERSION}/msvc/gstreamer-1.0-msvc-arm64-${VERSION}.exe")
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
# This preserves integrity verification during transient checksum endpoint outages.
function(gstreamer_get_fallback_checksum PLATFORM VERSION OUTPUT_VAR)
    set(_checksum "")

    if(VERSION STREQUAL "1.28.0")
        if(PLATFORM STREQUAL "android")
            set(_checksum "SHA256=3315be90b32b96aea5339f161725b4298f939cc502ac299eff81b3819d4f5cc3")
        elseif(PLATFORM STREQUAL "windows_msvc_x64")
            set(_checksum "SHA256=d6aca5785cae9d9ed447db491cc921163ff6af0c657eb733293c55416c8b71a6")
        elseif(PLATFORM STREQUAL "windows_msvc_arm64")
            set(_checksum "SHA256=99c41b2a4db08730cf17d89f17fed8eddb81e5da569285c83e00266a1f1b9357")
        elseif(PLATFORM STREQUAL "macos")
            set(_checksum "SHA256=9c252ae9d3ac5bc54505c4a4e93556c7d6e93218a18ad8060b30770d6db036a6")
        elseif(PLATFORM STREQUAL "macos_devel")
            set(_checksum "SHA256=592d6fe925799470a67da9cbbba6badf4d7ac63c89f6f4532baad7ec1daba5a4")
        elseif(PLATFORM STREQUAL "ios")
            set(_checksum "SHA256=c4e3365e37c3eae05d8acd470d338560c89a213fb58157c8773db151dc7ebcd7")
        elseif(PLATFORM STREQUAL "good_plugins")
            set(_checksum "SHA256=d97700f346fdf9ef5461c035e23ed1ce916ca7a31d6ddad987f774774361db77")
        endif()
    endif()

    set(${OUTPUT_VAR} "${_checksum}" PARENT_SCOPE)
endfunction()

# gstreamer_fetch_checksum(<PLATFORM> <VERSION> <OUTPUT_VAR>)
# Downloads the .sha256sum sidecar from freedesktop.org and returns "SHA256=<hash>".
# Returns empty string on failure (checksum unavailable or unparseable).
function(_gstreamer_is_release_like_config OUTPUT_VAR)
    set(_release_like FALSE)

    if(CMAKE_CONFIGURATION_TYPES)
        foreach(_cfg IN LISTS CMAKE_CONFIGURATION_TYPES)
            if(_cfg MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
                set(_release_like TRUE)
                break()
            endif()
        endforeach()
    elseif(CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
        set(_release_like TRUE)
    endif()

    set(${OUTPUT_VAR} "${_release_like}" PARENT_SCOPE)
endfunction()

function(gstreamer_fetch_checksum PLATFORM VERSION OUTPUT_VAR)
    gstreamer_get_package_url(${PLATFORM} ${VERSION} _pkg_url)

    # URLs with query strings (e.g. good_plugins_qt6) have no sidecar checksum
    string(FIND "${_pkg_url}" "?" _qs_pos)
    if(NOT _qs_pos EQUAL -1)
        _gstreamer_is_release_like_config(_require_checksum)
        if(_require_checksum)
            message(FATAL_ERROR
                "GStreamer: Checksums are required for release-like builds, but ${PLATFORM} ${VERSION} "
                "uses a query-string URL without sidecar checksum support.\n"
                "Use a source with a pinned EXPECTED_HASH.")
        endif()
        message(STATUS "GStreamer: Skipping checksum for ${PLATFORM} ${VERSION} (URL contains query string)")
        set(${OUTPUT_VAR} "" PARENT_SCOPE)
        return()
    endif()

    set(_checksum_url "${_pkg_url}.sha256sum")

    string(MD5 _url_hash "${_checksum_url}")
    set(_checksum_dir "${CMAKE_BINARY_DIR}/_deps/checksums")
    set(_checksum_file "${_checksum_dir}/${_url_hash}.sha256sum")

    # Try up to 2 rounds: if a cached file fails to parse, delete and re-download.
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
                _gstreamer_is_release_like_config(_require_checksum)
                if(_require_checksum)
                    message(FATAL_ERROR "GStreamer: Checksum not available for ${PLATFORM} ${VERSION} — "
                        "integrity verification is required for release builds")
                endif()
                message(FATAL_ERROR
                    "GStreamer: Checksum not available for ${PLATFORM} ${VERSION}.\n"
                    "Integrity verification is required for all downloads. "
                    "Add a pinned fallback checksum to GStreamerHelpers.cmake or install the SDK manually.")
            endif()
            file(RENAME "${_checksum_tmp}" "${_checksum_file}")
        endif()

        file(READ "${_checksum_file}" _content)
        # Strip surrounding whitespace and any non-hex prefix (e.g. UTF-8 BOM).
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

    _gstreamer_is_release_like_config(_require_checksum)
    if(_require_checksum)
        message(FATAL_ERROR
            "GStreamer: Could not parse checksum for ${PLATFORM} ${VERSION} after retry "
            "(invalid sidecar content). URL: ${_checksum_url}")
    endif()
    message(STATUS "GStreamer: Could not parse checksum for ${PLATFORM} ${VERSION} (invalid content)")
    set(${OUTPUT_VAR} "" PARENT_SCOPE)
endfunction()

# gstreamer_resilient_download(URLS <url>... FILENAME <name> DESTINATION_DIR <dir>
#                              RESULT_VAR <var> [TIMEOUT <sec>] [EXPECTED_HASH <ALGO=hash>])
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
    cmake_parse_arguments(ARG "ALLOW_FAILURE" "FILENAME;DESTINATION_DIR;RESULT_VAR;TIMEOUT;EXPECTED_HASH" "URLS" ${ARGN})

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
    file(GLOB modules_to_install "${ARG_SOURCE_DIR}/*.${ARG_EXTENSION}*")

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

    # Install only plugins listed in GSTREAMER_PLUGINS (set by FindGStreamerQGC).
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

    # The GStreamer SDK ships only GStreamer and its transitive dependencies —
    # there are no unrelated libraries to exclude. Maintaining a prefix allowlist
    # would be fragile and silently break when new dependencies appear.
    file(GLOB all_libs "${ARG_SOURCE_DIR}/*.${ARG_EXTENSION}")
    if(all_libs)
        install(FILES ${all_libs} DESTINATION "${ARG_DEST_DIR}")
    endif()
endfunction()
