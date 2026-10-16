# HwBuffers: platform zero-copy GPU video buffer paths. One row per path below; _qgc_add_gpu_path probes each row and
# registers its sources/libs/defines on QGCGStreamer (created in the parent CMakeLists). gstqgc shares that target, so
# the defines reach it with no cross-target ferrying. The defines are PUBLIC: the app facade (GStreamer.cc) and the
# GStreamer tests build outside QGCGStreamer and #if on them.

include(GStreamer/Probe)

# Private-Qt-free core (facade, telemetry, CPU pool): compiled unconditionally so a CPU-only build (no GPU path
# qualified) still links the base sources. Qt-private bits stay gated below.
target_sources(
    QGCGStreamer
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common/HwBuffers.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/common/HwBuffers.h
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwPathTelemetry.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwPathTelemetry.h
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwTypes.h
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwVideoInfo.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwVideoInfo.h
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwVideoBufferFactory.h
            ${CMAKE_CURRENT_SOURCE_DIR}/common/CpuVideoFramePool.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/common/CpuVideoFramePool.h
            ${CMAKE_CURRENT_SOURCE_DIR}/dmabuf/GstDmaDrmCaps.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/dmabuf/GstDmaDrmCaps.h
)

# _qgc_add_gpu_path(<name> PLATFORMS <vars>... DEFINE <macro> SOURCES <files>...
#                   [REQUIRES <vars>...] [PROBE <_qgc_probe_gst_lib args>...] [EGL REQUIRED|OPTIONAL] [LIBS <libs>...]
#                   [UNSTABLE_API])
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
        message(FATAL_ERROR "_qgc_add_gpu_path(${name}): PLATFORMS, DEFINE and SOURCES required; "
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
    option(QGC_GST_ENABLE_${_upper} "Build the ${name} zero-copy GPU path when its GStreamer pieces are found" ON)
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
        _qgc_probe_gst_lib(${name} ${ARG_PROBE})
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
    target_sources(QGCGStreamer PRIVATE ${ARG_SOURCES})
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

# Every path needs Qt's private multimedia/RHI headers. They ship with the root's REQUIRED Multimedia/Gui finds, so this
# only trips on a broken Qt install.
if(NOT TARGET Qt6::MultimediaPrivate OR NOT TARGET Qt6::GuiPrivate)
    message(STATUS "QGCGStreamer: Qt MultimediaPrivate/GuiPrivate missing — all zero-copy GPU paths disabled")
    return()
endif()

if(LINUX OR ANDROID)
    find_package(OpenGL QUIET COMPONENTS EGL)
endif()

# gst/cuda/gstcuda.h transitively pulls <cuda.h>: find a CUDA include dir (full Toolkit, or gst-plugins-bad's stub
# headers for compile-only builds without the SDK) for the CUDA row's probe. find_path cache var: a hit survives a CUDA
# toolkit swap until the cache is cleared.
if(LINUX AND QGC_GST_HAS_DMABUF)
    set(_qgc_cuda_hints ${CUDAToolkit_INCLUDE_DIRS})
    if(DEFINED ENV{CUDA_PATH} AND NOT "$ENV{CUDA_PATH}" STREQUAL "")
        list(APPEND _qgc_cuda_hints "$ENV{CUDA_PATH}/include")
    endif()
    list(APPEND _qgc_cuda_hints "/usr/local/cuda/include" ${QGC_GST_CUDA_STUB_DIR})
    find_path(
        QGC_CUDA_INCLUDE_DIR
        NAMES cuda.h
        HINTS ${_qgc_cuda_hints}
    )
endif()

# A static libgstvulkan (Android mobile archive; static-libs desktop) leaves gst-vulkan's own loader calls unresolved;
# the shared .so satisfies them itself. Link the loader explicitly or Android arm64 fails with ld.lld "undefined symbol:
# vkDestroyPipeline". find_library, not find_package(Vulkan): only the loader is needed (NDK sysroot included).
set(_qgc_vulkan_requires QT_FEATURE_vulkan)
set(_qgc_vulkan_loader_libs "")
if(GStreamer_USE_STATIC_LIBS OR ANDROID)
    if(DEFINED QGC_GST_VULKAN_LOADER_LIB AND (QGC_GST_VULKAN_LOADER_LIB MATCHES "NOTFOUND$"
                                              OR NOT EXISTS "${QGC_GST_VULKAN_LOADER_LIB}")
    )
        unset(QGC_GST_VULKAN_LOADER_LIB CACHE)
    endif()
    find_library(QGC_GST_VULKAN_LOADER_LIB NAMES vulkan vulkan-1)
    list(APPEND _qgc_vulkan_requires QGC_GST_VULKAN_LOADER_LIB)
    set(_qgc_vulkan_loader_libs "${QGC_GST_VULKAN_LOADER_LIB}")
endif()

if(MACOS OR IOS)
    # TARGET_DIRECTORY: source-file props are dir-scoped; QGCGStreamer is created in the parent CMakeLists, so name it
    # explicitly here for SKIP_PRECOMPILE_HEADERS to bind to that target.
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/apple/GstIOSurfaceVideoBuffer.mm TARGET_DIRECTORY
                                QGCGStreamer PROPERTIES SKIP_PRECOMPILE_HEADERS ON
    )
