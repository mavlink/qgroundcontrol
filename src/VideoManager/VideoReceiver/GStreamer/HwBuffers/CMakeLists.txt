# HwBuffers: platform zero-copy GPU video buffer paths. Each block gates on runtime feature tests;
# _qgc_register_gpu_path centralizes sources/libs/defines/logging onto QGCGStreamer (created in the
# parent CMakeLists) — gstqgc shares that target, so defs reach it with no cross-target ferrying.

include(GStreamer/Probe)

# Tracked as a GLOBAL property (not a scoped var) so _qgc_register_gpu_path can flip it from any
# call depth without a PARENT_SCOPE relay — registrars in helper functions stay correct.
set_property(GLOBAL PROPERTY QGC_GST_ANY_GPU_PATH FALSE)

# Private-Qt-free core (facade, telemetry, CPU pool): compiled unconditionally so a CPU-only
# build (no GPU path qualified) still links the base sources. Qt-private bits stay gated below.
target_sources(QGCGStreamer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/common/HwBuffers.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/HwBuffers.h
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwPathTelemetry.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwPathTelemetry.h
        ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwVideoBufferFactory.h
        ${CMAKE_CURRENT_SOURCE_DIR}/common/CpuVideoFramePool.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/CpuVideoFramePool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/dmabuf/GstDmaDrmCaps.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/dmabuf/GstDmaDrmCaps.h
)

# Base class + factory + bridge registry depend on Qt6::MultimediaPrivate and are added in the
# _any_gpu_path block at end of file, so a hypothetical no-GPU-paths build stays green.

# Qt6::MultimediaPrivate / Qt6::GuiPrivate ship with the root's REQUIRED Multimedia /
# Gui finds, so no re-query is needed; the flag just gates the private-Qt branches below.
if(TARGET Qt6::MultimediaPrivate AND TARGET Qt6::GuiPrivate)
    set(_qgc_qt_private TRUE)
else()
    set(_qgc_qt_private FALSE)
endif()

# _qgc_register_gpu_path(NAME <n> SOURCES <files> [LIBS <libs>] DEFINE <macro> LOG <text>)
# Adds sources/libs to QGCGStreamer, sets DEFINE=1, sets the QGC_GST_ANY_GPU_PATH global, logs status.
function(_qgc_register_gpu_path)
    cmake_parse_arguments(ARG "" "NAME;DEFINE;LOG" "SOURCES;LIBS" ${ARGN})
    foreach(_req IN ITEMS NAME DEFINE LOG SOURCES)
        if(NOT ARG_${_req})
            message(FATAL_ERROR "_qgc_register_gpu_path: ${_req} is required")
        endif()
    endforeach()
    target_sources(QGCGStreamer PRIVATE ${ARG_SOURCES})
    if(ARG_LIBS)
        target_link_libraries(QGCGStreamer PRIVATE ${ARG_LIBS})
    endif()
    # PUBLIC: these flags gate HwVideoBufferContext fields/EGL includes in GstHwVideoBufferFactory.h,
    # which the app facade also includes — PRIVATE gave it a mismatched struct layout (dropped EGLDisplay).
    target_compile_definitions(QGCGStreamer PUBLIC ${ARG_DEFINE}=1)
    set_property(GLOBAL PROPERTY QGC_GST_ANY_GPU_PATH TRUE)
    message(STATUS "QGCGStreamer: ${ARG_LOG}")
endfunction()

# ---- Linux DMABuf zero-copy (VA-API / Mesa) ----
if(LINUX AND QGC_GST_HAS_DMABUF)
    find_package(OpenGL QUIET COMPONENTS EGL)
    if(_qgc_qt_private AND TARGET OpenGL::EGL)
        _qgc_register_gpu_path(NAME DMABuf
            SOURCES
                ${CMAKE_CURRENT_SOURCE_DIR}/dmabuf/GstDmaBufVideoBuffer.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/dmabuf/GstDmaBufVideoBuffer.h
                ${CMAKE_CURRENT_SOURCE_DIR}/dmabuf/GstDmaFourcc.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/dmabuf/GstDmaFourcc.h
                ${CMAKE_CURRENT_SOURCE_DIR}/dmabuf/GstDmaBufVulkanImport.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/dmabuf/GstDmaBufVulkanImport.h
                ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwImportCache.h
            LIBS    OpenGL::EGL
            DEFINE  QGC_HAS_GST_DMABUF_GPU_PATH
            LOG     "DMABuf zero-copy GPU path enabled")
        set(_qgc_dmabuf_path_enabled TRUE)
    else()
        message(STATUS "QGCGStreamer: DMABuf detected but Qt private modules or EGL missing — GPU path disabled")
    endif()
