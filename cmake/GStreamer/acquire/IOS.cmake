# iOS GStreamer SDK acquisition: the official xcframework tarball.
# Included by Acquire.cmake, whose qgc_gst_acquire_sdk does the download, caching and locking.

include_guard(GLOBAL)

# _qgc_gst_ios_extract(<staging> <archive>)
# qgc_gst_acquire_sdk extractor: unpacks the iOS slices of the xcframework tarball into <staging>/expanded.
function(_qgc_gst_ios_extract staging archive)
    file(MAKE_DIRECTORY "${staging}")
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

# _qgc_gst_acquire_ios_sdk(<out_xcframework>)
# Downloads and unpacks the official iOS package; sets <out_xcframework> to its GStreamer.xcframework.
function(_qgc_gst_acquire_ios_sdk out_xcframework)
    qgc_gst_acquire_sdk(
        PLATFORM ios
        VERSION ${QGC_GST_REQUESTED_VERSION}
        CACHE_SUBDIR "gstreamer-ios-${QGC_GST_REQUESTED_VERSION}"
        ARCHIVES gstreamer-ios.tar.xz
        CONTENT sdk
        EXTRACT_FUNCTION _qgc_gst_ios_extract
        CONTENT_DIR_OUT _sdk
    )
    set(${out_xcframework}
        "${_sdk}/expanded/GStreamer.xcframework"
        PARENT_SCOPE
    )
endfunction()