endif()

# ---- GPU path table (row order matters: CUDA builds on DMABuf) ----
# cmake-format: off
# Linux DMABuf zero-copy (VA-API / V4L2 / Mesa). QGC_GST_HAS_DMABUF comes from cmake/GStreamer/Orchestrator.cmake.
set(_qgc_gpu_row_DMABuf
    PLATFORMS LINUX
    REQUIRES QGC_GST_HAS_DMABUF
    EGL REQUIRED
    DEFINE QGC_HAS_GST_DMABUF_GPU_PATH
    SOURCES dmabuf/GstDmaBufVideoBuffer.cc dmabuf/GstDmaBufVideoBuffer.h dmabuf/GstDmaFourcc.cc dmabuf/GstDmaFourcc.h
            dmabuf/GstDmaBufVulkanImport.cc dmabuf/GstDmaBufVulkanImport.h)
# NVMM/CUDA: exports CUDA memory as a DMABuf fd and reuses the DMABuf EGLImage path; gst_cuda_memory_export is 1.24+.
set(_qgc_gpu_row_CUDA
    PLATFORMS LINUX
    REQUIRES _qgc_gpu_DMABuf QGC_CUDA_INCLUDE_DIR
    PROBE HAS_VAR QGC_GST_HAS_CUDA_EXPORT HEADER gst/cuda/gstcuda.h SYMBOL gst_cuda_memory_export
          PC_MODULE gstreamer-cuda-1.0 INCLUDES ${QGC_CUDA_INCLUDE_DIR}
    UNSTABLE_API
    DEFINE QGC_HAS_GST_CUDA_GPU_PATH
    SOURCES cuda/GstCudaVideoBuffer.cc cuda/GstCudaVideoBuffer.h)