endif()

# ---- NVMM/CUDA zero-copy (NVIDIA/Jetson) ----
# Exports CUDA memory to a DMABuf fd and reuses the DMABuf EGLImage path; needs the DMABuf path (export target)
# plus gst-plugins-bad CUDA. Runtime needs NVIDIA HW + exportable (cuMemCreate/MMAP) memory.
if(LINUX AND _qgc_dmabuf_path_enabled)
    # gst/cuda/gstcuda.h transitively pulls <cuda.h>; resolve a CUDA include dir (full Toolkit, or gst-plugins-bad's
    # stub headers for compile-only builds without the SDK) before probing, else the header test fails to compile.
    set(_qgc_cuda_inc "")
    # find_path cache var: a hit survives a CUDA toolkit swap until the cache is cleared.
    find_path(QGC_CUDA_INCLUDE_DIR NAMES cuda.h
        HINTS ${CUDAToolkit_INCLUDE_DIRS} $ENV{CUDA_PATH}/include /usr/local/cuda/include ${QGC_GST_CUDA_STUB_DIR})
    if(QGC_CUDA_INCLUDE_DIR)
        set(_qgc_cuda_inc "${QGC_CUDA_INCLUDE_DIR}")
    endif()
    set(_gst_cuda_found FALSE)
    if(_qgc_cuda_inc AND PkgConfig_FOUND)
        pkg_check_modules(PC_GStreamer_Cuda QUIET gstreamer-cuda-1.0)
        if(PC_GStreamer_Cuda_FOUND)
            qgc_check_gst_header(VAR QGC_GST_HAS_CUDA HEADER gst/cuda/gstcuda.h SYMBOL gst_is_cuda_memory
                INCLUDES ${_qgc_cuda_inc} ${PC_GStreamer_Cuda_INCLUDE_DIRS})
            if(QGC_GST_HAS_CUDA)
                set(_gst_cuda_found TRUE)
            endif()
        endif()
    endif()
    if(_gst_cuda_found)
        target_include_directories(QGCGStreamer PRIVATE ${_qgc_cuda_inc} ${PC_GStreamer_Cuda_INCLUDE_DIRS})
        target_link_directories(QGCGStreamer PRIVATE ${PC_GStreamer_Cuda_LIBRARY_DIRS})
        target_compile_definitions(QGCGStreamer PRIVATE GST_USE_UNSTABLE_API)  # gst-bad CUDA is unstable-API by design
        _qgc_register_gpu_path(NAME CUDA
            SOURCES
                ${CMAKE_CURRENT_SOURCE_DIR}/cuda/GstCudaVideoBuffer.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/cuda/GstCudaVideoBuffer.h
            LIBS   ${PC_GStreamer_Cuda_LIBRARIES}
            DEFINE QGC_HAS_GST_CUDA_GPU_PATH
            LOG    "CUDA/NVMM zero-copy GPU path enabled (DMABuf export)")
    else()
        message(STATUS "QGCGStreamer: gst-cuda (gst/cuda/gstcuda.h + cuda.h + gstreamer-cuda-1.0) not found — CUDA GPU path disabled")
    endif()
endif()

