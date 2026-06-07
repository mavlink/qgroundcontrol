# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Add a finalizer function for the current CMake list file.
# It will be processed just before leaving the current source directory scope.
#
# When using CMake 3.18 or lower:
# You may add up to nine arguments that are passed to the finalizer.
# A finalizer that is registered with qt_add_list_file_finalizer(foo bar baz)
# will be called with nine arguments: foo(bar baz IGNORE IGNORE IGNORE...),
# because CMake's handling of empty list elements is a cruel joke.
# For CMake < 3.18 the function qt_watch_current_list_dir must know about the finalizer.
#
# When using CMake 3.19 or higher, no more IGNORE parameters are passed. Instead we
# use cmake_language(DEFER CALL) and pass arguments as usual.
# qt_watch_current_list_dir also doesn't need to know about the finalizer
function(qt_add_list_file_finalizer func)
    set(use_cmake_defer_call TRUE)
    if(CMAKE_VERSION VERSION_LESS "3.19.0")
        set(use_cmake_defer_call FALSE)
    endif()

    if(use_cmake_defer_call)
        cmake_language(EVAL CODE "cmake_language(DEFER CALL \"${func}\" ${ARGN}) ")
    else()
        set_property(GLOBAL APPEND
            PROPERTY QT_LIST_FILE_FINALIZER_FILES "${CMAKE_CURRENT_LIST_FILE}")
        set_property(GLOBAL APPEND
            PROPERTY QT_LIST_FILE_FINALIZER_FUNCS ${func})
        foreach(i RANGE 1 9)
            set(arg "${ARGV${i}}")
            if(i GREATER_EQUAL ARGC OR "${arg}" STREQUAL "")
                set(arg "IGNORE")
            endif()
            set_property(GLOBAL APPEND
                PROPERTY QT_LIST_FILE_FINALIZER_ARGV${i} "${arg}")
        endforeach()
    endif()
endfunction()

# Watcher function for the variable CMAKE_CURRENT_LIST_DIR.
# This is the driver of the finalizer facility.
function(qt_watch_current_list_dir variable access value current_list_file stack)
    if(NOT access STREQUAL "MODIFIED_ACCESS")
        # We are only interested in modifications of CMAKE_CURRENT_LIST_DIR.
        return()
    endif()
    list(GET stack -1 stack_top)
    if(stack_top STREQUAL current_list_file)
        # If the top of the stack equals the current list file then
        # we're entering a file. We're not interested in this case.
        return()
    endif()
    get_property(files GLOBAL PROPERTY QT_LIST_FILE_FINALIZER_FILES)
    if(NOT files)
        return()
    endif()
    get_property(funcs GLOBAL PROPERTY QT_LIST_FILE_FINALIZER_FUNCS)
    foreach(i RANGE 1 9)
        get_property(args${i} GLOBAL PROPERTY QT_LIST_FILE_FINALIZER_ARGV${i})
    endforeach()
    list(LENGTH files n)
    set(i 0)
    while(i LESS n)
        list(GET files ${i} file)
        if(file STREQUAL stack_top)
            list(GET funcs ${i} func)
            foreach(k RANGE 1 9)
                list(GET args${k} ${i} a${k})
            endforeach()
            # We've found a file we're looking for. Call the finalizer.
            if(${CMAKE_VERSION} VERSION_LESS "3.18.0")
                # Make finalizer known functions here:
                if(func STREQUAL "qt_finalize_module")
                    qt_finalize_module(${a1} ${a2} ${a3} ${a4} ${a5} ${a6} ${a7} ${a8} ${a9})
                elseif(func STREQUAL "qt_finalize_plugin")
                    qt_finalize_plugin(${a1} ${a2} ${a3} ${a4} ${a5} ${a6} ${a7} ${a8} ${a9})
                elseif(func STREQUAL "qt_internal_finalize_executable")
                    qt_internal_finalize_executable(
                        ${a1} ${a2} ${a3} ${a4} ${a5} ${a6} ${a7} ${a8} ${a9})
                elseif(func STREQUAL "qt_internal_finalize_app")
                    qt_internal_finalize_app(${a1} ${a2} ${a3} ${a4} ${a5} ${a6} ${a7} ${a8} ${a9})
                elseif(func STREQUAL "qt_internal_finalize_tool")
                    qt_internal_finalize_tool(${a1} ${a2} ${a3} ${a4} ${a5} ${a6} ${a7} ${a8} ${a9})
                elseif(func STREQUAL "qt_internal_finalize_3rdparty_library")
                    qt_internal_finalize_3rdparty_library(
                        ${a1} ${a2} ${a3} ${a4} ${a5} ${a6} ${a7} ${a8} ${a9})
                elseif(func STREQUAL "qt_internal_export_additional_targets_file_finalizer")
                    qt_internal_export_additional_targets_file_finalizer(
                        ${a1} ${a2} ${a3} ${a4} ${a5} ${a6} ${a7} ${a8} ${a9})
                elseif(func STREQUAL "_qt_internal_finalize_sbom")
                    _qt_internal_finalize_sbom(
                        ${a1} ${a2} ${a3} ${a4} ${a5} ${a6} ${a7} ${a8} ${a9})
                elseif(func STREQUAL "qt6_finalize_target")
                    qt6_finalize_target(
                        ${a1} ${a2} ${a3} ${a4} ${a5} ${a6} ${a7} ${a8} ${a9})
                else()
                    message(FATAL_ERROR "qt_watch_current_list_dir doesn't know about ${func}. Consider adding it.")
                endif()
            else()
                cmake_language(CALL ${func} ${a1} ${a2} ${a3} ${a4} ${a5} ${a6} ${a7} ${a8} ${a9})
            endif()
            list(REMOVE_AT files ${i})
            list(REMOVE_AT funcs ${i})
            foreach(k RANGE 1 9)
                list(REMOVE_AT args${k} ${i})
            endforeach()
            math(EXPR n "${n} - 1")
        else()
            math(EXPR i "${i} + 1")
        endif()
    endwhile()
    set_property(GLOBAL PROPERTY QT_LIST_FILE_FINALIZER_FILES ${files})
    set_property(GLOBAL PROPERTY QT_LIST_FILE_FINALIZER_FUNCS ${funcs})
    foreach(i RANGE 1 9)
        set_property(GLOBAL PROPERTY QT_LIST_FILE_FINALIZER_ARGV${i} "${args${i}}")
    endforeach()
endfunction()
