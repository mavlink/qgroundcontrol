if(ANDROID OR IOS)
    set(QGC_GST_STATIC_BUILD ON CACHE BOOL "Build GST Statically")

    if(DEFINED ENV{GST_VERSION})
        set(QGC_GST_TARGET_VERSION $ENV{GST_VERSION} CACHE STRING "Environment Provided GStreamer Version")
    else()
        set(QGC_GST_TARGET_VERSION 1.22.12 CACHE STRING "Requested GStreamer Version")
    endif()
endif()

if(QGC_GST_STATIC_BUILD)
    list(APPEND PKG_CONFIG_ARGN --static)
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
    find_program(PKG_CONFIG_PROGRAM pkg-config PATHS ${GSTREAMER_PREFIX}/bin)
    if(PKG_CONFIG_PROGRAM)
        set(PKG_CONFIG_EXECUTABLE ${PKG_CONFIG_PROGRAM})
    endif()
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_PREFIX}/lib/pkgconfig;${GSTREAMER_PREFIX}/lib/gstreamer-1.0/pkgconfig;$ENV{PKG_CONFIG_PATH}")
    cmake_path(CONVERT "${GSTREAMER_PREFIX}" TO_CMAKE_PATH_LIST PREFIX_PATH NORMALIZE)
    cmake_path(CONVERT "${GSTREAMER_PREFIX}/lib" TO_CMAKE_PATH_LIST LIBDIR_PATH NORMALIZE)
    cmake_path(CONVERT "${GSTREAMER_PREFIX}/include" TO_CMAKE_PATH_LIST INCLUDE_PATH NORMALIZE)
    list(APPEND PKG_CONFIG_ARGN
        --dont-define-prefix
        --define-variable=prefix=${PREFIX_PATH}
        --define-variable=libdir=${LIBDIR_PATH}
        --define-variable=includedir=${INCLUDE_PATH}
    )
