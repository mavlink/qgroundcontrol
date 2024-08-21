# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# FindGStreamer
# ---------
#
# Locate the gstreamer-1.0 library and some of its plugins.
# Defines the following imported target:
#
#   ``GStreamer::GStreamer``
#       If the gstreamer-1.0 library is available and target GStreamer::Base,
#       GStreamer::Audio, GStreamer::Video, GStreamer::Pbutils and
#       GStreamer::Allocators exist
#
#  If target GStreamer::GStreamer exists, the following targets may be defined:
#
#   ``GStreamer::App``
#       If the gstapp-1.0 library is available and target GStreamer::GStreamer exists
#   ``GStreamer::Photography``
#       If the gstphotography-1.0 library is available and target GStreamer::GStreamer exists
#   ``GStreamer::Gl``
#       If the gstgl-1.0 library is available and target GStreamer::GStreamer exists
#

################################################################################

if(ANDROID)
    set(QGC_GST_STATIC_BUILD ON)
    list(APPEND PKG_CONFIG_ARGN --static)
endif()

if(ANDROID OR IOS)
    if(DEFINED ENV{GST_VERSION})
        set(QGC_GST_TARGET_VERSION $ENV{GST_VERSION} CACHE STRING "Environment Provided GStreamer Version")
    else()
        set(QGC_GST_TARGET_VERSION 1.22.12 CACHE STRING "Requested GStreamer Version")
    endif()
endif()

################################################################################

# NOTE: CMP0144 in regards to GSTREAMER_ROOT
set(GSTREAMER_PREFIX)
if(WIN32)
    if(DEFINED ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64} AND EXISTS $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64})
        set(GSTREAMER_PREFIX $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64})
    elseif(DEFINED ENV{GSTREAMER_PREFIX_X86_64} AND EXISTS $ENV{GSTREAMER_PREFIX_X86_64})
        set(GSTREAMER_PREFIX $ENV{GSTREAMER_PREFIX_X86_64})
    else()
        set(GSTREAMER_PREFIX "C:/gstreamer/1.0/msvc_x86_64")
    endif()
    set(PKG_CONFIG_EXECUTABLE ${GSTREAMER_PREFIX}/bin/pkg-config.exe)
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_PREFIX}/lib/pkgconfig;${GSTREAMER_PREFIX}/lib/gstreamer-1.0/pkgconfig;$ENV{PKG_CONFIG_PATH}")
    # cmake_path(CONVERT "${GSTREAMER_PREFIX}/lib/pkgconfig;${GSTREAMER_PREFIX}/lib/gstreamer-1.0/pkgconfig;$ENV{PKG_CONFIG_PATH}" TO_NATIVE_PATH_LIST PKG_CONFIG_PATH NORMALIZE)
    # cmake_print_variables(PKG_CONFIG_PATH)
    # set(ENV{PKG_CONFIG_PATH} "${PKG_CONFIG_PATH}")
    cmake_path(CONVERT "${GSTREAMER_PREFIX}" TO_CMAKE_PATH_LIST PREFIX_PATH NORMALIZE)
    cmake_path(CONVERT "${GSTREAMER_PREFIX}/lib" TO_CMAKE_PATH_LIST LIBDIR_PATH NORMALIZE)
    cmake_path(CONVERT "${GSTREAMER_PREFIX}/include" TO_CMAKE_PATH_LIST INCLUDE_PATH NORMALIZE)
    list(APPEND PKG_CONFIG_ARGN
        --dont-define-prefix
        --define-variable=prefix=${PREFIX_PATH}
        --define-variable=libdir=${LIBDIR_PATH}
        --define-variable=includedir=${INCLUDE_PATH}
    )
    cmake_print_variables(PKG_CONFIG_ARGN)
