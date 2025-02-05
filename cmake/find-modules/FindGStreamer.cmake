if(ANDROID OR IOS)
    if(DEFINED ENV{GST_VERSION})
        set(QGC_GST_TARGET_VERSION $ENV{GST_VERSION} CACHE STRING "Environment Provided GStreamer Version")
    else()
        set(QGC_GST_TARGET_VERSION 1.22.12 CACHE STRING "Requested GStreamer Version")
    endif()
endif()

if(ANDROID OR IOS)
    set(QGC_GST_STATIC_BUILD ON)
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
    set(ENV{PKG_CONFIG} ${GSTREAMER_PREFIX}/bin)
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
    set(GSTREAMER_FRAMEWORK_PATH "/Library/Frameworks/GStreamer.framework" CACHE PATH "GStreamer Framework Path")
    set(GSTREAMER_PREFIX "${GSTREAMER_FRAMEWORK_PATH}/Versions/1.0")
    set(ENV{PKG_CONFIG} "${GSTREAMER_PREFIX}/bin")
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_PREFIX}/lib/pkgconfig:${GSTREAMER_PREFIX}/lib/gstreamer-1.0/pkgconfig:$ENV{PKG_CONFIG_PATH}")
elseif(LINUX)
    set(GSTREAMER_PREFIX "/usr")
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_PREFIX}/lib/x86_64-linux-gnu/pkgconfig:${GSTREAMER_PREFIX}/lib/x86_64-linux-gnu/gstreamer-1.0/pkgconfig:$ENV{PKG_CONFIG_PATH}")
elseif(IOS)
    list(APPEND CMAKE_FRAMEWORK_PATH "~/Library/Developer/GStreamer/iPhone.sdk")
    if(DEFINED ENV{GSTREAMER_PREFIX_IOS} AND EXISTS $ENV{GSTREAMER_PREFIX_IOS})
        set(GSTREAMER_PREFIX_IOS $ENV{GSTREAMER_PREFIX_IOS})
    elseif(EXISTS "~/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
        set(GSTREAMER_PREFIX_IOS "~/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
    else()
        FetchContent_Declare(gstreamer
            DOWNLOAD_EXTRACT_TIMESTAMP true
            URL "https://gstreamer.freedesktop.org/data/pkg/ios/${QGC_GST_TARGET_VERSION}/gstreamer-1.0-devel-${QGC_GST_TARGET_VERSION}-ios-universal.pkg"
        )
        FetchContent_MakeAvailable(gstreamer)
        set(GSTREAMER_PREFIX_IOS ${gstreamer_SOURCE_DIR})
    endif()
    set(GSTREAMER_PREFIX ${GSTREAMER_PREFIX_IOS})
elseif(ANDROID)
    set(GSTREAMER_ARCHIVE "gstreamer-1.0-android-universal-${QGC_GST_TARGET_VERSION}.tar.xz")
    include(CPM)
    CPMAddPackage(
        NAME gstreamer
        URL "https://gstreamer.freedesktop.org/data/pkg/android/${QGC_GST_TARGET_VERSION}/${GSTREAMER_ARCHIVE}"
    )
    set(GSTREAMER_PREFIX_ANDROID "${gstreamer_SOURCE_DIR}")

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
    set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH OFF)
    if(CMAKE_HOST_WIN32)
        set(ENV{PKG_CONFIG} "${GSTREAMER_PREFIX}/share/gst-android/ndk-build/tools/windows")
        set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_PREFIX}/lib/pkgconfig;${GSTREAMER_PREFIX}/lib/gstreamer-1.0/pkgconfig")
        set(PKG_CONFIG_EXECUTABLE "$ENV{PKG_CONFIG}/pkg-config.exe")
    elseif(CMAKE_HOST_LINUX OR CMAKE_HOST_APPLE)
        set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_PREFIX}/lib/pkgconfig:${GSTREAMER_PREFIX}/lib/gstreamer-1.0/pkgconfig")
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

# TODO: find_path, Change x86_64-linux-gnu based on host
if(LINUX)
    set(GSTREAMER_LIB_PATH ${GSTREAMER_PREFIX}/lib/x86_64-linux-gnu)
elseif(MACOS OR ANDROID OR WIN32)
    set(GSTREAMER_LIB_PATH ${GSTREAMER_PREFIX}/lib)
elseif(IOS)

endif()
set(GSTREAMER_PLUGIN_PATH ${GSTREAMER_LIB_PATH}/gstreamer-1.0)
cmake_print_variables(GSTREAMER_LIB_PATH)

################################################################################

message(STATUS "PKG_CONFIG $ENV{PKG_CONFIG}")
message(STATUS "PKG_CONFIG_PATH $ENV{PKG_CONFIG_PATH}")
message(STATUS "PKG_CONFIG_LIBDIR $ENV{PKG_CONFIG_LIBDIR}")
find_dependency(PkgConfig)
cmake_print_variables(PKG_CONFIG_EXECUTABLE PKG_CONFIG_ARGN)

include(CMakeFindDependencyMacro)
find_dependency(GObject)

set(GStreamer_VERSION ${QGC_GST_TARGET_VERSION})
pkg_check_modules(GStreamer gstreamer-1.0)
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

