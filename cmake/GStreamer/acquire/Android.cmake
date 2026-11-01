# Android GStreamer SDK acquisition: one ABI tree of the official universal tarball.
# Included by Acquire.cmake, whose qgc_gst_acquire_sdk does the download, caching and locking.

include_guard(GLOBAL)

# _qgc_gst_android_extract(<staging> <archive> <abi_dir>)
# qgc_gst_acquire_sdk extractor: unpacks only the <abi_dir>/ tree of the universal SDK tarball into
# <staging>. Every ABI build (including Qt's multi-ABI sub-builds) acquires its own tree.
function(_qgc_gst_android_extract staging archive abi_dir)
    # cmake-lint: disable=E1126
    file(
        ARCHIVE_EXTRACT
        INPUT
        "${archive}"
        DESTINATION
        "${staging}"
        PATTERNS
        "${abi_dir}/*"
    )
    if(NOT EXISTS "${staging}/${abi_dir}/lib/pkgconfig/gstreamer-1.0.pc")
        message(FATAL_ERROR "GStreamer: ${archive} has no ${abi_dir}/ SDK tree")
    endif()
endfunction()

# _qgc_gst_acquire_android_sdk(<abi_dir> <out_root>)
# Downloads the universal Android SDK and extracts its <abi_dir> tree (_qgc_gst_android_abi_info); sets <out_root> to
# that tree.
function(_qgc_gst_acquire_android_sdk abi_dir out_root)
    qgc_gst_acquire_sdk(
        PLATFORM android
        VERSION ${QGC_GST_REQUESTED_VERSION}
        CACHE_SUBDIR "gstreamer-android-${QGC_GST_REQUESTED_VERSION}"
        ARCHIVES "gstreamer-android-${QGC_GST_REQUESTED_VERSION}.tar.xz"
        CONTENT ${abi_dir}
        EXTRACT_FUNCTION _qgc_gst_android_extract
        CONTENT_DIR_OUT _sdk
    )
    set(${out_root}
        "${_sdk}/${abi_dir}"
        PARENT_SCOPE
    )
endfunction()
