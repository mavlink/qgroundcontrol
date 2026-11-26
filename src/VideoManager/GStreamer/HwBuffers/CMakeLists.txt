# HwBuffers: platform zero-copy GPU video buffer paths. One row per path below; _qgc_add_gpu_path probes each row and
# registers its sources/libs/defines on QGCGStreamer (created in the parent CMakeLists). gstqgc shares that target, so
# the defines reach it with no cross-target ferrying. The defines are PUBLIC: the app facade (GStreamer.cc) and the
# GStreamer tests build outside QGCGStreamer and #if on them.

include(GStreamer/Probe)

# The paths' folders are this library's own. Everything else includes the facade by its path, HwBuffers/HwBuffers.h,
# and the tests name the folders they look into themselves.
target_include_directories(
    QGCGStreamer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_CURRENT_SOURCE_DIR}/dmabuf
        ${CMAKE_CURRENT_SOURCE_DIR}/gl
        ${CMAKE_CURRENT_SOURCE_DIR}/d3d
        ${CMAKE_CURRENT_SOURCE_DIR}/apple
        ${CMAKE_CURRENT_SOURCE_DIR}/android
)

# Private-Qt-free core (facade, telemetry, CPU frame mapping): compiled unconditionally so a CPU-only build (no GPU
# path qualified) still links the base sources. Qt-private bits stay gated below.
target_sources(
    QGCGStreamer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/HwBuffers.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/HwBuffers.h
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwFrameMapping.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwFrameMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwPathTelemetry.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwPathTelemetry.h
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwTypes.h
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwVideoInfo.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwVideoInfo.h
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwPathRegistry.h
)