# ---- GLMemory zero-copy (Linux + macOS + Android, plus Windows ANGLE/EGL fallback via gst-gl) ----
# WIN32: only reachable when Qt uses the ANGLE/EGL GL backend (RHI defaults to D3D11/D3D12, which stay the primary
# Windows paths above); GstGlContextBridge wraps the EGL context only — no WGL. Review-only, no Windows compiler here.
if(LINUX OR MACOS OR ANDROID OR WIN32)
    qgc_check_gst_header(VAR QGC_GST_HAS_GLMEMORY HEADER gst/gl/gstglmemory.h SYMBOL gst_is_gl_memory)
    if(QGC_GST_HAS_GLMEMORY AND _qgc_qt_private)
        # A GLMemory-only build (DMABuf path disabled) must link EGL itself or GstGlContextBridge.cc fails with
        # "undefined symbol: eglGetDisplay" — the DMABuf path is what otherwise pulls in OpenGL::EGL.
        set(_qgc_glmemory_libs "")
        if(LINUX OR ANDROID)
            find_package(OpenGL QUIET COMPONENTS EGL)
            if(TARGET OpenGL::EGL)
                set(_qgc_glmemory_libs OpenGL::EGL)
            endif()
        endif()
        _qgc_register_gpu_path(NAME GLMemory
            SOURCES
                ${CMAKE_CURRENT_SOURCE_DIR}/gl/GstGlVideoBuffer.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/gl/GstGlVideoBuffer.h
                ${CMAKE_CURRENT_SOURCE_DIR}/gl/GstGlContextBridge.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/gl/GstGlContextBridge.h
            LIBS   ${_qgc_glmemory_libs}
            DEFINE QGC_HAS_GST_GLMEMORY_GPU_PATH
            LOG    "GLMemory zero-copy GPU path enabled")
        set(_qgc_glmemory_path_enabled TRUE)
    endif()
endif()

# glupload heads the bin on Android (GstGLContext handshake amcvideodec needs for Surface output) and Linux (gst-gl
# imports both LINEAR and Intel-tiled dmabuf to GLMemory; QGC's direct importer binds LINEAR only). Direct DMABuf is the
# fallback. PUBLIC so the app facade and test TU #if-select the same bin structure across the link boundary.
if((ANDROID OR LINUX) AND _qgc_glmemory_path_enabled)
    target_compile_definitions(QGCGStreamer
        PUBLIC QGC_GST_BIN_USE_GLUPLOAD=1
    )
    message(STATUS "QGCGStreamer: bin will route GPU path through glupload (gst-gl EGLImage import)")
elseif(_qgc_dmabuf_path_enabled)
    target_compile_definitions(QGCGStreamer
        PUBLIC QGC_GST_BIN_USE_DMABUF=1
    )
    message(STATUS "QGCGStreamer: bin will direct-import DMABuf (no gst-gl/GLMemory available)")
endif()

# ---- Apple IOSurface zero-copy (VideoToolbox vtdec on macOS, AVF on iOS) ----
if(MACOS OR IOS)
    if(_qgc_qt_private)
        # TARGET_DIRECTORY: source-file props are dir-scoped; QGCGStreamer is created in the parent
        # CMakeLists, so name it explicitly here for SKIP_PRECOMPILE_HEADERS to bind to that target.
        set_source_files_properties(
            ${CMAKE_CURRENT_SOURCE_DIR}/apple/GstIOSurfaceVideoBuffer.mm
            TARGET_DIRECTORY QGCGStreamer
            PROPERTIES SKIP_PRECOMPILE_HEADERS ON
        )
        if(IOS)
            set(_iosurface_log "IOSurface zero-copy GPU path enabled (iOS)")
        else()
            set(_iosurface_log "IOSurface zero-copy GPU path enabled (macOS)")
        endif()
        _qgc_register_gpu_path(NAME IOSurface
            SOURCES
                ${CMAKE_CURRENT_SOURCE_DIR}/apple/GstIOSurfaceVideoBuffer.mm
                ${CMAKE_CURRENT_SOURCE_DIR}/apple/GstIOSurfaceVideoBuffer.h
            LIBS
                "-framework CoreVideo"
                "-framework IOSurface"
                "-framework Metal"
            DEFINE QGC_HAS_GST_IOSURFACE_GPU_PATH
            LOG    "${_iosurface_log}")
    else()
        message(STATUS "QGCGStreamer: IOSurface path requires Qt MultimediaPrivate+GuiPrivate — GPU path disabled")
    endif()
