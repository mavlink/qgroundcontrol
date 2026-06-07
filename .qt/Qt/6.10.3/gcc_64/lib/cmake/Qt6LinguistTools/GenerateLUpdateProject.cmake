# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generate an lupdate project file in JSON format.
#
# This file is to be used in CMake script mode with the following variables set:
#
# IN_FILE: .cmake file that sets lupdate_* variables
# OUT_FILE: lupdate project .json file

cmake_minimum_required(VERSION 3.16)

include("${IN_FILE}")

# Converts a CMake list into a string containing a JSON array
#    a,b,c -> [ "a", "b", "c" ]
function(list_to_json_array srcList jsonList)
    set(quotedList "")
    foreach(entry ${srcList})
        list(APPEND quotedList "\"${entry}\"")
    endforeach()
    list(JOIN quotedList ", " joinedList)
    set(${jsonList} "[ ${joinedList} ]" PARENT_SCOPE)
endfunction()

# Remove all nonexistent files from ARGN and store the result in out_var.
#    filter_nonexistent_files(existing_files foo.txt bar.cpp)
#    -> foo.txt (if foo.txt exists and bar.cpp does not)
function(filter_nonexistent_files out_var)
    # Filter out non-existent (generated) source files
    set(existing_sources "")
    foreach(path IN LISTS ARGN)
        if(EXISTS "${path}")
            list(APPEND existing_sources "${path}")
        endif()
    endforeach()
    set("${out_var}" "${existing_sources}" PARENT_SCOPE)
endfunction()

# Remove source files that are unsuitable input for lupdate.
#    filter_unsuitable_lupdate_input(sources main.cpp foo_de.qm bar.qml whatever_metatypes.json)
#    -> main.cpp bar.qml
function(filter_unsuitable_lupdate_input out_var)
    set(result ${ARGN})
    list(FILTER result EXCLUDE REGEX "\\.(qm|json)$")
    set("${out_var}" "${result}" PARENT_SCOPE)
endfunction()

# Remove files from SOURCES that are in EXCLUDED_DIR.
#    filter_files_in_directory(existing_files
#        EXCLUDED_DIR .../target_autogen
#        SOURCES .../src/foo.ui .../target_autogen/include/ui_foo.h)
#    -> .../src/foo.ui
function(filter_files_in_directory out_var)
    set(no_value_options "")
    set(single_value_options EXCLUDED_DIR)
    set(multi_value_options SOURCES)
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    set(result "")
    foreach(source_file IN LISTS arg_SOURCES)
        file(RELATIVE_PATH relative_path "${arg_EXCLUDED_DIR}" "${source_file}")
        if(IS_ABSOLUTE "${relative_path}" OR (relative_path MATCHES "^\\.\\."))
            list(APPEND result "${source_file}")
        endif()
    endforeach()
    set("${out_var}" "${result}" PARENT_SCOPE)
endfunction()

function(prepare_json_sources out_var)
    set(no_value_options "")
    set(single_value_options AUTOGEN_DIR)
    set(multi_value_options SOURCES)
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    filter_nonexistent_files(sources ${arg_SOURCES})
    filter_unsuitable_lupdate_input(sources ${sources})
    if(DEFINED arg_AUTOGEN_DIR)
        filter_files_in_directory(sources EXCLUDED_DIR ${arg_AUTOGEN_DIR} SOURCES ${sources})
    endif()
    list_to_json_array("${sources}" result)
    set("${out_var}" "${result}" PARENT_SCOPE)
endfunction()

get_filename_component(project_root "${lupdate_project_file}" DIRECTORY)

# Set up a list of all path variables
set(path_variables sources include_paths translations)
if(lupdate_subproject_count GREATER 0)
    foreach(i RANGE 1 ${lupdate_subproject_count})
        list(APPEND path_variables
            subproject${i}_include_paths
            subproject${i}_sources
            subproject${i}_excluded
        )
    endforeach()
endif()

# Remove duplicates in include paths
# Once we require CMake 3.27, replace this with $<LIST:REMOVE_DUPLICATES,list>.
foreach(path_var IN LISTS path_variables)
    if(path_var MATCHES "_include_paths$")
        list(REMOVE_DUPLICATES lupdate_${path_var})
    endif()
endforeach()

# Make relative paths absolute to the project root
foreach(path_var IN LISTS path_variables)
    set(absolute_${path_var} "")
    foreach(path IN LISTS lupdate_${path_var})
        if(path STREQUAL "")
            continue()
        endif()
        if(NOT IS_ABSOLUTE "${path}")
            if(path_var MATCHES "^subproject[0-9]+_")
                set(base_dir "${lupdate_${CMAKE_MATCH_0}source_dir}")
            else()
                set(base_dir "${project_root}")
            endif()
            get_filename_component(path "${path}" ABSOLUTE BASE_DIR "${base_dir}")
        endif()
        list(APPEND absolute_${path_var} "${path}")
    endforeach()
endforeach()

prepare_json_sources(json_sources SOURCES ${absolute_sources})
list_to_json_array("${absolute_include_paths}" json_include_paths)
list_to_json_array("${absolute_translations}" json_translations)

set(content "{
  \"projectFile\": \"${lupdate_project_file}\",
  \"includePaths\": ${json_include_paths},
  \"sources\": ${json_sources},
  \"translations\": ${json_translations},
  \"subProjects\": [
")
if(lupdate_subproject_count GREATER 0)
    foreach(i RANGE 1 ${lupdate_subproject_count})
        prepare_json_sources(json_sources
            AUTOGEN_DIR ${lupdate_subproject${i}_autogen_dir}
            SOURCES ${absolute_subproject${i}_sources}
        )
        list_to_json_array("${absolute_subproject${i}_include_paths}" json_include_paths)
        list_to_json_array("${absolute_subproject${i}_excluded}" json_sources_exclusions)
        string(APPEND content "    {
      \"projectFile\": \"${lupdate_subproject${i}_source_dir}/CMakeLists.txt\",
      \"includePaths\": ${json_include_paths},
      \"sources\": ${json_sources},
      \"excluded\": ${json_sources_exclusions}
    }")
        if(i LESS lupdate_subproject_count)
            string(APPEND content ",")
        endif()
        string(APPEND content "\n")
    endforeach()
endif()
string(APPEND content "  ]
}
")
file(WRITE "${OUT_FILE}" "${content}")
