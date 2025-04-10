if(GStreamer_FOUND)
    return()
endif()

if(NOT DEFINED GStreamer_FIND_VERSION)
    if(ANDROID OR IOS OR WIN32)
        set(GStreamer_FIND_VERSION 1.22.12)
    else()
        set(GStreamer_FIND_VERSION 1.20)
    endif()
endif()

if(NOT DEFINED GStreamer_ROOT_DIR)
    if(DEFINED GSTREAMER_ROOT)
        set(GStreamer_ROOT_DIR ${GSTREAMER_ROOT})
    elseif(DEFINED GStreamer_ROOT)
        set(GStreamer_ROOT_DIR ${GStreamer_ROOT})
    endif()

    if(NOT EXISTS "${GStreamer_ROOT_DIR}")
        message(STATUS "The user provided directory GStreamer_ROOT_DIR=${GStreamer_ROOT_DIR} does not exist")
    endif()
endif()

if(NOT DEFINED GStreamer_USE_STATIC_LIBS)
    if(ANDROID OR IOS)
        set(GStreamer_USE_STATIC_LIBS ON)
    else()
        set(GStreamer_USE_STATIC_LIBS OFF)
    endif()
endif()

# if(NOT DEFINED GStreamer_EXTRA_DEPS)
#     set(GStreamer_EXTRA_DEPS)
#     if (DEFINED GSTREAMER_EXTRA_DEPS)
#         set(GStreamer_EXTRA_DEPS ${GSTREAMER_EXTRA_DEPS})
#     endif()
# endif()

set(PKG_CONFIG_ARGN)

################################################################################

if(WIN32)
    if(DEFINED ENV{GSTREAMER_1_0_ROOT_X86_64} AND EXISTS $ENV{GSTREAMER_1_0_ROOT_X86_64})
        set(GStreamer_ROOT_DIR $ENV{GSTREAMER_1_0_ROOT_X86_64})
    elseif(MSVC AND DEFINED ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64} AND EXISTS $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64})
        set(GStreamer_ROOT_DIR $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64})
    elseif(MINGW AND DEFINED ENV{GSTREAMER_1_0_ROOT_MINGW_X86_64} AND EXISTS $ENV{GSTREAMER_1_0_ROOT_MINGW_X86_64})
        set(GStreamer_ROOT_DIR $ENV{GSTREAMER_1_0_ROOT_MINGW_X86_64})
    else()
        if(MSVC)
            set(_gst_target "msvc")
        elseif(MINGW)
            set(_gst_target "mingw")
        else()
            message(FATAL_ERROR "Invalid Compiler Target for GStreamer")
        endif()

        CPMAddPackage(
            NAME gstreamer-runtime
            VERSION ${GStreamer_FIND_VERSION}
            URL "https://gstreamer.freedesktop.org/data/pkg/windows/${GStreamer_FIND_VERSION}/${_gst_target}/gstreamer-1.0-${_gst_target}-x86_64-${GStreamer_FIND_VERSION}.msi"
        )
        CPMAddPackage(
            NAME gstreamer-dev
            VERSION ${GStreamer_FIND_VERSION}
            URL "https://gstreamer.freedesktop.org/data/pkg/windows/${GStreamer_FIND_VERSION}/${_gst_target}/gstreamer-1.0-devel-${_gst_target}-x86_64-${GStreamer_FIND_VERSION}.msi"
        )

        # set(GStreamer_ROOT_DIR "${gstreamer-runtime_SOURCE_DIR}")
        set(GStreamer_ROOT_DIR "${gstreamer-dev_SOURCE_DIR}")
    endif()

    # cmake_path(CONVERT "${GStreamer_ROOT_DIR}" TO_CMAKE_PATH_LIST GStreamer_ROOT_DIR NORMALIZE)
    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    set(GSTREAMER_INCLUDE_PATH "${GStreamer_ROOT_DIR}/include")
    set(ENV{PKG_CONFIG} "${GStreamer_ROOT_DIR}/bin/pkg-config")
    set(PKG_CONFIG_EXECUTABLE "$ENV{PKG_CONFIG}/pkg-config.exe")
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_LIB_PATH}/pkgconfig;${GSTREAMER_PLUGIN_PATH}/pkgconfig;$ENV{PKG_CONFIG_PATH}")
    # set(CMAKE_PROGRAM_PATH "${GStreamer_ROOT_DIR}/bin")
    # cmake_path(CONVERT "${GStreamer_ROOT_DIR}" TO_CMAKE_PATH_LIST PREFIX_PATH NORMALIZE)
    # cmake_path(CONVERT "${GSTREAMER_LIB_PATH}" TO_CMAKE_PATH_LIST LIBDIR_PATH NORMALIZE)
    # cmake_path(CONVERT "${GSTREAMER_INCLUDE_PATH}" TO_CMAKE_PATH_LIST INCLUDE_PATH NORMALIZE)
    list(APPEND PKG_CONFIG_ARGN
        --dont-define-prefix
        --define-variable=prefix=${GStreamer_ROOT_DIR}
        --define-variable=libdir=${GSTREAMER_LIB_PATH}
        --define-variable=includedir=${GSTREAMER_INCLUDE_PATH}
    )