endif()

# ---- Windows D3D11 / D3D12 zero-copy (gst-plugins-bad d3d{11,12}h264dec / vp9dec) ----
# `_qgc_probe_gst_d3d_path` (cmake/GStreamer/Probe.cmake) does header+lib resolution; sources are
# registered here so their d3d/ paths resolve against this directory's CMAKE_CURRENT_SOURCE_DIR.
if(WIN32)
    # PkgConfig is already found upstream via Orchestrator -> PkgConfigTargets/FindGStreamer.
    foreach(_ver IN ITEMS 11 12)
        _qgc_probe_gst_d3d_path(${_ver} _d3d${_ver})
        if(_d3d${_ver}_FOUND)
            target_include_directories(QGCGStreamer PRIVATE ${_d3d${_ver}_INCLUDE_DIRS})
            target_link_directories(QGCGStreamer PRIVATE ${_d3d${_ver}_LIBRARY_DIRS})
            target_compile_definitions(QGCGStreamer PRIVATE GST_USE_UNSTABLE_API)
            _qgc_register_gpu_path(NAME D3D${_ver}
                SOURCES
                    ${CMAKE_CURRENT_SOURCE_DIR}/d3d/GstD3D${_ver}VideoBuffer.cc
                    ${CMAKE_CURRENT_SOURCE_DIR}/d3d/GstD3D${_ver}VideoBuffer.h
                    ${CMAKE_CURRENT_SOURCE_DIR}/d3d/GstD3D${_ver}ContextBridge.cc
                    ${CMAKE_CURRENT_SOURCE_DIR}/d3d/GstD3D${_ver}ContextBridge.h
                    ${CMAKE_CURRENT_SOURCE_DIR}/d3d/GstD3DVideoBufferCommon.h
                LIBS    ${_d3d${_ver}_LIBS}
                DEFINE  QGC_HAS_GST_D3D${_ver}_GPU_PATH
                LOG     "D3D${_ver} zero-copy GPU path enabled (libs=${_d3d${_ver}_PC_LIBRARIES})")
        endif()
    endforeach()
endif()

# Shared bridge helpers compiled once when at least one D3D path is on.
if(WIN32 AND (QGC_GST_HAS_D3D11 OR QGC_GST_HAS_D3D12) AND _qgc_qt_private)
    target_sources(QGCGStreamer
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/d3d/GstD3DContextBridgeCommon.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/d3d/GstD3DContextBridgeCommon.h
    )
endif()

# ---- Android AHardwareBuffer zero-copy (gst-plugins-bad >= 1.28) ----
if(ANDROID)
    qgc_check_gst_header(VAR QGC_GST_HAS_AHARDWAREBUFFER
        HEADER gst/android/gstandroid.h SYMBOL gst_is_ahardware_buffer_memory)
    if(NOT QGC_GST_HAS_AHARDWAREBUFFER)
        message(STATUS "QGCGStreamer: AHardwareBuffer header not in SDK (need gst-plugins-bad >= 1.28) — falling back to GLMemory path on Android")
    endif()
    if(QGC_GST_HAS_AHARDWAREBUFFER AND _qgc_qt_private)
        find_package(OpenGL QUIET COMPONENTS EGL)
        if(TARGET OpenGL::EGL)
            _qgc_register_gpu_path(NAME AHardwareBuffer
                SOURCES
                    ${CMAKE_CURRENT_SOURCE_DIR}/android/GstAHardwareBufferVideoBuffer.cc
                    ${CMAKE_CURRENT_SOURCE_DIR}/android/GstAHardwareBufferVideoBuffer.h
                LIBS    OpenGL::EGL
                DEFINE  QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH
                LOG     "AHardwareBuffer zero-copy GPU path enabled")
        else()
            message(STATUS "QGCGStreamer: AHardwareBuffer detected but EGL missing — GPU path disabled")
        endif()
    endif()
