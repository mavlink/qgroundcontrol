# GStreamer SDK download / URL / checksum helpers.

include_guard(GLOBAL)

# Absolute path disambiguates from this file (also named Download.cmake): when
# test_download.cmake adds cmake/GStreamer/ to MODULE_PATH, a bare `include(Download)`
# would resolve to this file and self-recurse.
include("${CMAKE_CURRENT_LIST_DIR}/../modules/Download.cmake") # qgc_resilient_download / qgc_parse_expected_hash

# gstreamer_get_package_url(<platform> <version> <output_var>)
# Sets <output_var> to the gstreamer.freedesktop.org URL of the <version> SDK package for <platform> (android, ios,
# macos, macos_devel, windows_msvc_x64 or windows_msvc_arm64).
function(gstreamer_get_package_url platform version output_var)
    set(_base "https://gstreamer.freedesktop.org")

    # URL templates for simple platforms — version substituted via _base.
    set(_url_android "${_base}/data/pkg/android/${version}/gstreamer-1.0-android-universal-${version}.tar.xz")
    set(_url_ios "${_base}/data/pkg/ios/${version}/gstreamer-1.0-devel-${version}-ios-universal.pkg")
    set(_url_macos "${_base}/data/pkg/macos/${version}/gstreamer-1.0-${version}-universal.pkg")
    set(_url_macos_devel "${_base}/data/pkg/macos/${version}/gstreamer-1.0-devel-${version}-universal.pkg")

    if(platform MATCHES "^windows_msvc_(x64|arm64)$")
        # Windows: 1.28+ ships .exe with arch-tagged filename (defensive guard against manual override).
        if(version VERSION_LESS "1.28.0")
            message(FATAL_ERROR "Windows GStreamer SDK requires version 1.28 or later (got '${version}'). "
                                "Bump gstreamer.version.windows in build-config.json."
            )
        endif()
        if(CMAKE_MATCH_1 STREQUAL "x64")
            set(_arch "x86_64")
        else()
            set(_arch "arm64")
        endif()
        set(_url "${_base}/data/pkg/windows/${version}/msvc/gstreamer-1.0-msvc-${_arch}-${version}.exe")
    elseif(DEFINED _url_${platform})
        set(_url "${_url_${platform}}")
    else()
        message(FATAL_ERROR "gstreamer_get_package_url: Unknown platform '${platform}'")
    endif()

    set(${output_var}
        "${_url}"
        PARENT_SCOPE
    )
endfunction()

# gstreamer_get_s3_mirror_url(<platform> <version> <output_var>)
# Sets <output_var> to the QGC S3 mirror of that package, or "" for a platform without a mirror.
function(gstreamer_get_s3_mirror_url platform version output_var)
    set(_s3_base "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer")

    if(platform STREQUAL "android")
        set(_dir "android")
    elseif(platform STREQUAL "ios")
        set(_dir "ios")
    elseif(platform MATCHES "^macos")
        set(_dir "macos")
    elseif(platform MATCHES "^windows_msvc_")
        set(_dir "windows")
    else()
        set(${output_var}
            ""
            PARENT_SCOPE
        )
        return()
    endif()

    gstreamer_get_package_url("${platform}" "${version}" _primary_url)
    cmake_path(GET _primary_url FILENAME _filename)

    set(${output_var}
        "${_s3_base}/${_dir}/${_filename}"
        PARENT_SCOPE
    )
endfunction()

# gstreamer_get_fallback_checksum(<platform> <version> <output_var>)
# Sets <output_var> to "SHA256=<hex>" pinned at gstreamer.checksums.<version>.<platform> in build-config.json, or ""
# when the package is not pinned.
function(gstreamer_get_fallback_checksum platform version output_var)
    set(_checksum "")

    if(DEFINED QGC_BUILD_CONFIG_CONTENT)
        string(
            JSON
            _hash
            ERROR_VARIABLE
            _err
            GET
            "${QGC_BUILD_CONFIG_CONTENT}"
            "gstreamer"
            "checksums"
            "${version}"
            "${platform}"
        )
        if(NOT _err AND _hash)
            string(TOLOWER "${_hash}" _hash)
            set(_checksum "SHA256=${_hash}")
        endif()
    endif()

    set(${output_var}
        "${_checksum}"
        PARENT_SCOPE
    )
endfunction()

# Without a pin, missing sidecar data is fatal under GStreamer_REQUIRE_CHECKSUM and a warning otherwise.
function(_qgc_gst_unverified_or_fail platform version output_var reason)
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

