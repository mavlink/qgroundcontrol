if(ANDROID OR IOS)
    if(DEFINED ENV{GST_VERSION})
        set(QGC_GST_TARGET_VERSION $ENV{GST_VERSION} CACHE STRING "Environment Provided GStreamer Version")
    else()
        set(QGC_GST_TARGET_VERSION 1.22.12 CACHE STRING "Requested GStreamer Version")
    endif()
endif()

if(ANDROID OR IOS)
    set(QGC_GST_STATIC_BUILD ON CACHE BOOL "Build GST Statically")
endif()

find_package(PkgConfig QUIET)

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
    set(GSTREAMER_PREFIX "/Library/Frameworks/GStreamer.framework/Versions/1.0")
    find_program(PKG_CONFIG_PROGRAM pkg-config PATHS ${GSTREAMER_PREFIX}/bin)
    if(PKG_CONFIG_PROGRAM)
        set(PKG_CONFIG_EXECUTABLE ${PKG_CONFIG_PROGRAM})
    endif()
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_PREFIX}/lib/pkgconfig:${GSTREAMER_PREFIX}/lib/gstreamer-1.0/pkgconfig:$ENV{PKG_CONFIG_PATH}")
elseif(LINUX)
    set(GSTREAMER_PREFIX "/usr")
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_PREFIX}/lib/x86_64-linux-gnu/pkgconfig:$ENV{PKG_CONFIG_PATH}")
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
    if(CMAKE_HOST_WIN32)
        find_program(PKG_CONFIG_PROGRAM pkg-config PATHS ${GSTREAMER_PREFIX}/share/gst-android/ndk-build/tools/windows)
        if(PKG_CONFIG_PROGRAM)
            set(PKG_CONFIG_EXECUTABLE ${PKG_CONFIG_PROGRAM})
            set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_PREFIX}/lib/pkgconfig;${GSTREAMER_PREFIX}/lib/gstreamer-1.0/pkgconfig")
        endif()
    elseif(CMAKE_HOST_LINUX)
        if(PkgConfig_FOUND)
            set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_PREFIX}/lib/pkgconfig:${GSTREAMER_PREFIX}/lib/gstreamer-1.0/pkgconfig")
        endif()
    endif()

    list(APPEND PKG_CONFIG_ARGN
        --dont-define-prefix
        --define-variable=prefix=${GSTREAMER_PREFIX}
        --define-variable=libdir=${GSTREAMER_PREFIX}/lib
        --define-variable=includedir=${GSTREAMER_PREFIX}/include
    )
endif()
list(PREPEND CMAKE_PREFIX_PATH ${GSTREAMER_PREFIX})
cmake_print_variables(GSTREAMER_PREFIX)

################################################################################

include(CMakeFindDependencyMacro)
find_dependency(GObject)

set(GStreamer_VERSION ${QGC_GST_TARGET_VERSION})
if(PkgConfig_FOUND)
    message(STATUS "PKG_CONFIG_PATH $ENV{PKG_CONFIG_PATH}")
    message(STATUS "PKG_CONFIG_LIBDIR $ENV{PKG_CONFIG_LIBDIR}")
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

# Use Latest Revisions for each minor version: 1.16.3, 1.18.6, 1.20.7, 1.22.12, 1.24.8
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
    set(GST_VERSION_PATCH 8)
endif()

set(GST_PLUGINS_VERSION ${GST_VERSION_MAJOR}.${GST_VERSION_MINOR}.${GST_VERSION_PATCH})
cmake_print_variables(GST_PLUGINS_VERSION)

################################################################################

