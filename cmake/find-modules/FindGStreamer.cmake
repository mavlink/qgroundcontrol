if(ANDROID OR IOS)
    set(QGC_GST_TARGET_VERSION 1.22.12)
    set(QGC_GST_STATIC_BUILD ON)
endif()

set(PKG_CONFIG_ARGN)
if(QGC_GST_STATIC_BUILD)
    list(APPEND PKG_CONFIG_ARGN "--static")
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
    set(GSTREAMER_LIB_PATH "${GSTREAMER_PREFIX}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    set(GSTREAMER_INCLUDE_PATH "${GSTREAMER_PREFIX}/include")
    set(ENV{PKG_CONFIG} "${GSTREAMER_PREFIX}/bin")
    set(PKG_CONFIG_EXECUTABLE "$ENV{PKG_CONFIG}/pkg-config.exe")
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_LIB_PATH}/pkgconfig;${GSTREAMER_PLUGIN_PATH}/pkgconfig;$ENV{PKG_CONFIG_PATH}")
    cmake_path(CONVERT "${GSTREAMER_PREFIX}" TO_CMAKE_PATH_LIST PREFIX_PATH NORMALIZE)
    cmake_path(CONVERT "${GSTREAMER_LIB_PATH}" TO_CMAKE_PATH_LIST LIBDIR_PATH NORMALIZE)
    cmake_path(CONVERT "${GSTREAMER_INCLUDE_PATH}" TO_CMAKE_PATH_LIST INCLUDE_PATH NORMALIZE)
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
    set(GSTREAMER_LIB_PATH "${GSTREAMER_PREFIX}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    set(GSTREAMER_INCLUDE_PATH "${GSTREAMER_FRAMEWORK_PATH}/Headers")
    set(ENV{PKG_CONFIG} "${GSTREAMER_PREFIX}/bin")
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_LIB_PATH}/pkgconfig:${GSTREAMER_PLUGIN_PATH}/pkgconfig:$ENV{PKG_CONFIG_PATH}")
elseif(LINUX)
    set(GSTREAMER_PREFIX "/usr")
    if(EXISTS "${GSTREAMER_PREFIX}/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu")
        set(GSTREAMER_LIB_PATH "${GSTREAMER_PREFIX}/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu")
    else()
        set(GSTREAMER_LIB_PATH "${GSTREAMER_PREFIX}/lib")
    endif()
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    set(GSTREAMER_INCLUDE_PATH "${GSTREAMER_PREFIX}/include")
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_LIB_PATH}/pkgconfig:$ENV{PKG_CONFIG_PATH}")
elseif(IOS)
    list(APPEND CMAKE_FRAMEWORK_PATH "~/Library/Developer/GStreamer/iPhone.sdk")
    set(GSTREAMER_FRAMEWORK_PATH "~/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
    if(EXISTS "${GSTREAMER_FRAMEWORK_PATH}")
        set(GSTREAMER_INCLUDE_PATH "${GSTREAMER_FRAMEWORK_PATH}/Headers")
    else()
        include(CPM)
        CPMAddPackage(
            NAME gstreamer
            VERSION ${QGC_GST_TARGET_VERSION}
            URL "https://gstreamer.freedesktop.org/data/pkg/ios/${QGC_GST_TARGET_VERSION}/gstreamer-1.0-devel-${QGC_GST_TARGET_VERSION}-ios-universal.pkg"
        )
        set(GSTREAMER_PREFIX ${gstreamer_SOURCE_DIR})
        set(GSTREAMER_INCLUDE_PATH "${GSTREAMER_PREFIX}/include")
    endif()
elseif(ANDROID)
    set(GSTREAMER_ARCHIVE "gstreamer-1.0-android-universal-${QGC_GST_TARGET_VERSION}.tar.xz")
    include(CPM)
    CPMAddPackage(
        NAME gstreamer
        VERSION ${QGC_GST_TARGET_VERSION}
        URL "https://gstreamer.freedesktop.org/data/pkg/android/${QGC_GST_TARGET_VERSION}/${GSTREAMER_ARCHIVE}"
    )
    set(GSTREAMER_PREFIX_ANDROID "${gstreamer_SOURCE_DIR}")

    if(${CMAKE_ANDROID_ARCH_ABI} STREQUAL armeabi-v7a)
        set(GSTREAMER_PREFIX "${GSTREAMER_PREFIX_ANDROID}/armv7")
    elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL arm64-v8a)
        set(GSTREAMER_PREFIX "${GSTREAMER_PREFIX_ANDROID}/arm64")
    elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL x86)
        set(GSTREAMER_PREFIX "${GSTREAMER_PREFIX_ANDROID}/x86")
    elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL x86_64)
        set(GSTREAMER_PREFIX "${GSTREAMER_PREFIX_ANDROID}/x86_64")
    endif()
    set(GSTREAMER_LIB_PATH "${GSTREAMER_PREFIX}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    set(GSTREAMER_INCLUDE_PATH "${GSTREAMER_PREFIX}/include")

    set(ENV{PKG_CONFIG_PATH} "")
    set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
    if(CMAKE_HOST_WIN32)
        set(ENV{PKG_CONFIG} "${GSTREAMER_PREFIX}/share/gst-android/ndk-build/tools/windows")
        set(PKG_CONFIG_EXECUTABLE "$ENV{PKG_CONFIG}/pkg-config.exe")
        set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_LIB_PATH}/pkgconfig;${GSTREAMER_PLUGIN_PATH}/pkgconfig")
    elseif(CMAKE_HOST_UNIX)
        set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_LIB_PATH}/pkgconfig:${GSTREAMER_PLUGIN_PATH}/pkgconfig")
    endif()

    list(APPEND PKG_CONFIG_ARGN
        --dont-define-prefix
        --define-variable=prefix=${GSTREAMER_PREFIX}
        --define-variable=libdir=${GSTREAMER_LIB_PATH}
        --define-variable=includedir=${GSTREAMER_INCLUDE_PATH}
    )
