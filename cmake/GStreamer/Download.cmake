# GStreamer SDK download / URL / checksum helpers.

include_guard(GLOBAL)

# Absolute path disambiguates from this file (also named Download.cmake): when
# test_download.cmake adds cmake/GStreamer/ to MODULE_PATH, a bare `include(Download)`
# would resolve to this file and self-recurse.
include("${CMAKE_CURRENT_LIST_DIR}/../modules/Download.cmake")  # qgc_resilient_download / qgc_parse_expected_hash

function(gstreamer_get_package_url PLATFORM VERSION OUTPUT_VAR)
    set(_base "https://gstreamer.freedesktop.org")

    # URL templates for simple platforms — VERSION substituted via _base.
    set(_url_android       "${_base}/data/pkg/android/${VERSION}/gstreamer-1.0-android-universal-${VERSION}.tar.xz")
    set(_url_ios           "${_base}/data/pkg/ios/${VERSION}/gstreamer-1.0-devel-${VERSION}-ios-universal.pkg")
    set(_url_macos         "${_base}/data/pkg/macos/${VERSION}/gstreamer-1.0-${VERSION}-universal.pkg")
    set(_url_macos_devel   "${_base}/data/pkg/macos/${VERSION}/gstreamer-1.0-devel-${VERSION}-universal.pkg")

    if(PLATFORM MATCHES "^windows_msvc_(x64|arm64)$")
        # Windows: 1.28+ ships .exe with arch-tagged filename (defensive guard against manual override).
        if(VERSION VERSION_LESS "1.28.0")
            message(FATAL_ERROR
                "Windows GStreamer SDK requires version 1.28 or later (got '${VERSION}'). "
                "Bump gstreamer.version.windows in build-config.json.")
        endif()
        if(CMAKE_MATCH_1 STREQUAL "x64")
            set(_arch "x86_64")
        else()
            set(_arch "arm64")
        endif()
        set(_url "${_base}/data/pkg/windows/${VERSION}/msvc/gstreamer-1.0-msvc-${_arch}-${VERSION}.exe")
    elseif(DEFINED _url_${PLATFORM})
        set(_url "${_url_${PLATFORM}}")
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
    elseif(PLATFORM MATCHES "^macos")
        set(_dir "macos")
    elseif(PLATFORM MATCHES "^windows_msvc_")
        set(_dir "windows")
    else()
        set(${OUTPUT_VAR} "" PARENT_SCOPE)
        return()
    endif()

    gstreamer_get_package_url("${PLATFORM}" "${VERSION}" _primary_url)
    cmake_path(GET _primary_url FILENAME _filename)

    set(${OUTPUT_VAR} "${_s3_base}/${_dir}/${_filename}" PARENT_SCOPE)
endfunction()

function(gstreamer_get_fallback_checksum PLATFORM VERSION OUTPUT_VAR)
    set(_checksum "")

    if(DEFINED QGC_BUILD_CONFIG_CONTENT)
        string(JSON _hash ERROR_VARIABLE _err
            GET "${QGC_BUILD_CONFIG_CONTENT}" "gstreamer" "checksums" "${VERSION}" "${PLATFORM}")
        if(NOT _err AND _hash)
            string(TOLOWER "${_hash}" _hash)
            set(_checksum "SHA256=${_hash}")
        endif()
    endif()

    set(${OUTPUT_VAR} "${_checksum}" PARENT_SCOPE)
endfunction()

# Without a pin, missing sidecar data is fatal under GStreamer_REQUIRE_CHECKSUM and a warning otherwise.
function(_gstreamer_unverified_or_fail platform version output_var reason)
    if(GStreamer_REQUIRE_CHECKSUM)
        message(FATAL_ERROR "GStreamer: ${reason} for ${platform} ${version} "
                            "and no pinned checksum exists. Set GStreamer_REQUIRE_CHECKSUM=OFF to bypass."
        )
    endif()
    message(
        WARNING "GStreamer: ${reason} for ${platform} ${version}; "
                "the SDK will be used WITHOUT integrity verification — supply-chain "
                "authenticity is NOT guaranteed. Only proceed on a trusted network/mirror. "
                "(GStreamer_REQUIRE_CHECKSUM=ON makes this a hard error.)"
    )
    set(${output_var}
        ""
        PARENT_SCOPE
    )
