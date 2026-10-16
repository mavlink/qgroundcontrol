# GStreamer SDK download / URL / checksum helpers.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")

# Absolute path disambiguates from this file (also named Download.cmake): when
# test_download.cmake adds cmake/GStreamer/ to MODULE_PATH, a bare `include(Download)`
# would resolve to this file and self-recurse.
include("${CMAKE_CURRENT_LIST_DIR}/../modules/Download.cmake") # qgc_resilient_download / qgc_parse_expected_hash
include("${CMAKE_CURRENT_LIST_DIR}/../modules/BuildConfigReader.cmake")

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
# Sets <output_var> to the QGC S3 mirror of that package: the same file under the mirror's <platform directory>/.
function(gstreamer_get_s3_mirror_url platform version output_var)
    gstreamer_get_package_url("${platform}" "${version}" _url)
    string(REGEX
           REPLACE "^https://gstreamer\\.freedesktop\\.org/data/pkg/([a-z]+)/.*/([^/]+)$"
                   "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer/\\1/\\2" _url "${_url}"
    )
    set(${output_var}
        "${_url}"
        PARENT_SCOPE
    )
endfunction()

# gstreamer_get_fallback_checksum(<platform> <version> <output_var>)
# Sets <output_var> to "SHA256=<hex>" pinned at gstreamer.checksums.<version>.<platform> in build-config.json, or ""
# when the package is not pinned.
function(gstreamer_get_fallback_checksum platform version output_var)
    qgc_config_read(_hash OPTIONAL gstreamer checksums ${version} ${platform})
    set(_checksum "")
    if(_hash)
        set(_checksum "SHA256=${_hash}")
    endif()
    set(${output_var}
        "${_checksum}"
        PARENT_SCOPE
    )
endfunction()