# _qgc_add_gpu_path(<name> PLATFORMS <vars>... DEFINE <macro> SOURCES <files>...
#                   [REQUIRES <vars>...] [PROBE <qgc_gst_probe_library args>...] [PROBE_ALSO <args>...]
#                   [EGL REQUIRED|OPTIONAL] [LIBS <libs>...] [UNSTABLE_API])
# One row of the GPU path table. On a matching platform with QGC_GST_ENABLE_GPU_PATHS on, every REQUIRES variable truthy
# and the probed libraries found (PROBE_ALSO is a second one the path needs), adds the sources and libraries to
# QGCGStreamer and defines <macro>=1. Sets _qgc_gpu_<name> and
# _qgc_any_gpu_path TRUE in the caller's scope; UNSTABLE_API rows set _qgc_gpu_unstable_api.
function(_qgc_add_gpu_path name)
    # A row's whole decision (platform, switch, requirements, probe, EGL) and its registration, in one scope.
    # cmake-lint: disable=R0912,R0915
    cmake_parse_arguments(
        PARSE_ARGV 1 ARG "UNSTABLE_API" "DEFINE;EGL" "PLATFORMS;REQUIRES;PROBE;PROBE_ALSO;SOURCES;LIBS"
    )
    if(ARG_UNPARSED_ARGUMENTS
       OR NOT ARG_PLATFORMS
       OR NOT ARG_DEFINE
       OR NOT ARG_SOURCES
    )
        message(
            FATAL_ERROR "_qgc_add_gpu_path(${name}): PLATFORMS, DEFINE and SOURCES required; "
                        "unknown: ${ARG_UNPARSED_ARGUMENTS}"
        )
    endif()
    set(_platform_match FALSE)
    foreach(_platform IN LISTS ARG_PLATFORMS)
        if(${_platform})
            set(_platform_match TRUE)
        endif()
    endforeach()
    if(NOT _platform_match)
        return()
    endif()

    set(_reason "")
    if(NOT QGC_GST_ENABLE_GPU_PATHS)
        set(_reason "QGC_GST_ENABLE_GPU_PATHS=OFF")
    endif()
    foreach(_var IN LISTS ARG_REQUIRES)
        if(_reason OR ${_var})
            continue()
        elseif(_var MATCHES "^_qgc_gpu_(.+)$")
            set(_reason "needs the ${CMAKE_MATCH_1} path")
        elseif(_var MATCHES "^QGC_GST_ENABLE_")
            set(_reason "${_var}=OFF")
        else()
            set(_reason "${_var} unavailable")
        endif()
    endforeach()
    if(NOT _reason AND ARG_PROBE)
        qgc_gst_probe_library(${name} ${ARG_PROBE})
        if(NOT ${name}_FOUND)
            set(_reason "${${name}_REASON}")
        endif()
    endif()
    if(NOT _reason AND ARG_PROBE_ALSO)
        qgc_gst_probe_library(${name}_also ${ARG_PROBE_ALSO})
        if(NOT ${name}_also_FOUND)
            set(_reason "${${name}_also_REASON}")
        endif()
    endif()
    if(NOT _reason
       AND ARG_EGL STREQUAL "REQUIRED"
       AND NOT TARGET OpenGL::EGL
    )
        set(_reason "EGL not found")
    endif()
    if(_reason)
        message(STATUS "QGCGStreamer: ${name} GPU path disabled (${_reason})")
        return()
    endif()

    list(TRANSFORM ARG_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
    target_sources(
        QGCGStreamer
        PRIVATE
            ${ARG_SOURCES}
    )
    target_include_directories(
        QGCGStreamer
        PRIVATE
            ${${name}_INCLUDE_DIRS}
            ${${name}_also_INCLUDE_DIRS}
    )
    target_link_directories(
        QGCGStreamer
        PRIVATE
            ${${name}_LIBRARY_DIRS}
            ${${name}_also_LIBRARY_DIRS}
    )
    set(_libs ${${name}_LIBRARIES} ${${name}_also_LIBRARIES} ${ARG_LIBS})
    if(ARG_EGL
       AND TARGET OpenGL::EGL
       AND (LINUX OR ANDROID)
    )
        list(APPEND _libs OpenGL::EGL)
    endif()
    if(_libs)
        target_link_libraries(QGCGStreamer PRIVATE ${_libs})
    endif()
    target_compile_definitions(QGCGStreamer PUBLIC ${ARG_DEFINE}=1)
    message(STATUS "QGCGStreamer: ${name} zero-copy GPU path enabled")
    # cmake-lint: disable=C0103
    set(_qgc_gpu_${name}
        TRUE
        PARENT_SCOPE
    )
    set(_qgc_any_gpu_path
        TRUE
        PARENT_SCOPE
    )
    if(ARG_UNSTABLE_API)
        set(_qgc_gpu_unstable_api
            TRUE
            PARENT_SCOPE
        )
    endif()
endfunction()

if(LINUX OR ANDROID)
    find_package(OpenGL QUIET COMPONENTS EGL)
endif()

# The DMABuf import under Vulkan calls Vulkan through Qt and only needs the headers Qt found for its own Vulkan support.
if(TARGET WrapVulkanHeaders::WrapVulkanHeaders)
    set(QGC_VULKAN_HEADERS TRUE)
endif()

if(MACOS OR IOS)
    # TARGET_DIRECTORY: source-file props are dir-scoped; QGCGStreamer is created in the parent CMakeLists, so name it
    # explicitly here for SKIP_PRECOMPILE_HEADERS to bind to that target.
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/apple/GstIOSurfaceVideoBuffer.mm
        TARGET_DIRECTORY
        QGCGStreamer
        PROPERTIES
            SKIP_PRECOMPILE_HEADERS ON
    )
endif()

# cmake-format: off
# Linux DMABuf zero-copy (VA-API / V4L2 / Mesa) through Qt Multimedia's DMABuf importer. QGC_GST_HAS_DMABUF comes from
# cmake/GStreamer/Orchestrator.cmake. The sources read libdrm's drm_fourcc.h: a distribution's gstreamer-allocators-1.0
# requires libdrm and so names its include directory, one built without the DRM allocator does not. They read DMA_DRM
# caps too, which GStreamer has from 1.24: an older one shows its DMABuf frames through glupload. Built with the
# GLMemory path only, which takes every stream this one does not.
set(_qgc_gpu_row_DMABuf
    PLATFORMS LINUX
    REQUIRES _qgc_gpu_GLMemory QGC_GST_HAS_DMABUF QT_FEATURE_linux_dmabuf
    PROBE HAS_VAR QGC_GST_HAS_DRM_FOURCC HEADER drm_fourcc.h SYMBOL DRM_FORMAT_MOD_LINEAR PC_MODULE libdrm
    PROBE_ALSO HAS_VAR QGC_GST_HAS_DMA_DRM HEADER gst/video/video-info-dma.h SYMBOL gst_video_info_dma_drm_from_caps
    EGL REQUIRED
    DEFINE QGC_HAS_GST_DMABUF_GPU_PATH
    SOURCES dmabuf/GstDmaBufVideoBuffer.cc dmabuf/GstDmaBufVideoBuffer.h dmabuf/GstDmaDrmCaps.cc
            dmabuf/GstDmaDrmCaps.h dmabuf/GstQtDmaBufImport.cc dmabuf/GstQtDmaBufImport.h)