endfunction()

# _gstreamer_download_sidecar(<url> <dest_file> <result_var>)
# Downloads the .sha256sum sidecar to DEST_FILE; sets RESULT_VAR TRUE on success.
function(_gstreamer_download_sidecar URL DEST_FILE RESULT_VAR)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_deps/checksums")
    set(_tmp "${DEST_FILE}.tmp")
    foreach(_attempt RANGE 1 3)
        file(DOWNLOAD "${URL}" "${_tmp}" STATUS _status TIMEOUT 30 INACTIVITY_TIMEOUT 10 TLS_VERIFY ON)
        list(GET _status 0 _code)
        if(_code EQUAL 0)
            file(RENAME "${_tmp}" "${DEST_FILE}")
            set(${RESULT_VAR} TRUE PARENT_SCOPE)
            return()
        endif()
        list(GET _status 1 _msg)
        message(STATUS "GStreamer: Checksum download attempt ${_attempt}/3 failed: ${_msg}")
        file(REMOVE "${_tmp}")
    endforeach()
    set(${RESULT_VAR} FALSE PARENT_SCOPE)
endfunction()

function(gstreamer_fetch_checksum PLATFORM VERSION OUTPUT_VAR)
    # The in-tree pin is authoritative and the download is verified against it, so the
    # same-origin .sha256sum sidecar is only fetched for unpinned SDKs.
    gstreamer_get_fallback_checksum(${PLATFORM} ${VERSION} _pinned_hash)
    if(_pinned_hash)
        set(${OUTPUT_VAR}
            "${_pinned_hash}"
            PARENT_SCOPE
        )
        return()
    endif()

    gstreamer_get_package_url(${PLATFORM} ${VERSION} _pkg_url)

    string(FIND "${_pkg_url}" "?" _qs_pos)
    if(NOT _qs_pos EQUAL -1)
        message(STATUS "GStreamer: Skipping checksum for ${PLATFORM} ${VERSION} (URL contains query string)")
        set(${OUTPUT_VAR} "" PARENT_SCOPE)
        return()
    endif()

    set(_checksum_url "${_pkg_url}.sha256sum")
    string(SHA256 _url_hash "${_checksum_url}")
    set(_checksum_file "${CMAKE_BINARY_DIR}/_deps/checksums/${_url_hash}.sha256sum")

    set(_dl_ok FALSE)
    foreach(_round RANGE 1 2)
        if(NOT EXISTS "${_checksum_file}")
            _gstreamer_download_sidecar("${_checksum_url}" "${_checksum_file}" _dl_ok)
            if(NOT _dl_ok)
                _gstreamer_unverified_or_fail(${PLATFORM} ${VERSION} _result "Checksum sidecar unavailable")
                set(${OUTPUT_VAR} "${_result}" PARENT_SCOPE)
                return()
            endif()
        endif()

        file(READ "${_checksum_file}" _content)
        string(STRIP "${_content}" _content)
        # CMake regex has no {n} quantifier.
        string(REPEAT "[0-9a-fA-F]" 64 _sha256_hex)
        string(REGEX MATCH "(${_sha256_hex})" _match "${_content}")
        if(_match)
            string(TOLOWER "${CMAKE_MATCH_1}" _fetched_hex)
            # A same-origin sidecar proves nothing against a bad mirror/MITM, so it must not
            # satisfy GStreamer_REQUIRE_CHECKSUM=ON.
            if(GStreamer_REQUIRE_CHECKSUM)
                message(FATAL_ERROR
                    "GStreamer: no trusted (in-tree pinned) checksum for ${PLATFORM} ${VERSION}; "
                    "the fetched same-origin .sha256sum sidecar cannot satisfy "
                    "GStreamer_REQUIRE_CHECKSUM=ON. Pin the hash in .github/build-config.json "
                    "(gstreamer.checksums.${VERSION}.${PLATFORM}) or set GStreamer_REQUIRE_CHECKSUM=OFF.")
            endif()
            message(WARNING "GStreamer: using same-origin .sha256sum sidecar for ${PLATFORM} ${VERSION}; "
                            "this detects corruption only, NOT supply-chain tampering. Pin the hash in "
                            ".github/build-config.json for authenticity."
            )
            set(${OUTPUT_VAR}
                "SHA256=${_fetched_hex}"
                PARENT_SCOPE
            )
            return()
        endif()

        message(STATUS "GStreamer: Could not parse checksum for ${PLATFORM} ${VERSION}")
        file(REMOVE "${_checksum_file}")
    endforeach()

    _gstreamer_unverified_or_fail(${PLATFORM} ${VERSION} _result "Checksum content invalid/unparseable")
    set(${OUTPUT_VAR} "${_result}" PARENT_SCOPE)