# gstreamer_fetch_checksum(<platform> <version> <output_var>)
# Sets <output_var> to the checksum to verify the package against: the pinned one, else the upstream .sha256sum
# sidecar, else "" (unverified). Without a pin, GStreamer_REQUIRE_CHECKSUM fails configure.
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
    # A same-origin sidecar proves nothing against a bad mirror/MITM.
    if(GStreamer_REQUIRE_CHECKSUM)
        message(
            FATAL_ERROR "GStreamer: no trusted (in-tree pinned) checksum for ${platform} ${version}; "
                        "the upstream same-origin .sha256sum sidecar cannot satisfy "
                        "GStreamer_REQUIRE_CHECKSUM=ON. Pin the hash in .github/build-config.json "
                        "(gstreamer.checksums.${version}.${platform}) or set GStreamer_REQUIRE_CHECKSUM=OFF."
        )
    endif()

    gstreamer_get_package_url(${platform} ${version} _pkg_url)

    set(_checksum_url "${_pkg_url}.sha256sum")
    string(SHA256 _url_hash "${_checksum_url}")
    # CMake regex has no {n} quantifier.
    string(REPEAT "[0-9a-fA-F]" 64 _sha256_hex)
    # A cached sidecar that does not parse is fetched again once.
    foreach(_round RANGE 1 2 1)
        qgc_resilient_download(
            FILENAME
            "${_url_hash}.sha256sum"
            DESTINATION_DIR
            "${CMAKE_BINARY_DIR}/_deps/checksums"
            RESULT_VAR
            _checksum_file
            URLS
            "${_checksum_url}"
            RETRIES
            3
            RETRY_BACKOFF
            0
            TIMEOUT
            30
            INACTIVITY_TIMEOUT
            10
            ALLOW_FAILURE
            LOG_TAG
            GStreamer
        )
        if(NOT _checksum_file)
            break()
        endif()
        file(READ "${_checksum_file}" _content)
        if(_content MATCHES "(${_sha256_hex})")
            message(WARNING "GStreamer: using same-origin .sha256sum sidecar for ${platform} ${version}; "
                            "this detects corruption only, NOT supply-chain tampering. Pin the hash in "
                            ".github/build-config.json for authenticity."
            )
            set(${output_var}
                "SHA256=${CMAKE_MATCH_1}"
                PARENT_SCOPE
            )
            return()
        endif()
        message(STATUS "GStreamer: Could not parse checksum for ${platform} ${version}")
        file(REMOVE "${_checksum_file}")
    endforeach()

    message(WARNING "GStreamer: no usable checksum sidecar for ${platform} ${version}; the SDK will be used WITHOUT "
                    "integrity verification — supply-chain authenticity is NOT guaranteed. Only proceed on a "
                    "trusted network/mirror. (GStreamer_REQUIRE_CHECKSUM=ON makes this a hard error.)"
    )
    set(${output_var}
        ""
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_download_archive(<platform> <version> <filename> <dir> <hash_out>)
# Downloads the <version> SDK package for <platform> (upstream, else the QGC S3 mirror) to <dir>/<filename>, reusing a
# cached copy that matches its checksum, and sets <hash_out> to the "SHA256=<hex>" it was verified against, or "" when
# it is unverified. The mirror's copy is held to the same checksum. A failed download fails configure.
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

# _qgc_gst_pinned_stamp(<platform> <version> <stamp_out> <archive>...)
# Sets <stamp_out> to the completion stamp (see _qgc_gst_extract_once) of the <archive>s, the primary one for <platform>
# and a second one for <platform>_devel, when every one is pinned in build-config.json, else to "".
function(_qgc_gst_pinned_stamp platform version stamp_out)
    set(_platforms "${platform}" "${platform}_devel")
    set(_stamp "")
    foreach(_archive IN LISTS ARGN)
        list(POP_FRONT _platforms _platform)
        gstreamer_get_fallback_checksum(${_platform} ${version} _pin)
        if(NOT _pin)
            set(_stamp "")
            break()
        endif()
        string(APPEND _stamp "${_archive}=${_pin}\n")
    endforeach()
    set(${stamp_out}
        "${_stamp}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_download_archives(<dir> <platform> <version> <paths_out> <stamp_out> <archive>...)
# Downloads the <archive>s, the primary one for <platform> and a second one for <platform>_devel, into <dir>; sets
# <paths_out> to their paths and <stamp_out> to their completion stamp, each archive's line naming the checksum it was
# verified against, else its own SHA256.
function(_qgc_gst_download_archives dir platform version paths_out stamp_out)
    set(_platforms "${platform}" "${platform}_devel")
    set(_paths "")
    set(_stamp "")
    foreach(_archive IN LISTS ARGN)
        list(POP_FRONT _platforms _platform)
        _qgc_gst_download_archive(${_platform} ${version} "${_archive}" "${dir}" _hash)
        if(NOT _hash)
            file(SHA256 "${dir}/${_archive}" _sha256)
            set(_hash "SHA256=${_sha256}")
        endif()
        list(APPEND _paths "${dir}/${_archive}")
        string(APPEND _stamp "${_archive}=${_hash}\n")
    endforeach()
    set(${paths_out}
        "${_paths}"
        PARENT_SCOPE
    )
    set(${stamp_out}
        "${_stamp}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_record_packages(<platform> <version> <stamp>)
# Records the upstream packages a completion stamp (see _qgc_gst_extract_once) names, the primary one for <platform>
# and a second one for <platform>_devel, as "<url>|<sha256>" entries in the INTERNAL cache entry
# QGC_GST_RESOLVED_PACKAGES, which the SDK's notice (Notice.cmake) and SBOM entry (generate_cpm_sbom.py) name.
function(_qgc_gst_record_packages platform version stamp)
    # CI tooling reads the name from CMakeCache.txt, so it keeps the public QGC_ prefix like SdkState.cmake's entries.
    # cmake-lint: disable=C0103
    set(_platforms "${platform}" "${platform}_devel")
    set(_packages "")
    string(REGEX MATCHALL "=SHA256=[0-9a-fA-F]+" _hashes "${stamp}")
    foreach(_hash IN LISTS _hashes)
        list(POP_FRONT _platforms _platform)
        gstreamer_get_package_url(${_platform} ${version} _url)
        string(REPLACE "=SHA256=" "" _sha256 "${_hash}")
        list(APPEND _packages "${_url}|${_sha256}")
    endforeach()
    set(QGC_GST_RESOLVED_PACKAGES
        "${_packages}"
        CACHE INTERNAL "Upstream packages (<url>|<sha256>) of the auto-downloaded GStreamer SDK"
    )
endfunction()

# qgc_gst_acquire_sdk(PLATFORM <platform> VERSION <version> CACHE_SUBDIR <subdir>
#                     ARCHIVES <primary> [<devel>] CONTENT <name>
#                     EXTRACT_FUNCTION <command> CONTENT_DIR_OUT <var>)
# Downloads the SDK archive(s) for <platform> (the second one as <platform>_devel) into
# <cache>/<subdir> and extracts them once, with <command>(<staging> <archives> <name>), into
# <cache>/<subdir>/<name>, returned in CONTENT_DIR_OUT. <subdir> must name the version. When every archive is pinned,
# a tree already extracted from them is used without downloading them.
# <cache> is CPM_SOURCE_CACHE, which the top-level CMakeLists.txt always sets, so build directories and CI jobs
# share it. Concurrent configures are serialized on <cache>/<subdir>/.lock, and the extracted
# tree is stamped with the archive hashes; see _qgc_gst_extract_once. <command> sees
# _qgc_gst_archives_pinned TRUE when every archive was verified against its checksum pinned in build-config.json.
# Records the packages in QGC_GST_RESOLVED_PACKAGES (_qgc_gst_record_packages).
function(qgc_gst_acquire_sdk)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "PLATFORM;VERSION;CACHE_SUBDIR;CONTENT;EXTRACT_FUNCTION;CONTENT_DIR_OUT"
                          "ARCHIVES"
    )
    _qgc_gst_check_args(
        qgc_gst_acquire_sdk
        PLATFORM
        VERSION
        CACHE_SUBDIR
        ARCHIVES
        CONTENT
        EXTRACT_FUNCTION
        CONTENT_DIR_OUT
    )
    list(LENGTH ARG_ARCHIVES _archive_count)
    if(_archive_count GREATER 2)
        message(FATAL_ERROR "qgc_gst_acquire_sdk: ARCHIVES takes a primary and an optional devel archive")
    endif()
    if(NOT COMMAND ${ARG_EXTRACT_FUNCTION})
        message(FATAL_ERROR "qgc_gst_acquire_sdk: EXTRACT_FUNCTION '${ARG_EXTRACT_FUNCTION}' is not a command")
    endif()

    set(_cache_dir "${CPM_SOURCE_CACHE}/${ARG_CACHE_SUBDIR}")
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

    # A tree stamped with the pinned checksums is used without hashing, or even having, its archives: CI caches keep
    # only the tree.
    _qgc_gst_pinned_stamp(${ARG_PLATFORM} ${ARG_VERSION} _pinned_stamp ${ARG_ARCHIVES})
    set(_content_dir "${_cache_dir}/${ARG_CONTENT}")
    if(_pinned_stamp AND EXISTS "${_content_dir}/.complete")
        file(READ "${_content_dir}/.complete" _existing_stamp)
        if(_existing_stamp STREQUAL _pinned_stamp)
            _qgc_gst_record_packages(${ARG_PLATFORM} ${ARG_VERSION} "${_pinned_stamp}")
            set(${ARG_CONTENT_DIR_OUT}
                "${_content_dir}"
                PARENT_SCOPE
            )
            return()
        endif()
    endif()
    _qgc_gst_download_archives("${_cache_dir}" ${ARG_PLATFORM} ${ARG_VERSION} _archive_paths _stamp ${ARG_ARCHIVES})
    set(_qgc_gst_archives_pinned FALSE)
    if(_pinned_stamp)
        set(_qgc_gst_archives_pinned TRUE)
    endif()
    _qgc_gst_extract_once("${_content_dir}" "${_stamp}" ${ARG_EXTRACT_FUNCTION} "${_archive_paths}")
    _qgc_gst_record_packages(${ARG_PLATFORM} ${ARG_VERSION} "${_stamp}")
    set(${ARG_CONTENT_DIR_OUT}
        "${_content_dir}"
        PARENT_SCOPE
    )
endfunction()
