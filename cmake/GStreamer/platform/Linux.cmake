# Linux GStreamer SDK discovery — invoked by Orchestrator.cmake.

macro(_qgc_discover_linux_sdk)
    if(NOT DEFINED GStreamer_ROOT_DIR)
        set(GStreamer_ROOT_DIR "/usr")
    endif()

    # Probe multi-arch and legacy lib paths; LIB_PATH overrides the default SDK_ROOT/lib.
    if((EXISTS "${GStreamer_ROOT_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu") AND (EXISTS "${GStreamer_ROOT_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu/gstreamer-1.0"))
        set(_gst_linux_lib "${GStreamer_ROOT_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu")
    elseif((EXISTS "${GStreamer_ROOT_DIR}/lib64") AND (EXISTS "${GStreamer_ROOT_DIR}/lib64/gstreamer-1.0"))
        set(_gst_linux_lib "${GStreamer_ROOT_DIR}/lib64")
    elseif((EXISTS "${GStreamer_ROOT_DIR}/lib") AND (EXISTS "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0"))
        set(_gst_linux_lib "${GStreamer_ROOT_DIR}/lib")
    else()
        message(FATAL_ERROR "Could not locate GStreamer libraries - check installation or set environment/cmake variables")
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
    endif()
endmacro()