endfunction()

# GStreamer-specific wrapper around qgc_resilient_download (cmake/modules/Download.cmake).
# Adds the GStreamer log prefix, install hint, and honours legacy
# QGC_GST_DOWNLOAD_TIMEOUT / QGC_GST_DOWNLOAD_INACTIVITY_TIMEOUT cache vars
# (CI flake-fixer knobs that predate the project-level QGC_DOWNLOAD_* vars).
function(gstreamer_resilient_download)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "ALLOW_FAILURE"
        "FILENAME;DESTINATION_DIR;RESULT_VAR;TIMEOUT;INACTIVITY_TIMEOUT;EXPECTED_HASH"
        "URLS")
    if(ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR
            "gstreamer_resilient_download: missing values for: ${ARG_KEYWORDS_MISSING_VALUES}")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "gstreamer_resilient_download: unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if("${ARG_RESULT_VAR}" STREQUAL "")
        message(FATAL_ERROR "gstreamer_resilient_download: RESULT_VAR is required")
    endif()
    if(NOT DEFINED ARG_TIMEOUT AND DEFINED QGC_GST_DOWNLOAD_TIMEOUT)
        set(ARG_TIMEOUT ${QGC_GST_DOWNLOAD_TIMEOUT})
    endif()
    if(NOT DEFINED ARG_INACTIVITY_TIMEOUT AND DEFINED QGC_GST_DOWNLOAD_INACTIVITY_TIMEOUT)
        set(ARG_INACTIVITY_TIMEOUT ${QGC_GST_DOWNLOAD_INACTIVITY_TIMEOUT})
    endif()

    # GStreamer SDKs are large (the Android universal tarball is ~1 GB); the module's
    # 120 s wall-clock default kills slow-but-healthy CI downloads mid-transfer. Use a
    # generous cap and let INACTIVITY_TIMEOUT (60 s) catch genuine stalls instead.
    # Note: this 1200 s default overrides the project-level QGC_DOWNLOAD_TIMEOUT for
    # GStreamer downloads — use QGC_GST_DOWNLOAD_TIMEOUT to tune the GStreamer cap.
    if(NOT DEFINED ARG_TIMEOUT)
        set(ARG_TIMEOUT 1200)
    endif()
    if(NOT DEFINED ARG_INACTIVITY_TIMEOUT)
        set(ARG_INACTIVITY_TIMEOUT 60)
    endif()

    set(_fwd
        FILENAME         "${ARG_FILENAME}"
        DESTINATION_DIR  "${ARG_DESTINATION_DIR}"
        RESULT_VAR       _qrd_result
        URLS             "${ARG_URLS}"
        LOG_TAG          "GStreamer"
        FAILURE_HINT     "Install manually from https://gstreamer.freedesktop.org/download/ or set GStreamer_ROOT_DIR."
    )
    if(DEFINED ARG_TIMEOUT)
        list(APPEND _fwd TIMEOUT ${ARG_TIMEOUT})
    endif()
    if(DEFINED ARG_INACTIVITY_TIMEOUT)
        list(APPEND _fwd INACTIVITY_TIMEOUT ${ARG_INACTIVITY_TIMEOUT})
    endif()
    if(ARG_EXPECTED_HASH)
        list(APPEND _fwd EXPECTED_HASH "${ARG_EXPECTED_HASH}")
    endif()
    if(ARG_ALLOW_FAILURE)
        list(APPEND _fwd ALLOW_FAILURE)
    endif()
    qgc_resilient_download(${_fwd})

    set(${ARG_RESULT_VAR} "${_qrd_result}" PARENT_SCOPE)