elseif(MACOS)
    list(APPEND CMAKE_FRAMEWORK_PATH "/Library/Frameworks")
    set(GSTREAMER_PREFIX "/Library/Frameworks/GStreamer.framework")
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_PREFIX}/Versions/Current/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
elseif(LINUX)
    set(GSTREAMER_PREFIX "/usr")
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_PREFIX}/lib/pkgconfig:${GSTREAMER_PREFIX}/lib/x86_64-linux-gnu/pkgconfig:$ENV{PKG_CONFIG_PATH}")
elseif(IOS)
    list(APPEND CMAKE_FRAMEWORK_PATH "~/Library/Developer/GStreamer/iPhone.sdk")
    if(DEFINED ENV{GSTREAMER_PREFIX_IOS} AND EXISTS $ENV{GSTREAMER_PREFIX_IOS})
        set(GSTREAMER_PREFIX_IOS $ENV{GSTREAMER_PREFIX_IOS})
    elseif(EXISTS "~/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
        set(GSTREAMER_PREFIX_IOS "~/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
    else()
        FetchContent_Declare(gstreamer
            URL "https://gstreamer.freedesktop.org/data/pkg/ios/${QGC_GST_TARGET_VERSION}/gstreamer-1.0-devel-${QGC_GST_TARGET_VERSION}-ios-universal.pkg"
        )
        FetchContent_MakeAvailable(gstreamer)
        set(GSTREAMER_PREFIX_IOS ${gstreamer_SOURCE_DIR})
    endif()
    set(GSTREAMER_PREFIX ${GSTREAMER_PREFIX_IOS})
elseif(ANDROID)
    set(GSTREAMER_PREFIX_ANDROID)
    if(DEFINED ENV{GSTREAMER_PREFIX_ANDROID} AND EXISTS $ENV{GSTREAMER_PREFIX_ANDROID})
        set(GSTREAMER_PREFIX_ANDROID $ENV{GSTREAMER_PREFIX_ANDROID})
    else()
        set(GSTREAMER_ARCHIVE "gstreamer-1.0-android-universal-${QGC_GST_TARGET_VERSION}.tar.xz")
        set(GSTREAMER_URL "https://gstreamer.freedesktop.org/data/pkg/android/${QGC_GST_TARGET_VERSION}/${GSTREAMER_ARCHIVE}")
        set(GSTREAMER_TARBALL "${CMAKE_BINARY_DIR}/_deps/gstreamer/${GSTREAMER_ARCHIVE}")
        set(GSTREAMER_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/gstreamer/install/gstreamer")
        if(NOT EXISTS ${GSTREAMER_TARBALL})
            message(STATUS "Downloading GStreamer from ${GSTREAMER_URL}")
            file(DOWNLOAD ${GSTREAMER_URL} ${GSTREAMER_TARBALL} SHOW_PROGRESS)
        endif()
        if(NOT EXISTS ${GSTREAMER_INSTALL_DIR})
            message(STATUS "Extracting GStreamer to ${GSTREAMER_INSTALL_DIR}")
            file(MAKE_DIRECTORY ${GSTREAMER_INSTALL_DIR})
            file(ARCHIVE_EXTRACT INPUT ${GSTREAMER_TARBALL} DESTINATION ${GSTREAMER_INSTALL_DIR})
        endif()
        set(GSTREAMER_PREFIX_ANDROID ${GSTREAMER_INSTALL_DIR})
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
    set(ENV{PKG_CONFIG_PATH} "")
    set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_PREFIX}/lib/pkgconfig:${GSTREAMER_PREFIX}/lib/gstreamer-1.0/pkgconfig")
    list(APPEND PKG_CONFIG_ARGN
        --dont-define-prefix
        --define-variable=prefix=${GSTREAMER_PREFIX}
        --define-variable=libdir=${GSTREAMER_PREFIX}/lib
        --define-variable=includedir=${GSTREAMER_PREFIX}/include
    )

    if(CMAKE_HOST_WIN32)
        find_program(PKG_CONFIG_PROGRAM pkg-config PATHS ${GSTREAMER_PREFIX}/share/gst-android/ndk-build/tools/windows)
        if(PKG_CONFIG_PROGRAM)
            set(PKG_CONFIG_EXECUTABLE ${PKG_CONFIG_PROGRAM})
        endif()
    endif()
endif()
cmake_print_variables(GSTREAMER_PREFIX)
list(PREPEND CMAKE_PREFIX_PATH ${GSTREAMER_PREFIX})

################################################################################

include(CMakeFindDependencyMacro)
find_dependency(GObject)

set(GStreamer_VERSION ${QGC_GST_TARGET_VERSION})
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    message(STATUS "PKG_CONFIG_PATH $ENV{PKG_CONFIG_PATH}")
    message(STATUS "PKG_CONFIG_LIBDIR $ENV{PKG_CONFIG_LIBDIR}")
    # message(STATUS "PKG_CONFIG_SYSROOT_DIR $ENV{PKG_CONFIG_SYSROOT_DIR}")
    cmake_print_variables(PKG_CONFIG_EXECUTABLE PKG_CONFIG_ARGN)
    pkg_check_modules(GStreamer gstreamer-1.0)
else()
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
    endif()
endif()
cmake_print_variables(GStreamer_VERSION)

################################################################################

function(find_gstreamer_component component prefix header library)
    if(NOT TARGET GStreamer::${component})
        string(TOUPPER ${component} upper)
        if(PkgConfig_FOUND)
            pkg_check_modules(PC_GSTREAMER_${upper} ${prefix} IMPORTED_TARGET)
        endif()
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
    if (${component} STREQUAL "Allocators")
        find_gstreamer_component(Allocators gstreamer-allocators-1.0 gst/allocators/allocators.h gstallocators-1.0)
        if(TARGET GStreamer::Allocators AND TARGET GStreamer::Core)
            target_link_libraries(GStreamer::Allocators INTERFACE GStreamer::Core)
        endif()
    elseif (${component} STREQUAL "App")
        find_gstreamer_component(App gstreamer-app-1.0 gst/app/gstappsink.h gstapp-1.0)
        if(TARGET GStreamer::App AND TARGET GStreamer::Base)
            target_link_libraries(GStreamer::App INTERFACE GStreamer::Base)
        endif()
    elseif (${component} STREQUAL "Audio")
        find_gstreamer_component(Audio gstreamer-audio-1.0 gst/audio/audio.h gstaudio-1.0)
        if(TARGET GStreamer::Audio AND TARGET GStreamer::Base)
            target_link_libraries(GStreamer::Audio INTERFACE GStreamer::Base)
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
    elseif (${component} STREQUAL "Fft")
        find_gstreamer_component(Fft gstreamer-fft-1.0 gst/fft/fft.h gstfft-1.0)
        if(TARGET GStreamer::Fft AND TARGET GStreamer::Core)
            target_link_libraries(GStreamer::Fft INTERFACE GStreamer::Core)
        endif()
    elseif (${component} STREQUAL "Mpegts")
        find_gstreamer_component(Mpegts gstreamer-mpegts-1.0 gst/mpegts/mpegts.h gstmpegts-1.0)
        if(TARGET GStreamer::Mpegts AND TARGET GStreamer::Base)
            target_link_libraries(GStreamer::Mpegts INTERFACE GStreamer::Base)
        endif()
    elseif (${component} STREQUAL "Net")
        find_gstreamer_component(Net gstreamer-net-1.0 gst/net/net.h gstnet-1.0)
        if(TARGET GStreamer::Net AND TARGET GStreamer::Base)
            target_link_libraries(GStreamer::Net INTERFACE GStreamer::Base)
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
    elseif (${component} STREQUAL "Va")
        find_gstreamer_component(Va gstreamer-va-1.0 gst/va/gstva.h gstva-1.0)
        if(TARGET GStreamer::Va AND TARGET GStreamer::Base AND TARGET GStreamer::Allocators)
            target_link_libraries(GStreamer::Va INTERFACE GStreamer::Base GStreamer::Allocators)
        endif()
    elseif (${component} STREQUAL "Prototypes")
        find_gstreamer_component(Prototypes gstreamer-gl-prototypes-1.0 gst/gl/glprototypes/all_functions.h gstglproto-1.0)
        if(TARGET GStreamer::Prototypes AND TARGET GStreamer::Gl)
            target_link_libraries(GStreamer::Prototypes INTERFACE GStreamer::Gl)
        endif()
    elseif (${component} STREQUAL "X11")
        find_gstreamer_component(X11 gstreamer-gl-x11-1.0 gst/gl/x11/x11.h x11-xcb)
        if(TARGET GStreamer::X11)
            if(GStreamer::Gl)
                target_link_libraries(GStreamer::X11 INTERFACE GStreamer::Gl)
            endif()
            find_package(X11)
            if(X11_FOUND)
                target_link_libraries(GStreamer::X11 INTERFACE X11::X11)
            endif()
            find_package(XCB COMPONENTS XCB GLX)
            if(XCB_FOUND)
                target_link_libraries(GStreamer::X11 INTERFACE XCB::XCB XCB::GLX)
            endif()
            find_package(X11_XCB)
            if(X11_XCB_FOUND)
                target_link_libraries(GStreamer::X11 INTERFACE X11::XCB)
            endif()
        endif()
    elseif (${component} STREQUAL "EGL")
        find_gstreamer_component(EGL gstreamer-gl-egl-1.0 gst/gl/egl/egl.h egl)
        if(TARGET GStreamer::EGL)
            if(TARGET GStreamer::Gl)
                target_link_libraries(GStreamer::EGL INTERFACE GStreamer::Gl)
            endif()
            find_package(EGL)
            if(EGL_FOUND)
                target_link_libraries(GStreamer::EGL INTERFACE EGL::EGL)
            endif()
        endif()
    elseif (${component} STREQUAL "Wayland")
        find_gstreamer_component(Wayland gstreamer-gl-wayland-1.0 gst/gl/wayland/wayland.h wayland-egl)
        if(TARGET GStreamer::Wayland)
            if(TARGET GStreamer::Gl)
                target_link_libraries(GStreamer::Wayland INTERFACE GStreamer::Gl)
            endif()
            find_package(Wayland COMPONENTS Client Cursor Egl)
            if(Wayland_FOUND)
                target_link_libraries(GStreamer::Wayland INTERFACE Wayland::Client Wayland::Cursor Wayland::Egl)
            endif()
            find_package(WaylandProtocols)
            if(WaylandProtocols_FOUND)
                # WaylandProtocols_DATADIR
            endif()
            find_package(WaylandScanner)
            if(WaylandScanner_FOUND)
                # target_link_libraries(GStreamer::Wayland INTERFACE Wayland::Scanner)
            endif()
            find_package(Qt6 COMPONENTS WaylandClient)
            if(Qt6WaylandClient_FOUND)
                target_link_libraries(GStreamer::Wayland INTERFACE Qt6::WaylandClient)
            endif()
        endif()
    elseif (${component} STREQUAL "PluginsBase")
        find_gstreamer_component(PluginsBase gstreamer-plugins-base-1.0 gst/gst.h )
        if(TARGET GStreamer::PluginsBase AND TARGET GStreamer::Core)
            target_link_libraries(GStreamer::PluginsBase INTERFACE GStreamer::Core)
        endif()
    elseif (${component} STREQUAL "PluginsGood")
        find_gstreamer_component(PluginsGood gstreamer-plugins-good-1.0 gst/gst.h )
        if(TARGET GStreamer::PluginsGood AND TARGET GStreamer::Base)
            target_link_libraries(GStreamer::PluginsGood INTERFACE GStreamer::Base)
        endif()
    elseif (${component} STREQUAL "PluginsBad")
        find_gstreamer_component(PluginsBad gstreamer-plugins-bad-1.0 gst/gst.h )
        if(TARGET GStreamer::PluginsBad AND TARGET GStreamer::PluginsGood)
            target_link_libraries(GStreamer::PluginsBad INTERFACE GStreamer::PluginsGood)
        endif()
    else()
        message(WARNING "FindGStreamer.cmake: Invalid Gstreamer component \"${component}\" requested")
    endif()
endforeach()

################################################################################

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
    set_target_properties(GStreamer::GStreamer PROPERTIES VERSION ${GStreamer_VERSION})
endif()

################################################################################

set(GST_TARGET_PLUGINS
    gstcoreelements
    gstisomp4
    gstlibav
    gstmatroska
    gstmpegtsdemux
    gstopengl
    gstplayback
    gstrtp
    gstrtpmanager
    gstrtsp
    gstsdpelem
    gsttcp
    gstudp
    gstvideoparsersbad
    gstx264
    # gstqml6
    gstasf
    gstva
)
if(ANDROID)
    list(APPEND GST_TARGET_PLUGINS gstandroidmedia)
elseif(IOS)
    list(APPEND GST_TARGET_PLUGINS gstapplemedia)
endif()

find_package(PkgConfig QUIET)
foreach(plugin IN LISTS GST_TARGET_PLUGINS)
    if(PkgConfig_FOUND)
        pkg_check_modules(GST_PLUGIN_${plugin} IMPORTED_TARGET GST_PLUGIN_${plugin} QUIET)
        if(GST_PLUGIN_${plugin}_FOUND)
            cmake_print_variables(plugin)
            target_link_libraries(GStreamer::GStreamer INTERFACE PkgConfig::GST_PLUGIN_${plugin})
        endif()
    endif()
    if(NOT GST_PLUGIN_${plugin}_FOUND)
        find_library(GST_PLUGIN_${plugin}_LIBRARY
            NAMES ${plugin}
            PATHS
                ${GSTREAMER_PREFIX}/lib
                ${GSTREAMER_PREFIX}/lib/gstreamer-1.0
                ${GSTREAMER_PREFIX}/lib/x86_64-linux-gnu
                ${GSTREAMER_PREFIX}/lib/x86_64-linux-gnu/gstreamer-1.0
        )
        if(GST_PLUGIN_${plugin}_LIBRARY)
            cmake_print_variables(plugin)
            target_link_libraries(GStreamer::GStreamer INTERFACE ${GST_PLUGIN_${plugin}_LIBRARY})
        endif()
    endif()
endforeach()

# set(GST_DEPENDENCIES
#     gstreamer-plugins-base-1.0
#     gstreamer-plugins-good-1.0
#     gstreamer-plugins-bad-1.0
#     glib-2.0
#     gio
#     gobject-2.0
#     gthread-2.0
#     gmodule-2.0
#     gmodule-no-export-2.0
#     zlib
#     drm
#     graphene-1.0
#     opus
#     ffi
#     egl
#     dl
#     m
#     pcre2-8
#     gudev-1.0
#     avcodec
#     avdevice
#     avfilter
#     avformat
#     avutil
#     postproc
#     swscale
#     va
#     va-drm
#     va-glx
#     va-wayland
#     va-x11
#     orc
#     pango
#     vpl
#     vdpau
#     vpx
#     x11
#     x264
#     x265
#     x11-xcb
#     drm
#     png
#     zlib
# )

pkg_check_modules(GRAPHENE IMPORTED_TARGET graphene-1.0)
if(GRAPHENE_FOUND)
    target_link_libraries(GStreamer::GStreamer INTERFACE PkgConfig::GRAPHENE)
endif()

pkg_check_modules(X264 IMPORTED_TARGET x264)
if(X264_FOUND)
    target_link_libraries(GStreamer::GStreamer INTERFACE PkgConfig::X264)
endif()

find_package(VAAPI)
if(VAAPI_FOUND)
    target_link_libraries(GStreamer::GStreamer INTERFACE VAAPI::VAAPI)
endif()

find_package(ZLIB)
if(ZLIB_FOUND)
    target_link_libraries(GStreamer::GStreamer INTERFACE ZLIB::ZLIB)
endif()

find_package(OpenGL)
if(OpenGL_FOUND)
    target_link_libraries(GStreamer::GStreamer INTERFACE OpenGL::GL)
endif()

find_package(GLESv2)
if(GLESv2_FOUND)
    target_link_libraries(GStreamer::GStreamer INTERFACE GLESv2::GLESv2)
endif()

find_package(FFmpeg COMPONENTS AVCODEC AVFORMAT AVUTIL AVFILTER SWRESAMPLE) # AVDEVICE POSTPROC SWSCALE
if(FFMPEG_FOUND)
    target_link_libraries(GStreamer::GStreamer INTERFACE FFmpeg::FFmpeg)
endif()

find_package(BZip2)
if(BZIP2_FOUND)
    target_link_libraries(GStreamer::GStreamer INTERFACE BZip2::BZip2)
endif()

find_package(JPEG)
if(JPEG_FOUND)
    target_link_libraries(GStreamer::GStreamer INTERFACE JPEG::JPEG)
endif()

find_package(PNG)
if(PNG_FOUND)
    target_link_libraries(GStreamer::GStreamer INTERFACE PNG::PNG)
endif()

find_package(Intl)
if(Intl_FOUND)
    target_link_libraries(GStreamer::GStreamer INTERFACE Intl::Intl)
endif()

find_package(Iconv)
if(Iconv_FOUND)
    target_link_libraries(GStreamer::GStreamer INTERFACE Iconv::Iconv)
endif()

find_package(Threads)
if(Threads_FOUND)
    target_link_libraries(GStreamer::GStreamer INTERFACE Threads::Threads)
endif()

if(ANDROID)
    target_link_options(GStreamer::GStreamer INTERFACE "-Wl,-Bsymbolic")
endif()

if(QGC_GST_STATIC_BUILD)
    target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_STATIC_BUILD)
endif()

if(ANDROID OR WIN32)
    # find_path(GStreamer_INCLUDE_DIR
    #     NAMES GStreamer
    #     PATH_SUFFIXES gstreamer-1.0
    #     PATHS ${GSTREAMER_PREFIX}/include
    # )
    # target_include_directories(GStreamer::GStreamer
    #     INTERFACE
    #         ${GSTREAMER_PREFIX}/include/gstreamer-1.0
    #         ${GSTREAMER_PREFIX}/include/glib-2.0
    #         ${GSTREAMER_PREFIX}/lib/glib-2.0/include
    #         ${GSTREAMER_PREFIX}/lib/graphene-1.0/include
    #         ${GSTREAMER_PREFIX}/lib/gstreamer-1.0/include
    #         ${GSTREAMER_PREFIX}/include
    # )
endif()

################################################################################

# Use Latest Revisions for each minor version: 1.16.3, 1.18.6, 1.20.7, 1.22.12, 1.24.7
string(REPLACE "." ";" GST_VERSION_LIST ${GStreamer_VERSION})
list(GET GST_VERSION_LIST 0 GST_VERSION_MAJOR)
list(GET GST_VERSION_LIST 1 GST_VERSION_MINOR)
list(GET GST_VERSION_LIST 2 GST_VERSION_PATCH)
cmake_print_variables(GST_VERSION_MAJOR GST_VERSION_MINOR GST_VERSION_PATCH)

if(GST_VERSION_MINOR EQUAL 16)
    set(GST_VERSION_PATCH 3)
elseif(GST_VERSION_MINOR EQUAL 18)
    set(GST_VERSION_PATCH 6)
elseif(GST_VERSION_MINOR EQUAL 20)
    set(GST_VERSION_PATCH 7)
elseif(GST_VERSION_MINOR EQUAL 22)
    set(GST_VERSION_PATCH 12)
elseif(GST_VERSION_MINOR EQUAL 24)
    set(GST_VERSION_PATCH 7)
endif()

set(GST_PLUGINS_VERSION ${GST_VERSION_MAJOR}.${GST_VERSION_MINOR}.${GST_VERSION_PATCH})
cmake_print_variables(GST_PLUGINS_VERSION)
