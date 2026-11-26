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
#                   [REQUIRES <vars>...] [PROBE <qgc_gst_probe_library args>...] [EGL REQUIRED|OPTIONAL]
#                   [LIBS <libs>...] [UNSTABLE_API])
# One row of the GPU path table. On a matching platform with QGC_GST_ENABLE_<NAME> on, every REQUIRES variable truthy
# and the probe found, adds the sources and libraries to QGCGStreamer and defines <macro>=1. Sets _qgc_gpu_<name> and
# _qgc_any_gpu_path TRUE in the caller's scope; UNSTABLE_API rows set _qgc_gpu_unstable_api.
function(_qgc_add_gpu_path name)
    # A row's whole decision (platform, switch, requirements, probe, EGL) and its registration, in one scope.
    # cmake-lint: disable=R0912,R0915
    cmake_parse_arguments(PARSE_ARGV 1 ARG "UNSTABLE_API" "DEFINE;EGL" "PLATFORMS;REQUIRES;PROBE;SOURCES;LIBS")
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

    string(TOUPPER "${name}" _upper)
    set(_default ON)
    if(name IN_LIST _qgc_gpu_opt_in_rows)
        set(_default OFF)
    endif()
    option(
        QGC_GST_ENABLE_${_upper} "Build the ${name} zero-copy GPU path when its GStreamer pieces are found" ${_default}
    )
    set(_reason "")
    if(NOT QGC_GST_ENABLE_${_upper})
        set(_reason "QGC_GST_ENABLE_${_upper}=OFF")
    endif()
    foreach(_var IN LISTS ARG_REQUIRES)
        if(_reason OR ${_var})
            continue()
        elseif(_var MATCHES "^_qgc_gpu_(.+)$")
            set(_reason "needs the ${CMAKE_MATCH_1} path")
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
    target_include_directories(QGCGStreamer PRIVATE ${${name}_INCLUDE_DIRS})
    target_link_directories(QGCGStreamer PRIVATE ${${name}_LIBRARY_DIRS})
    set(_libs ${${name}_LIBRARIES} ${ARG_LIBS})
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
# requires libdrm and so names its include directory, one built without the DRM allocator does not.
set(_qgc_gpu_row_DMABuf
    PLATFORMS LINUX
    REQUIRES QGC_GST_HAS_DMABUF QT_FEATURE_linux_dmabuf
    PROBE HAS_VAR QGC_GST_HAS_DRM_FOURCC HEADER drm_fourcc.h SYMBOL DRM_FORMAT_MOD_LINEAR PC_MODULE libdrm
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
# Windows D3D11/D3D12 (gst-plugins-bad d3d{11,12}h264dec / vp9dec).
set(_qgc_d3d11_extra_sources d3d/GstD3D11PictureCopy.cc d3d/GstD3D11PictureCopy.h)
foreach(_ver IN ITEMS 11 12)
    set(_qgc_gpu_row_D3D${_ver}
        PLATFORMS WIN32
        PROBE HAS_VAR QGC_GST_HAS_D3D${_ver} HEADER gst/d3d${_ver}/gstd3d${_ver}.h SYMBOL gst_is_d3d${_ver}_memory
              PC_MODULE gstreamer-d3d${_ver}-1.0 LIBNAMES gstd3d${_ver}-1.0 gstd3d${_ver} LIB_VAR GST_D3D${_ver}_LIB
        LIBS d3d${_ver}
        UNSTABLE_API
        DEFINE QGC_HAS_GST_D3D${_ver}_GPU_PATH
        SOURCES d3d/GstD3D${_ver}VideoBuffer.cc d3d/GstD3D${_ver}ContextBridge.cc d3d/GstD3D${_ver}ContextBridge.h
                d3d/GstD3DVideoBufferCommon.h ${_qgc_d3d${_ver}_extra_sources})
endforeach()
# Android AHardwareBuffer memory. The probe finds its header in no GStreamer SDK: gst/android/gstandroid.h is in no
# release, and the AHardwareBuffer memory of GStreamer 1.30 (gst/allocators/gstahardwarebuffer.h) is another API, so
# the row compiles only against an SDK that adds this one. GLMemory is Android's path.
set(_qgc_gpu_row_AHardwareBuffer
    PLATFORMS ANDROID
    PROBE HAS_VAR QGC_GST_HAS_AHARDWAREBUFFER HEADER gst/android/gstandroid.h SYMBOL gst_is_ahardware_buffer_memory
    EGL REQUIRED
    DEFINE QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH
    SOURCES android/GstAHardwareBufferVideoBuffer.cc android/GstAHardwareBufferVideoBuffer.h)
# cmake-format: on

# No bin offers this memory, so it is compiled only on request.
set(_qgc_gpu_opt_in_rows AHardwareBuffer)
# In this order: DMABuf_Vulkan requires the DMABuf path (_qgc_gpu_DMABuf).
set(_qgc_gpu_rows
    DMABuf
    DMABuf_Vulkan
    GLMemory
    IOSurface
    D3D11
    D3D12
    AHardwareBuffer
)
foreach(_row IN LISTS _qgc_gpu_rows)
    _qgc_add_gpu_path(${_row} ${_qgc_gpu_row_${_row}})
endforeach()

if(_qgc_gpu_unstable_api)
    target_compile_definitions(QGCGStreamer PRIVATE GST_USE_UNSTABLE_API)
endif()

if(_qgc_gpu_D3D11 OR _qgc_gpu_D3D12)
    target_sources(
        QGCGStreamer
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/d3d/GstD3DContextBridge.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/d3d/GstD3DContextBridge.h
    )
endif()

# GPU bin variants; GstQgc::selectGpuBin() picks one at runtime. glupload is Android's only bin (GstGLContext handshake
# amcvideodec needs for Surface output); gst-gl imports both LINEAR and vendor-tiled dmabuf to GLMemory. Direct DMABuf
# feeds Qt's per-plane importer, with the plain tilings EGL reports. Linux builds with both route each stream by the
# memory its decoder negotiates (DMABuf direct, the rest through glupload) unless QGC_GST_GPU_BIN forces one.
if(_qgc_gpu_GLMemory)
    target_compile_definitions(QGCGStreamer PUBLIC QGC_GST_BIN_HAS_GLUPLOAD=1)
endif()
if(LINUX AND _qgc_gpu_DMABuf)
    target_compile_definitions(QGCGStreamer PUBLIC QGC_GST_BIN_HAS_DIRECT_DMABUF=1)
endif()
if(LINUX
   AND _qgc_gpu_GLMemory
   AND _qgc_gpu_DMABuf
)
    message(
        STATUS
            "QGCGStreamer: GPU bins compiled: glupload and direct DMABuf, picked per stream; QGC_GST_GPU_BIN forces one"
    )
elseif(_qgc_gpu_GLMemory)
    message(STATUS "QGCGStreamer: GPU bin routes through glupload (gst-gl EGLImage import)")
elseif(LINUX AND _qgc_gpu_DMABuf)
    message(STATUS "QGCGStreamer: GPU bin direct-imports DMABuf (no gst-gl/GLMemory available)")
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
