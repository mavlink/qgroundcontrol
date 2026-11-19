# The GStreamer SDK packages: their upstream and QGC mirror URLs, the checksums they are verified against, and their
# download. Acquire.cmake turns the downloaded packages into SDK trees.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../modules/Download.cmake") # qgc_resilient_download / qgc_parse_expected_hash
include("${CMAKE_CURRENT_LIST_DIR}/../modules/BuildConfigReader.cmake")

# qgc_gst_get_package_url(<platform> <version> <output_var>)
# Sets <output_var> to the gstreamer.freedesktop.org URL of the <version> SDK package for <platform> (android, ios,
# macos, macos_devel, windows_msvc_x64 or windows_msvc_arm64).
function(qgc_gst_get_package_url platform version output_var)
    set(_base "https://gstreamer.freedesktop.org")

    # URL templates for simple platforms — version substituted via _base.
    set(_url_android "${_base}/data/pkg/android/${version}/gstreamer-1.0-android-universal-${version}.tar.xz")
    # From 1.28.8 on, the iOS SDK is an xcframework tarball instead of a devel .pkg.
    if(version VERSION_GREATER_EQUAL 1.28.8)
        set(_url_ios "${_base}/data/pkg/ios/${version}/gstreamer-${version}-xcframework.tar.xz")
    else()
        set(_url_ios "${_base}/data/pkg/ios/${version}/gstreamer-1.0-devel-${version}-ios-universal.pkg")
    endif()
    set(_url_macos "${_base}/data/pkg/macos/${version}/gstreamer-1.0-${version}-universal.pkg")
    set(_url_macos_devel "${_base}/data/pkg/macos/${version}/gstreamer-1.0-devel-${version}-universal.pkg")

    if(platform MATCHES "^windows_msvc_(x64|arm64)$")
        # The MSVC installers are named by architecture.
        if(CMAKE_MATCH_1 STREQUAL "x64")
            set(_arch "x86_64")
        else()
            set(_arch "arm64")
        endif()
        set(_url "${_base}/data/pkg/windows/${version}/msvc/gstreamer-1.0-msvc-${_arch}-${version}.exe")
    elseif(DEFINED _url_${platform})
        set(_url "${_url_${platform}}")
    else()
        message(FATAL_ERROR "qgc_gst_get_package_url: Unknown platform '${platform}'")
    endif()

    set(${output_var}
        "${_url}"
        PARENT_SCOPE
    )
endfunction()

# qgc_gst_get_s3_mirror_url(<platform> <version> <output_var>)
# Sets <output_var> to the QGC S3 mirror of that package: the same file under the mirror's <platform directory>/.
function(qgc_gst_get_s3_mirror_url platform version output_var)
    qgc_gst_get_package_url("${platform}" "${version}" _url)
    string(
        REGEX REPLACE "^https://gstreamer\\.freedesktop\\.org/data/pkg/([a-z]+)/.*/([^/]+)$"
                      "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer/\\1/\\2" _url "${_url}"
    )
    set(${output_var}
        "${_url}"
        PARENT_SCOPE
    )
endfunction()

# qgc_gst_get_pinned_checksum(<platform> <version> <output_var>)
# Sets <output_var> to "SHA256=<hex>", the checksum pinned at gstreamer.checksums.<version>.<platform> in
# build-config.json and the only one a package is verified against. Fails configure when the package is not pinned.
function(qgc_gst_get_pinned_checksum platform version output_var)
    qgc_config_read(_hash OPTIONAL gstreamer checksums ${version} ${platform})
    if(NOT _hash)
        message(
            FATAL_ERROR
                "GStreamer: no checksum is pinned for the ${platform} ${version} package. Add "
                "gstreamer.checksums.${version}.${platform} to .github/build-config.json, or point "
                "GStreamer_ROOT_DIR at an installed SDK."
        )
    endif()
    set(${output_var}
        "SHA256=${_hash}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_download_archive(<platform> <version> <filename> <dir>)
# Downloads the <version> SDK package for <platform> (upstream, else the QGC S3 mirror) to <dir>/<filename>, reusing a
# cached copy that matches its pinned checksum. The mirror's copy is held to the same checksum. A failed download fails
# configure.
function(_qgc_gst_download_archive platform version filename dir)
    qgc_gst_get_package_url("${platform}" "${version}" _url)
    qgc_gst_get_s3_mirror_url("${platform}" "${version}" _s3_url)
    qgc_gst_get_pinned_checksum("${platform}" "${version}" _hash)

    # The SDKs are large (the Android universal tarball is ~1 GB), so the wall-clock cap is generous and the
    # inactivity timeout catches stalls.
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
        1200
        LOG_TAG
        GStreamer
        FAILURE_HINT
        "Install manually from https://gstreamer.freedesktop.org/download/ or set GStreamer_ROOT_DIR."
        EXPECTED_HASH
        "${_hash}"
    )
endfunction()
