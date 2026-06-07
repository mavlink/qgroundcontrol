# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Handles attribution information for a target.
#
# If CREATE_SBOM_FOR_EACH_ATTRIBUTION is set, a separate sbom target is created for each parsed
# attribution entry, and the new targets are added as dependencies to the parent target.
#
# If CREATE_SBOM_FOR_EACH_ATTRIBUTION is not set, the information read from the first attribution
# entry is added directly to the parent target, aka the the values are propagated to the outer
# function scope to be read.. The rest of the attribution entries are created as separate targets
# and added as dependencies, as if the option was passed.
#
# Handles multiple attribution files and entries within a file.
# Attribution files can be specified either via directories and direct file paths.
# If ATTRIBUTION_ENTRY_INDEX is set, only that specific attribution entry will be processed
# from the given attribution file.
function(_qt_internal_sbom_handle_qt_attribution_files out_prefix_outer)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    if(CMAKE_VERSION LESS_EQUAL "3.19")
        message(DEBUG "CMake version is too low, can't parse attribution.json file.")
        return()
    endif()

    set(opt_args "")
    set(single_args "")
    set(multi_args "")

    _qt_internal_get_sbom_specific_options(sbom_opt_args sbom_single_args sbom_multi_args)
    list(APPEND opt_args ${sbom_opt_args})
    list(APPEND single_args ${sbom_single_args})
    list(APPEND multi_args ${sbom_multi_args})

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(attribution_files "")
    set(attribution_file_count 0)

    foreach(attribution_file_path IN LISTS arg_ATTRIBUTION_FILE_PATHS)
        get_filename_component(real_path "${attribution_file_path}" REALPATH)
        list(APPEND attribution_files "${real_path}")
        math(EXPR attribution_file_count "${attribution_file_count} + 1")
    endforeach()

    foreach(attribution_file_dir_path IN LISTS arg_ATTRIBUTION_FILE_DIR_PATHS)
        get_filename_component(real_path
            "${attribution_file_dir_path}/qt_attribution.json" REALPATH)
        list(APPEND attribution_files "${real_path}")
        math(EXPR attribution_file_count "${attribution_file_count} + 1")
    endforeach()

    # If CREATE_SBOM_FOR_EACH_ATTRIBUTION is set, that means the parent target is likely not a
    # 3rd party library, so each attribution entry should create a separate attribution target.
    # In which case we don't want to proagate options like CPE to the child attribution targets,
    # because the CPE is meant for the parent target.
    set(propagate_sbom_options_to_new_attribution_targets TRUE)
    if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_ATTRIBUTION_FILES
            AND arg_ATTRIBUTION_PARENT_TARGET_SBOM_ENTITY_TYPE)
        _qt_internal_sbom_is_qt_entity_type("${arg_ATTRIBUTION_PARENT_TARGET_SBOM_ENTITY_TYPE}"
            parent_is_qt_entity_type)
        if(parent_is_qt_entity_type)
            set(arg_CREATE_SBOM_FOR_EACH_ATTRIBUTION TRUE)
        endif()
    endif()

    if(arg_CREATE_SBOM_FOR_EACH_ATTRIBUTION)
        set(propagate_sbom_options_to_new_attribution_targets FALSE)
        if(NOT arg_ATTRIBUTION_PARENT_TARGET)
            message(FATAL_ERROR "ATTRIBUTION_PARENT_TARGET must be set")
        endif()
    endif()

    if(arg_ATTRIBUTION_ENTRY_INDEX AND attribution_file_count GREATER 1)
        message(FATAL_ERROR
            "ATTRIBUTION_ENTRY_INDEX should only be set if a single attribution "
            "file is specified."
        )
    endif()

    set(ids_to_add "")
    set(ids_found "")
    if(arg_ATTRIBUTION_IDS)
        set(ids_to_add ${arg_ATTRIBUTION_IDS})
    endif()

    set(file_index 0)
    set(first_attribution_processed FALSE)
    foreach(attribution_file_path IN LISTS attribution_files)
        # Collect all processed attribution files to later create a configure-time dependency on
        # them so that the SBOM is regenerated (and CMake is re-ran) if they are modified.
        set_property(GLOBAL APPEND PROPERTY _qt_internal_project_attribution_files
            "${attribution_file_path}")

        # Set a unique out_prefix that will not overlap when multiple entries are processed.
        set(out_prefix_file "${out_prefix_outer}_${file_index}")

        # Get the number of entries in the attribution file.
        _qt_internal_sbom_read_qt_attribution(${out_prefix_file}
            GET_ATTRIBUTION_ENTRY_COUNT
            OUT_VAR_VALUE attribution_entry_count
            FILE_PATH "${attribution_file_path}"
        )

        # If a specific entry was specified, we will only process it from the file.
        if(NOT "${arg_ATTRIBUTION_ENTRY_INDEX}" STREQUAL "")
            set(entry_index ${arg_ATTRIBUTION_ENTRY_INDEX})
        else()
            set(entry_index 0)
        endif()

        # Go through each entry in the attribution file.
        while("${entry_index}" LESS "${${out_prefix_file}_attribution_entry_count}")
            # If this is the first entry to be processed, or if CREATE_SBOM_FOR_EACH_ATTRIBUTION
            # is not set, we read the attribution file entry directly, and propagate the values
            # to the parent scope.
            if(NOT first_attribution_processed AND NOT arg_CREATE_SBOM_FOR_EACH_ATTRIBUTION)
                # Set a prefix without indices, so that the parent scope add_sbom call can
                # refer to the values directly with the outer prefix, without any index infix.
                set(out_prefix "${out_prefix_outer}")

                _qt_internal_sbom_read_qt_attribution(${out_prefix}
                    GET_DEFAULT_KEYS
                    ENTRY_INDEX "${entry_index}"
                    OUT_VAR_ASSIGNED_VARIABLE_NAMES variable_names
                    FILE_PATH "${attribution_file_path}"
                )

                # Check if we need to filter for specific ids.
                if(ids_to_add AND ${out_prefix}_attribution_id)
                    if("${${out_prefix}_attribution_id}" IN_LIST ids_to_add)
                        list(APPEND ids_found "${${out_prefix}_attribution_id}")
                    else()
                        # Skip to next entry.
                        math(EXPR entry_index "${entry_index} + 1")
                        continue()
                    endif()
                endif()

                # Propagate the values to the outer scope.
                foreach(variable_name IN LISTS variable_names)
                    set(${out_prefix}_${variable_name} "${${out_prefix}_${variable_name}}"
                        PARENT_SCOPE)
                endforeach()

                get_filename_component(relative_attribution_file_path
                    "${attribution_file_path}" REALPATH)

                set(${out_prefix}_chosen_attribution_file_path "${relative_attribution_file_path}"
                    PARENT_SCOPE)
                set(${out_prefix}_chosen_attribution_entry_index "${entry_index}"
                    PARENT_SCOPE)

                set(first_attribution_processed TRUE)
                if(NOT "${arg_ATTRIBUTION_ENTRY_INDEX}" STREQUAL "")
                    # We had a specific index to process, so break right after processing it.
                    break()
                endif()
            else()
                # We are processing the second or later entry, or CREATE_SBOM_FOR_EACH_ATTRIBUTION
                # was set. Instead of directly reading all the keys from the attribution file,
                # we get the Id, and create a new sbom target for the entry.
                # That will recursively call this function with a specific attribution file path
                # and index, to process the specific entry.

                set(out_prefix "${out_prefix_outer}_${file_index}_${entry_index}")

                # Get the attribution id.
                _qt_internal_sbom_read_qt_attribution(${out_prefix}
                    GET_KEY
                    KEY Id
                    OUT_VAR_VALUE attribution_id
                    ENTRY_INDEX "${entry_index}"
                    FILE_PATH "${attribution_file_path}"
                )

                # Check if we need to filter for specific ids
                if(ids_to_add AND ${out_prefix}_attribution_id)
                    if("${${out_prefix}_attribution_id}" IN_LIST ids_to_add)
                        list(APPEND ids_found "${${out_prefix}_attribution_id}")
                    else()
                        # Skip to next entry.
                        math(EXPR entry_index "${entry_index} + 1")
                        continue()
                    endif()
                endif()

                # If no Id was retrieved, just add a numeric one, to make the sbom target
                # unique.
                set(attribution_target "${arg_ATTRIBUTION_PARENT_TARGET}_Attribution_")
                if(NOT ${out_prefix}_attribution_id)
                    string(APPEND attribution_target "${file_index}_${entry_index}")
                else()
                    string(APPEND attribution_target "${${out_prefix}_attribution_id}")
                endif()

                # Sanitize the target name, to avoid issues with slashes and other unsupported chars
                # in target names.
                string(REGEX REPLACE "[^a-zA-Z0-9_-]" "_"
                    attribution_target "${attribution_target}")

                set(sbom_args "")

                # Always propagate the package supplier, because we assume the supplier for 3rd
                # party libs is the same as the current project supplier.
                # Also propagate the internal qt entity type values like CPE, supplier, PURL
                # handling options, attribution file values, if set.
                _qt_internal_forward_function_args(
                    FORWARD_APPEND
                    FORWARD_PREFIX arg
                    FORWARD_OUT_VAR sbom_args
                    FORWARD_OPTIONS
                        USE_ATTRIBUTION_FILES
                        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_CPE
                        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_SUPPLIER
                        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_PURL
                        __QT_INTERNAL_HANDLE_QT_ENTITY_ATTRIBUTION_FILES
                    FORWARD_SINGLE
                        SUPPLIER
                )

                if(propagate_sbom_options_to_new_attribution_targets)
                    # Filter out the attributtion options, they will be passed mnaually
                    # depending on which file and index is currently being processed.
                    _qt_internal_get_sbom_specific_options(
                        sbom_opt_args sbom_single_args sbom_multi_args)
                    list(REMOVE_ITEM sbom_opt_args
                        NO_CURRENT_DIR_ATTRIBUTION
                        CREATE_SBOM_FOR_EACH_ATTRIBUTION
                    )
                    list(REMOVE_ITEM sbom_single_args ATTRIBUTION_ENTRY_INDEX)
                    list(REMOVE_ITEM sbom_multi_args
                        ATTRIBUTION_IDS
                        ATTRIBUTION_FILE_PATHS
                        ATTRIBUTION_FILE_DIR_PATHS
                    )

                    # Also filter out the FRIENDLY_PACKAGE_NAME option, otherwise we'd try to
                    # file(GENERATE) multiple times with the same file name, but different content.
                    list(REMOVE_ITEM sbom_single_args FRIENDLY_PACKAGE_NAME)

                    # Filter out the sbom entity types, they are specified manually.
                    list(REMOVE_ITEM sbom_single_args
                        SBOM_ENTITY_TYPE
                        DEFAULT_SBOM_ENTITY_TYPE
                    )

                    _qt_internal_forward_function_args(
                        FORWARD_APPEND
                        FORWARD_PREFIX arg
                        FORWARD_OUT_VAR sbom_args
                        FORWARD_OPTIONS
                            ${sbom_opt_args}
                        FORWARD_SINGLE
                            ${sbom_single_args}
                        FORWARD_MULTI
                            ${sbom_multi_args}
                    )
                endif()

                # Create another sbom target with the id as a hint for the target name,
                # the attribution file passed, and make the new target a dependency of the
                # parent one.
                if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_ATTRIBUTION_FILES)
                    set(attribution_entity_type QT_THIRD_PARTY_SOURCES)
                else()
                    set(attribution_entity_type THIRD_PARTY_SOURCES)
                endif()

                _qt_internal_add_sbom("${attribution_target}"
                    IMMEDIATE_FINALIZATION
                    SBOM_ENTITY_TYPE "${attribution_entity_type}"
                    ATTRIBUTION_FILE_PATHS "${attribution_file_path}"
                    ATTRIBUTION_ENTRY_INDEX "${entry_index}"
                    NO_CURRENT_DIR_ATTRIBUTION
                    ${sbom_args}
                )

                _qt_internal_extend_sbom_dependencies(${arg_ATTRIBUTION_PARENT_TARGET}
                    SBOM_DEPENDENCIES ${attribution_target}
                )
            endif()

            math(EXPR entry_index "${entry_index} + 1")
        endwhile()

        math(EXPR file_index "${file_index} + 1")
    endforeach()

    # Show an error if an id is unaccounted for, it might be it has moved to a different file, that
    # is not referenced.
    if(ids_to_add)
        set(attribution_ids_diff ${ids_to_add})
        list(REMOVE_ITEM attribution_ids_diff ${ids_found})
        if(attribution_ids_diff)
            set(error_message
                "The following required attribution ids were not found in the attribution files")
            if(arg_ATTRIBUTION_PARENT_TARGET)
                string(APPEND error_message " for target: ${arg_ATTRIBUTION_PARENT_TARGET}")
            endif()
            string(APPEND error_message " ids: ${attribution_ids_diff}")
            message(FATAL_ERROR "${error_message}")
        endif()
    endif()
