# The license notice that ships with the GStreamer an application bundles: the LGPL text and a NOTICE.txt naming the
# GStreamer version, where its source code is, and the official packages an auto-downloaded SDK came from.

include_guard(GLOBAL)

# qgc_gst_write_notice(<dir> [PLUGINS <plugin>...])
# Writes LGPL-2.1.txt and NOTICE.txt into <dir> for GStreamer_VERSION. When GStreamer_AUTO_DOWNLOADED, NOTICE.txt also
# lists the packages in QGC_GST_RESOLVED_PACKAGES ("<url>|<sha256>" entries, see qgc_gst_acquire_sdk) and the Cerbero
# release that built them. It names the third-party libraries that the shipped <plugin>s, and on every platform but
# Linux the OpenSSL GIO module, bring along, and on iOS, which links GStreamer statically, how to relink it.
function(qgc_gst_write_notice dir)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "PLUGINS")
    set(_version "${GStreamer_VERSION}")
    if(GStreamer_AUTO_DOWNLOADED AND QGC_GST_RESOLVED_PACKAGES)
        set(_binaries "The libraries and plugins come from the official GStreamer packages")
        foreach(_package IN LISTS QGC_GST_RESOLVED_PACKAGES)
            string(REPLACE "|" ";" _package "${_package}")
            list(GET _package 0 _url)
            list(GET _package 1 _sha256)
            string(APPEND _binaries "\n    ${_url} (SHA-256 ${_sha256})")
        endforeach()
        string(
            APPEND _binaries "\nbuilt by Cerbero, which also builds the libraries GStreamer uses:\n"
            "    https://gitlab.freedesktop.org/gstreamer/cerbero/-/tree/${_version}"
        )
    else()
        string(
            CONCAT _binaries "The libraries and plugins come from the GStreamer installation QGroundControl was "
                   "built with; its provider (an SDK or a Linux distribution) publishes the matching source code."
        )
    endif()

    # <plugin>|<library>|<license>|<source>
    set(_ffmpeg "FFmpeg (libavcodec, libavformat, libavutil and their companions)")
    set(_libraries
        "libav|${_ffmpeg}|LGPL-2.1-or-later|https://ffmpeg.org/download.html"
        "vpx|libvpx|BSD-3-Clause|https://chromium.googlesource.com/webm/libvpx"
        "dav1d|dav1d|BSD-2-Clause|https://code.videolan.org/videolan/dav1d"
        "openh264|OpenH264|BSD-2-Clause|https://github.com/cisco/openh264"
    )
    if(NOT LINUX)
        list(APPEND _libraries "|OpenSSL (TLS for GIO)|Apache-2.0|https://openssl-library.org/source/")
    endif()
    set(_third_party "")
    foreach(_entry IN LISTS _libraries)
        string(REPLACE "|" ";" _entry "${_entry}")
        list(GET _entry 0 _plugin)
        list(GET _entry 1 _library)
        list(GET _entry 2 _license)
        list(GET _entry 3 _source)
        if(_plugin AND NOT _plugin IN_LIST ARG_PLUGINS)
            continue()
        endif()
        string(APPEND _third_party "    ${_library}, ${_license}: ${_source}\n")
        if(_plugin STREQUAL "openh264")
            string(
                APPEND _third_party
                "      Cisco's H.264 (AVC) patent license covers only the OpenH264 binaries Cisco distributes; the "
                "official\n      GStreamer SDKs build their own, which it does not cover.\n"
            )
        endif()
    endforeach()
    if(_third_party)
        string(
            PREPEND _third_party "\nThe GStreamer plugins QGroundControl ships use these third-party libraries, under "
            "their own licenses:\n"
        )
    endif()

    set(_relink "")
    if(IOS)
        string(
            CONCAT
                _relink
                "\nOn iOS, GStreamer is linked statically into the GStreamerMobile framework inside the app. To use a "
                "modified\nGStreamer, build QGroundControl from its source code, "
                "https://github.com/mavlink/qgroundcontrol, with\nGStreamer_ROOT_DIR set to your GStreamer SDK: the "
                "build links the framework, and the app, against it\n"
                "(src/VideoManager/VideoReceiver/GStreamer/README.md describes the build).\n"
        )
    endif()
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/notice/NOTICE.txt.in" "${dir}/NOTICE.txt" @ONLY)
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/notice/LGPL-2.1.txt" "${dir}/LGPL-2.1.txt" COPYONLY)
endfunction()