# _qgc_gst_download_sidecar(<url> <dest_file> <result_var>)
# Downloads the .sha256sum sidecar to <dest_file>; sets <result_var> TRUE on success.
function(_qgc_gst_download_sidecar url dest_file result_var)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_deps/checksums")
    set(_tmp "${dest_file}.tmp")
    foreach(_attempt RANGE 1 3 1)
        file(
            DOWNLOAD "${url}" "${_tmp}"
            STATUS _status
            TIMEOUT 30
            INACTIVITY_TIMEOUT 10
            TLS_VERIFY ON
        )
        list(GET _status 0 _code)
        if(_code EQUAL 0)
            file(RENAME "${_tmp}" "${dest_file}")
            set(${result_var}
                TRUE
                PARENT_SCOPE
            )
            return()
        endif()
        list(GET _status 1 _msg)
        message(STATUS "GStreamer: Checksum download attempt ${_attempt}/3 failed: ${_msg}")
        file(REMOVE "${_tmp}")
    endforeach()
    set(${result_var}
        FALSE
        PARENT_SCOPE
    )
endfunction()

# gstreamer_fetch_checksum(<platform> <version> <output_var>)
# Sets <output_var> to the checksum to verify the package against: the pinned one, else the upstream .sha256sum
# sidecar, else "" (unverified). See GStreamer_REQUIRE_CHECKSUM for when the latter two are fatal.
function(gstreamer_fetch_checksum platform version output_var)
    # The in-tree pin is authoritative and the download is verified against it, so the
    # same-origin .sha256sum sidecar is only fetched for unpinned SDKs.
    gstreamer_get_fallback_checksum(${platform} ${version} _pinned_hash)
    if(_pinned_hash)
        set(${output_var}
            "${_pinned_hash}"
            PARENT_SCOPE
        )
        return()
    endif()

    gstreamer_get_package_url(${platform} ${version} _pkg_url)

    string(FIND "${_pkg_url}" "?" _qs_pos)
    if(NOT _qs_pos EQUAL -1)
        message(STATUS "GStreamer: Skipping checksum for ${platform} ${version} (URL contains query string)")
        set(${output_var}
            ""
            PARENT_SCOPE
        )
        return()
    endif()

    set(_checksum_url "${_pkg_url}.sha256sum")
    string(SHA256 _url_hash "${_checksum_url}")
    set(_checksum_file "${CMAKE_BINARY_DIR}/_deps/checksums/${_url_hash}.sha256sum")

    set(_dl_ok FALSE)
    foreach(_round RANGE 1 2 1)
        if(NOT EXISTS "${_checksum_file}")
            _qgc_gst_download_sidecar("${_checksum_url}" "${_checksum_file}" _dl_ok)
            if(NOT _dl_ok)
                _qgc_gst_unverified_or_fail(${platform} ${version} _result "Checksum sidecar unavailable")
                set(${output_var}
                    "${_result}"
                    PARENT_SCOPE
                )
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
                message(
                    FATAL_ERROR "GStreamer: no trusted (in-tree pinned) checksum for ${platform} ${version}; "
                                "the fetched same-origin .sha256sum sidecar cannot satisfy "
                                "GStreamer_REQUIRE_CHECKSUM=ON. Pin the hash in .github/build-config.json "
                                "(gstreamer.checksums.${version}.${platform}) or set GStreamer_REQUIRE_CHECKSUM=OFF."
                )
            endif()
            message(WARNING "GStreamer: using same-origin .sha256sum sidecar for ${platform} ${version}; "
                            "this detects corruption only, NOT supply-chain tampering. Pin the hash in "
                            ".github/build-config.json for authenticity."
            )
            set(${output_var}
                "SHA256=${_fetched_hex}"
                PARENT_SCOPE
            )
            return()
        endif()

        message(STATUS "GStreamer: Could not parse checksum for ${platform} ${version}")
        file(REMOVE "${_checksum_file}")
    endforeach()

    _qgc_gst_unverified_or_fail(${platform} ${version} _result "Checksum content invalid/unparseable")
    set(${output_var}
        "${_result}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_download_archive(<platform> <version> <filename> <dir> <hash_out>)
# Downloads the <version> SDK package for <platform> (upstream, else the QGC S3 mirror) to <dir>/<filename>, reusing a
# cached copy that matches its checksum, and sets <hash_out> to the "SHA256=<hex>" it was verified against, or "" when
# it is unverified. A failed download fails configure.
function(_qgc_gst_download_archive platform version filename dir hash_out)
    gstreamer_get_package_url("${platform}" "${version}" _url)
    gstreamer_get_s3_mirror_url("${platform}" "${version}" _s3_url)
    gstreamer_fetch_checksum("${platform}" "${version}" _hash)
    set(_hash_args "")
    if(_hash)
        set(_hash_args EXPECTED_HASH "${_hash}")
    endif()

    # The SDKs are large (the Android universal tarball is ~1 GB), so the wall-clock cap is generous and the
    # inactivity timeout catches stalls. QGC_GST_DOWNLOAD_TIMEOUT, not QGC_DOWNLOAD_TIMEOUT, tunes the cap.
    set(_timeout 1200)
    if(NOT "${QGC_GST_DOWNLOAD_TIMEOUT}" STREQUAL "")
        set(_timeout "${QGC_GST_DOWNLOAD_TIMEOUT}")
    endif()
    set(_inactivity_timeout 60)
    if(NOT "${QGC_GST_DOWNLOAD_INACTIVITY_TIMEOUT}" STREQUAL "")
        set(_inactivity_timeout "${QGC_GST_DOWNLOAD_INACTIVITY_TIMEOUT}")
    endif()

    qgc_resilient_download(
        FILENAME
        "${filename}"
        DESTINATION_DIR
        "${dir}"
        RESULT_VAR
        _
        URLS
        ${_url}
        ${_s3_url}
        TIMEOUT
        ${_timeout}
        INACTIVITY_TIMEOUT
        ${_inactivity_timeout}
        LOG_TAG
        GStreamer
        FAILURE_HINT
        "Install manually from https://gstreamer.freedesktop.org/download/ or set GStreamer_ROOT_DIR."
        ${_hash_args}
    )
    set(${hash_out}
        "${_hash}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_extract_once(<dir> <stamp> <extract_function> <archives>)
# Publishes <dir> exactly once per <stamp>: a matching <dir>/.complete means it is already extracted.
# Otherwise <extract_function>(<staging> <archives> <name>), <name> being <dir>'s file name, must create
# and fill <staging>, which is then stamped and renamed to <dir>, so <dir> only ever exists complete. The
# caller must hold the cache lock (see qgc_gst_acquire_sdk).
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
    cmake_path(GET dir FILENAME _name)
    cmake_language(CALL ${extract_function} "${_staging}" "${archives}" "${_name}")
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
#                     EXTRACT_FUNCTION <command> CONTENT_DIR_OUT <var>)
# Downloads the SDK archive(s) for <platform> (the second one as <platform>_devel) into
# <cache>/<subdir> and extracts them once, with <command>(<staging> <archives> <name>), into
# <cache>/<subdir>/<name>, returned in CONTENT_DIR_OUT. <subdir> must name the version.
# <cache> is CPM_SOURCE_CACHE when set (shared by build directories and CI jobs), else
# <build>/_deps. Concurrent configures are serialized on <cache>/<subdir>/.lock, and the extracted
# tree is stamped with the archive hashes; see _qgc_gst_extract_once.
function(qgc_gst_acquire_sdk)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "PLATFORM;VERSION;CACHE_SUBDIR;CONTENT;EXTRACT_FUNCTION;CONTENT_DIR_OUT"
                          "ARCHIVES"
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
    set(_stamp "")
    foreach(_archive IN LISTS ARG_ARCHIVES)
        list(POP_FRONT _platforms _platform)
        set(_archive_path "${_cache_dir}/${_archive}")
        _qgc_gst_download_archive(${_platform} ${ARG_VERSION} "${_archive}" "${_cache_dir}" _archive_hash)
        if(NOT _archive_hash)
            file(SHA256 "${_archive_path}" _archive_sha256)
            set(_archive_hash "SHA256=${_archive_sha256}")
        endif()
        list(APPEND _archive_paths "${_archive_path}")
        string(APPEND _stamp "${_archive}=${_archive_hash}\n")
    endforeach()

    _qgc_gst_extract_once("${_cache_dir}/${ARG_CONTENT}" "${_stamp}" ${ARG_EXTRACT_FUNCTION} "${_archive_paths}")
    set(${ARG_CONTENT_DIR_OUT}
        "${_cache_dir}/${ARG_CONTENT}"
        PARENT_SCOPE
    )
endfunction()