elseif(LINUX)
    set(GStreamer_ROOT_DIR "/usr")
    if(EXISTS "${GStreamer_ROOT_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu")
        set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu") # ${CMAKE_LIBRARY_ARCHITECTURE}
    else()
        set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    endif()
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    set(GSTREAMER_INCLUDE_PATH "${GStreamer_ROOT_DIR}/include")
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_LIB_PATH}/pkgconfig:$ENV{PKG_CONFIG_PATH}")
elseif(ANDROID)
    # https://gstreamer.freedesktop.org/data/pkg/android/${GStreamer_FIND_VERSION}/gstreamer-1.0-android-universal-${GStreamer_FIND_VERSION}.tar.xz.sha256sum
    CPMAddPackage(
        NAME gstreamer
        VERSION ${GStreamer_FIND_VERSION}
        URL https://gstreamer.freedesktop.org/data/pkg/android/${GStreamer_FIND_VERSION}/gstreamer-1.0-android-universal-${GStreamer_FIND_VERSION}.tar.xz
        # URL_HASH be92cf477d140c270b480bd8ba0e26b1e01c8db042c46b9e234d87352112e485
    )

    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
        set(GStreamer_ROOT_DIR "${gstreamer_SOURCE_DIR}/armv7")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set(GStreamer_ROOT_DIR "${gstreamer_SOURCE_DIR}/arm64")
    elseif(CMAKE_ANDROID_ARCH_ABI} STREQUAL "x86")
        set(GStreamer_ROOT_DIR "${gstreamer_SOURCE_DIR}/x86")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
        set(GStreamer_ROOT_DIR "${gstreamer_SOURCE_DIR}/x86_64")
    endif()
    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    set(GSTREAMER_INCLUDE_PATH "${GStreamer_ROOT_DIR}/include")

    set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH OFF)
    set(ENV{PKG_CONFIG_PATH} "")
    if(CMAKE_HOST_WIN32)
        set(ENV{PKG_CONFIG} "${GStreamer_ROOT_DIR}/share/gst-android/ndk-build/tools/windows/pkg-config")
        set(PKG_CONFIG_EXECUTABLE "$ENV{PKG_CONFIG}/pkg-config.exe")
        set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_LIB_PATH}/pkgconfig;${GSTREAMER_PLUGIN_PATH}/pkgconfig")
    elseif(CMAKE_HOST_UNIX)
        set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_LIB_PATH}/pkgconfig:${GSTREAMER_PLUGIN_PATH}/pkgconfig")
    endif()

    list(APPEND PKG_CONFIG_ARGN
        --dont-define-prefix
        --define-variable=prefix=${GStreamer_ROOT_DIR}
        --define-variable=libdir=${GSTREAMER_LIB_PATH}
        --define-variable=includedir=${GSTREAMER_INCLUDE_PATH}
    )