function(find_gstreamer_component component pkgconfig_name)
    set(target GStreamer::${component})

    if(NOT TARGET ${target})
        string(TOUPPER ${component} upper)
        pkg_check_modules(PC_GSTREAMER_${upper} IMPORTED_TARGET ${pkgconfig_name})
        if(TARGET PkgConfig::PC_GSTREAMER_${upper})
            add_library(GStreamer::${component} INTERFACE IMPORTED)
            target_link_libraries(GStreamer::${component} INTERFACE PkgConfig::PC_GSTREAMER_${upper})
            set_target_properties(GStreamer::${component} PROPERTIES VERSION ${PC_GSTREAMER_${upper}_VERSION})
        endif()
        mark_as_advanced(GStreamer_${component}_INCLUDE_DIR GStreamer_${component}_LIBRARY)
    endif()

    if(TARGET ${target})
        set(GStreamer_${component}_FOUND TRUE PARENT_SCOPE)
        # TODO; define_property
        get_target_property(Component_VERSION GStreamer::${component} VERSION)
        set(GStreamer_${component}_VERSION ${Component_VERSION} PARENT_SCOPE)
    endif()
endfunction()

################################################################################

find_gstreamer_component(Core gstreamer-1.0)
find_gstreamer_component(Base gstreamer-base-1.0)
find_gstreamer_component(Video gstreamer-video-1.0)
find_gstreamer_component(Gl gstreamer-gl-1.0)

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

if(Allocators IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Allocators gstreamer-allocators-1.0)
endif()

if(App IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(App gstreamer-app-1.0)
endif()

if(Audio IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Audio gstreamer-audio-1.0)
endif()

if(Codecparsers IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Codecparsers gstreamer-codecparsers-1.0)
endif()

if(Controller IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Controller gstreamer-controller-1.0)
endif()

if(Fft IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Fft gstreamer-fft-1.0)
endif()

if(GlEgl IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(GlEgl gstreamer-gl-egl-1.0)
endif()

if(GlPrototypes IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(GlPrototypes gstreamer-gl-prototypes-1.0)
endif()

if(GlWayland IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(GlWayland gstreamer-gl-wayland-1.0)
endif()

if(GlX11 IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(GlX11 gstreamer-gl-x11-1.0)
endif()

if(Mpegts IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Mpegts gstreamer-mpegts-1.0)
endif()

if(Net IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Net gstreamer-net-1.0)
endif()

if(Pbutils IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Pbutils gstreamer-pbutils-1.0)
endif()

if(Photography IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Photography gstreamer-photography-1.0)
endif()

if(Play IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Play gstreamer-play-1.0)
endif()

if(PluginsBad IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(PluginsBad gstreamer-plugins-bad-1.0)
endif()

if(PluginsBase IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(PluginsBase gstreamer-plugins-base-1.0)
endif()

if(PluginsGood IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(PluginsGood gstreamer-plugins-good-1.0)
endif()

if(Riff IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Riff gstreamer-riff-1.0)
endif()

if(Rtp IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Rtp gstreamer-rtp-1.0)
endif()

if(Sdp IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Sdp gstreamer-sdp-1.0)
endif()

if(Rtsp IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Rtsp gstreamer-rtsp-1.0)
endif()

if(Tag IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Tag gstreamer-tag-1.0)
endif()

if(Va IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(Va gstreamer-va-1.0)
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
    target_link_libraries(GStreamer::GStreamer
        INTERFACE
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

target_include_directories(GStreamer::GStreamer
    INTERFACE
        ${GSTREAMER_PREFIX}/include
        ${GSTREAMER_PREFIX}/include/glib-2.0
        # ${GSTREAMER_PREFIX}/include/graphene-1.0
        ${GSTREAMER_PREFIX}/include/gstreamer-1.0
        ${GSTREAMER_LIB_PATH}/glib-2.0/include
        # ${GSTREAMER_LIB_PATH}/graphene-1.0/include
        ${GSTREAMER_LIB_PATH}/gstreamer-1.0/include
        ${GSTREAMER_PLUGIN_PATH}/include
)

target_link_directories(GStreamer::GStreamer INTERFACE ${GSTREAMER_LIB_PATH} ${GSTREAMER_PLUGIN_PATH})

if(MACOS AND EXISTS ${GSTREAMER_FRAMEWORK_PATH})
    target_link_libraries(GStreamer::GStreamer INTERFACE "-F /Library/Frameworks -framework GStreamer")
endif()

################################################################################

# TODO: https://gstreamer.freedesktop.org/documentation/qt6d3d11/index.html#qml6d3d11sink-page

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
    gstva
    gstapp
    gstvaapi
    # gstqml6
)
if(ANDROID)
    list(APPEND GST_PLUGINS gstandroidmedia)
elseif(IOS)
    list(APPEND GST_PLUGINS gstapplemedia)
endif()

foreach(plugin IN LISTS GST_PLUGINS)
    pkg_check_modules(GST_PLUGIN_${plugin} QUIET IMPORTED_TARGET ${plugin})
    if(GST_PLUGIN_${plugin}_FOUND)
        target_link_libraries(GStreamer::Plugins INTERFACE PkgConfig::GST_PLUGIN_${plugin})
    else()
        find_library(GST_PLUGIN_${plugin}_LIBRARY
            NAMES ${plugin}
            PATHS
                ${GSTREAMER_LIB_PATH}
                ${GSTREAMER_PLUGIN_PATH}
        )
        if(GST_PLUGIN_${plugin}_LIBRARY)
            target_link_libraries(GStreamer::Plugins INTERFACE ${GST_PLUGIN_${plugin}_LIBRARY})
            set(GST_PLUGIN_${plugin}_FOUND TRUE)
        endif()
    endif()

    if(GST_PLUGIN_${plugin}_FOUND)
        cmake_print_variables(plugin)
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