elseif(MACOS)
    set(GSTREAMER_PREFIX "/Library/Frameworks/GStreamer.framework")
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_PREFIX}/Versions/Current/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
elseif(LINUX)
    set(GSTREAMER_PREFIX "/usr")
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_PREFIX}/lib/pkgconfig:${GSTREAMER_PREFIX}/lib/x86_64-linux-gnu/pkgconfig:$ENV{PKG_CONFIG_PATH}")
elseif(IOS)
    set(GSTREAMER_PREFIX "~/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
    if(DEFINED ENV{GSTREAMER_PREFIX_IOS} AND EXISTS $ENV{GSTREAMER_PREFIX_IOS})
        set(GSTREAMER_PREFIX_IOS $ENV{GSTREAMER_PREFIX_IOS})
    else()
        FetchContent_Declare(gstreamer
            URL "https://gstreamer.freedesktop.org/data/pkg/ios/${QGC_GST_TARGET_VERSION}/gstreamer-1.0-devel-${QGC_GST_TARGET_VERSION}-ios-universal.pkg"
            DOWNLOAD_EXTRACT_TIMESTAMP true
        )
        FetchContent_MakeAvailable(gstreamer)
        set(GSTREAMER_PREFIX_IOS ${gstreamer_SOURCE_DIR})
    endif()
    # TODO: set(GSTREAMER_PREFIX ${GSTREAMER_PREFIX_IOS}/)
elseif(ANDROID)
    set(GSTREAMER_PREFIX_ANDROID)
    if(DEFINED ENV{GSTREAMER_PREFIX_ANDROID} AND EXISTS $ENV{GSTREAMER_PREFIX_ANDROID})
        set(GSTREAMER_PREFIX_ANDROID $ENV{GSTREAMER_PREFIX_ANDROID})
    else()
        FetchContent_Declare(gstreamer
            URL "https://gstreamer.freedesktop.org/data/pkg/android/${QGC_GST_TARGET_VERSION}/gstreamer-1.0-android-universal-${QGC_GST_TARGET_VERSION}.tar.xz"
            DOWNLOAD_EXTRACT_TIMESTAMP true
        )
        FetchContent_MakeAvailable(gstreamer)
        set(GSTREAMER_PREFIX_ANDROID ${gstreamer_SOURCE_DIR})
    endif()
    if(${CMAKE_ANDROID_ARCH_ABI} STREQUAL armeabi-v7a)
        set(GSTREAMER_PREFIX ${GSTREAMER_PREFIX_ANDROID}/armv7)
    elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL arm64-v8a)
        set(GSTREAMER_PREFIX ${GSTREAMER_PREFIX_ANDROID}/arm64)
    elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL x86)
        set(GSTREAMER_PREFIX ${GSTREAMER_PREFIX_ANDROID}/x86)
    elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL x86_64)
        set(GSTREAMER_PREFIX ${GSTREAMER_PREFIX_ANDROID}/x86_64)
    endif()
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_PREFIX}/lib/pkgconfig:${GSTREAMER_PREFIX}/lib/gstreamer-1.0/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_PREFIX}/lib/pkgconfig:${GSTREAMER_PREFIX}/lib/gstreamer-1.0/pkgconfig")
    # set(ENV{PKG_CONFIG_SYSROOT_DIR} "${GSTREAMER_PREFIX}")
    message(STATUS "PKG_CONFIG_PATH $ENV{PKG_CONFIG_PATH}")
    message(STATUS "PKG_CONFIG_LIBDIR $ENV{PKG_CONFIG_LIBDIR}")
    # message(STATUS "PKG_CONFIG_SYSROOT_DIR $ENV{PKG_CONFIG_SYSROOT_DIR}")
    list(APPEND PKG_CONFIG_ARGN
        --dont-define-prefix
        --define-variable=prefix=${GSTREAMER_PREFIX}
        --define-variable=libdir=${GSTREAMER_PREFIX}/lib
        --define-variable=includedir=${GSTREAMER_PREFIX}/include
    )
    cmake_print_variables(PKG_CONFIG_ARGN)
endif()
cmake_print_variables(GSTREAMER_PREFIX)
list(PREPEND CMAKE_PREFIX_PATH ${GSTREAMER_PREFIX})

################################################################################

include(CMakeFindDependencyMacro)
find_dependency(GObject)

set(GStreamer_VERSION ${QGC_GST_TARGET_VERSION})
find_package(PkgConfig QUIET)
if(NOT PkgConfig_FOUND)
    find_file(GStreamer_VERSION_HEADER
        NAMES gst/gstversion.h
        PATHS ${GSTREAMER_PREFIX}/include/gstreamer-1.0
    )
    if(GStreamer_VERSION_HEADER)
        file(READ "${GStreamer_VERSION_HEADER}" _gstversion_header_contents)
        string(REGEX MATCH
            "GST_VERSION_MAJOR \\([0-9]+"
            _gst_major_version_line
            "${_gstversion_header_contents}"
        )
        string(SUBSTRING "${_gst_major_version_line}" 19 -1 GStreamer_VERSION_MAJOR)
        string(REGEX MATCH
            "GST_VERSION_MINOR \\([0-9]+"
            _gst_minor_version_line
            "${_gstversion_header_contents}"
        )
        string(SUBSTRING "${_gst_minor_version_line}" 19 -1 GStreamer_VERSION_MINOR)
        string(REGEX MATCH
            "GST_VERSION_MICRO \\([0-9]+"
            _gst_micro_version_line
            "${_gstversion_header_contents}"
        )
        string(SUBSTRING "${_gst_micro_version_line}" 19 -1 GStreamer_VERSION_PATCH)
        unset(_gstversion_header_contents)
        set(GStreamer_VERSION "${GStreamer_VERSION_MAJOR}.${GStreamer_VERSION_MINOR}.${GStreamer_VERSION_PATCH}")
        cmake_print_variables(GStreamer_VERSION)
    endif()
