# The GStreamer SDK packages: their upstream and QGC mirror URLs, the checksums they are verified against, and their
# download. Acquire.cmake turns the downloaded packages into SDK trees.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")
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
            message(
                FATAL_ERROR "Windows GStreamer SDK requires version 1.28 or later (got '${version}'). "
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
    string(
        REGEX REPLACE "^https://gstreamer\\.freedesktop\\.org/data/pkg/([a-z]+)/.*/([^/]+)$"
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
            FATAL_ERROR
                "GStreamer: no trusted (in-tree pinned) checksum for ${platform} ${version}; "
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
            message(
                WARNING
                    "GStreamer: using same-origin .sha256sum sidecar for ${platform} ${version}; "
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

    message(
        WARNING
            "GStreamer: no usable checksum sidecar for ${platform} ${version}; the SDK will be used WITHOUT "
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
