# Library resolution and link-time helpers (find-suffix juggling, system
# library passthrough, SRT regex patching, hidden-symbol linking).

# Shared list of system libraries that should be linked by name, not resolved via find_library.
# Used by both FindGStreamer.cmake and the Android mobile-target macro.
if(NOT DEFINED _gst_IGNORED_SYSTEM_LIBRARIES)
    set(_gst_IGNORED_SYSTEM_LIBRARIES c c++ unwind m dl atomic)
    if(ANDROID)
        list(APPEND _gst_IGNORED_SYSTEM_LIBRARIES log GLESv2 EGL OpenSLES android vulkan)
    elseif(APPLE)
        list(APPEND _gst_IGNORED_SYSTEM_LIBRARIES iconv resolv System)
    elseif(WIN32)
        list(APPEND _gst_IGNORED_SYSTEM_LIBRARIES
            ws2_32 ole32 oleaut32 winmm shlwapi secur32 iphlpapi dnsapi
            userenv bcrypt crypt32 advapi32 kernel32 shell32 uuid)
    endif()
endif()
if(NOT DEFINED _gst_SRT_REGEX_PATCH)
    set(_gst_SRT_REGEX_PATCH "^:lib(.+)\\.(a|so|lib|dylib)$")
endif()

# Save/restore macros for CMAKE_FIND_LIBRARY_SUFFIXES/PREFIXES.
# Used by FindGStreamer.cmake and the Android mobile-target macro when resolving static libs.
macro(_gst_save_find_suffixes)
    set(_gst_saved_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(_gst_saved_prefixes ${CMAKE_FIND_LIBRARY_PREFIXES})
    set(CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
    if(GStreamer_USE_STATIC_LIBS)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    elseif(APPLE)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".dylib" ".so" ".tbd")
    elseif(UNIX)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".so")
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")
    endif()
endmacro()

macro(_gst_restore_find_suffixes)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${_gst_saved_suffixes})
    set(CMAKE_FIND_LIBRARY_PREFIXES ${_gst_saved_prefixes})
endmacro()

# _gst_resolve_and_link_libraries(<TARGET> <SCOPE> <LIBS> <HINTS> [HIDE] [WARN_MISSING])
#
# Resolves a list of library names via find_library and links them to TARGET with the given SCOPE
# (PRIVATE, INTERFACE, or PUBLIC). Handles SRT regex patching and system library passthrough.
#
#   HIDE         - Use -hidden-l (Apple) or --exclude-libs (Unix) to hide symbols
#   WARN_MISSING - Warn and skip missing libraries instead of failing
function(_gst_resolve_and_link_libraries _grll_TARGET _grll_SCOPE _grll_LIBS _grll_HINTS)
    cmake_parse_arguments(_grll "HIDE;WARN_MISSING" "" "" ${ARGN})

    if(_grll_HIDE AND APPLE)
        target_link_directories(${_grll_TARGET} ${_grll_SCOPE} ${_grll_HINTS})
    endif()

    _gst_save_find_suffixes()

    foreach(_grll_LIB IN LISTS _grll_LIBS)
        if(_grll_LIB MATCHES "${_gst_SRT_REGEX_PATCH}")
            string(REGEX REPLACE "${_gst_SRT_REGEX_PATCH}" "\\1" _grll_LIB "${_grll_LIB}")
        endif()

        if("${_grll_LIB}" IN_LIST _gst_IGNORED_SYSTEM_LIBRARIES)
            target_link_libraries(${_grll_TARGET} ${_grll_SCOPE} "${_grll_LIB}")
            continue()
        endif()

        string(MAKE_C_IDENTIFIER "_gst_${_grll_LIB}" _grll_CACHE_VAR)
        if(DEFINED ${_grll_CACHE_VAR} AND "${${_grll_CACHE_VAR}}" MATCHES "NOTFOUND$")
            unset(${_grll_CACHE_VAR} CACHE)
        endif()
        if(NOT DEFINED ${_grll_CACHE_VAR} OR "${${_grll_CACHE_VAR}}" MATCHES "NOTFOUND$")
            if(_grll_WARN_MISSING)
                find_library(${_grll_CACHE_VAR} NAMES ${_grll_LIB} HINTS ${_grll_HINTS} NO_DEFAULT_PATH)
            else()
                find_library(${_grll_CACHE_VAR} NAMES ${_grll_LIB} HINTS ${_grll_HINTS}
                    NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)
            endif()
        endif()

        if(NOT ${_grll_CACHE_VAR})
            if(_grll_WARN_MISSING)
                message(WARNING "GStreamer: Library '${_grll_LIB}' not found in ${_grll_HINTS}, skipping")
            endif()
            continue()
        endif()

        if(_grll_HIDE AND APPLE)
            get_filename_component(_grll_NAME_WE "${${_grll_CACHE_VAR}}" NAME_WE)
            string(REGEX REPLACE "^lib" "" _grll_NAME_WE "${_grll_NAME_WE}")
            target_link_libraries(${_grll_TARGET} ${_grll_SCOPE} "-hidden-l${_grll_NAME_WE}")
        elseif(_grll_HIDE AND (UNIX OR ANDROID))
            target_link_libraries(${_grll_TARGET} ${_grll_SCOPE} -Wl,--exclude-libs,${${_grll_CACHE_VAR}})
        else()
            target_link_libraries(${_grll_TARGET} ${_grll_SCOPE} "${${_grll_CACHE_VAR}}")
        endif()
    endforeach()

    _gst_restore_find_suffixes()
endfunction()
