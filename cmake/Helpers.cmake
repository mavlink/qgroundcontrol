# ----------------------------------------------------------------------------
# QGroundControl CMake Helper Functions
# ----------------------------------------------------------------------------

include_guard(GLOBAL)

# ----------------------------------------------------------------------------
# qgc_set_qt_resource_alias
# Sets Qt resource aliases for files based on their filenames
# Args: List of resource files
# ----------------------------------------------------------------------------
function(qgc_set_qt_resource_alias)
    foreach(resource_file IN LISTS ARGN)
        if(NOT EXISTS "${resource_file}")
            message(WARNING "QGC: Resource file does not exist: ${resource_file}")
            continue()
        endif()
        get_filename_component(alias "${resource_file}" NAME)
        set_source_files_properties("${resource_file}"
            PROPERTIES
                QT_RESOURCE_ALIAS "${alias}"
        )
    endforeach()
endfunction()

# ----------------------------------------------------------------------------
# qgc_config_caching
# Configures compiler caching using ccache or sccache if available
# ----------------------------------------------------------------------------
function(qgc_config_caching)
    function(_qgc_verify_cache_tool _ok _path)
        execute_process(
            COMMAND "${_path}" --version
            RESULT_VARIABLE _res
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if(NOT _res EQUAL 0)
            set(${_ok} FALSE PARENT_SCOPE)
        endif()
    endfunction()

    # Allow presets to prefer a specific backend via QGC_CACHE_BACKEND
    if(QGC_CACHE_BACKEND)
        find_program(QGC_CACHE_PROGRAM
            NAMES "${QGC_CACHE_BACKEND}"
            VALIDATOR _qgc_verify_cache_tool
        )
    endif()
    if(NOT QGC_CACHE_PROGRAM)
        find_program(QGC_CACHE_PROGRAM
            NAMES ccache sccache
            VALIDATOR _qgc_verify_cache_tool
        )
    endif()

    if(QGC_CACHE_PROGRAM)
        get_filename_component(_cache_tool "${QGC_CACHE_PROGRAM}" NAME_WE)
        message(STATUS "QGC: Using ${_cache_tool} (${QGC_CACHE_PROGRAM})")
        string(TOLOWER "${_cache_tool}" _cache_tool)

        if(_cache_tool STREQUAL "ccache")
            set(_ccache_conf "${CMAKE_SOURCE_DIR}/tools/configs/ccache.conf")
            if(WIN32)
                # Windows: set env vars at configure time (inherited by Ninja).
                # Only set defaults so external cache setups (CI/IDE) are not clobbered.
                if(EXISTS "${_ccache_conf}" AND (NOT DEFINED ENV{CCACHE_CONFIGPATH} OR "$ENV{CCACHE_CONFIGPATH}" STREQUAL ""))
                    set(ENV{CCACHE_CONFIGPATH} "${_ccache_conf}")
                endif()
                if(NOT DEFINED ENV{CCACHE_DIR} OR "$ENV{CCACHE_DIR}" STREQUAL "")
                    set(ENV{CCACHE_DIR} "${CMAKE_SOURCE_DIR}/.ccache")
                endif()
                if(NOT DEFINED ENV{CCACHE_BASEDIR} OR "$ENV{CCACHE_BASEDIR}" STREQUAL "")
                    set(ENV{CCACHE_BASEDIR} "${CMAKE_SOURCE_DIR}")
                endif()
                set(_cache_launcher "${QGC_CACHE_PROGRAM}")
            else()
                # Unix: wrapper script to set env vars at build time.
                # Use defaults so external cache setups (CI/IDE) can override.
                set(_ccache_wrapper "${CMAKE_BINARY_DIR}/ccache-launcher")
                set(_wrapper "#!/bin/sh\n")
                if(EXISTS "${_ccache_conf}")
                    string(APPEND _wrapper "export CCACHE_CONFIGPATH=\"\${CCACHE_CONFIGPATH:-${_ccache_conf}}\"\n")
                endif()
                string(APPEND _wrapper "export CCACHE_DIR=\"\${CCACHE_DIR:-${CMAKE_SOURCE_DIR}/.ccache}\"\n")
                string(APPEND _wrapper "export CCACHE_BASEDIR=\"\${CCACHE_BASEDIR:-${CMAKE_SOURCE_DIR}}\"\n")
                string(APPEND _wrapper "exec \"${QGC_CACHE_PROGRAM}\" \"$@\"\n")
                file(WRITE "${_ccache_wrapper}" "${_wrapper}")
                file(CHMOD "${_ccache_wrapper}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
                set(_cache_launcher "${_ccache_wrapper}")
            endif()
        elseif(_cache_tool STREQUAL "sccache")
            set(_cache_launcher "${QGC_CACHE_PROGRAM}")
        else()
            return()
        endif()

        set(CMAKE_C_COMPILER_LAUNCHER "${_cache_launcher}" CACHE STRING "C compiler launcher" FORCE)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${_cache_launcher}" CACHE STRING "CXX compiler launcher" FORCE)
        # Linker launchers not currently used but available if needed
        # set(CMAKE_C_LINKER_LAUNCHER "${QGC_CACHE_PROGRAM}" CACHE STRING "C linker cache")
        # set(CMAKE_CXX_LINKER_LAUNCHER "${QGC_CACHE_PROGRAM}" CACHE STRING "CXX linker cache")

        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_compile_options(-Xclang -fno-pch-timestamp)
        endif()
    else()
        message(WARNING "QGC: No ccache/sccache found - building without a compiler cache")
    endif()
endfunction()

# ----------------------------------------------------------------------------
# qgc_set_linker
# Attempts to use a faster linker (mold, lld, or gold) if available
# Falls back to the system default linker
# ----------------------------------------------------------------------------
function(qgc_set_linker)
    # Cross-compilation toolchains (Android NDK, iOS) configure their own linker
    if(CMAKE_CROSSCOMPILING)
        return()
    endif()

    if(QGC_ENABLE_COVERAGE)
        message(STATUS "QGC: Alternative linker disabled for coverage build")
        return()
    endif()

    include(CheckLinkerFlag)

    # Try linkers in order of preference: mold > lld > gold
    foreach(_ld mold lld gold)
        set(_flag "-fuse-ld=${_ld}")
        check_linker_flag(CXX "${_flag}" HAVE_LD_${_ld})

        if(HAVE_LD_${_ld})
            add_link_options("${_flag}")
            set(QGC_LINKER "${_ld}" PARENT_SCOPE)
            message(STATUS "QGC: Using ${_ld} linker")
            return()
        endif()
    endforeach()

    message(STATUS "QGC: No alternative linker (mold/lld/gold) found - using system default")
endfunction()

# ----------------------------------------------------------------------------
# qgc_enable_pie
# Enables Position Independent Executables (PIE) for improved security
# Note: MSVC/Windows uses ASLR instead of PIE (enabled by default)
# ----------------------------------------------------------------------------
function(qgc_enable_pie)
    if(MSVC)
        return()
    endif()

    include(CheckPIESupported)
    check_pie_supported(OUTPUT_VARIABLE _output LANGUAGES C CXX)

    if(CMAKE_C_LINK_PIE_SUPPORTED)
        set(CMAKE_POSITION_INDEPENDENT_CODE ON PARENT_SCOPE)
        message(STATUS "QGC: PIE enabled")
    else()
        message(WARNING "QGC: PIE not supported - ${_output}")
    endif()
endfunction()

# ----------------------------------------------------------------------------
# qgc_enable_split_dwarf
# Enables -gsplit-dwarf + --gdb-index for the current build.
#
# Split-DWARF emits debug info into sidecar .dwo files instead of embedding
# it in .o; the linker then never reads/relocates DWARF, cutting Debug link
# time by 40-60% on QGC-sized projects. gdb follows the .dwo sidecars
# transparently at debug time.
#
# Only applies on ELF targets (Linux, Android). macOS uses dSYM bundles and
# Windows MSVC uses PDBs — both already separate debug info, so split-DWARF
# is a no-op or actively harmful there. Skipped under IPO/LTO because LTO
# rewrites debug info at link time and defeats the split.
#
# --gdb-index requires gold, lld, or mold; qgc_set_linker() prefers those.
# ----------------------------------------------------------------------------
function(qgc_enable_split_dwarf)
    if(NOT QGC_SPLIT_DWARF)
        return()
    endif()
    if(APPLE OR MSVC OR NOT (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
        return()
    endif()
    if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
        message(STATUS "QGC: split-DWARF disabled (IPO/LTO active)")
        return()
    endif()

    # Per-config so Release (no debug info) is untouched.
    add_compile_options($<$<CONFIG:Debug,RelWithDebInfo>:-gsplit-dwarf>)

    # --gdb-index is a linker feature; only emit when the selected linker
    # supports it. mold/lld/gold do; bfd (system default on older distros)
    # does not. qgc_set_linker() exports QGC_LINKER when it picks one.
    if(QGC_LINKER MATCHES "^(mold|lld|gold)$")
        add_link_options($<$<CONFIG:Debug,RelWithDebInfo>:LINKER:--gdb-index>)
        set(_gdb_index ON)
    else()
        set(_gdb_index OFF)
        message(STATUS "QGC: --gdb-index skipped (needs mold/lld/gold; have '${QGC_LINKER}')")
    endif()

    set(QGC_SPLIT_DWARF_ACTIVE ON PARENT_SCOPE)
    set(QGC_SPLIT_DWARF_GDB_INDEX ${_gdb_index} PARENT_SCOPE)
    message(STATUS "QGC: split-DWARF enabled (-gsplit-dwarf, gdb-index=${_gdb_index})")
endfunction()

# ----------------------------------------------------------------------------
# qgc_enable_ipo
# Enables Interprocedural Optimization (IPO/LTO) for Release builds
# ----------------------------------------------------------------------------
function(qgc_enable_ipo)
    if(LINUX)
        return()
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        include(CheckIPOSupported)
        check_ipo_supported(RESULT _result OUTPUT _output LANGUAGES C CXX)

        if(_result)
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE PARENT_SCOPE)
            message(STATUS "QGC: IPO/LTO enabled for Release build")
        else()
            message(WARNING "QGC: IPO/LTO not supported - ${_output}")
        endif()
    else()
        message(STATUS "QGC: IPO/LTO disabled for ${CMAKE_BUILD_TYPE} build")
    endif()
endfunction()