endfunction()

# gstreamer_download_sdk(<platform> <version> <filename> <destination_dir> <result_var>
#                        [ALLOW_FAILURE] [HASH_VAR <var>])
# HASH_VAR receives the "SHA256=<hex>" the archive was verified against, or "" when unverified.
function(gstreamer_download_sdk PLATFORM VERSION FILENAME DESTINATION_DIR RESULT_VAR)
    cmake_parse_arguments(PARSE_ARGV 5 _DL "ALLOW_FAILURE" "HASH_VAR" "")
    if(_DL_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "gstreamer_download_sdk: unknown arguments: ${_DL_UNPARSED_ARGUMENTS}")
    endif()
    foreach(_required IN ITEMS PLATFORM VERSION FILENAME DESTINATION_DIR RESULT_VAR)
        if("${${_required}}" STREQUAL "")
            message(FATAL_ERROR "gstreamer_download_sdk: ${_required} is required")
        endif()
    endforeach()

    gstreamer_get_package_url("${PLATFORM}" "${VERSION}" _url)
    gstreamer_get_s3_mirror_url("${PLATFORM}" "${VERSION}" _s3_url)
    gstreamer_fetch_checksum("${PLATFORM}" "${VERSION}" _hash)

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
    if(_DL_HASH_VAR)
        set(${_DL_HASH_VAR}
            "${_hash}"
            PARENT_SCOPE
        )
    endif()
endfunction()

# _qgc_gst_extract_once(<dir> <stamp> <extract_function> <archives> [<extract_arg>...])
# Publishes <dir> exactly once per <stamp>: a matching <dir>/.complete means it is already extracted.
# Otherwise <extract_function>(<staging> <archives> [<extract_arg>...]) must create and fill <staging>,
# which is then stamped and renamed to <dir>, so <dir> only ever exists complete. The caller must
# hold the cache lock (see qgc_gst_acquire_sdk).
function(_qgc_gst_extract_once dir stamp extract_function archives)
    set(_stamp_file "${dir}/.complete")
    if(EXISTS "${_stamp_file}")
        file(READ "${_stamp_file}" _existing_stamp)
        if(_existing_stamp STREQUAL stamp)
            return()
        endif()
        message(STATUS "GStreamer: ${dir} was extracted from another archive; extracting again")
    elseif(EXISTS "${dir}")
        message(STATUS "GStreamer: ${dir} has no completion stamp; extracting again")
    endif()

    set(_staging "${dir}.partial")
    file(REMOVE_RECURSE "${dir}" "${_staging}")
    cmake_language(CALL ${extract_function} "${_staging}" "${archives}" ${ARGN})
    if(NOT IS_DIRECTORY "${_staging}")
        message(FATAL_ERROR "GStreamer: ${extract_function} did not create ${_staging}")
    endif()
    file(WRITE "${_staging}/.complete" "${stamp}")

    # Freshly written files can briefly hold the directory open on Windows (e.g. antivirus scans).
    foreach(_attempt IN ITEMS 1 2 3 4 5)
        file(RENAME "${_staging}" "${dir}" RESULT _rename_result)
        if(_rename_result EQUAL 0)
            return()
        endif()
        execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 2)
    endforeach()
    message(FATAL_ERROR "GStreamer: could not move ${_staging} to ${dir}: ${_rename_result}")
endfunction()