function(find_gstreamer_component component)
    cmake_parse_arguments(PARSE_ARGV 1 ARGS "" "PC_NAME;HEADER;LIBRARY" "DEPENDENCIES")

    set(pkgconfig_name ${ARGS_PC_NAME})
    set(header ${ARGS_HEADER})
    set(library ${ARGS_LIBRARY})

    set(target GStreamer::${component})

    if(NOT TARGET ${target})
        string(TOUPPER ${component} upper)
        pkg_check_modules(PC_GSTREAMER_${upper} IMPORTED_TARGET ${pkgconfig_name})
        if(TARGET PkgConfig::PC_GSTREAMER_${upper})
            add_library(GStreamer::${component} INTERFACE IMPORTED)
            target_link_libraries(GStreamer::${component} INTERFACE PkgConfig::PC_GSTREAMER_${upper})
            set_target_properties(GStreamer::${component} PROPERTIES VERSION ${PC_GSTREAMER_${upper}_VERSION})
        else()
            foreach(dependency IN LISTS ARGS_DEPENDENCIES)
                if (NOT TARGET ${dependency})
                    set(GStreamer_${component}_FOUND FALSE PARENT_SCOPE)
                    return()
                endif()
            endforeach()

            find_path(GStreamer_${component}_INCLUDE_DIR
                NAMES ${header}
                PATH_SUFFIXES gstreamer-1.0
            )
            find_library(GStreamer_${component}_LIBRARY
                NAMES ${library}
            )
            if(${component} STREQUAL "Gl")
                # search the gstglconfig.h include dir under the same root where the library is found
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
                if(ARGS_DEPENDENCIES)
                    target_link_libraries(GStreamer::${component} INTERFACE ${ARGS_DEPENDENCIES})
                endif()
                set_target_properties(GStreamer::${component} PROPERTIES VERSION ${GStreamer_VERSION})
            endif()
            mark_as_advanced(GStreamer_${component}_INCLUDE_DIR GStreamer_${component}_LIBRARY)
        endif()
    endif()

    if(TARGET ${target})
        set(GStreamer_${component}_FOUND TRUE PARENT_SCOPE)
        # TODO; define_property
        get_target_property(Component_VERSION GStreamer::${component} VERSION)
        set(GStreamer_${component}_VERSION ${Component_VERSION} PARENT_SCOPE)
    endif()
endfunction()

################################################################################

find_gstreamer_component(Core
    PC_NAME gstreamer-1.0
    HEADER gst/gst.h
    LIBRARY gstreamer-1.0
    DEPENDENCIES GLIB2::GLIB2 GObject::GObject)
find_gstreamer_component(Base
    PC_NAME gstreamer-base-1.0
    HEADER gst/gst.h
    LIBRARY gstbase-1.0
    DEPENDENCIES GStreamer::Core)
find_gstreamer_component(Video
    PC_NAME gstreamer-video-1.0
    HEADER gst/video/video.h
    LIBRARY gstvideo-1.0
    DEPENDENCIES GStreamer::Core GStreamer::Base)
find_gstreamer_component(Gl
    PC_NAME gstreamer-gl-1.0
    HEADER gst/gl/gl.h
    LIBRARY gstgl-1.0
    DEPENDENCIES GStreamer::Core GStreamer::Base GStreamer::Video)

################################################################################

if(Allocators IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Allocators
        PC_NAME gstreamer-allocators-1.0
        HEADER gst/allocators/allocators.h
        LIBRARY gstallocators-1.0
        DEPENDENCIES GStreamer::Core)
endif()

