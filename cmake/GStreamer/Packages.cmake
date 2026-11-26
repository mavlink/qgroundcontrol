# The GStreamer SDK packages: their upstream and QGC mirror URLs, the checksums they are verified against, and their
# download. Acquire.cmake turns the downloaded packages into SDK trees.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../modules/Download.cmake") # qgc_resilient_download / qgc_parse_expected_hash
include("${CMAKE_CURRENT_LIST_DIR}/../modules/BuildConfigReader.cmake")

# _qgc_gst_package_path(<package> <version> <output_var>)
# Sets <output_var> to the path of the <version> SDK package <package> under gstreamer.package_url: its entry in
# gstreamer.packages of build-config.json (android, ios, macos, macos_devel, windows_msvc_x64 or windows_msvc_arm64).
function(_qgc_gst_package_path package version output_var)
    qgc_config_read(_path OPTIONAL gstreamer packages ${package})
    if(NOT _path)
        message(FATAL_ERROR "GStreamer: unknown SDK package '${package}' (gstreamer.packages in build-config.json)")
    endif()
    string(REPLACE "{version}" "${version}" _path "${_path}")
    set(${output_var}
        "${_path}"
        PARENT_SCOPE
    )
endfunction()

# qgc_gst_get_package_url(<package> <version> <output_var>)
# Sets <output_var> to the upstream URL of the <version> SDK package <package>.
function(qgc_gst_get_package_url package version output_var)
    _qgc_gst_package_path("${package}" "${version}" _path)
    qgc_config_read(_base gstreamer package_url)
    set(${output_var}
        "${_base}/${_path}"
        PARENT_SCOPE
    )
endfunction()

# qgc_gst_get_s3_mirror_url(<package> <version> <output_var>)
# Sets <output_var> to the QGC mirror of that package: the same file under gstreamer.package_mirror_url, in the
# directory of its platform, the first directory of its path.
function(qgc_gst_get_s3_mirror_url package version output_var)
    _qgc_gst_package_path("${package}" "${version}" _path)
    qgc_config_read(_mirror gstreamer package_mirror_url)
    string(REGEX REPLACE "^([^/]+)/.*/([^/]+)$" "\\1/\\2" _path "${_path}")
    set(${output_var}
        "${_mirror}/${_path}"
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
# configure. Each attempt may take 1200 s, or QGC_DOWNLOAD_TIMEOUT when that is longer.
function(_qgc_gst_download_archive platform version filename dir)
    qgc_gst_get_package_url("${platform}" "${version}" _url)
    qgc_gst_get_s3_mirror_url("${platform}" "${version}" _s3_url)
    qgc_gst_get_pinned_checksum("${platform}" "${version}" _hash)

    # The SDKs are large (the iOS tarball is 1.2 GB) and a failed attempt starts over, so the wall-clock cap is generous
    # and a slow link can raise it; the inactivity timeout catches stalls.
    set(_timeout 1200)
    if(QGC_DOWNLOAD_TIMEOUT GREATER _timeout)
        set(_timeout "${QGC_DOWNLOAD_TIMEOUT}")
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
        EXPECTED_HASH
        "${_hash}"
    )
endfunction()
