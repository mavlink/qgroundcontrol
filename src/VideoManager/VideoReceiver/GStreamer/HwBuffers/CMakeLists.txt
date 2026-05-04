# HwBuffers: platform-specific zero-copy GPU video buffer implementations.
# Each block gates on runtime feature tests and emits its own status message.
# QGC_HAS_ANY_GPU_PATH is set as a compile definition when any path is enabled,
# removing the umbrella macro from GstAppSinkAdapter.h.

include(CheckCXXSourceCompiles)

set(_any_gpu_path FALSE)

# Base class + factory + bridge registry pulled in once any GPU path is enabled below
# (deferred to the _any_gpu_path block at end of file). They depend on Qt6::MultimediaPrivate
# (private/qhwvideobuffer_p.h) which is only linked in that block, so compiling them
# unconditionally would break a hypothetical no-GPU-paths build.

find_package(Qt6 QUIET COMPONENTS MultimediaPrivate GuiPrivate)

# ---- Linux DMABuf zero-copy (VA-API / Mesa) ----
if(LINUX AND QGC_GST_HAS_DMABUF)
    find_package(OpenGL QUIET COMPONENTS EGL)
    if(TARGET Qt6::MultimediaPrivate AND TARGET Qt6::GuiPrivate AND TARGET OpenGL::EGL)
        target_sources(${CMAKE_PROJECT_NAME}
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/GstDmaBufVideoBuffer.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/GstDmaBufVideoBuffer.h
        )
        target_link_libraries(${CMAKE_PROJECT_NAME}
            PRIVATE
                OpenGL::EGL
        )
        target_compile_definitions(${CMAKE_PROJECT_NAME}
            PRIVATE QGC_HAS_GST_DMABUF_GPU_PATH=1
        )
        set(_any_gpu_path TRUE)
        message(STATUS "QGCGStreamer: DMABuf zero-copy GPU path enabled")
    else()
        message(STATUS "QGCGStreamer: DMABuf detected but Qt private modules or EGL missing — GPU path disabled")
    endif()
endif()

# ---- GLMemory zero-copy (Linux + macOS + Android via gst-gl) ----
if(LINUX OR MACOS OR ANDROID)
    set(CMAKE_REQUIRED_LIBRARIES_BACKUP "${CMAKE_REQUIRED_LIBRARIES}")
    # On Android the primary gst target is GStreamerMobile; check_cxx_source_compiles
    # only reads INTERFACE include dirs, so target the right one.
    if(ANDROID AND TARGET GStreamerMobile)
        set(CMAKE_REQUIRED_LIBRARIES GStreamerMobile)
    else()
        set(CMAKE_REQUIRED_LIBRARIES GStreamer::GStreamer)
    endif()
    check_cxx_source_compiles("
        #include <gst/gl/gstglmemory.h>
        int main() { (void)gst_is_gl_memory; return 0; }
    " QGC_GST_HAS_GLMEMORY)
    set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_BACKUP}")
    if(QGC_GST_HAS_GLMEMORY AND TARGET Qt6::MultimediaPrivate AND TARGET Qt6::GuiPrivate)
        target_sources(${CMAKE_PROJECT_NAME}
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/GstGlVideoBuffer.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/GstGlVideoBuffer.h
                ${CMAKE_CURRENT_SOURCE_DIR}/GstGlContextBridge.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/GstGlContextBridge.h
        )
        target_compile_definitions(${CMAKE_PROJECT_NAME}
            PRIVATE QGC_HAS_GST_GLMEMORY_GPU_PATH=1
        )
        set(_any_gpu_path TRUE)
        message(STATUS "QGCGStreamer: GLMemory zero-copy GPU path enabled")
        # Linux bin routing deferred to after the DMABuf block (DMABuf preempts glupload).
    endif()
endif()

# ---- Linux bin GPU routing: DMABuf direct vs glupload fallback ----
# Direct DMABuf when available; runtime modifier probe + CPU fallback live in
# GstDmaBufVideoBuffer. Pre-1.24 va plugins lack modifier metadata — buffer treats as LINEAR
# and CPU fallback catches tiled hardware.
if(LINUX AND QGC_GST_HAS_DMABUF AND TARGET OpenGL::EGL)
    target_compile_definitions(${CMAKE_PROJECT_NAME}
        PRIVATE QGC_GST_BIN_USE_DMABUF=1
    )
    message(STATUS "QGCGStreamer: bin will direct-import DMABuf (no glupload)")