endif()

# ---- Vulkan zero-copy (gst-vulkan) — forward-looking, all desktop/mobile platforms ----
# Runtime-dormant: the per-frame device-match guard sends foreign-VkDevice frames to CPU until same-VkDevice
# sharing lands.
if(LINUX OR WIN32 OR MACOS OR ANDROID)
    qgc_check_gst_header(VAR QGC_GST_HAS_VULKAN HEADER gst/vulkan/vulkan.h SYMBOL gst_is_vulkan_image_memory)
    # The impl TUs gate their bodies on `... && QT_CONFIG(vulkan)` (they need qrhi Vulkan + vulkan.h),
    # but headers/callsites gate on the define alone — so the define must imply Qt Vulkan support, else
    # callsites reference symbols the impls #if'd out. macOS Qt ships no Vulkan yet GStreamer.framework
    # carries libgstvulkan-1.0, which is exactly the case that linked broken.
    if(NOT QT_FEATURE_vulkan)
        message(STATUS "QGCGStreamer: Qt built without Vulkan support (QT_FEATURE_vulkan off) — Vulkan GPU path disabled")
    elseif(QGC_GST_HAS_VULKAN AND _qgc_qt_private)
        # PkgConfig already found upstream via Orchestrator (see WIN32 block above).
        set(_gst_vulkan_found FALSE)
        if(PkgConfig_FOUND)
            pkg_check_modules(PC_GStreamer_Vulkan QUIET gstreamer-vulkan-1.0)
            if(PC_GStreamer_Vulkan_FOUND)
                # macOS GStreamer.framework ships the .pc + headers but not libgstvulkan-1.0; verify the
                # library actually resolves on the link path (as the D3D probe does) before enabling, or
                # the build dies at link with "ld: library 'gstvulkan-1.0' not found".
                if(DEFINED QGC_GST_VULKAN_LIB AND
                   (QGC_GST_VULKAN_LIB MATCHES "NOTFOUND$" OR NOT EXISTS "${QGC_GST_VULKAN_LIB}"))
                    unset(QGC_GST_VULKAN_LIB CACHE)
                endif()
                find_library(QGC_GST_VULKAN_LIB NAMES gstvulkan-1.0 gstvulkan
                    HINTS ${PC_GStreamer_Vulkan_LIBRARY_DIRS} "${GSTREAMER_LIB_PATH}")
                if(QGC_GST_VULKAN_LIB)
                    # Link the absolute resolved path (as the D3D probe does), not the bare
                    # pkg-config name: on macOS the framework's gstreamer-vulkan-1.0.pc libdir
                    # doesn't match where libgstvulkan-1.0 actually lives, so find_library locates
                    # it via GSTREAMER_LIB_PATH while `-lgstvulkan-1.0` with the .pc's -L fails at
                    # link ("ld: library 'gstvulkan-1.0' not found").
                    set(PC_GStreamer_Vulkan_LIBRARIES "${QGC_GST_VULKAN_LIB}")
                    set(_gst_vulkan_found TRUE)
                else()
                    message(STATUS "QGCGStreamer: gstreamer-vulkan-1.0 pkg-config found but libgstvulkan-1.0 not on link path — Vulkan GPU path disabled")
                endif()
            endif()
        endif()
        # A static libgstvulkan (Android mobile archive; static-libs desktop) drags gst-vulkan's own
        # loader calls into our link unresolved — the shared .so self-satisfies them, the .a does not.
        # Link the loader explicitly or Android arm64 dies: ld.lld "undefined symbol: vkDestroyPipeline".
        set(_qgc_vulkan_loader_libs "")
        if(_gst_vulkan_found AND (GStreamer_USE_STATIC_LIBS OR ANDROID))
            # find_library, not find_package(Vulkan): we need only the loader, not SDK headers/glslc,
            # and it resolves libvulkan in the NDK sysroot. NAMES: vulkan (Android/Linux), vulkan-1 (MSVC).
            if(DEFINED QGC_GST_VULKAN_LOADER_LIB AND
               (QGC_GST_VULKAN_LOADER_LIB MATCHES "NOTFOUND$" OR NOT EXISTS "${QGC_GST_VULKAN_LOADER_LIB}"))
                unset(QGC_GST_VULKAN_LOADER_LIB CACHE)
            endif()
            find_library(QGC_GST_VULKAN_LOADER_LIB NAMES vulkan vulkan-1)
            if(QGC_GST_VULKAN_LOADER_LIB)
                set(_qgc_vulkan_loader_libs "${QGC_GST_VULKAN_LOADER_LIB}")
            else()
                set(_gst_vulkan_found FALSE)
                set(_qgc_vulkan_loader_missing TRUE)
                message(STATUS "QGCGStreamer: gst-vulkan found but the Vulkan loader (libvulkan) is not on the link path for a static link — Vulkan GPU path disabled")
            endif()
        endif()
        if(_gst_vulkan_found)
            target_include_directories(QGCGStreamer PRIVATE ${PC_GStreamer_Vulkan_INCLUDE_DIRS})
            target_link_directories(QGCGStreamer PRIVATE ${PC_GStreamer_Vulkan_LIBRARY_DIRS})
            _qgc_register_gpu_path(NAME Vulkan
                SOURCES
                    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/GstVulkanVideoBuffer.cc
                    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/GstVulkanVideoBuffer.h
                    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/GstVulkanContextBridge.cc
                    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/GstVulkanContextBridge.h
                LIBS   ${PC_GStreamer_Vulkan_LIBRARIES} ${_qgc_vulkan_loader_libs}
                DEFINE QGC_HAS_GST_VULKAN_GPU_PATH
                LOG    "Vulkan zero-copy GPU path enabled")
        elseif(NOT _qgc_vulkan_loader_missing)
            message(STATUS "QGCGStreamer: gstreamer-vulkan-1.0 pkg-config not found — Vulkan GPU path disabled")
        endif()
    endif()
