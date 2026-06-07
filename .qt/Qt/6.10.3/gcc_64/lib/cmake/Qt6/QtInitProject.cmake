# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
cmake_minimum_required(VERSION 3.16)

if(NOT PROJECT_DIR)
    set(PROJECT_DIR "${CMAKE_SOURCE_DIR}")
endif()

get_filename_component(project_name "${PROJECT_DIR}" NAME)

get_filename_component(project_abs_dir "${PROJECT_DIR}" ABSOLUTE)
if(NOT IS_DIRECTORY "${project_abs_dir}")
    message(FATAL_ERROR "Unable to scan ${project_abs_dir}. The directory doesn't exist.")
endif()

set(known_extensions "")
set(types "")

# The function allows extending the capabilities of this script and establishes simple relation
# chains between the file types.
# Option Arguments:
#     EXPERIMENTAL
#         Marks that the support of the following files is experimental and the required Qt modules
#         are in Technical preview state.
#     DEPRECATED
#         Marks that the support of the following files will be discontinued soon and the required
#         Qt modules are deprecated.
# One-value Arguments:
#     TEMPLATE
#         The CMake code template. Use the '@files@' string for the files substitution.
# Multi-value Arguments:
#     EXTENSIONS
#         List of the file extensions treated as this source 'type'.
#     MODULES
#         List of Qt modules required for these file types.
#     DEPENDS
#         The prerequisite source 'type' needed by this source 'type'
macro(handle_type type)
    cmake_parse_arguments(arg
        "EXPERIMENTAL;DEPRECATED"
        "TEMPLATE"
        "EXTENSIONS;MODULES;DEPENDS"
        ${ARGN}
    )

    if(NOT arg_EXTENSIONS)
        message(FATAL_ERROR "Unexpected call handle_type of with no EXTENSIONS specified."
            " This is the Qt issue, please report a bug at https://bugreports.qt.io.")
    endif()
    set(unique_extensions_subset "${known_extensions}")
    list(REMOVE_ITEM unique_extensions_subset ${arg_EXTENSIONS})
    if(NOT "${known_extensions}" STREQUAL "${unique_extensions_subset}")
        message(FATAL_ERROR "${type} contains duplicated extensions, this is not supported."
            " This is the Qt issue, please report a bug at https://bugreports.qt.io.")
    endif()
    set(${type}_file_extensions "${arg_EXTENSIONS}")

    if(NOT arg_TEMPLATE)
        message(FATAL_ERROR "Unexpected call handle_type of with no TEMPLATE specified."
            " This is the Qt issue, please report a bug at https://bugreports.qt.io.")
    endif()
    set(${type}_template "${arg_TEMPLATE}")

    if(arg_MODULES)
        set(${type}_required_modules "${arg_MODULES}")
    endif()

    list(APPEND types ${type})

    if(arg_EXPERIMENTAL)
        set(${type}_is_experimental TRUE)
    else()
        set(${type}_is_experimental FALSE)
    endif()

    if(arg_DEPRECATED)
        set(${type}_is_deprecated TRUE)
    else()
        set(${type}_is_deprecated FALSE)
    endif()

    if(arg_DEPENDS)
        set(${type}_dependencies ${arg_DEPENDS})
    endif()
endmacro()

handle_type(cpp EXTENSIONS .c .cc .cpp .cxx .h .hh .hxx .hpp MODULES Core TEMPLATE
"\n\nqt_add_executable(${project_name}
    @files@
)"
)

handle_type(qml EXTENSIONS .qml .js .mjs MODULES Gui Qml Quick TEMPLATE
"\n\nqt_add_qml_module(${project_name}
    URI ${project_name}
    QML_FILES
        @files@
)
set_property(TARGET ${project_name} PROPERTY RUNTIME_OUTPUT_NAME \"${project_name}app\")
"
)

handle_type(ui EXTENSIONS .ui MODULES Gui Widgets DEPENDS cpp TEMPLATE
"\n\ntarget_sources(${project_name}
    PRIVATE
        @files@
)"
)

handle_type(qrc EXTENSIONS .qrc DEPENDS cpp TEMPLATE
"\n\nqt_add_resources(${project_name}_resources @files@)
target_sources(${project_name}
    PRIVATE
        \\\${${project_name}_resources}
)"
)

handle_type(protobuf EXPERIMENTAL EXTENSIONS .proto MODULES Protobuf Grpc TEMPLATE
"\n\nqt_add_protobuf(${project_name}
    GENERATE_PACKAGE_SUBFOLDERS
    PROTO_FILES
        @files@
)"
)

set(extra_packages "")
file(GLOB_RECURSE files RELATIVE "${project_abs_dir}" "${project_abs_dir}/*")
foreach(f IN LISTS files)
    get_filename_component(file_extension "${f}" LAST_EXT)
    string(TOLOWER "${file_extension}" file_extension)

    foreach(type IN LISTS types)
        if(file_extension IN_LIST ${type}_file_extensions)
            list(APPEND ${type}_sources "${f}")
            list(APPEND packages ${${type}_required_modules})
            if(${type}_is_experimental)
                message("We found files with the following extensions in your directory:"
                    " ${${type}_file_extensions}\n"
                    "Note that the modules ${${type}_required_modules} are"
                    " in the technical preview state.")
            endif()
            if(${type}_is_deprecated)
                message("We found files with the following extensions in your directory:"
                    " ${${type}_file_extensions}\n"
                    "Note that the modules ${${type}_required_modules} are deprecated.")
            endif()
            break()
        endif()
    endforeach()
endforeach()

if(packages)
    list(REMOVE_DUPLICATES packages)
    list(JOIN packages " " packages_string)
    list(JOIN packages "\n        Qt::" deps_string)
    set(deps_string "Qt::${deps_string}")
endif()

set(content
"cmake_minimum_required(VERSION 3.16)
project(${project_name} LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS ${packages_string})
qt_standard_project_setup(REQUIRES 6.8)"
)

set(has_useful_sources FALSE)
foreach(type IN LISTS types)
    if(${type}_sources)
        set(skip FALSE)
        foreach(dep IN LISTS ${type}_dependencies)
            if(NOT ${dep}_sources)
                set(skip TRUE)
                message("Sources of type ${${type}_file_extensions} cannot live in the project"
                " without ${${dep}_file_extensions} files. Skipping.")
                break()
            endif()
        endforeach()
        if(skip)
            continue()
        endif()

        set(has_useful_sources TRUE)
        string(REGEX MATCH "( +)@files@" unused "${${type}_template}")
        list(JOIN ${type}_sources "\n${CMAKE_MATCH_1}" ${type}_sources)
        string(REPLACE "@files@" "${${type}_sources}" ${type}_content
            "${${type}_template}")
        string(APPEND content "${${type}_content}")
    endif()
endforeach()

string(APPEND content "\n\ntarget_link_libraries(${project_name}
    PRIVATE
        ${deps_string}
)\n"
)

if(EXISTS "${project_abs_dir}/CMakeLists.txt")
    message(FATAL_ERROR "Project is already initialized in current directory."
        " Please remove CMakeLists.txt if you want to regenerate the project.")
endif()

if(NOT has_useful_sources)
    message(FATAL_ERROR "Could not find any files to generate the project.")
endif()
file(WRITE "${project_abs_dir}/CMakeLists.txt" "${content}")

message("The project file is successfully generated. To build the project run:"
    "\nmkdir build"
    "\ncd build"
    "\nqt-cmake ${project_abs_dir}"
    "\ncmake --build ."
)