if(App IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(App
        PC_NAME gstreamer-app-1.0
        HEADER gst/app/gstappsink.h
        LIBRARY gstapp-1.0
        DEPENDENCIES GStreamer::Core GStreamer::Base)
endif()

if(Audio IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Audio
        PC_NAME gstreamer-audio-1.0
        HEADER gst/audio/audio.h
        LIBRARY gstaudio-1.0
        DEPENDENCIES GStreamer::Core GStreamer::Base)
endif()

if(Codecparsers IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Codecparsers
        PC_NAME gstreamer-codecparsers-1.0
        HEADER gst/codecparsers/codecparsers-prelude.h
        LIBRARY gstcodecparsers-1.0
        DEPENDENCIES GStreamer::Core GStreamer::Base)
endif()

if(Controller IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Controller
        PC_NAME gstreamer-controller-1.0
        HEADER gst/controller/controller.h
        LIBRARY gstcontroller-1.0
        DEPENDENCIES GStreamer::Core)
endif()

if(Fft IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Fft
        PC_NAME gstreamer-fft-1.0
        HEADER gst/fft/cfft.h
        LIBRARY gstfft-1.0
        DEPENDENCIES GStreamer::Core)
endif()

if(GlEgl IN_LIST GStreamer_FIND_COMPONENTS)
    # find_package(EGL)
    find_gstreamer_component(GlEgl
        PC_NAME gstreamer-gl-egl-1.0
        HEADER gst/gl/egl/gstgldisplay_egl.h
        LIBRARY gstgl-1.0
        DEPENDENCIES GStreamer::Gl EGL::EGL)
endif()

if(GlPrototypes IN_LIST GStreamer_FIND_COMPONENTS)
    # find_package(GLESv2)
    # find_package(OpenGL OPTIONAL_COMPONENTS EGL GLX OpenGL) # GLES2 GLES3
    find_gstreamer_component(GlPrototypes
        PC_NAME gstreamer-gl-prototypes-1.0
        HEADER gst/gl/glprototypes/all_functions.h
        LIBRARY gstglproto-1.0
        DEPENDENCIES GStreamer::Gl GLESv2::GLESv2 OpenGL::GL)
endif()

if(GlWayland IN_LIST GStreamer_FIND_COMPONENTS)
    # find_package(Wayland COMPONENTS Client Cursor Egl)
    # find_package(WaylandProtocols)
    # find_package(WaylandScanner)
    # find_package(Qt6 COMPONENTS WaylandClient)
    find_gstreamer_component(GlWayland
        PC_NAME gstreamer-gl-wayland-1.0
        HEADER gst/gl/wayland/gstgldisplay_wayland.h
        LIBRARY gstgl-1.0
        DEPENDENCIES GStreamer::Gl Wayland::EGL Wayland::Client)
endif()

if(GlX11 IN_LIST GStreamer_FIND_COMPONENTS)
    # find_package(X11)
    # find_package(XCB COMPONENTS XCB GLX)
    # find_package(X11_XCB)
    find_gstreamer_component(GlX11
        PC_NAME gstreamer-gl-x11-1.0
        HEADER gst/gl/x11/gstgldisplay_x11.h
        LIBRARY gstgl-1.0
        DEPENDENCIES GStreamer::Gl X11::XCB)
endif()

if(Mpegts IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Mpegts
        PC_NAME gstreamer-mpegts-1.0
        HEADER gst/mpegts/mpegts.h
        LIBRARY gstmpegts-1.0
        DEPENDENCIES GStreamer::Core GStreamer::Base)
endif()

if(Net IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Net
        PC_NAME gstreamer-net-1.0
        HEADER gst/net/net.h
        LIBRARY gstnet-1.0
        DEPENDENCIES GStreamer::Core)
endif()

if(Pbutils IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Pbutils
        PC_NAME gstreamer-pbutils-1.0
        HEADER gst/pbutils/pbutils.h
        LIBRARY gstpbutils-1.0
        DEPENDENCIES GStreamer::Video GStreamer::Audio GStreamer::Core GStreamer::Base)
endif()

if(Photography IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Photography
        PC_NAME gstreamer-photography-1.0
        HEADER gst/interfaces/photography.h
        LIBRARY gstphotography-1.0
        DEPENDENCIES GStreamer::Core GStreamer::Base)
endif()

if(PluginsBad IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(PluginsBad
        PC_NAME gstreamer-plugins-bad-1.0
        HEADER gst/gst.h
        LIBRARY gstreamer-1.0
        DEPENDENCIES GStreamer::Core)
endif()

if(PluginsBase IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(PluginsBase
        PC_NAME gstreamer-plugins-base-1.0
        HEADER gst/gst.h
        LIBRARY gstreamer-1.0
        DEPENDENCIES GStreamer::Core)
endif()

if(PluginsGood IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(PluginsGood
        PC_NAME gstreamer-plugins-good-1.0
        HEADER gst/gst.h
        LIBRARY gstphotography-1.0
        DEPENDENCIES GStreamer::Core GStreamer::Base)
endif()

if(Riff IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Riff
        PC_NAME gstreamer-riff-1.0
        HEADER gst/riff/riff.h
        LIBRARY gstriff-1.0
        DEPENDENCIES GStreamer::Core)
endif()

if(Rtp IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Rtp
        PC_NAME gstreamer-rtp-1.0
        HEADER gst/rtp/rtp.h
        LIBRARY gstrtp-1.0
        DEPENDENCIES GStreamer::Core GStreamer::Base)
endif()

if(Sdp IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Sdp
        PC_NAME gstreamer-sdp-1.0
        HEADER gst/sdp/sdp.h
        LIBRARY gstsdp-1.0
        DEPENDENCIES GStreamer::Core)
endif()

if(Rtsp IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Rtsp
        PC_NAME gstreamer-rtsp-1.0
        HEADER gst/rtsp/rtsp.h
        LIBRARY gstrtsp-1.0
        DEPENDENCIES GStreamer::Sdp GStreamer::Core GLIB2::GIO)
endif()

if(Tag IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Tag
        PC_NAME gstreamer-tag-1.0
        HEADER gst/tag/tag.h
        LIBRARY gsttag-1.0
        DEPENDENCIES GStreamer::Core)
endif()

if(Va IN_LIST GStreamer_FIND_COMPONENTS)
    # find_package(VAAPI)
    find_gstreamer_component(Va
        PC_NAME gstreamer-va-1.0
        HEADER gst/va/gstva.h
        LIBRARY gstva-1.0
        DEPENDENCIES GStreamer::Core GStreamer::Video)
endif()

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

if(ANDROID)
    target_link_options(GStreamer::GStreamer INTERFACE "-Wl,-Bsymbolic")
endif()

if(QGC_GST_STATIC_BUILD)
    target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_STATIC_BUILD)
endif()

# TODO: find_path
if(LINUX)
    set(GSTREAMER_LIB_PATH ${GSTREAMER_PREFIX}/lib/x86_64-linux-gnu)
elseif(MACOS)
    set(GSTREAMER_LIB_PATH ${GSTREAMER_PREFIX}/lib)
elseif(ANDROID OR WIN32)
    set(GSTREAMER_LIB_PATH ${GSTREAMER_PREFIX}/lib)
elseif(IOS)

endif()
set(GSTREAMER_PLUGIN_PATH ${GSTREAMER_LIB_PATH}/gstreamer-1.0)

target_include_directories(GStreamer::GStreamer
    INTERFACE
        ${GSTREAMER_PREFIX}/include
        ${GSTREAMER_PREFIX}/include/glib-2.0
        ${GSTREAMER_PREFIX}/include/graphene-1.0
        ${GSTREAMER_PREFIX}/include/gstreamer-1.0
        ${GSTREAMER_LIB_PATH}/glib-2.0/include
        ${GSTREAMER_LIB_PATH}/graphene-1.0/include
        ${GSTREAMER_PLUGIN_PATH}/include
)

target_link_directories(GStreamer::GStreamer
    INTERFACE
        ${GSTREAMER_LIB_PATH}
        ${GSTREAMER_PLUGIN_PATH}
)

################################################################################

add_library(GStreamer::Plugins INTERFACE IMPORTED)

set(GST_PLUGINS
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
    gstasf
    gstva
    # gstqml6
)
if(ANDROID)
    list(APPEND GST_PLUGINS gstandroidmedia)
elseif(IOS)
    list(APPEND GST_PLUGINS gstapplemedia)
endif()

foreach(plugin IN LISTS GST_PLUGINS)
    if(PkgConfig_FOUND)
        pkg_check_modules(GST_PLUGIN_${plugin} IMPORTED_TARGET ${plugin})
        if(GST_PLUGIN_${plugin}_FOUND)
            target_link_libraries(GStreamer::Plugins INTERFACE PkgConfig::GST_PLUGIN_${plugin})
        endif()
    endif()

    if(NOT GST_PLUGIN_${plugin}_FOUND)
        find_library(GST_PLUGIN_${plugin}_LIBRARY
            NAMES ${plugin}
            PATHS
                ${GSTREAMER_LIB_PATH}
                ${GSTREAMER_PLUGIN_PATH}
        )
        if(GST_PLUGIN_${plugin}_LIBRARY)
            cmake_print_variables(plugin)
            target_link_libraries(GStreamer::Plugins INTERFACE ${GST_PLUGIN_${plugin}_LIBRARY})
            set(GST_PLUGIN_${plugin}_FOUND TRUE)
        endif()
    endif()
endforeach()

if(NOT MACOS)
    target_link_libraries(GStreamer::GStreamer INTERFACE GStreamer::Plugins)
endif()

################################################################################

# set(PLUGINS_DECLARATION)
# set(PLUGINS_REGISTRATION)
# foreach(GST_P ${GST_PLUGINS})
#     list(APPEND LINK_LIBS "gst${GST_P}")
#     list(APPEND PLUGINS_DECLARATION "\nGST_PLUGIN_STATIC_DECLARE(${GST_P})")
#     list(APPEND PLUGINS_REGISTRATION "\nGST_PLUGIN_STATIC_REGISTER(${GST_P})")
# endforeach()

# if(ANDROID)
#     if(EXISTS ${GSTREAMER_PREFIX}/share/gst-android/ndk-build/androidmedia)
#         install(DIRECTORY ${GSTREAMER_PREFIX}/share/gst-android/ndk-build/androidmedia DESTINATION ${CMAKE_BINARY_DIR}/android-build/src/org/freedesktop/androidmedia)
#     endif()
#     if(EXISTS ${GSTREAMER_PREFIX}/share/gst-android/ndk-build/GStreamer.java)
#         install(FILES ${GSTREAMER_PREFIX}/share/gst-android/ndk-build/GStreamer.java DESTINATION ${CMAKE_BINARY_DIR}/android-build/src/org/freedesktop/GStreamer.java)
#     endif()
#     if(EXISTS ${GSTREAMER_PREFIX}/share/gst-android/ndk-build/gstreamer_android-1.0.c.in)
#         configure_file(${GSTREAMER_PREFIX}/share/gst-android/ndk-build/gstreamer_android-1.0.c.in ${CMAKE_CURRENT_BINARY_DIR}/gst_plugin_init_android.c)
#     endif()
# endif()