endif()

function(find_gstreamer_component component prefix header library)
    if(NOT TARGET GStreamer::${component})
        string(TOUPPER ${component} upper)
        # if(ANDROID)
            # pkg_check_modules(PC_GSTREAMER_${upper} ${prefix} IMPORTED_TARGET NO_CMAKE_ENVIRONMENT_PATH)
        # else()
            pkg_check_modules(PC_GSTREAMER_${upper} ${prefix} IMPORTED_TARGET)
        # endif()
        if(TARGET PkgConfig::PC_GSTREAMER_${upper})
            add_library(GStreamer::${component} INTERFACE IMPORTED)
            target_link_libraries(GStreamer::${component} INTERFACE PkgConfig::PC_GSTREAMER_${upper})
            set_target_properties(GStreamer::${component} PROPERTIES VERSION ${PC_GSTREAMER_${upper}_VERSION})
        else()
            find_path(GStreamer_${component}_INCLUDE_DIR
                NAMES ${header}
                PATH_SUFFIXES gstreamer-1.0
                PATHS ${GSTREAMER_PREFIX}/include
            )
            find_library(GStreamer_${component}_LIBRARY
                NAMES ${library}
                PATHS ${GSTREAMER_PREFIX}/lib
            )
            if(${component} STREQUAL "Gl")
                # search the gstglconfig.h include dir under the same root where the library is found
                # TODO: replace with cmake_path
                get_filename_component(gstglLibDir "${GStreamer_Gl_LIBRARY}" PATH)
                find_path(GStreamer_GlConfig_INCLUDE_DIR
                    NAMES gst/gl/gstglconfig.h
                    PATH_SUFFIXES gstreamer-1.0/include
                    HINTS ${PC_GSTREAMER_GL_INCLUDE_DIRS} ${PC_GSTREAMER_GL_INCLUDEDIR} "${gstglLibDir}"
                )
                if(GStreamer_GlConfig_INCLUDE_DIR)
                    list(APPEND GStreamer_Gl_INCLUDE_DIR "${GStreamer_GlConfig_INCLUDE_DIR}")
                    list(REMOVE_DUPLICATES GStreamer_Gl_INCLUDE_DIR)
                endif()
            endif()
            if(GStreamer_${component}_LIBRARY AND GStreamer_${component}_INCLUDE_DIR)
                add_library(GStreamer::${component} INTERFACE IMPORTED)
                target_include_directories(GStreamer::${component} INTERFACE ${GStreamer_${component}_INCLUDE_DIR})
                target_link_libraries(GStreamer::${component} INTERFACE ${GStreamer_${component}_LIBRARY})
                set_target_properties(GStreamer::${component} PROPERTIES VERSION ${GStreamer_VERSION})
            endif()
            mark_as_advanced(GStreamer_${component}_INCLUDE_DIR GStreamer_${component}_LIBRARY)
        endif()
    endif()

    if(TARGET GStreamer::${component})
        # TODO; define_property
        set(GStreamer_${component}_FOUND TRUE PARENT_SCOPE)
        get_target_property(Component_VERSION GStreamer::${component} VERSION)
        set(GStreamer_${component}_VERSION ${Component_VERSION} PARENT_SCOPE)
    endif()
endfunction()

################################################################################

# GStreamer required dependencies
find_gstreamer_component(Core gstreamer-1.0 gst/gst.h gstreamer-1.0)
find_gstreamer_component(Base gstreamer-base-1.0 gst/gst.h gstbase-1.0)
find_gstreamer_component(Video gstreamer-video-1.0 gst/video/video.h gstvideo-1.0)
find_gstreamer_component(Gl gstreamer-gl-1.0 gst/gl/gl.h gstgl-1.0)

if(TARGET GStreamer::Core)
    target_link_libraries(GStreamer::Core INTERFACE GObject::GObject)
endif()
if(TARGET GStreamer::Base AND TARGET GStreamer::Core)
    target_link_libraries(GStreamer::Base INTERFACE GStreamer::Core)
