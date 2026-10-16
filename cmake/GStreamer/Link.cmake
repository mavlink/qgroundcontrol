# Library resolution and link-time helpers (find-suffix juggling, system
# library passthrough, SRT regex patching, hidden-symbol linking).

include_guard(GLOBAL)

# Shared list of system libraries that should be linked by name, not resolved via find_library.
# Used by both Targets.cmake and the Android mobile-target macro.
set(_gst_IGNORED_SYSTEM_LIBRARIES c c++ unwind m dl atomic)
if(ANDROID)
    list(
        APPEND
        _gst_IGNORED_SYSTEM_LIBRARIES
        log
        GLESv2
        EGL
        OpenSLES
        android
        vulkan
    )
elseif(APPLE)
    list(APPEND _gst_IGNORED_SYSTEM_LIBRARIES iconv resolv System)
elseif(WIN32)
    list(
        APPEND
        _gst_IGNORED_SYSTEM_LIBRARIES
        ws2_32
        ole32
        oleaut32
        winmm
        shlwapi
        secur32
        iphlpapi
        dnsapi
        userenv
        bcrypt
        crypt32
        advapi32
        kernel32
        shell32
        uuid
    )
endif()
set(_gst_SRT_REGEX_PATCH "^:lib(.+)\\.(a|so|lib|dylib)$")

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
    set(${_gcep_VAR}
        "${_gcep_out}"
        PARENT_SCOPE
    )
endfunction()

# Some Windows pkg-config builds split `C:/Program\ Files/...` across different
# FindPkgConfig output variables: the path list gets `C:/Program`, while the
# suffixes land in CFLAGS_OTHER/LDFLAGS_OTHER as `Files/...`. Rebuild paths only
# when root + suffix exists, then remove the consumed suffixes from options. Without pairs, it repairs the include and
# library directories of the plain and STATIC_ results.
function(_gst_recover_split_pkgconfig_paths _grspp_PREFIX)
    # One pass rebuilds each path/option pair; splitting it would only pass the pair state around.
    # cmake-lint: disable=R0915
    set(_grspp_pairs ${ARGN})
    if(NOT _grspp_pairs)
        foreach(_grspp_kind IN ITEMS "" STATIC_)
            list(APPEND _grspp_pairs ${_grspp_kind}INCLUDE_DIRS ${_grspp_kind}CFLAGS_OTHER ${_grspp_kind}LIBRARY_DIRS
                 ${_grspp_kind}LDFLAGS_OTHER
            )
        endforeach()
    endif()
    list(LENGTH _grspp_pairs _grspp_len)
    math(EXPR _grspp_remainder "${_grspp_len} % 2")
    if(_grspp_remainder)
        message(FATAL_ERROR "_gst_recover_split_pkgconfig_paths requires path/option suffix pairs")
    endif()

    set(_grspp_i 0)
    while(_grspp_i LESS _grspp_len)
        list(GET _grspp_pairs ${_grspp_i} _grspp_path_suffix)
        math(EXPR _grspp_next_i "${_grspp_i} + 1")
        list(GET _grspp_pairs ${_grspp_next_i} _grspp_option_suffix)

        set(_grspp_path_var "${_grspp_PREFIX}_${_grspp_path_suffix}")
        set(_grspp_option_var "${_grspp_PREFIX}_${_grspp_option_suffix}")
        if(DEFINED ${_grspp_path_var} AND DEFINED ${_grspp_option_var})
            set(_grspp_paths ${${_grspp_path_var}})
            set(_grspp_options ${${_grspp_option_var}})
            _gst_coalesce_existing_paths(_grspp_paths)

            set(_grspp_recovered_paths "")
            set(_grspp_consumed_options "")
            foreach(_grspp_path IN LISTS _grspp_paths)
                if(EXISTS "${_grspp_path}")
                    list(APPEND _grspp_recovered_paths "${_grspp_path}")
                    continue()
                endif()

                set(_grspp_recovered FALSE)
                foreach(_grspp_option IN LISTS _grspp_options)
                    if(_grspp_option MATCHES "^-")
                        continue()
                    endif()
                    set(_grspp_joined "${_grspp_path} ${_grspp_option}")
                    if(EXISTS "${_grspp_joined}")
                        list(APPEND _grspp_recovered_paths "${_grspp_joined}")
                        list(APPEND _grspp_consumed_options "${_grspp_option}")
                        set(_grspp_recovered TRUE)
                    endif()
                endforeach()
                if(NOT _grspp_recovered)
                    list(APPEND _grspp_recovered_paths "${_grspp_path}")
                endif()
            endforeach()

            list(REMOVE_DUPLICATES _grspp_recovered_paths)
            set(_grspp_filtered_options "")
            foreach(_grspp_option IN LISTS _grspp_options)
                if(NOT _grspp_option IN_LIST _grspp_consumed_options)
                    list(APPEND _grspp_filtered_options "${_grspp_option}")
                endif()
            endforeach()

            set(${_grspp_path_var}
                "${_grspp_recovered_paths}"
                PARENT_SCOPE
            )
            set(${_grspp_option_var}
                "${_grspp_filtered_options}"
                PARENT_SCOPE
            )
        endif()

        math(EXPR _grspp_i "${_grspp_i} + 2")
    endwhile()
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
    set(${_gsmal_VAR}
        "${_gsmal_out}"
        PARENT_SCOPE
    )
endfunction()

# _gst_resolve_and_link_libraries(<TARGET> <SCOPE> <LIBS> <HINTS> [HIDE] [WARN_MISSING])
#
# Resolves a list of library names via find_library and links them to TARGET with the given SCOPE
# (PRIVATE, INTERFACE, or PUBLIC). Handles SRT regex patching and system library passthrough.
#
#   HIDE         - Link the libraries with their symbols hidden from the result's exports: -hidden-l (Apple), or
#                  the library plus --exclude-libs,<name> (ELF)
#   WARN_MISSING - Warn and skip missing libraries instead of failing
function(_gst_resolve_and_link_libraries _grll_TARGET _grll_SCOPE _grll_LIBS _grll_HINTS)
    # Each library goes through one resolve/patch/link decision chain; splitting it would scatter that chain.
    # cmake-lint: disable=C0103,R0912,R0915
    cmake_parse_arguments(PARSE_ARGV 4 _grll "HIDE;WARN_MISSING" "" "")
    if(_grll_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "_gst_resolve_and_link_libraries: unknown arguments: ${_grll_UNPARSED_ARGUMENTS}")
    endif()
    _gst_coalesce_existing_paths(_grll_HINTS)

    if(_grll_HIDE AND APPLE)
        target_link_directories(${_grll_TARGET} ${_grll_SCOPE} ${_grll_HINTS})
    endif()

    # The SDK linkage decides which library files to find; this function's scope ends the change.
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
        if(_grll_WARN_MISSING)
            find_library(
                ${_grll_CACHE_VAR}
                NAMES ${_grll_LIB}
                HINTS ${_grll_HINTS}
                NO_DEFAULT_PATH
            )
        else()
            find_library(
                ${_grll_CACHE_VAR}
                NAMES ${_grll_LIB}
                HINTS ${_grll_HINTS}
                NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED
            )
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
            target_link_libraries(${_grll_TARGET} ${_grll_SCOPE} "${${_grll_CACHE_VAR}}")
            target_link_options(${_grll_TARGET} ${_grll_SCOPE} "LINKER:--exclude-libs,${_grll_EXCL_NAME}")
        else()
            target_link_libraries(${_grll_TARGET} ${_grll_SCOPE} "${${_grll_CACHE_VAR}}")
        endif()
    endforeach()

endfunction()
