function(qgc_set_qt_resource_alias)
    foreach(resource_file IN LISTS ARGN)
        get_filename_component(alias "${resource_file}" NAME)
        set_source_files_properties(
            "${resource_file}"
            PROPERTIES QT_RESOURCE_ALIAS "${alias}"
        )
    endforeach()
endfunction()

function(qgc_config_caching)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        execute_process(
            COMMAND "${CCACHE_PROGRAM}" --version
            RESULT_VARIABLE CCACHE_OK
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if(CCACHE_OK EQUAL 0)
            message(STATUS "QGC: Using ccache ${CCACHE_PROGRAM}")

            # set(ENV{CCACHE_CONFIGPATH} "${CMAKE_SOURCE_DIR}/tools/ccache.conf")
            # set(ENV{CCACHE_DIR} "${CMAKE_SOURCE_DIR}/.ccache")
            set(ENV{CCACHE_BASEDIR} "${CMAKE_SOURCE_DIR}")
            set(ENV{CCACHE_COMPRESS} "true")
            set(ENV{CCACHE_COMPRESSLEVEL "6")
            # set(ENV{CCACHE_NOHASHDIR} "true")
            # set(ENV{CCACHE_COMPILERCHECK} "content")
            set(ENV{CCACHE_SLOPPINESS} "pch_defines,time_macros,include_file_mtime,include_file_ctime")

            set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "C compiler launcher")
            set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "CXX compiler launcher")
            set(CMAKE_C_LINKER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "C linker cache used")
            set(CMAKE_CXX_LINKER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "CXX linker cache used")

            if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                add_compile_options(-Xclang -fno-pch-timestamp)
            endif()
        endif()
    else()
        find_program(SCCACHE_PROGRAM sccache)
        if(SCCACHE_PROGRAM)
            execute_process(
                COMMAND "${SCCACHE_PROGRAM}" --version
                RESULT_VARIABLE SCCACHE_OK
                OUTPUT_QUIET
                ERROR_QUIET
            )
            if(SCCACHE_OK EQUAL 0)
                message(STATUS "QGC: Using sccache ${SCCACHE_PROGRAM}")

                set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}" CACHE STRING "C compiler launcher")
                # set(CMAKE_C_LINKER_LAUNCHER "${SCCACHE_PROGRAM}" CACHE STRING "C linker cache used")
                set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}" CACHE STRING "CXX compiler launcher")
                # set(CMAKE_CXX_LINKER_LAUNCHER "${SCCACHE_PROGRAM}" CACHE STRING "CXX linker cache used")
            endif()
        endif()
    endif()
endfunction()

function(qgc_set_linker)
    foreach(linker mold lld gold)
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -fuse-ld=${linker} -Wl,--version
            ERROR_QUIET OUTPUT_VARIABLE _linker_version
        )
        if("${_linker_version}" MATCHES "LLD")
            add_link_options(-fuse-ld=lld)
            message(STATUS "QGC: Using lld linker. (${_linker_version})")
            break()
        elseif("${_linker_version}" MATCHES "GNU gold")
            add_link_options(-fuse-ld=gold)
            message(STATUS "QGC: Using GNU gold linker. (${_linker_version})")
            break()
        elseif("${_linker_version}" MATCHES "mold")
            add_link_options(-fuse-ld=mold)
            message(STATUS "QGC: Using mold linker. (${_linker_version})")
            break()
        endif()
    endforeach()
endfunction()