elseif(MACOS)
    set(CMAKE_FIND_FRAMEWORK ON)
    list(APPEND CMAKE_FRAMEWORK_PATH "/Library/Frameworks")
    set(GSTREAMER_FRAMEWORK_PATH "/Library/Frameworks/GStreamer.framework" CACHE PATH "GStreamer Framework Path")
    set(GStreamer_ROOT_DIR "${GSTREAMER_FRAMEWORK_PATH}/Versions/1.0")
    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    set(GSTREAMER_INCLUDE_PATH "${GSTREAMER_FRAMEWORK_PATH}/Headers")
    set(ENV{PKG_CONFIG} "${GStreamer_ROOT_DIR}/bin")
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_LIB_PATH}/pkgconfig:${GSTREAMER_PLUGIN_PATH}/pkgconfig:$ENV{PKG_CONFIG_PATH}")
elseif(IOS)
    # CPMAddPackage(
    #     NAME gstreamer
    #     VERSION ${GStreamer_FIND_VERSION}
    #     URL "https://gstreamer.freedesktop.org/data/pkg/ios/${GStreamer_FIND_VERSION}/gstreamer-1.0-devel-${GStreamer_FIND_VERSION}-ios-universal.pkg"
    # )

    # set(CMAKE_FIND_FRAMEWORK ON)
    # list(APPEND CMAKE_FRAMEWORK_PATH "~/Library/Developer/GStreamer/iPhone.sdk")
    # set(GSTREAMER_FRAMEWORK_PATH "~/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
    # set(GSTREAMER_INCLUDE_PATH "${GSTREAMER_FRAMEWORK_PATH}/Headers")

    message(FATAL_ERROR "GStreamer for iOS is Currently Unsupported.")
endif()

# cmake_print_variables(GStreamer_ROOT_DIR GSTREAMER_LIB_PATH GSTREAMER_PLUGIN_PATH GSTREAMER_INCLUDE_PATH)

################################################################################

if(GStreamer_USE_STATIC_LIBS)
    list(APPEND PKG_CONFIG_ARGN "--static")
endif()

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(ENV{PKG_CONFIG_DONT_DEFINE_PREFIX} 1)
endif()

# message(STATUS "PKG_CONFIG $ENV{PKG_CONFIG}")
# message(STATUS "PKG_CONFIG_PATH $ENV{PKG_CONFIG_PATH}")
# message(STATUS "PKG_CONFIG_LIBDIR $ENV{PKG_CONFIG_LIBDIR}")
find_package(PkgConfig REQUIRED QUIET)

# cmake_print_variables(PKG_CONFIG_EXECUTABLE PKG_CONFIG_ARGN)

list(PREPEND CMAKE_PREFIX_PATH ${GStreamer_ROOT_DIR})
pkg_check_modules(GStreamer REQUIRED gstreamer-1.0) # QUIET
# cmake_print_variables(GStreamer_VERSION)

################################################################################

string(REPLACE "." ";" GST_VERSION_LIST ${GStreamer_VERSION})
list(GET GST_VERSION_LIST 0 GST_VERSION_MAJOR)
list(GET GST_VERSION_LIST 1 GST_VERSION_MINOR)
list(GET GST_VERSION_LIST 2 GST_VERSION_PATCH)

# Use Latest Revisions for each minor version: 1.16.3, 1.18.6, 1.20.7, 1.22.12, 1.24.12, 1.26.0
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
elseif(GST_VERSION_MINOR EQUAL 26)
    set(GST_VERSION_PATCH 0)
endif()

set(GST_PLUGINS_VERSION ${GST_VERSION_MAJOR}.${GST_VERSION_MINOR}.${GST_VERSION_PATCH})
# cmake_print_variables(GST_PLUGINS_VERSION)