endif()

# QGC_HAS_ANY_GPU_PATH: set when at least one platform zero-copy path is enabled.
get_property(_any_gpu_path GLOBAL PROPERTY QGC_GST_ANY_GPU_PATH)
if(_any_gpu_path)
    # PUBLIC: the app facade (GStreamer.cc) gates gpu-zerocopy bin construction on this macro but
    # compiles into the app target — PRIVATE hid it, so createVideoSink() always built the CPU-only bin.
    target_compile_definitions(QGCGStreamer
        PUBLIC QGC_HAS_ANY_GPU_PATH=1
    )
    if(_qgc_qt_private)
        # Qt6::MultimediaPrivate is already PUBLIC on QGCGStreamer (the facade reaches it through the
        # HwBuffers headers it includes); only Qt6::GuiPrivate (qrhi.h) is GPU-path-exclusive here.
        target_link_libraries(QGCGStreamer PRIVATE Qt6::GuiPrivate)
        # GPU-only TUs (base class, factory, bridge registry, RHI capture) depend on qrhi.h; co-located
        # with the link so the no-GPU build stays green.
        target_sources(QGCGStreamer
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/dmabuf/GstEglHelpers.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/dmabuf/GstEglHelpers.h
                ${CMAKE_CURRENT_SOURCE_DIR}/gl/GstGlFrameTextures.h
                ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwFrameTexturesBase.h
                ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwImportPreflight.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwImportPreflight.h
                ${CMAKE_CURRENT_SOURCE_DIR}/common/GstBridgePrimeRetry.h
                ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/GstVulkanFrameTextures.h
                ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwVideoBuffer.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwVideoBuffer.h
                ${CMAKE_CURRENT_SOURCE_DIR}/common/GstContextBridgeCommon.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/common/GstContextBridgeCommon.h
                ${CMAKE_CURRENT_SOURCE_DIR}/common/GstContextBridgeRegistry.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/common/GstContextBridgeRegistry.h
                ${CMAKE_CURRENT_SOURCE_DIR}/common/GstHwVideoBufferFactory.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/common/QGCRhiCapture.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/common/QGCRhiCapture.h
        )
    endif()
endif()
