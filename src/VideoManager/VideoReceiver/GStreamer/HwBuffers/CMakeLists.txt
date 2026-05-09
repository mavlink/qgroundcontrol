# HwBuffers: platform-specific zero-copy GPU video buffer implementations.
# Each block gates on runtime feature tests and emits its own status message.
# Registration is table-driven via _qgc_register_gpu_path (defined below) which
# centralizes target_sources / target_link_libraries / target_compile_definitions
# / status logging / _any_gpu_path bookkeeping so new paths only specify what's
# unique (sources, libs, defines). QGC_HAS_ANY_GPU_PATH is emitted when any
# path is enabled.

include(GStreamer/Probe)

set(_any_gpu_path FALSE)

# Base class + factory + bridge registry pulled in once any GPU path is enabled below
# (deferred to the _any_gpu_path block at end of file). They depend on Qt6::MultimediaPrivate
# (private/qhwvideobuffer_p.h) which is only linked in that block, so compiling them
# unconditionally would break a hypothetical no-GPU-paths build.

find_package(Qt6 QUIET COMPONENTS MultimediaPrivate GuiPrivate)

# _qgc_register_gpu_path(NAME <name> SOURCES <files> [LIBS <libs>] DEFINE <macro> LOG <text>)
# Adds sources to the QGC target, links optional libs, sets DEFINE as a compile
# definition, flips _any_gpu_path TRUE in the parent scope, and emits a status
# message. NAME is informational (used only in error messages); the registered
# macro is DEFINE. LIBS may include Qt frameworks via "-framework Foo" syntax.
function(_qgc_register_gpu_path)
    cmake_parse_arguments(ARG "" "NAME;DEFINE;LOG" "SOURCES;LIBS" ${ARGN})
    foreach(_req IN ITEMS NAME DEFINE LOG SOURCES)
        if(NOT ARG_${_req})
            message(FATAL_ERROR "_qgc_register_gpu_path: ${_req} is required")
        endif()
    endforeach()
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${ARG_SOURCES})
    if(ARG_LIBS)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${ARG_LIBS})
    endif()
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ${ARG_DEFINE}=1)
    set(_any_gpu_path TRUE PARENT_SCOPE)
    message(STATUS "QGCGStreamer: ${ARG_LOG}")
endfunction()

# ---- Linux DMABuf zero-copy (VA-API / Mesa) ----
if(LINUX AND QGC_GST_HAS_DMABUF)
    find_package(OpenGL QUIET COMPONENTS EGL)
    if(TARGET Qt6::MultimediaPrivate AND TARGET Qt6::GuiPrivate AND TARGET OpenGL::EGL)
        _qgc_register_gpu_path(NAME DMABuf
            SOURCES
                ${CMAKE_CURRENT_SOURCE_DIR}/GstDmaBufVideoBuffer.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/GstDmaBufVideoBuffer.h
            LIBS    OpenGL::EGL
            DEFINE  QGC_HAS_GST_DMABUF_GPU_PATH
            LOG     "DMABuf zero-copy GPU path enabled")
    else()
        message(STATUS "QGCGStreamer: DMABuf detected but Qt private modules or EGL missing — GPU path disabled")
    endif()
endif()