################################################################################

function(find_gstreamer_component component pkgconfig_name)
    set(target GStreamer::${component})

    if(NOT TARGET ${target})
        string(TOUPPER ${component} upper)
        pkg_check_modules(PC_GSTREAMER_${upper} IMPORTED_TARGET ${pkgconfig_name}>=${GStreamer_FIND_VERSION} ) # QUIET
        if(TARGET PkgConfig::PC_GSTREAMER_${upper})
            qt_add_library(GStreamer::${component} INTERFACE IMPORTED)
            target_link_libraries(GStreamer::${component} INTERFACE PkgConfig::PC_GSTREAMER_${upper})
            # set_target_properties(GStreamer::${component} PROPERTIES VERSION ${PC_GSTREAMER_${upper}_VERSION})
        endif()
        mark_as_advanced(GStreamer_${component}_INCLUDE_DIR GStreamer_${component}_LIBRARY)
    endif()

    if(TARGET ${target})
        set(GStreamer_${component}_FOUND TRUE PARENT_SCOPE)
        # get_target_property(Component_VERSION GStreamer::${component} VERSION)
        # set(GStreamer_${component}_VERSION ${Component_VERSION} PARENT_SCOPE)
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
    REQUIRED_VARS
        GStreamer_Core_FOUND
        GStreamer_Base_FOUND
        GStreamer_Video_FOUND
        GStreamer_Gl_FOUND
    VERSION_VAR GStreamer_VERSION
    HANDLE_COMPONENTS
)

if(GStreamer_FOUND AND NOT TARGET GStreamer::GStreamer)
    qt_add_library(GStreamer::GStreamer INTERFACE IMPORTED)
    # set_target_properties(GStreamer::GStreamer PROPERTIES VERSION ${GStreamer_VERSION})

    if(APPLE)
        find_library(GSTREAMER_FRAMEWORK GStreamer PATHS ${GSTREAMER_FRAMEWORK_PATH})
        if(GSTREAMER_FRAMEWORK)
            if(MACOS)
                target_link_libraries(GStreamer::GStreamer INTERFACE ${GSTREAMER_FRAMEWORK})
                target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_MACOS_FRAMEWORK)
            elseif(IOS)
                target_link_libraries(GStreamer::GStreamer INTERFACE "-F ~/Library/Developer/GStreamer/iPhone.sdk -framework GStreamer -framework AVFoundation -framework CoreMedia -framework CoreVideo -framework VideoToolbox -liconv -lresolv")
            endif()
            target_include_directories(GStreamer::GStreamer INTERFACE "${GSTREAMER_INCLUDE_PATH}")
            return()
        endif()
    elseif(ANDROID)
        target_link_options(GStreamer::GStreamer INTERFACE "-Wl,-Bsymbolic")
        if(${CMAKE_ANDROID_ARCH_ABI} MATCHES "armeabi-v7a;x86")
            target_link_options(GStreamer::GStreamer INTERFACE "-Wl,-z,notext")
        endif()
    endif()

    target_link_libraries(GStreamer::GStreamer
        INTERFACE
            GStreamer::Core
            GStreamer::Base
            GStreamer::Video
            GStreamer::Gl
    )

    foreach(component IN LISTS GStreamer_FIND_COMPONENTS)
        if(GStreamer_${component}_FOUND)
            target_link_libraries(GStreamer::GStreamer INTERFACE GStreamer::${component})
        endif()
    endforeach()

    target_link_directories(GStreamer::GStreamer INTERFACE ${GSTREAMER_LIB_PATH})

    if(GStreamer_USE_STATIC_LIBS)
        target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_STATIC_BUILD)
    endif()
endif()

################################################################################

# TODO: https://gstreamer.freedesktop.org/documentation/qt6d3d11/index.html#qml6d3d11sink-page

qt_add_library(GStreamer::Plugins INTERFACE IMPORTED)
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
