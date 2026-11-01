# iOS GStreamer SDK acquisition: the official xcframework tarball, or the older devel .pkg.
# Included by Acquire.cmake, whose qgc_gst_acquire_sdk does the download, caching and locking.

include_guard(GLOBAL)

# _qgc_gst_ios_extract(<staging> <archive>)
# qgc_gst_acquire_sdk extractor: unpacks the iOS xcframework tarball (1.28.8+), or expands the older devel .pkg, into
# <staging>/expanded.
function(_qgc_gst_ios_extract staging archive)
    file(MAKE_DIRECTORY "${staging}")
    if(NOT archive MATCHES "\\.tar\\.xz$")
        _qgc_gst_pkgutil_expand_and_validate("${archive}" "${staging}/expanded" "iOS")
        return()
    endif()
    message(STATUS "Extracting GStreamer iOS xcframework...")
    # The xcframework also holds tvOS slices, ~2 GB that QGC never links.
    # cmake-lint: disable=E1126
    file(
        ARCHIVE_EXTRACT
        INPUT
        "${archive}"
        DESTINATION
        "${staging}/expanded"
        PATTERNS
        "GStreamer.xcframework/Info.plist"
        "GStreamer.xcframework/ios-*"
    )
    if(NOT EXISTS "${staging}/expanded/GStreamer.xcframework/Info.plist")
        message(FATAL_ERROR "GStreamer: ${archive} has no GStreamer.xcframework/")
    endif()
endfunction()

# _qgc_gst_acquire_ios_sdk(<out_dir>)
# Downloads and unpacks the official iOS package; sets <out_dir> to the directory holding its contents.
function(_qgc_gst_acquire_ios_sdk out_dir)
    qgc_gst_get_package_url(ios ${QGC_GST_REQUESTED_VERSION} _url)
    set(_archive gstreamer-ios.pkg)
    if(_url MATCHES "\\.tar\\.xz$")
        set(_archive gstreamer-ios.tar.xz)
    endif()
    qgc_gst_acquire_sdk(
        PLATFORM ios
        VERSION ${QGC_GST_REQUESTED_VERSION}
        CACHE_SUBDIR "gstreamer-ios-${QGC_GST_REQUESTED_VERSION}"
        ARCHIVES ${_archive}
        CONTENT sdk
        EXTRACT_FUNCTION _qgc_gst_ios_extract
        CONTENT_DIR_OUT _sdk
    )
    set(${out_dir}
        "${_sdk}/expanded"
        PARENT_SCOPE
    )
endfunction()