# The DMABuf path under a Vulkan QRhi (QSG_RHI_BACKEND=vulkan): each plane imported as a VkImage on Qt's device. Needs
# Qt's Vulkan support, not gst-vulkan.
set(_qgc_gpu_row_DMABuf_Vulkan
    PLATFORMS LINUX
    REQUIRES _qgc_gpu_DMABuf QT_FEATURE_vulkan QGC_VULKAN_HEADERS
    DEFINE QGC_HAS_GST_DMABUF_VULKAN_IMPORT
    SOURCES dmabuf/GstDmaBufVulkanImport.cc dmabuf/GstDmaBufVulkanImport.h)
# GLMemory via gst-gl. A GLMemory-only Linux/Android build links EGL itself, else GstGlContextBridge.cc fails with
# "undefined symbol: eglGetDisplay". Not Windows: Qt 6's OpenGL there is WGL, which GstGlContextBridge can't wrap, so
# the sink never offers GLMemory. Not Apple: vtdec prefers GLMemory, which Metal can't import, over the IOSurface-backed
# CoreVideo memory the IOSurface path takes.
set(_qgc_gpu_row_GLMemory
    PLATFORMS LINUX ANDROID
    PROBE HAS_VAR QGC_GST_HAS_GLMEMORY HEADER gst/gl/gstglmemory.h SYMBOL gst_is_gl_memory
    EGL OPTIONAL
    DEFINE QGC_HAS_GST_GLMEMORY_GPU_PATH
    SOURCES gl/GstGlVideoBuffer.cc gl/GstGlVideoBuffer.h gl/GstGlContextBridge.cc gl/GstGlContextBridge.h)
# Apple IOSurface (VideoToolbox vtdec on macOS, AVF on iOS).
set(_qgc_gpu_row_IOSurface
    PLATFORMS MACOS IOS
    LIBS "-framework CoreVideo" "-framework IOSurface" "-framework Metal"
    DEFINE QGC_HAS_GST_IOSURFACE_GPU_PATH
    SOURCES apple/GstIOSurfaceVideoBuffer.mm apple/GstIOSurfaceVideoBuffer.h)
# Windows Direct3D (gst-plugins-bad d3d{11,12}h264dec / vp9dec), one path of both: the SDK has the two libraries, and a
# Direct3D 12 decoder's frames reach a Direct3D 11 QRhi through the Direct3D 11 code.
set(_qgc_gpu_row_D3D
    PLATFORMS WIN32
    PROBE HAS_VAR QGC_GST_HAS_D3D11 HEADER gst/d3d11/gstd3d11.h SYMBOL gst_is_d3d11_memory
          PC_MODULE gstreamer-d3d11-1.0 LIBNAMES gstd3d11-1.0 gstd3d11 LIB_VAR GST_D3D11_LIB
    PROBE_ALSO HAS_VAR QGC_GST_HAS_D3D12 HEADER gst/d3d12/gstd3d12.h SYMBOL gst_is_d3d12_memory
               PC_MODULE gstreamer-d3d12-1.0 LIBNAMES gstd3d12-1.0 gstd3d12 LIB_VAR GST_D3D12_LIB
    LIBS d3d11 d3d12
    UNSTABLE_API
    DEFINE QGC_HAS_GST_D3D_GPU_PATH
    SOURCES d3d/GstD3D11VideoBuffer.cc d3d/GstD3D11ContextBridge.cc d3d/GstD3D11ContextBridge.h
            d3d/GstD3D11PictureCopy.cc d3d/GstD3D11PictureCopy.h d3d/GstD3D12VideoBuffer.cc
            d3d/GstD3D12ContextBridge.cc d3d/GstD3D12ContextBridge.h d3d/GstD3DContextBridge.cc
            d3d/GstD3DContextBridge.h d3d/GstD3DVideoBufferCommon.h)