# ---- GLMemory zero-copy (Linux + macOS + Android via gst-gl) ----
if(LINUX OR MACOS OR ANDROID)
    qgc_check_gst_header(VAR QGC_GST_HAS_GLMEMORY HEADER gst/gl/gstglmemory.h SYMBOL gst_is_gl_memory)
    if(QGC_GST_HAS_GLMEMORY AND TARGET Qt6::MultimediaPrivate AND TARGET Qt6::GuiPrivate)
        _qgc_register_gpu_path(NAME GLMemory
            SOURCES
                ${CMAKE_CURRENT_SOURCE_DIR}/GstGlVideoBuffer.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/GstGlVideoBuffer.h
                ${CMAKE_CURRENT_SOURCE_DIR}/GstGlContextBridge.cc
                ${CMAKE_CURRENT_SOURCE_DIR}/GstGlContextBridge.h
            DEFINE QGC_HAS_GST_GLMEMORY_GPU_PATH
            LOG    "GLMemory zero-copy GPU path enabled")
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
        # TARGET_DIRECTORY: source-file props are dir-scoped; QGC target's PCH is registered in root CMakeLists.txt — without it, SKIP isn't honored when the file compiles against the target.
        set_source_files_properties(
            ${CMAKE_CURRENT_SOURCE_DIR}/GstIOSurfaceVideoBuffer.mm
            TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
            PROPERTIES SKIP_PRECOMPILE_HEADERS ON
        )
        if(IOS)
            set(_iosurface_log "IOSurface zero-copy GPU path enabled (iOS)")
        else()
            set(_iosurface_log "IOSurface zero-copy GPU path enabled (macOS)")
        endif()
        _qgc_register_gpu_path(NAME IOSurface
            SOURCES
                ${CMAKE_CURRENT_SOURCE_DIR}/GstIOSurfaceVideoBuffer.mm
                ${CMAKE_CURRENT_SOURCE_DIR}/GstIOSurfaceVideoBuffer.h
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
# gst_is_d3d{N}_memory exports live in gstd3d{N}-1.0.lib (gst-plugins-bad shared helper);
# resolve via pkg-config, fall back to find_library on SDKs that don't ship the .pc.
function(_qgc_try_gst_d3d_path VERSION)
    set(_pc_var "PC_GStreamer_D3D${VERSION}")
    qgc_check_gst_header(
        VAR    QGC_GST_HAS_D3D${VERSION}
        HEADER gst/d3d${VERSION}/gstd3d${VERSION}.h
        SYMBOL gst_is_d3d${VERSION}_memory)
    set(QGC_GST_HAS_D3D${VERSION} "${QGC_GST_HAS_D3D${VERSION}}" PARENT_SCOPE)
    if(NOT QGC_GST_HAS_D3D${VERSION} OR NOT TARGET Qt6::MultimediaPrivate OR NOT TARGET Qt6::GuiPrivate)
        return()
    endif()

    set(_pc_found FALSE)
    if(PkgConfig_FOUND)
        pkg_check_modules(${_pc_var} QUIET gstreamer-d3d${VERSION}-1.0)
        if(${_pc_var}_FOUND)
            set(_pc_found TRUE)
        endif()
    endif()
    if(NOT _pc_found)
        find_library(GST_D3D${VERSION}_LIB NAMES gstd3d${VERSION}-1.0 gstd3d${VERSION})
        if(GST_D3D${VERSION}_LIB)
            set(${_pc_var}_LIBRARIES "${GST_D3D${VERSION}_LIB}")
            set(_pc_found TRUE)
        endif()
    endif()
    if(NOT _pc_found)
        message(STATUS "QGCGStreamer: gstreamer-d3d${VERSION}-1.0 pkg-config not found and gstd3d${VERSION}-1.0 not on link path - D3D${VERSION} GPU path disabled")
        return()
    endif()

    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${${_pc_var}_INCLUDE_DIRS})
    target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${${_pc_var}_LIBRARY_DIRS})
    _qgc_register_gpu_path(NAME D3D${VERSION}
        SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/GstD3D${VERSION}VideoBuffer.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/GstD3D${VERSION}VideoBuffer.h
            ${CMAKE_CURRENT_SOURCE_DIR}/GstD3D${VERSION}ContextBridge.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/GstD3D${VERSION}ContextBridge.h
            ${CMAKE_CURRENT_SOURCE_DIR}/GstD3DVideoBufferCommon.h
        LIBS
            d3d${VERSION}
            ${${_pc_var}_LIBRARIES}
        DEFINE QGC_HAS_GST_D3D${VERSION}_GPU_PATH
        LOG    "D3D${VERSION} zero-copy GPU path enabled (libs=${${_pc_var}_LIBRARIES})")
    set(_any_gpu_path TRUE PARENT_SCOPE)
endfunction()

if(WIN32)
    find_package(PkgConfig QUIET)
    _qgc_try_gst_d3d_path(11)
    _qgc_try_gst_d3d_path(12)
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
    qgc_check_gst_header(VAR QGC_GST_HAS_AHARDWAREBUFFER
        HEADER gst/android/gstandroid.h SYMBOL gst_is_ahardware_buffer_memory)
    if(NOT QGC_GST_HAS_AHARDWAREBUFFER)
        message(STATUS "QGCGStreamer: AHardwareBuffer header not in SDK (need gst-plugins-bad >= 1.28) — falling back to GLMemory path on Android")
    endif()
    if(QGC_GST_HAS_AHARDWAREBUFFER AND TARGET Qt6::MultimediaPrivate AND TARGET Qt6::GuiPrivate)
        find_package(OpenGL QUIET COMPONENTS EGL)
        if(TARGET OpenGL::EGL)
            _qgc_register_gpu_path(NAME AHardwareBuffer
                SOURCES
                    ${CMAKE_CURRENT_SOURCE_DIR}/GstAHardwareBufferVideoBuffer.cc
                    ${CMAKE_CURRENT_SOURCE_DIR}/GstAHardwareBufferVideoBuffer.h
                LIBS    OpenGL::EGL
                DEFINE  QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH
                LOG     "AHardwareBuffer zero-copy GPU path enabled")
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