endif()
if(TARGET GStreamer::Video AND TARGET GStreamer::Base)
    target_link_libraries(GStreamer::Video INTERFACE GStreamer::Base)
endif()
if(TARGET GStreamer::Gl AND TARGET GStreamer::Video)
    target_link_libraries(GStreamer::Gl INTERFACE GStreamer::Video)
endif()

################################################################################

# GStreamer optional components
foreach(component ${GStreamer_FIND_COMPONENTS})
    if (${component} STREQUAL "App")
        find_gstreamer_component(App gstreamer-app-1.0 gst/app/gstappsink.h gstapp-1.0)
        if(TARGET GStreamer::App AND TARGET GStreamer::Base)
            target_link_libraries(GStreamer::App INTERFACE GStreamer::Base)
        endif()
    elseif (${component} STREQUAL "Audio")
        find_gstreamer_component(Audio gstreamer-audio-1.0 gst/audio/audio.h gstaudio-1.0)
        if(TARGET GStreamer::Audio AND TARGET GStreamer::Base)
            target_link_libraries(GStreamer::Audio INTERFACE GStreamer::Base)
        endif()
    elseif (${component} STREQUAL "Allocators")
        find_gstreamer_component(Allocators gstreamer-allocators-1.0 gst/allocators/allocators.h gstallocators-1.0)
        if(TARGET GStreamer::Allocators AND TARGET GStreamer::Core)
            target_link_libraries(GStreamer::Allocators INTERFACE GStreamer::Core)
        endif()
    elseif (${component} STREQUAL "Pbutils")
        find_gstreamer_component(Pbutils gstreamer-pbutils-1.0 gst/pbutils/pbutils.h gstpbutils-1.0)
        if(TARGET GStreamer::Pbutils AND TARGET GStreamer::Audio AND TARGET GStreamer::Video)
            target_link_libraries(GStreamer::Pbutils INTERFACE GStreamer::Audio GStreamer::Video)
        endif()
    elseif (${component} STREQUAL "Photography")
        find_gstreamer_component(Photography gstreamer-photography-1.0 gst/interfaces/photography.h gstphotography-1.0)
        if(TARGET GStreamer::Photography AND TARGET GStreamer::Core)
            target_link_libraries(GStreamer::Photography INTERFACE GStreamer::Core)
        endif()
    elseif (${component} STREQUAL "Controller")
        find_gstreamer_component(Controller gstreamer-controller-1.0 gst/controller/controller.h gstcontroller-1.0)
        if(TARGET GStreamer::Controller AND TARGET GStreamer::Core)
            target_link_libraries(GStreamer::Controller INTERFACE GStreamer::Core)
        endif()
    elseif (${component} STREQUAL "Codecparsers")
        find_gstreamer_component(Codecparsers gstreamer-codecparsers-1.0 gst/codecparsers/codecparsers-prelude.h gstcodecparsers-1.0)
        if(TARGET GStreamer::Codecparsers AND TARGET GStreamer::Base)
            target_link_libraries(GStreamer::Codecparsers INTERFACE GStreamer::Base)
        endif()
    elseif (${component} STREQUAL "Net")
        find_gstreamer_component(Net gstreamer-net-1.0 gst/net/net.h gstnet-1.0)
        if(TARGET GStreamer::Net AND TARGET GStreamer::Base)
            target_link_libraries(GStreamer::Net INTERFACE GStreamer::Base)
        endif()
    elseif (${component} STREQUAL "Fft")
        find_gstreamer_component(Fft gstreamer-fft-1.0 gst/fft/fft.h gstfft-1.0)
        if(TARGET GStreamer::Fft AND TARGET GStreamer::Core)
            target_link_libraries(GStreamer::Fft INTERFACE GStreamer::Core)
        endif()
    elseif (${component} STREQUAL "Riff")
        find_gstreamer_component(Riff gstreamer-riff-1.0 gst/riff/riff.h gstriff-1.0)
        if(TARGET GStreamer::Riff AND TARGET GStreamer::Base)
            target_link_libraries(GStreamer::Riff INTERFACE GStreamer::Base)
        endif()
    elseif (${component} STREQUAL "Rtp")
        find_gstreamer_component(Rtp gstreamer-rtp-1.0 gst/rtp/rtp.h gstrtp-1.0)
        if(TARGET GStreamer::Rtp AND TARGET GStreamer::Base)
            target_link_libraries(GStreamer::Rtp INTERFACE GStreamer::Base)
        endif()
    elseif (${component} STREQUAL "Rtsp")
        find_gstreamer_component(Rtsp gstreamer-rtsp-1.0 gst/rtsp/rtsp.h gstrtsp-1.0)
        if(TARGET GStreamer::Rtsp AND TARGET GStreamer::Rtp)
            target_link_libraries(GStreamer::Rtsp INTERFACE GStreamer::Rtp)
        endif()
    elseif (${component} STREQUAL "Mpegts")
        find_gstreamer_component(Mpegts gstreamer-mpegts-1.0 gst/mpegts/mpegts.h gstmpegts-1.0)
        if(TARGET GStreamer::Mpegts AND TARGET GStreamer::Base)
            target_link_libraries(GStreamer::Mpegts INTERFACE GStreamer::Base)
        endif()
    elseif (${component} STREQUAL "Sdp")
        find_gstreamer_component(Sdp gstreamer-sdp-1.0 gst/sdp/sdp.h gstsdp-1.0)
        if(TARGET GStreamer::Sdp AND TARGET GStreamer::Rtp)
            target_link_libraries(GStreamer::Sdp INTERFACE GStreamer::Rtp)
        endif()
    elseif (${component} STREQUAL "Tag")
        find_gstreamer_component(Tag gstreamer-tag-1.0 gst/tag/tag.h gsttag-1.0)
        if(TARGET GStreamer::Tag AND TARGET GStreamer::Base)
            target_link_libraries(GStreamer::Tag INTERFACE GStreamer::Base)
        endif()
    elseif (${component} STREQUAL "Prototypes")
        find_gstreamer_component(Prototypes gstreamer-gl-prototypes-1.0 gst/gl/glprototypes/all_functions.h gstglproto-1.0)
        if(TARGET GStreamer::Prototypes AND TARGET GStreamer::Gl)
            target_link_libraries(GStreamer::Prototypes INTERFACE GStreamer::Gl)
        endif()
    elseif (${component} STREQUAL "X11")
        find_gstreamer_component(X11 gstreamer-gl-x11-1.0 gst/gl/x11/x11.h x11-xcb)
        if(TARGET GStreamer::X11 AND TARGET GStreamer::Gl)
            target_link_libraries(GStreamer::X11 INTERFACE GStreamer::Gl)
        endif()
    elseif (${component} STREQUAL "EGL")
        find_gstreamer_component(EGL gstreamer-gl-egl-1.0 gst/gl/egl/egl.h egl)
        if(TARGET GStreamer::EGL AND TARGET GStreamer::Gl)
            target_link_libraries(GStreamer::EGL INTERFACE GStreamer::Gl)
        endif()
    elseif (${component} STREQUAL "Wayland")
        find_gstreamer_component(Wayland gstreamer-gl-wayland-1.0 gst/gl/wayland/wayland.h wayland-egl)
        if(TARGET GStreamer::Wayland AND TARGET GStreamer::Gl)
            target_link_libraries(GStreamer::Wayland INTERFACE GStreamer::Gl)
        endif()
    else()
        message(WARNING "FindGStreamer.cmake: Invalid Gstreamer component \"${component}\" requested")
    endif()
