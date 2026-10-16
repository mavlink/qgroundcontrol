# The license notice that ships with the GStreamer an application bundles: the LGPL text and a NOTICE.txt naming the
# GStreamer version, where its source code is, and the official packages an auto-downloaded SDK came from.

include_guard(GLOBAL)

# qgc_gst_write_notice(<dir>)
# Writes LGPL-2.1.txt and NOTICE.txt into <dir> for GStreamer_VERSION. When GStreamer_AUTO_DOWNLOADED, NOTICE.txt also
# lists the packages in QGC_GST_RESOLVED_PACKAGES ("<url>|<sha256>" entries, see qgc_gst_acquire_sdk) and the Cerbero
# release that built them.
function(qgc_gst_write_notice dir)
    set(_version "${GStreamer_VERSION}")
    if(GStreamer_AUTO_DOWNLOADED AND QGC_GST_RESOLVED_PACKAGES)
        set(_binaries "The libraries and plugins come from the official GStreamer packages")
        foreach(_package IN LISTS QGC_GST_RESOLVED_PACKAGES)
            string(REPLACE "|" ";" _package "${_package}")
            list(GET _package 0 _url)
            list(GET _package 1 _sha256)
            string(APPEND _binaries "\n    ${_url} (SHA-256 ${_sha256})")
        endforeach()
        string(APPEND _binaries "\nbuilt by Cerbero, which also builds the libraries GStreamer uses:\n"
               "    https://gitlab.freedesktop.org/gstreamer/cerbero/-/tree/${_version}"
        )
    else()
        string(CONCAT _binaries "The libraries and plugins come from the GStreamer installation QGroundControl was "
                      "built with; its provider (an SDK or a Linux distribution) publishes the matching source code."
        )
    endif()
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/notice/NOTICE.txt.in" "${dir}/NOTICE.txt" @ONLY)
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/notice/LGPL-2.1.txt" "${dir}/LGPL-2.1.txt" COPYONLY)
endfunction()