# GLMemory via gst-gl. WIN32: only reachable when Qt uses the ANGLE/EGL GL backend (D3D11/D3D12 are the primary
# Windows paths); GstGlContextBridge wraps the EGL context only — no WGL. A GLMemory-only Linux/Android build links EGL
# itself, else GstGlContextBridge.cc fails with "undefined symbol: eglGetDisplay".
set(_qgc_gpu_row_GLMemory
    PLATFORMS LINUX MACOS ANDROID WIN32
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
foreach(_ver IN ITEMS 11 12)
    set(_qgc_gpu_row_D3D${_ver}
        PLATFORMS WIN32
        PROBE HAS_VAR QGC_GST_HAS_D3D${_ver} HEADER gst/d3d${_ver}/gstd3d${_ver}.h SYMBOL gst_is_d3d${_ver}_memory
              PC_MODULE gstreamer-d3d${_ver}-1.0 LIBNAMES gstd3d${_ver}-1.0 gstd3d${_ver} LIB_VAR GST_D3D${_ver}_LIB
        LIBS d3d${_ver}
        UNSTABLE_API
        DEFINE QGC_HAS_GST_D3D${_ver}_GPU_PATH
        SOURCES d3d/GstD3D${_ver}VideoBuffer.cc d3d/GstD3D${_ver}ContextBridge.cc d3d/GstD3D${_ver}ContextBridge.h
                d3d/GstD3DVideoBufferCommon.h)
endforeach()
# Android AHardwareBuffer (gst-plugins-bad >= 1.28; GLMemory covers older SDKs).
set(_qgc_gpu_row_AHardwareBuffer
    PLATFORMS ANDROID
    PROBE HAS_VAR QGC_GST_HAS_AHARDWAREBUFFER HEADER gst/android/gstandroid.h SYMBOL gst_is_ahardware_buffer_memory
    EGL REQUIRED
    DEFINE QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH
    SOURCES android/GstAHardwareBufferVideoBuffer.cc android/GstAHardwareBufferVideoBuffer.h)
# gst-vulkan. The impl TUs also gate on QT_CONFIG(vulkan) but callsites gate on the define alone, so the define must
# imply Qt Vulkan support (macOS Qt ships none while GStreamer.framework carries libgstvulkan-1.0). The framework also
# ships the .pc and headers without the library, hence the link-path check.
set(_qgc_gpu_row_Vulkan
    PLATFORMS LINUX WIN32 MACOS ANDROID
    REQUIRES ${_qgc_vulkan_requires}
    PROBE HAS_VAR QGC_GST_HAS_VULKAN HEADER gst/vulkan/vulkan.h SYMBOL gst_is_vulkan_image_memory
          PC_MODULE gstreamer-vulkan-1.0 LIBNAMES gstvulkan-1.0 gstvulkan LIB_VAR QGC_GST_VULKAN_LIB
    LIBS ${_qgc_vulkan_loader_libs}
    DEFINE QGC_HAS_GST_VULKAN_GPU_PATH
    SOURCES vulkan/GstVulkanVideoBuffer.cc vulkan/GstVulkanVideoBuffer.h vulkan/GstVulkanContextBridge.cc
            vulkan/GstVulkanContextBridge.h)
# cmake-format: on

set(_qgc_gpu_rows
    DMABuf
    CUDA
    GLMemory
    IOSurface
    D3D11
    D3D12
    AHardwareBuffer
    Vulkan
)
foreach(_row IN LISTS _qgc_gpu_rows)
    _qgc_add_gpu_path(${_row} ${_qgc_gpu_row_${_row}})
endforeach()

if(_qgc_gpu_unstable_api)
    target_compile_definitions(QGCGStreamer PRIVATE GST_USE_UNSTABLE_API)
endif()

if(_qgc_gpu_D3D11 OR _qgc_gpu_D3D12)
    target_sources(QGCGStreamer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/d3d/GstD3DContextBridge.cc
                                        ${CMAKE_CURRENT_SOURCE_DIR}/d3d/GstD3DContextBridge.h
    )
endif()

# GPU bin variants; HwBuffers::gpuBinMode() picks one at runtime. glupload is Android's only bin (GstGLContext handshake
# amcvideodec needs for Surface output) and Linux's default (gst-gl imports both LINEAR and vendor-tiled dmabuf to
# GLMemory). Direct DMABuf feeds QGC's own importer, which binds LINEAR only. Linux builds with both pick one via
# QGC_GST_GPU_BIN.
set(_qgc_unreachable_paths "")
if((ANDROID OR LINUX) AND _qgc_gpu_GLMemory)
    target_compile_definitions(QGCGStreamer PUBLIC QGC_GST_BIN_HAS_GLUPLOAD=1)
endif()
if(LINUX AND _qgc_gpu_DMABuf)
    target_compile_definitions(QGCGStreamer PUBLIC QGC_GST_BIN_HAS_DIRECT_DMABUF=1)
endif()
if(LINUX
   AND _qgc_gpu_GLMemory
   AND _qgc_gpu_DMABuf
)
    message(STATUS "QGCGStreamer: GPU bins compiled: glupload (default) and direct DMABuf; QGC_GST_GPU_BIN selects one")
elseif((ANDROID OR LINUX) AND _qgc_gpu_GLMemory)
    message(STATUS "QGCGStreamer: GPU bin routes through glupload (gst-gl EGLImage import)")
elseif(LINUX AND _qgc_gpu_DMABuf)
    message(STATUS "QGCGStreamer: GPU bin direct-imports DMABuf (no gst-gl/GLMemory available)")
endif()
# The glupload bin offers only GLMemory caps.
if(ANDROID
   AND _qgc_gpu_GLMemory
   AND _qgc_gpu_AHardwareBuffer
)
    list(APPEND _qgc_unreachable_paths "AHardwareBuffer (glupload bin)")
endif()
# The sink never advertises memory:CUDAMemory or memory:VulkanImage caps.
foreach(_path IN ITEMS CUDA Vulkan)
    if(_qgc_gpu_${_path})
        list(APPEND _qgc_unreachable_paths "${_path} (caps not advertised)")
    endif()
endforeach()
if(_qgc_unreachable_paths)
    list(JOIN _qgc_unreachable_paths ", " _qgc_unreachable_text)
    message(STATUS "QGCGStreamer: compiled but unreachable at runtime: ${_qgc_unreachable_text}")
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
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dmabuf/GstEglHelpers.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/dmabuf/GstEglHelpers.h
            ${CMAKE_CURRENT_SOURCE_DIR}/gl/GstGlFrameTextures.h
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwFrameTexturesBase.h
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwImportCache.h
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwImportPreflight.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwImportPreflight.h
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwVideoBuffer.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwVideoBuffer.h
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstContextBridgeBase.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstContextBridgeBase.h
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwPathRegistry.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwPathRegistry.h
            ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwVideoBufferFactory.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/common/QGCRhiCapture.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/common/QGCRhiCapture.h
)