endforeach()

################################################################################

# Create target GStreamer::GStreamer
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GStreamer
                                REQUIRED_VARS
                                GStreamer_Core_FOUND
                                GStreamer_Base_FOUND
                                GStreamer_Video_FOUND
                                GStreamer_Gl_FOUND
                                HANDLE_COMPONENTS
)

if(GStreamer_FOUND AND NOT TARGET GStreamer::GStreamer)
    add_library(GStreamer::GStreamer INTERFACE IMPORTED)
    target_link_libraries(GStreamer::GStreamer INTERFACE
                            GStreamer::Core
                            GStreamer::Base
                            GStreamer::Video
                            GStreamer::Gl
    )
    set_target_properties(GStreamer::GStreamer PROPERTIES VERSION ${GStreamer_Core_VERSION})
    set(GStreamer_VERSION ${GStreamer_Core_VERSION})
endif()

if(TARGET PkgConfig::PC_GSTREAMER_GL)
    get_target_property(_qt_incs PkgConfig::PC_GSTREAMER_GL INTERFACE_INCLUDE_DIRECTORIES)
    set(__qt_fixed_incs)
    foreach(path IN LISTS _qt_incs)
        if(IS_DIRECTORY "${path}")
            list(APPEND __qt_fixed_incs "${path}")
        endif()
    endforeach()
    set_property(TARGET PkgConfig::PC_GSTREAMER_GL PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${__qt_fixed_incs}")
endif()

################################################################################