# qgc_gst_acquire_sdk(PLATFORM <platform> VERSION <version> CACHE_SUBDIR <subdir>
#                     ARCHIVES <primary> [<devel>] CONTENT <name>
#                     EXTRACT_FUNCTION <command> [EXTRACT_ARGS <arg>...] CONTENT_DIR_OUT <var>)
# Downloads the SDK archive(s) for <platform> (the second one as <platform>_devel) into
# <cache>/<subdir> and extracts them once into <cache>/<subdir>/<name>, returned in CONTENT_DIR_OUT.
# <cache> is CPM_SOURCE_CACHE when set (shared by build directories and CI jobs), else
# <build>/_deps. Concurrent configures are serialized on <cache>/<subdir>/.lock, and the extracted
# tree is stamped with the version and archive hashes; see _qgc_gst_extract_once.
function(qgc_gst_acquire_sdk)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "PLATFORM;VERSION;CACHE_SUBDIR;CONTENT;EXTRACT_FUNCTION;CONTENT_DIR_OUT"
                          "ARCHIVES;EXTRACT_ARGS"
    )
    if(ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "qgc_gst_acquire_sdk: missing values for: ${ARG_KEYWORDS_MISSING_VALUES}")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qgc_gst_acquire_sdk: unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    foreach(
        _required IN
        ITEMS PLATFORM
              VERSION
              CACHE_SUBDIR
              ARCHIVES
              CONTENT
              EXTRACT_FUNCTION
              CONTENT_DIR_OUT
    )
        if("${ARG_${_required}}" STREQUAL "")
            message(FATAL_ERROR "qgc_gst_acquire_sdk: ${_required} is required")
        endif()
    endforeach()
    list(LENGTH ARG_ARCHIVES _archive_count)
    if(_archive_count GREATER 2)
        message(FATAL_ERROR "qgc_gst_acquire_sdk: ARCHIVES takes a primary and an optional devel archive")
    endif()
    if(NOT COMMAND ${ARG_EXTRACT_FUNCTION})
        message(FATAL_ERROR "qgc_gst_acquire_sdk: EXTRACT_FUNCTION '${ARG_EXTRACT_FUNCTION}' is not a command")
    endif()

    if(CPM_SOURCE_CACHE)
        set(_cache_dir "${CPM_SOURCE_CACHE}/${ARG_CACHE_SUBDIR}")
    else()
        set(_cache_dir "${CMAKE_BINARY_DIR}/_deps/${ARG_CACHE_SUBDIR}")
    endif()
    file(MAKE_DIRECTORY "${_cache_dir}")
    set(_lock "${_cache_dir}/.lock")
    file(
        LOCK "${_lock}"
        GUARD FUNCTION
        TIMEOUT 0
        RESULT_VARIABLE _lock_result
    )
    if(NOT _lock_result EQUAL 0)
        message(STATUS "GStreamer: waiting for another configure to release ${_lock}")
        file(
            LOCK "${_lock}"
            GUARD FUNCTION
            TIMEOUT 1800
            RESULT_VARIABLE _lock_result
        )
        if(NOT _lock_result EQUAL 0)
            message(FATAL_ERROR "GStreamer: could not lock ${_lock}: ${_lock_result}")
        endif()
    endif()

    set(_platforms "${ARG_PLATFORM}" "${ARG_PLATFORM}_devel")
    set(_archive_paths "")
    set(_stamp "version=${ARG_VERSION}\nextract=${ARG_EXTRACT_FUNCTION} ${ARG_EXTRACT_ARGS}\n")
    foreach(_archive IN LISTS ARG_ARCHIVES)
        list(POP_FRONT _platforms _platform)
        gstreamer_download_sdk(
            ${_platform}
            ${ARG_VERSION}
            "${_archive}"
            "${_cache_dir}"
            _archive_path
            HASH_VAR
            _archive_hash
        )
        if(NOT _archive_hash)
            file(SHA256 "${_archive_path}" _archive_sha256)
            set(_archive_hash "SHA256=${_archive_sha256}")
        endif()
        list(APPEND _archive_paths "${_archive_path}")
        string(APPEND _stamp "${_archive}=${_archive_hash}\n")
    endforeach()

    _qgc_gst_extract_once("${_cache_dir}/${ARG_CONTENT}" "${_stamp}" ${ARG_EXTRACT_FUNCTION} "${_archive_paths}"
                          ${ARG_EXTRACT_ARGS}
    )
    set(${ARG_CONTENT_DIR_OUT}
        "${_cache_dir}/${ARG_CONTENT}"
        PARENT_SCOPE
    )
endfunction()