elseif(LINUX AND QGC_GST_HAS_GLMEMORY)
    target_compile_definitions(${CMAKE_PROJECT_NAME}
        PRIVATE QGC_GST_BIN_USE_GLUPLOAD=1
    )
    message(STATUS "QGCGStreamer: bin will route GPU path through glupload (Linux fallback)")
endif()

# ---- Apple IOSurface zero-copy (VideoToolbox vtdec on macOS, AVF on iOS) ----
if(MACOS OR IOS)
    if(TARGET Qt6::MultimediaPrivate AND TARGET Qt6::GuiPrivate)
        target_sources(${CMAKE_PROJECT_NAME}
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/GstIOSurfaceVideoBuffer.mm
                ${CMAKE_CURRENT_SOURCE_DIR}/GstIOSurfaceVideoBuffer.h
        )
        # TARGET_DIRECTORY required: source-file properties are directory-scoped, and the QGC target's PCH is registered in the root CMakeLists.txt — without TARGET_DIRECTORY the SKIP would be visible only inside this subdirectory and not honored when the file is compiled against the target.
        set_source_files_properties(
            ${CMAKE_CURRENT_SOURCE_DIR}/GstIOSurfaceVideoBuffer.mm
            TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
            PROPERTIES SKIP_PRECOMPILE_HEADERS ON
        )
        target_link_libraries(${CMAKE_PROJECT_NAME}
            PRIVATE
                "-framework CoreVideo"
                "-framework IOSurface"
                "-framework Metal"
        )
        target_compile_definitions(${CMAKE_PROJECT_NAME}
            PRIVATE QGC_HAS_GST_IOSURFACE_GPU_PATH=1
        )
        set(_any_gpu_path TRUE)
        if(IOS)
            message(STATUS "QGCGStreamer: IOSurface zero-copy GPU path enabled (iOS)")
        else()
            message(STATUS "QGCGStreamer: IOSurface zero-copy GPU path enabled (macOS)")
        endif()
    else()
        message(STATUS "QGCGStreamer: IOSurface path requires Qt MultimediaPrivate+GuiPrivate — GPU path disabled")
    endif()
endif()

# ---- Windows D3D11 zero-copy (d3d11h264dec / d3d11vp9dec) ----
if(WIN32)
    set(CMAKE_REQUIRED_LIBRARIES_BACKUP "${CMAKE_REQUIRED_LIBRARIES}")
    set(CMAKE_REQUIRED_LIBRARIES GStreamer::GStreamer)
    check_cxx_source_compiles("
        #include <gst/d3d11/gstd3d11.h>
        int main() { (void)gst_is_d3d11_memory; return 0; }
    " QGC_GST_HAS_D3D11)
    set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_BACKUP}")
    if(QGC_GST_HAS_D3D11 AND TARGET Qt6::MultimediaPrivate AND TARGET Qt6::GuiPrivate)
        # The headers compile, but the actual gst_d3d11_* exports live in gstd3d11-1.0.lib (gst-plugins-bad shared helper).
        # Pull it via pkg-config so the resolved import lib is added to the link line — without this the MSVC ARM64
        # link fails with __imp_gst_d3d11_device_new_wrapped et al unresolved.
        find_package(PkgConfig QUIET)
        set(_gst_d3d11_pc_found FALSE)
        if(PkgConfig_FOUND)
            pkg_check_modules(PC_GStreamer_D3D11 QUIET gstreamer-d3d11-1.0)
            if(PC_GStreamer_D3D11_FOUND)
                set(_gst_d3d11_pc_found TRUE)
            endif()
        endif()
        if(NOT _gst_d3d11_pc_found)
            # Fallback: pkg-config may not ship gstreamer-d3d11-1.0.pc on every Windows SDK build,
            # but the import lib is usually present on the link search path.
            find_library(GST_D3D11_LIB NAMES gstd3d11-1.0 gstd3d11)
            if(GST_D3D11_LIB)
                set(PC_GStreamer_D3D11_LIBRARIES "${GST_D3D11_LIB}")
                set(_gst_d3d11_pc_found TRUE)
            endif()
        endif()
        if(NOT _gst_d3d11_pc_found)
            message(STATUS "QGCGStreamer: gstreamer-d3d11-1.0 pkg-config not found and gstd3d11-1.0 not on link path — D3D11 GPU path disabled")
        else()
            target_sources(${CMAKE_PROJECT_NAME}
                PRIVATE
                    ${CMAKE_CURRENT_SOURCE_DIR}/GstD3D11VideoBuffer.cc
                    ${CMAKE_CURRENT_SOURCE_DIR}/GstD3D11VideoBuffer.h
                    ${CMAKE_CURRENT_SOURCE_DIR}/GstD3D11ContextBridge.cc
                    ${CMAKE_CURRENT_SOURCE_DIR}/GstD3D11ContextBridge.h
                    ${CMAKE_CURRENT_SOURCE_DIR}/GstD3DVideoBufferCommon.h
            )
            target_include_directories(${CMAKE_PROJECT_NAME}
                PRIVATE ${PC_GStreamer_D3D11_INCLUDE_DIRS}
            )
            target_link_directories(${CMAKE_PROJECT_NAME}
                PRIVATE ${PC_GStreamer_D3D11_LIBRARY_DIRS}
            )
            target_link_libraries(${CMAKE_PROJECT_NAME}
                PRIVATE
                    d3d11
                    ${PC_GStreamer_D3D11_LIBRARIES}
            )
            target_compile_definitions(${CMAKE_PROJECT_NAME}
                PRIVATE QGC_HAS_GST_D3D11_GPU_PATH=1
            )
            set(_any_gpu_path TRUE)
            message(STATUS "QGCGStreamer: D3D11 zero-copy GPU path enabled (libs=${PC_GStreamer_D3D11_LIBRARIES})")
        endif()
    endif()