endfunction()

# Helper to parse a qt_attribution.json file and do various operations:
# - GET_DEFAULT_KEYS extracts the license id, copyrights, version, etc.
# - GET_KEY extracts a single given json key's value, as specified with KEY and saved into
#   OUT_VAR_VALUE
# - GET_ATTRIBUTION_ENTRY_COUNT returns the number of entries in the json file, set in
#   OUT_VAR_VALUE
#
# ENTRY_INDEX can be used to specify the array index to select a specific entry in the json file.
#
# Any retrieved value is set in the outer scope.
# The variables are prefixed with ${out_prefix}.
# OUT_VAR_ASSIGNED_VARIABLE_NAMES contains the list of variables set in the parent scope, the
# variables names in this list are not prefixed with ${out_prefix}.
#
# Requires cmake 3.19 for json parsing.
function(_qt_internal_sbom_read_qt_attribution out_prefix)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    if(CMAKE_VERSION LESS_EQUAL "3.19")
        message(DEBUG "CMake version is too low, can't parse attribution.json file.")
        return()
    endif()

    set(opt_args
        GET_DEFAULT_KEYS
        GET_KEY
        GET_ATTRIBUTION_ENTRY_COUNT
    )
    set(single_args
        FILE_PATH
        KEY
        ENTRY_INDEX
        OUT_VAR_VALUE
        OUT_VAR_ASSIGNED_VARIABLE_NAMES
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(file_path "${arg_FILE_PATH}")

    if(NOT file_path)
        message(FATAL_ERROR "qt attribution file path not given")
    endif()

    file(READ "${file_path}" contents)
    if(NOT contents)
        message(FATAL_ERROR "qt attribution file is empty: ${file_path}")
    endif()

    if(NOT arg_GET_DEFAULT_KEYS AND NOT arg_GET_KEY AND NOT arg_GET_ATTRIBUTION_ENTRY_COUNT)
        message(FATAL_ERROR
            "No valid operation specified to _qt_internal_sbom_read_qt_attribution call.")
    endif()

    if(arg_GET_KEY)
        if(NOT arg_KEY)
            message(FATAL_ERROR "KEY must be set")
        endif()
        if(NOT arg_OUT_VAR_VALUE)
            message(FATAL_ERROR "OUT_VAR_VALUE must be set")
        endif()
    endif()

    get_filename_component(attribution_file_dir "${file_path}" DIRECTORY)

    # Parse the json file.
    # The first element might be an array, or an object. We need to detect which one.
    # Do that by trying to query index 0 of the potential root array.
    # If the index is found, that means the root is an array, and elem_error is set to NOTFOUND,
    # because there was no error.
    # Otherwise elem_error will be something like 'member '0' not found', and we can assume the
    # root is an object.
    string(JSON first_elem_type ERROR_VARIABLE elem_error TYPE "${contents}" 0)
    if(elem_error STREQUAL "NOTFOUND")
        # Root is an array. The attribution file might contain multiple entries.
        # Pick the first one if no specific index was specified, otherwise use the given index.
        if(NOT "${arg_ENTRY_INDEX}" STREQUAL "")
            set(indices "${arg_ENTRY_INDEX}")
        else()
            set(indices "0")
        endif()
        set(is_array TRUE)
    else()
        # Root is an object, not an array, which means the file has a single entry.
        set(indices "")
        set(is_array FALSE)
    endif()

    set(variable_names "")

    if(arg_GET_KEY)
        _qt_internal_sbom_get_attribution_key(${arg_KEY} ${arg_OUT_VAR_VALUE} ${out_prefix})
    endif()

    if(arg_GET_ATTRIBUTION_ENTRY_COUNT)
        if(NOT arg_OUT_VAR_VALUE)
            message(FATAL_ERROR "OUT_VAR_VALUE must be set")
        endif()

        if(is_array)
            string(JSON attribution_entry_count ERROR_VARIABLE elem_error LENGTH "${contents}")
            # There was an error getting the length of the array, so we assume it's empty.
            if(NOT elem_error STREQUAL "NOTFOUND")
                set(attribution_entry_count 0)
            endif()
        else()
            set(attribution_entry_count 1)
        endif()

        set(${out_prefix}_${arg_OUT_VAR_VALUE} "${attribution_entry_count}" PARENT_SCOPE)
    endif()

    if(arg_GET_DEFAULT_KEYS)
        _qt_internal_sbom_get_attribution_key(Id attribution_id "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(LicenseId license_id "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(License license "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(LicenseFile license_file "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(Version version "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(Homepage homepage "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(Name attribution_name "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(Description description "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(QtUsage qt_usage "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(DownloadLocation download_location "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(Copyright copyrights "${out_prefix}" IS_MULTI_VALUE)
        _qt_internal_sbom_get_attribution_key(CopyrightFile copyright_file "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(PURL purls "${out_prefix}" IS_MULTI_VALUE)
        _qt_internal_sbom_get_attribution_key(CPE cpes "${out_prefix}" IS_MULTI_VALUE)

        # Some attribution files contain a copyright file that contains the actual list of
        # copyrights. Read it and use it.
        set(copyright_file_path "${attribution_file_dir}/${copyright_file}")
        get_filename_component(copyright_file_path "${copyright_file_path}" REALPATH)
        if(NOT copyrights AND copyright_file AND EXISTS "${copyright_file_path}")
            file(READ "${copyright_file_path}" copyright_contents)
            if(copyright_contents)
                set(copyright_contents "${copyright_contents}")
                set(copyrights "${copyright_contents}")
                set(${out_prefix}_copyrights "${copyright_contents}" PARENT_SCOPE)
                list(APPEND variable_names "copyrights")
            endif()
        endif()
    endif()

    if(arg_OUT_VAR_ASSIGNED_VARIABLE_NAMES)
        set(${arg_OUT_VAR_ASSIGNED_VARIABLE_NAMES} "${variable_names}" PARENT_SCOPE)
    endif()
endfunction()

# Extracts a string or an array of strings from a json index path, depending on the extracted value
# type.
#
# Given the 'contents' of the whole json document and the EXTRACTED_VALUE of a json key specified
# by the INDICES path, it tries to determine whether the value is an array, in which case the array
# is converted to a cmake list and assigned to ${out_var} in the parent scope.
# Otherwise the function assumes the EXTRACTED_VALUE was not an array, and just assigns the value
# of EXTRACTED_VALUE to ${out_var}
function(_qt_internal_sbom_handle_attribution_json_array contents)
    set(opt_args "")
    set(single_args
        EXTRACTED_VALUE
        OUT_VAR
    )
    set(multi_args
        INDICES
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    # Write the original value to the parent scope, in case it was not an array.
    set(${arg_OUT_VAR} "${arg_EXTRACTED_VALUE}" PARENT_SCOPE)

    if(NOT arg_EXTRACTED_VALUE)
        return()
    endif()

    string(JSON element_type TYPE "${contents}" ${arg_INDICES})

    if(NOT element_type STREQUAL "ARRAY")
        return()
    endif()

    set(json_array "${arg_EXTRACTED_VALUE}")
    string(JSON array_len LENGTH "${json_array}")

    set(value_list "")

    math(EXPR array_len "${array_len} - 1")
    foreach(index RANGE 0 "${array_len}")
        string(JSON value GET "${json_array}" ${index})
        if(value)
            list(APPEND value_list "${value}")
        endif()
    endforeach()

    if(value_list)
        set(${arg_OUT_VAR} "${value_list}" PARENT_SCOPE)
    endif()
endfunction()

# Escapes various characters in json content, so that the generate cmake code to append the content
# to the spdx document is syntactically valid.
function(_qt_internal_sbom_escape_json_content content out_var)
    # Escape backslashes
    string(REPLACE "\\" "\\\\" escaped_content "${content}")

    # Escape quotes
    string(REPLACE "\"" "\\\"" escaped_content "${escaped_content}")

    set(${out_var} "${escaped_content}" PARENT_SCOPE)
endfunction()

# This macro reads a json key from a qt_attribution.json file, and assigns the escaped value to
# out_var.
# Also appends the name of the out_var to the parent scope 'variable_names' var.
#
# Expects 'contents' and 'indices' to already be set in the calling scope.
#
# If IS_MULTI_VALUE is set, handles the key as if it contained an array of
# values, by converting the array of json values to a cmake list.
macro(_qt_internal_sbom_get_attribution_key json_key out_var out_prefix)
    cmake_parse_arguments(arg "IS_MULTI_VALUE" "" "" ${ARGN})

    # Reset any leftover value that might have been set in a previous iteration.
    set(${out_prefix}_${out_var} "" PARENT_SCOPE)

    string(JSON "${out_var}" ERROR_VARIABLE get_error GET "${contents}" ${indices} "${json_key}")
    if(NOT "${${out_var}}" STREQUAL "" AND NOT get_error)
        set(extracted_value "${${out_var}}")

        if(arg_IS_MULTI_VALUE)
            _qt_internal_sbom_handle_attribution_json_array("${contents}"
                EXTRACTED_VALUE "${extracted_value}"
                INDICES ${indices} ${json_key}
                OUT_VAR value_list
            )
            if(value_list)
                set(extracted_value "${value_list}")
            endif()
        endif()

        _qt_internal_sbom_escape_json_content("${extracted_value}" escaped_content)

        set(${out_prefix}_${out_var} "${escaped_content}" PARENT_SCOPE)
        list(APPEND variable_names "${out_var}")

        unset(extracted_value)
        unset(escaped_content)
        unset(value_list)
    endif()
endmacro()

# Replaces placeholders in CPE and PURL strings read from qt_attribution.json files.
#
# VALUES - list of CPE or PURL strings
# OUT_VAR - variable to store the replaced values
# VERSION - version to replace in the placeholders

# Known placeholders:
# $<VERSION> - Replaces occurrences of the placeholder with the value passed to the VERSION option.
# $<VERSION_DASHED> - Replaces occurrences of the placeholder with the value passed to the VERSION
# option, but with dots replaced by dashes.
function(_qt_internal_sbom_replace_qa_placeholders)
    set(opt_args "")
    set(single_args
        OUT_VAR
        VERSION
    )
    set(multi_args
        VALUES
    )

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR)
        message(FATAL_ERROR "OUT_VAR must be set")
    endif()

    set(result "")

    if(arg_VERSION)
        string(REPLACE "." "-" dashed_version "${arg_VERSION}")
    endif()

    foreach(value IN LISTS arg_VALUES)
        if(arg_VERSION)
            string(REPLACE "$<VERSION>" "${arg_VERSION}" value "${value}")
            string(REPLACE "$<VERSION_DASHED>" "${dashed_version}" value "${value}")
        endif()

        list(APPEND result "${value}")
    endforeach()

    set(${arg_OUT_VAR} "${result}" PARENT_SCOPE)
endfunction()
