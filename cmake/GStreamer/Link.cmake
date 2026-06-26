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

# CMake's FindPkgConfig can split pkg-config paths containing escaped spaces
# (for example `C:/Program\ Files/...`) into multiple list items. Rejoin adjacent
# tokens when they form a real path, preserving unresolved tokens so callers can
# still warn or fall back normally.
function(_gst_coalesce_existing_paths _gcep_VAR)
    set(_gcep_in ${${_gcep_VAR}})
    set(_gcep_out "")
    list(LENGTH _gcep_in _gcep_len)
    set(_gcep_i 0)
    while(_gcep_i LESS _gcep_len)
        list(GET _gcep_in ${_gcep_i} _gcep_candidate)
        set(_gcep_joined "${_gcep_candidate}")
        set(_gcep_found FALSE)
        if(EXISTS "${_gcep_joined}")
            set(_gcep_found TRUE)
        else()
            math(EXPR _gcep_j "${_gcep_i} + 1")
            while(_gcep_j LESS _gcep_len)
                list(GET _gcep_in ${_gcep_j} _gcep_next)
                string(APPEND _gcep_joined " ${_gcep_next}")
                if(EXISTS "${_gcep_joined}")
                    set(_gcep_i ${_gcep_j})
                    set(_gcep_found TRUE)
                    break()
                endif()
                math(EXPR _gcep_j "${_gcep_j} + 1")
            endwhile()
        endif()

        if(_gcep_found)
            list(APPEND _gcep_out "${_gcep_joined}")
        else()
            list(APPEND _gcep_out "${_gcep_candidate}")
        endif()
        math(EXPR _gcep_i "${_gcep_i} + 1")
    endwhile()
    set(${_gcep_VAR} "${_gcep_out}" PARENT_SCOPE)
endfunction()

# Strip link libs that the prebuilt macOS GStreamer distribution references but does not ship.
# gstreamer-gl-1.0.pc carries gstvulkan-1.0 in Libs.private, yet no libgstvulkan-1.0 exists on
# macOS — and these names reach INTERFACE_LINK_LIBRARIES verbatim (the shared-lib path bypasses
# _gst_resolve_and_link_libraries), so the bare token hits the linker as 'library not found'.
# Operates on the named list variable in the caller's scope; no-op off APPLE.
function(_gst_strip_macos_absent_link_libs _gsmal_VAR)
    if(NOT APPLE)
        return()
    endif()
    set(_gsmal_absent gstvulkan-1.0 gstvulkan)
    set(_gsmal_out "")
    foreach(_gsmal_lib IN LISTS ${_gsmal_VAR})
        if(NOT _gsmal_lib IN_LIST _gsmal_absent)
            list(APPEND _gsmal_out "${_gsmal_lib}")
        endif()
    endforeach()
    set(${_gsmal_VAR} "${_gsmal_out}" PARENT_SCOPE)
endfunction()

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
    _gst_coalesce_existing_paths(_grll_HINTS)

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

        # ABI/arch must be part of the cache key — Android builds multiple ABIs in
        # one tree and would otherwise reuse the first ABI's resolved .so/.a path.
        set(_grll_ABI_TAG "")
        if(ANDROID AND CMAKE_ANDROID_ARCH_ABI)
            set(_grll_ABI_TAG "${CMAKE_ANDROID_ARCH_ABI}_")
        endif()
        string(MAKE_C_IDENTIFIER "_gst_${_grll_ABI_TAG}${_grll_LIB}" _grll_CACHE_VAR)
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
            # --exclude-libs keys on the archive basename, not the resolved full path.
            get_filename_component(_grll_EXCL_NAME "${${_grll_CACHE_VAR}}" NAME)
            target_link_libraries(${_grll_TARGET} ${_grll_SCOPE} -Wl,--exclude-libs,${_grll_EXCL_NAME})
        else()
            target_link_libraries(${_grll_TARGET} ${_grll_SCOPE} "${${_grll_CACHE_VAR}}")
        endif()
    endforeach()

    _gst_restore_find_suffixes()
endfunction()

# Emit GST_PLUGIN_STATIC_DECLARE(...) / QGC_REGISTER_STATIC_PLUGIN(...) blocks for
# the gst_static_plugins.c.in template. Shared by the iOS and desktop-static backends.
function(_gst_emit_static_plugin_registration PLUGIN_LIST DECL_OUT REG_OUT)
    set(_decl "")
    set(_reg  "")
    foreach(_p IN LISTS ${PLUGIN_LIST})
        string(APPEND _decl "GST_PLUGIN_STATIC_DECLARE(${_p});\n")
        string(APPEND _reg  "    QGC_REGISTER_STATIC_PLUGIN(${_p});\n")
    endforeach()
    set(${DECL_OUT} "${_decl}" PARENT_SCOPE)
    set(${REG_OUT}  "${_reg}"  PARENT_SCOPE)
endfunction()