endif()

list(PREPEND CMAKE_PREFIX_PATH ${GSTREAMER_PREFIX})
cmake_print_variables(GSTREAMER_PREFIX GSTREAMER_LIB_PATH GSTREAMER_PLUGIN_PATH)

################################################################################

message(STATUS "PKG_CONFIG $ENV{PKG_CONFIG}")
message(STATUS "PKG_CONFIG_PATH $ENV{PKG_CONFIG_PATH}")
message(STATUS "PKG_CONFIG_LIBDIR $ENV{PKG_CONFIG_LIBDIR}")
find_dependency(PkgConfig)

cmake_print_variables(PKG_CONFIG_EXECUTABLE PKG_CONFIG_ARGN)

include(CMakeFindDependencyMacro)
find_dependency(GObject)

pkg_check_modules(GStreamer gstreamer-1.0)
cmake_print_variables(GStreamer_VERSION)

# Use Latest Revisions for each minor version: 1.16.3, 1.18.6, 1.20.7, 1.22.12, 1.24.12
string(REPLACE "." ";" GST_VERSION_LIST ${GStreamer_VERSION})
list(GET GST_VERSION_LIST 0 GST_VERSION_MAJOR)
list(GET GST_VERSION_LIST 1 GST_VERSION_MINOR)
list(GET GST_VERSION_LIST 2 GST_VERSION_PATCH)

if(GST_VERSION_MINOR EQUAL 16)
    set(GST_VERSION_PATCH 3)
elseif(GST_VERSION_MINOR EQUAL 18)
    set(GST_VERSION_PATCH 6)
elseif(GST_VERSION_MINOR EQUAL 20)
    set(GST_VERSION_PATCH 7)
elseif(GST_VERSION_MINOR EQUAL 22)
    set(GST_VERSION_PATCH 12)
elseif(GST_VERSION_MINOR EQUAL 24)
    set(GST_VERSION_PATCH 12)
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

################################################################################

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GStreamer
    VERSION_VAR GStreamer_VERSION
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
    if(${ANDROID_ABI} STREQUAL "armeabi-v7a" OR ${ANDROID_ABI} STREQUAL "x86")
        target_link_options(GStreamer::GStreamer INTERFACE "-Wl,-z,notext")
    endif()
endif()

if(QGC_GST_STATIC_BUILD)
    target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_STATIC_BUILD)
endif()

if(EXISTS ${GSTREAMER_FRAMEWORK_PATH})
    if(MACOS)
        target_link_libraries(GStreamer::GStreamer INTERFACE "-F /Library/Frameworks -framework GStreamer")
    elseif(IOS)
        target_link_libraries(GStreamer::GStreamer INTERFACE "-F ~/Library/Developer/GStreamer/iPhone.sdk -framework GStreamer -framework AVFoundation -framework CoreMedia -framework CoreVideo -framework VideoToolbox -liconv -lresolv")
    endif()
    target_include_directories(GStreamer::GStreamer INTERFACE "${GSTREAMER_INCLUDE_PATH}/Headers")
    return()
endif()

target_link_directories(GStreamer::GStreamer INTERFACE ${GSTREAMER_LIB_PATH})

################################################################################

# TODO: https://gstreamer.freedesktop.org/documentation/qt6d3d11/index.html#qml6d3d11sink-page

add_library(GStreamer::Plugins INTERFACE IMPORTED)
target_link_directories(GStreamer::Plugins INTERFACE ${GSTREAMER_PLUGIN_PATH})

set(GST_PLUGINS
    gstapp
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
    gstva
    gstvaapi
    gstvideoparsersbad
    gstx264
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

target_link_libraries(GStreamer::GStreamer INTERFACE GStreamer::Plugins)

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