# Android AHardwareBuffer memory. The probe finds its header in no GStreamer SDK: gst/android/gstandroid.h is in no
# release, and the AHardwareBuffer memory of GStreamer 1.30 (gst/allocators/gstahardwarebuffer.h) is another API, so
# the row compiles only against an SDK that adds this one. GLMemory is Android's path.
set(_qgc_gpu_row_AHardwareBuffer
    PLATFORMS ANDROID
    REQUIRES QGC_GST_ENABLE_AHARDWAREBUFFER
    PROBE HAS_VAR QGC_GST_HAS_AHARDWAREBUFFER HEADER gst/android/gstandroid.h SYMBOL gst_is_ahardware_buffer_memory
    EGL REQUIRED
    DEFINE QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH
    SOURCES android/GstAHardwareBufferVideoBuffer.cc android/GstAHardwareBufferVideoBuffer.h)
# cmake-format: on

option(QGC_GST_ENABLE_GPU_PATHS "Build the zero-copy GPU video paths whose GStreamer pieces are found" ON)
# No bin offers this memory, so it is compiled only on request.
if(ANDROID)
    option(QGC_GST_ENABLE_AHARDWAREBUFFER "Build the AHardwareBuffer GPU path, which no sink bin reaches" OFF)
endif()
# In this order: DMABuf requires the GLMemory path (_qgc_gpu_GLMemory), and DMABuf_Vulkan the DMABuf path.
set(_qgc_gpu_rows GLMemory DMABuf DMABuf_Vulkan IOSurface D3D AHardwareBuffer)
foreach(_row IN LISTS _qgc_gpu_rows)
    _qgc_add_gpu_path(${_row} ${_qgc_gpu_row_${_row}})
endforeach()

if(_qgc_gpu_unstable_api)
    target_compile_definitions(QGCGStreamer PRIVATE GST_USE_UNSTABLE_API)
endif()

# GPU bin variants; GstQgc::selectGpuBin() picks one at runtime. glupload is Android's only bin (GstGLContext handshake
# amcvideodec needs for Surface output); gst-gl imports both LINEAR and vendor-tiled dmabuf to GLMemory. Direct DMABuf
# feeds Qt's per-plane importer, with the plain tilings EGL reports. Linux builds with both route each stream by the
# memory its decoder negotiates (DMABuf direct, the rest through glupload) unless QGC_GST_GPU_BIN forces one.
if(_qgc_gpu_DMABuf)
    message(
        STATUS
            "QGCGStreamer: GPU bins compiled: glupload and direct DMABuf, picked per stream; QGC_GST_GPU_BIN forces one"
    )
elseif(_qgc_gpu_GLMemory)
    message(STATUS "QGCGStreamer: GPU bin routes through glupload (gst-gl EGLImage import)")
endif()
# The glupload bin offers only GLMemory caps.
if(ANDROID
   AND _qgc_gpu_GLMemory
   AND _qgc_gpu_AHardwareBuffer
)
    message(STATUS "QGCGStreamer: compiled but unreachable at runtime: AHardwareBuffer (glupload bin)")
endif()

if(NOT _qgc_any_gpu_path)
    return()
endif()

target_compile_definitions(QGCGStreamer PUBLIC QGC_HAS_ANY_GPU_PATH=1)
# Qt6::MultimediaPrivate is already PUBLIC on QGCGStreamer (the facade reaches it through the HwBuffers headers it
# includes); only Qt6::GuiPrivate (qrhi.h) is GPU-path-exclusive here.
target_link_libraries(QGCGStreamer PRIVATE Qt6::GuiPrivate)
# GPU-only TUs (base class, factory, bridge registry, RHI capture) depend on qrhi.h.
target_sources(
    QGCGStreamer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstEglHelpers.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstEglHelpers.h
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwImportPreflight.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwImportPreflight.h
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwVideoBuffer.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwVideoBuffer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstContextBridgeBase.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstContextBridgeBase.h
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwPathRegistry.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/QGCRhiCapture.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/QGCRhiCapture.h
)
