# Linux GStreamer SDK discovery — invoked by Orchestrator.cmake.

macro(_qgc_discover_linux_sdk)
    if(NOT DEFINED GStreamer_ROOT_DIR)
        if(CMAKE_SYSROOT)
            set(GStreamer_ROOT_DIR "${CMAKE_SYSROOT}/usr")
        else()
            set(GStreamer_ROOT_DIR "/usr")
        endif()
    endif()

    # Candidate multiarch triplets: explicit CMAKE_LIBRARY_ARCHITECTURE first, then
    # any lib/<triplet> that actually contains gstreamer-1.0 (handles musl,
    # arm-linux-gnueabihf, and non-Debian layouts where the triplet is unset).
    set(_gst_linux_triplet_candidates)
    if(CMAKE_LIBRARY_ARCHITECTURE)
        list(APPEND _gst_linux_triplet_candidates "${CMAKE_LIBRARY_ARCHITECTURE}")
    endif()
    list(APPEND _gst_linux_triplet_candidates "${CMAKE_SYSTEM_PROCESSOR}-linux-gnu")
    # musl sysroots use a -linux-musl triplet and often leave CMAKE_LIBRARY_ARCHITECTURE unset.
    list(APPEND _gst_linux_triplet_candidates "${CMAKE_SYSTEM_PROCESSOR}-linux-musl")
    file(GLOB _gst_linux_multiarch_dirs LIST_DIRECTORIES true "${GStreamer_ROOT_DIR}/lib/*-linux-*")
    foreach(_d IN LISTS _gst_linux_multiarch_dirs)
        cmake_path(GET _d FILENAME _d_name)
        list(APPEND _gst_linux_triplet_candidates "${_d_name}")
    endforeach()
    list(REMOVE_DUPLICATES _gst_linux_triplet_candidates)

    set(_gst_linux_lib "")
    foreach(_triplet IN LISTS _gst_linux_triplet_candidates)
        if(EXISTS "${GStreamer_ROOT_DIR}/lib/${_triplet}/gstreamer-1.0")
            set(_gst_linux_lib "${GStreamer_ROOT_DIR}/lib/${_triplet}")
            break()
        endif()
    endforeach()
    if(NOT _gst_linux_lib)
        if(EXISTS "${GStreamer_ROOT_DIR}/lib64/gstreamer-1.0")
            set(_gst_linux_lib "${GStreamer_ROOT_DIR}/lib64")
        elseif(EXISTS "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0")
            set(_gst_linux_lib "${GStreamer_ROOT_DIR}/lib")
        else()
            message(FATAL_ERROR "Could not locate GStreamer libraries - check installation or set environment/cmake variables")
        endif()
    endif()

    gstreamer_create_layout_target(
        SDK_ROOT "${GStreamer_ROOT_DIR}"
        TYPE     FLAT
        LIB_PATH "${_gst_linux_lib}"
    )

    # Prepend SDK pkgconfig dir so system glib/gobject .pc files remain discoverable.
    gstreamer_apply_pkgconfig_env(
        MODE SYSTEM_AUGMENT
        LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig"
    )
endmacro()

# Zero-copy DMABuf GPU path probe. Sets QGC_GST_HAS_DMABUF; consumed by
# HwBuffers/CMakeLists.txt to compile GstDmaBufVideoBuffer when present.
# Requires gst-allocators (pulled in via the Allocators QGCGStreamer component
# requested by the Linux-only branch in src/.../GStreamer/CMakeLists.txt).
# NOTE: QGC_GST_HAS_DMABUF is a cached try-compile result; if the GStreamer
# include dirs change, clear the cache var to force a re-probe.
macro(_qgc_detect_dmabuf)
    include(CheckCXXSourceCompiles)
    if(TARGET GStreamer::GStreamer)
        set(_gst_dmabuf_req_libs_backup "${CMAKE_REQUIRED_LIBRARIES}")
        set(CMAKE_REQUIRED_LIBRARIES GStreamer::GStreamer)
        check_cxx_source_compiles("
            #include <gst/allocators/gstdmabuf.h>
            int main() { (void)gst_is_dmabuf_memory; return 0; }
        " QGC_GST_HAS_DMABUF)
        set(CMAKE_REQUIRED_LIBRARIES "${_gst_dmabuf_req_libs_backup}")
    else()
        message(STATUS "GStreamer: DMABuf probe skipped — GStreamer::GStreamer target not defined")
    endif()
endmacro()
