# ----------------------------------------------------------------------------
# QGroundControl CMake Helper Functions
# ----------------------------------------------------------------------------

if(QGC_HELPERS_INCLUDED)
    return()
endif()
set(QGC_HELPERS_INCLUDED TRUE)

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

    find_program(QGC_CACHE_PROGRAM
        NAMES ccache sccache
        VALIDATOR _qgc_verify_cache_tool
    )

    if(QGC_CACHE_PROGRAM)
        get_filename_component(_cache_tool "${QGC_CACHE_PROGRAM}" NAME_WE)
        message(STATUS "QGC: Using ${_cache_tool} (${QGC_CACHE_PROGRAM})")
        string(TOLOWER "${_cache_tool}" _cache_tool)

        if(_cache_tool STREQUAL "ccache")
            # Use config file for static settings (max_size, compression, sloppiness)
            set(ENV{CCACHE_CONFIGPATH} "${CMAKE_SOURCE_DIR}/tools/ccache.conf")
            # Dynamic settings that need CMAKE_SOURCE_DIR
            set(ENV{CCACHE_DIR} "${CMAKE_SOURCE_DIR}/.cache/ccache")
            set(ENV{CCACHE_BASEDIR} "${CMAKE_SOURCE_DIR}")
            if(APPLE)
                set(ENV{CCACHE_COMPILERCHECK} "content")
            endif()
        elseif(_cache_tool STREQUAL "sccache")
            # set(ENV{SCCACHE_PATH} "")
            # set(ENV{SCCACHE_DIR} "")
            # set(ENV{SCCACHE_SERVER_PORT} "")
            # set(ENV{SCCACHE_SERVER_UDS} "")
            # set(ENV{SCCACHE_IGNORE_SERVER_IO_ERROR} "")
            # set(ENV{SCCACHE_C_CUSTOM_CACHE_BUSTER} "")
            # set(ENV{SCCACHE_RECACHE} "")
            # set(ENV{SCCACHE_ERROR_LOG} "")
            # set(ENV{SCCACHE_LOG} "")
            # set(ENV{SCCACHE_CACHE_SIZE} "")
            # set(ENV{SCCACHE_IDLE_TIMEOUT} "")
        else()
            return()
        endif()

        set(CMAKE_C_COMPILER_LAUNCHER "${QGC_CACHE_PROGRAM}" CACHE STRING "C compiler launcher" FORCE)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${QGC_CACHE_PROGRAM}" CACHE STRING "CXX compiler launcher" FORCE)
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
    include(CheckLinkerFlag)

    # Try linkers in order of preference: mold > lld > gold
    foreach(_ld mold lld gold)
        set(_flag "LINKER:-fuse-ld=${_ld}")
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
# qgc_enable_ipo
# Enables Interprocedural Optimization (IPO/LTO) for Release builds
# ----------------------------------------------------------------------------
function(qgc_enable_ipo)
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