endif()

# ---- Windows D3D12 zero-copy (d3d12h264dec / d3d12vp9dec) ----
if(WIN32)
    set(CMAKE_REQUIRED_LIBRARIES_BACKUP "${CMAKE_REQUIRED_LIBRARIES}")
    set(CMAKE_REQUIRED_LIBRARIES GStreamer::GStreamer)
    check_cxx_source_compiles("
        #include <gst/d3d12/gstd3d12.h>
        int main() { (void)gst_is_d3d12_memory; return 0; }
    " QGC_GST_HAS_D3D12)
    set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_BACKUP}")
    if(QGC_GST_HAS_D3D12 AND TARGET Qt6::MultimediaPrivate AND TARGET Qt6::GuiPrivate)
        find_package(PkgConfig QUIET)
        set(_gst_d3d12_pc_found FALSE)
        if(PkgConfig_FOUND)
            pkg_check_modules(PC_GStreamer_D3D12 QUIET gstreamer-d3d12-1.0)
            if(PC_GStreamer_D3D12_FOUND)
                set(_gst_d3d12_pc_found TRUE)
            endif()
        endif()
        if(NOT _gst_d3d12_pc_found)
            find_library(GST_D3D12_LIB NAMES gstd3d12-1.0 gstd3d12)
            if(GST_D3D12_LIB)
                set(PC_GStreamer_D3D12_LIBRARIES "${GST_D3D12_LIB}")
                set(_gst_d3d12_pc_found TRUE)
            endif()
        endif()
        if(NOT _gst_d3d12_pc_found)
            message(STATUS "QGCGStreamer: gstreamer-d3d12-1.0 pkg-config not found and gstd3d12-1.0 not on link path — D3D12 GPU path disabled")
        else()
            target_sources(${CMAKE_PROJECT_NAME}
                PRIVATE
                    ${CMAKE_CURRENT_SOURCE_DIR}/GstD3D12VideoBuffer.cc
                    ${CMAKE_CURRENT_SOURCE_DIR}/GstD3D12VideoBuffer.h
                    ${CMAKE_CURRENT_SOURCE_DIR}/GstD3D12ContextBridge.cc
                    ${CMAKE_CURRENT_SOURCE_DIR}/GstD3D12ContextBridge.h
                    ${CMAKE_CURRENT_SOURCE_DIR}/GstD3DVideoBufferCommon.h
            )
            target_include_directories(${CMAKE_PROJECT_NAME}
                PRIVATE ${PC_GStreamer_D3D12_INCLUDE_DIRS}
            )
            target_link_directories(${CMAKE_PROJECT_NAME}
                PRIVATE ${PC_GStreamer_D3D12_LIBRARY_DIRS}
            )
            target_link_libraries(${CMAKE_PROJECT_NAME}
                PRIVATE
                    d3d12
                    ${PC_GStreamer_D3D12_LIBRARIES}
            )
            target_compile_definitions(${CMAKE_PROJECT_NAME}
                PRIVATE QGC_HAS_GST_D3D12_GPU_PATH=1
            )
            set(_any_gpu_path TRUE)
            message(STATUS "QGCGStreamer: D3D12 zero-copy GPU path enabled (libs=${PC_GStreamer_D3D12_LIBRARIES})")
        endif()
    endif()
endif()

# Shared bridge helpers compiled once when at least one D3D path is on.
if(WIN32 AND (QGC_GST_HAS_D3D11 OR QGC_GST_HAS_D3D12) AND TARGET Qt6::MultimediaPrivate AND TARGET Qt6::GuiPrivate)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/GstD3DContextBridgeCommon.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/GstD3DContextBridgeCommon.h
    )
endif()

# ---- Android AHardwareBuffer zero-copy (gst-plugins-bad >= 1.28) ----
if(ANDROID)
    set(CMAKE_REQUIRED_LIBRARIES_BACKUP "${CMAKE_REQUIRED_LIBRARIES}")
    if(TARGET GStreamerMobile)
        set(CMAKE_REQUIRED_LIBRARIES GStreamerMobile)
    else()
        set(CMAKE_REQUIRED_LIBRARIES GStreamer::GStreamer)
    endif()
    check_cxx_source_compiles("
        #include <gst/android/gstandroid.h>
        int main() { (void)gst_is_ahardware_buffer_memory; return 0; }
    " QGC_GST_HAS_AHARDWAREBUFFER)
    set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_BACKUP}")
    if(NOT QGC_GST_HAS_AHARDWAREBUFFER)
        message(STATUS "QGCGStreamer: AHardwareBuffer header not in SDK (need gst-plugins-bad >= 1.28) — falling back to GLMemory path on Android")
    endif()
    if(QGC_GST_HAS_AHARDWAREBUFFER AND TARGET Qt6::MultimediaPrivate AND TARGET Qt6::GuiPrivate)
        find_package(OpenGL QUIET COMPONENTS EGL)
        if(TARGET OpenGL::EGL)
            target_sources(${CMAKE_PROJECT_NAME}
                PRIVATE
                    ${CMAKE_CURRENT_SOURCE_DIR}/GstAHardwareBufferVideoBuffer.cc
                    ${CMAKE_CURRENT_SOURCE_DIR}/GstAHardwareBufferVideoBuffer.h
            )
            target_link_libraries(${CMAKE_PROJECT_NAME}
                PRIVATE
                    OpenGL::EGL
            )
            target_compile_definitions(${CMAKE_PROJECT_NAME}
                PRIVATE QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH=1
            )
            set(_any_gpu_path TRUE)
            message(STATUS "QGCGStreamer: AHardwareBuffer zero-copy GPU path enabled")
        else()
            message(STATUS "QGCGStreamer: AHardwareBuffer detected but EGL missing — GPU path disabled")
        endif()
    endif()
endif()

# QGC_HAS_ANY_GPU_PATH: set when at least one platform zero-copy path is enabled.
if(_any_gpu_path)
    target_compile_definitions(${CMAKE_PROJECT_NAME}
        PRIVATE QGC_HAS_ANY_GPU_PATH=1
    )
    if(TARGET Qt6::MultimediaPrivate AND TARGET Qt6::GuiPrivate)
        target_link_libraries(${CMAKE_PROJECT_NAME}
            PRIVATE
                Qt6::MultimediaPrivate
                Qt6::GuiPrivate
        )
        # Base class + factory + bridge registry + RHI capture: all depend on Qt6::MultimediaPrivate
        # (private/qhwvideobuffer_p.h, qrhi.h). Co-located with the link to keep the no-GPU build green.
        target_sources(${CMAKE_PROJECT_NAME}
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/GstHwVideoBuffer.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/GstHwVideoBuffer.h
                ${CMAKE_CURRENT_SOURCE_DIR}/GstContextBridgeRegistry.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/GstContextBridgeRegistry.h
                ${CMAKE_CURRENT_SOURCE_DIR}/GstHwVideoBufferFactory.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/GstHwVideoBufferFactory.h
                ${CMAKE_CURRENT_SOURCE_DIR}/QGCRhiCapture.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/QGCRhiCapture.h
        )
    endif()
endif()
