# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Gets the spdx id of a target to be used in a sbom relationship entry.
# Errors out when either the target does not exist, or when an spdx id cannot be determined for
# the target.
# OPTION_NAME is used for error messages to provide context on which argument is being processed.
function(_qt_internal_sbom_get_spdx_id_for_target_in_relationship out_var)
    set(opt_args "")
    set(single_args
        TARGET
        OPTION_NAME
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT TARGET "${arg_TARGET}")
        message(FATAL_ERROR
            "${arg_OPTION_NAME} target \"${arg_TARGET}\" does not exist.")
    endif()

    _qt_internal_sbom_get_spdx_id_for_target("${arg_TARGET}" target_spdx_id)

    if(NOT target_spdx_id)
        message(FATAL_ERROR
            "Could not determine SPDX ID for OPTION_NAME '${arg_OPTION_NAME}' target "
            "\"${arg_TARGET}\".")
    endif()

    set(${out_var} "${target_spdx_id}" PARENT_SCOPE)
endfunction()

# Parses a relationship entry and outputs an appropriately formatted relationship string.
# FROM and TO should list target names enriched with sbom info.
function(_qt_internal_sbom_serialize_sbom_relationship_entry)
    set(opt_args
        ESCAPE_CYDX_QUOTES
    )
    set(single_args
        SBOM_RELATIONSHIP_FROM
        SBOM_RELATIONSHIP_TYPE
        SBOM_RELATIONSHIP_COMMENT
        OUTPUT_SBOM_FORMAT
        CYDX_TOML_KEY
        OUT_VAR_RELATIONSHIP_STRINGS
    )
    set(multi_args
        SBOM_RELATIONSHIP_TO # This is multi-value, to accomodate future spdx v3
        SBOM_FORMATS
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUTPUT_SBOM_FORMAT)
        message(FATAL_ERROR "OUTPUT_SBOM_FORMAT is required.")
    endif()

    if(NOT arg_OUT_VAR_RELATIONSHIP_STRINGS)
        message(FATAL_ERROR "OUT_VAR_RELATIONSHIP_STRINGS is required.")
    endif()

    if(NOT arg_SBOM_RELATIONSHIP_FROM)
        message(FATAL_ERROR "SBOM_RELATIONSHIP_FROM is required.")
    endif()

    if(NOT arg_SBOM_RELATIONSHIP_TO)
        message(FATAL_ERROR "SBOM_RELATIONSHIP_TO is required.")
    endif()

    if(NOT arg_SBOM_RELATIONSHIP_TYPE)
        message(FATAL_ERROR "SBOM_RELATIONSHIP_TYPE is required.")
    endif()

    _qt_internal_sbom_validate_spdx_v2_relationship_type(
        RELATIONSHIP_TYPE "${arg_SBOM_RELATIONSHIP_TYPE}"
        OUT_VAR_IS_VALID relationship_type_is_valid
    )
    if(NOT relationship_type_is_valid)
        message(FATAL_ERROR
            "Invalid SPDX v2 SBOM_RELATIONSHIP_TYPE: '${arg_SBOM_RELATIONSHIP_TYPE}'.")
    endif()

    _qt_internal_sbom_get_spdx_id_for_target_in_relationship(from_spdx_id
        TARGET "${arg_SBOM_RELATIONSHIP_FROM}"
        OPTION_NAME "SBOM_RELATIONSHIP_FROM"
    )

    _qt_internal_sbom_is_external_target_dependency("${arg_SBOM_RELATIONSHIP_FROM}"
        OUT_VAR from_is_external
    )

    if(NOT arg_SBOM_FORMATS OR arg_SBOM_FORMATS STREQUAL "ALL")
        set(active_formats "SPDX_V2" "CYDX_V1_6")
    else()
        set(active_formats ${arg_SBOM_FORMATS})
    endif()

    if(arg_OUTPUT_SBOM_FORMAT STREQUAL "SPDX_V2" AND arg_OUTPUT_SBOM_FORMAT IN_LIST active_formats)
        # For SPDX, we build up the relationship string, and its associated comment if relevant.
        set(spdx_relationship_list "")

        if(from_is_external)
            _qt_internal_sbom_get_external_document_ref_spdx_id_from_sbom_target(
                TARGET "${arg_SBOM_RELATIONSHIP_FROM}"
                OUT_VAR external_reference_from
                CREATE_TEMPORARY_REF_WHEN_MISSING
            )

            if(NOT external_reference_from)
                message(FATAL_ERROR
                    "SBOM_RELATIONSHIP_FROM target '${arg_SBOM_RELATIONSHIP_FROM}' is an external "
                    "dependency, but its SPDX v2 external document reference id can't be "
                    "found. Consider limiting the relationship to a non-SPDX SBOM format using "
                    "the SBOM_FORMATS argument.")
            endif()
            string(APPEND external_reference_from ":")
        else()
            set(external_reference_from "")
        endif()

        foreach(relationship_to IN LISTS arg_SBOM_RELATIONSHIP_TO)
            _qt_internal_sbom_get_spdx_id_for_target_in_relationship(to_spdx_id
                TARGET "${relationship_to}"
                OPTION_NAME "SBOM_RELATIONSHIP_TO"
            )

            _qt_internal_sbom_is_external_target_dependency("${relationship_to}"
                OUT_VAR to_is_external
            )
            if(to_is_external)
                _qt_internal_sbom_get_external_document_ref_spdx_id_from_sbom_target(
                    TARGET "${relationship_to}"
                    OUT_VAR external_reference_to
                    CREATE_TEMPORARY_REF_WHEN_MISSING
                )

                if(NOT external_reference_to)
                    message(FATAL_ERROR
                        "SBOM_RELATIONSHIP_TO target '${relationship_to}' is an external "
                        "dependency, but its SPDX v2 external document reference id can't be "
                        "found. Consider limiting the relationship to a non-SPDX SBOM format using "
                        "the SBOM_FORMATS argument.")
                endif()
                string(APPEND external_reference_to ":")
            else()
                set(external_reference_to "")
            endif()

            string(CONCAT spdx_relationship
                "Relationship:"
                " ${external_reference_from}${from_spdx_id}"
                " ${arg_SBOM_RELATIONSHIP_TYPE}"
                " ${external_reference_to}${to_spdx_id}")
            list(APPEND spdx_relationship_list "${spdx_relationship}")

            if(arg_SBOM_RELATIONSHIP_COMMENT)
                string(CONCAT spdx_relationship_comment
                    "RelationshipComment: "
                    "<text>${arg_SBOM_RELATIONSHIP_COMMENT}</text>")
                list(APPEND spdx_relationship_list "${spdx_relationship_comment}")
            endif()
        endforeach()

        set(${arg_OUT_VAR_RELATIONSHIP_STRINGS} "${spdx_relationship_list}" PARENT_SCOPE)
    endif()

    if(arg_OUTPUT_SBOM_FORMAT STREQUAL "CYDX_V1_6"
            AND arg_OUTPUT_SBOM_FORMAT IN_LIST active_formats)
        set(cydx_toml_key "")
        if(arg_CYDX_TOML_KEY)
            set(cydx_toml_key "${arg_CYDX_TOML_KEY}")
        endif()

        if(arg_ESCAPE_CYDX_QUOTES)
            set(quote "\\\"")
        else()
            set(quote "\"")
        endif()

        set(cydx_relationship_list "")
        foreach(relationship_to IN LISTS arg_SBOM_RELATIONSHIP_TO)
            _qt_internal_sbom_get_spdx_id_for_target_in_relationship(to_spdx_id
                TARGET "${relationship_to}"
                OPTION_NAME "SBOM_RELATIONSHIP_TO"
            )

            list(APPEND cydx_relationship_list "
[[${cydx_toml_key}relationships]]
relationship_from = ${quote}${from_spdx_id}${quote}
relationship_type = ${quote}${arg_SBOM_RELATIONSHIP_TYPE}${quote}
relationship_to = ${quote}${to_spdx_id}${quote}
relationship_comment = ${quote}${arg_SBOM_RELATIONSHIP_COMMENT}${quote}
")
        endforeach()

        set(${arg_OUT_VAR_RELATIONSHIP_STRINGS} "${cydx_relationship_list}" PARENT_SCOPE)
    endif()
endfunction()

# Checks if a relationship type is valid, according to the list at
# https://spdx.github.io/spdx-spec/v2.3/relationships-between-SPDX-elements/
function(_qt_internal_sbom_validate_spdx_v2_relationship_type)
    if(NOT QT_GENERATE_SBOM)
        if(arg_OUT_VAR_IS_VALID)
            set(${arg_OUT_VAR_IS_VALID} "" PARENT_SCOPE)
        endif()

        return()
    endif()

    set(opt_args "")
    set(single_args
        RELATIONSHIP_TYPE
        OUT_VAR_IS_VALID
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_RELATIONSHIP_TYPE)
        message(FATAL_ERROR "RELATIONSHIP_TYPE is required.")
    endif()

    if(NOT arg_OUT_VAR_IS_VALID)
        message(FATAL_ERROR "OUT_VAR_IS_VALID is required.")
    endif()

    set(valid_types
        DESCRIBES
        DESCRIBED_BY
        CONTAINS
        CONTAINED_BY
        DEPENDS_ON
        DEPENDENCY_OF
        DEPENDENCY_MANIFEST_OF
        BUILD_DEPENDENCY_OF
        DEV_DEPENDENCY_OF
        OPTIONAL_DEPENDENCY_OF
        PROVIDED_DEPENDENCY_OF
        TEST_DEPENDENCY_OF
        RUNTIME_DEPENDENCY_OF
        EXAMPLE_OF
        GENERATES
        GENERATED_FROM
        ANCESTOR_OF
        DESCENDANT_OF
        VARIANT_OF
        DISTRIBUTION_ARTIFACT
        PATCH_FOR
        PATCH_APPLIED
        COPY_OF
        FILE_ADDED
        FILE_DELETED
        FILE_MODIFIED
        EXPANDED_FROM_ARCHIVE
        DYNAMIC_LINK
        STATIC_LINK
        DATA_FILE_OF
        TEST_CASE_OF
        BUILD_TOOL_OF
        DEV_TOOL_OF
        TEST_OF
        TEST_TOOL_OF
        DOCUMENTATION_OF
        OPTIONAL_COMPONENT_OF
        METAFILE_OF
        PACKAGE_OF
        AMENDS
        PREREQUISITE_FOR
        HAS_PREREQUISITE
        REQUIREMENT_DESCRIPTION_FOR
        SPECIFICATION_FOR
        OTHER
    )

    if(arg_RELATIONSHIP_TYPE IN_LIST valid_types)
        set(is_valid TRUE)
    else()
        set(is_valid FALSE)
    endif()

    set(${arg_OUT_VAR_IS_VALID} "${is_valid}" PARENT_SCOPE)
endfunction()


# Processes a list of relationships entries and outputs a list of strings.
# ESCAPE_CYDX_QUOTES needs to be passed when the output is meant to be used in a
# file(APPEND staging_area_spdx_file block.
function(_qt_internal_sbom_serialize_relationship_entries)
    set(opt_args
        ESCAPE_CYDX_QUOTES
    )
    set(single_args
        OUTPUT_SBOM_FORMAT
        CYDX_TOML_KEY
        OUT_VAR_RELATIONSHIPS_STRINGS
    )
    set(multi_args
        SBOM_RELATIONSHIP_ENTRIES
    )

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUTPUT_SBOM_FORMAT)
        message(FATAL_ERROR "OUTPUT_SBOM_FORMAT is required.")
    endif()

    if(NOT arg_OUT_VAR_RELATIONSHIPS_STRINGS)
        message(FATAL_ERROR "OUT_VAR_RELATIONSHIPS_STRINGS is required.")
    endif()

    # Collect each SBOM_RELATIONSHIP_ENTRY args into a separate variable.
    set(entry_idx -1)
    set(entry_indices "")

    foreach(entry_arg IN LISTS arg_SBOM_RELATIONSHIP_ENTRIES)
        if(entry_arg STREQUAL "SBOM_RELATIONSHIP_ENTRY")
            math(EXPR entry_idx "${entry_idx}+1")
            list(APPEND entry_indices "${entry_idx}")
        elseif(entry_idx GREATER_EQUAL 0)
            list(APPEND entry_${entry_idx}_args "${entry_arg}")
        else()
            message(FATAL_ERROR "Missing SBOM_RELATIONSHIP_ENTRY keyword.")
        endif()
    endforeach()

    set(relationships_strings "")

    set(extra_entry_args "")
    if(arg_CYDX_TOML_KEY)
        list(APPEND extra_entry_args CYDX_TOML_KEY "${arg_CYDX_TOML_KEY}")
    endif()
    if(arg_ESCAPE_CYDX_QUOTES)
        list(APPEND extra_entry_args ESCAPE_CYDX_QUOTES)
    endif()

    foreach(entry_idx IN LISTS entry_indices)
        _qt_internal_sbom_serialize_sbom_relationship_entry(
            ${entry_${entry_idx}_args}
            ${extra_entry_args}
            OUTPUT_SBOM_FORMAT "${arg_OUTPUT_SBOM_FORMAT}"
            OUT_VAR_RELATIONSHIP_STRINGS entry_relationship_strings
        )
        if(entry_relationship_strings)
            list(APPEND relationships_strings ${entry_relationship_strings})
        endif()
    endforeach()

    set(${arg_OUT_VAR_RELATIONSHIPS_STRINGS} "${relationships_strings}" PARENT_SCOPE)
endfunction()

# Collects relationships for a given target, and outputs them for each supported format.
#
# Collects relationships from the following sources:
# - target linkage
# - custom relationship entries
# - always adds a "contains" relationship to the parent project package
function(_qt_internal_sbom_handle_target_relationships target)
    set(opt_args "")
    set(single_args
        SPDX_ID
        PROJECT_SPDX_ID
        OUT_VAR_SBOM_RELATIONSHIP_ENTRIES
        OUT_VAR_SPDX_V2_RELATIONSHIPS # spdx v2, deprecated, still used by WebEngine
    )
    set(multi_args
        LIBRARIES
        PUBLIC_LIBRARIES
        SBOM_RELATIONSHIPS # spdx v2 input, deprecated, still used by WebEngine
        SBOM_RELATIONSHIP_ENTRIES
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_SPDX_ID)
        message(FATAL_ERROR "SPDX_ID must be set")
    endif()

    set(sbom_relationship_entries "")
    set(external_target_dependencies "")
    set(spdx_relationships "") # deprecated, but needed

    # Get relationships based on target linkage.
    _qt_internal_sbom_handle_target_dependencies("${target}"
        SPDX_ID "${arg_SPDX_ID}"
        LIBRARIES "${arg_LIBRARIES}"
        PUBLIC_LIBRARIES "${arg_PUBLIC_LIBRARIES}"
        OUT_SBOM_RELATIONSHIP_ENTRIES target_relationship_entries
        OUT_EXTERNAL_TARGET_DEPENDENCIES external_target_dependencies
    )

    if(target_relationship_entries)
        list(APPEND sbom_relationship_entries ${target_relationship_entries})
    endif()

    # Get relationships specified manually.
    if(arg_SBOM_RELATIONSHIP_ENTRIES)
        list(APPEND sbom_relationship_entries ${arg_SBOM_RELATIONSHIP_ENTRIES})
    endif()

    # CycloneDX external dependencies need to be processed at the end of document generation,
    # so record them.
    if(external_target_dependencies)
        _qt_internal_sbom_record_external_target_dependecies(
            TARGETS ${external_target_dependencies}
        )
    endif()

    # Handle deprecated SBOM_RELATIONSHIPS argument.
    if(arg_SBOM_RELATIONSHIPS)
        # This option is deprecated, but it's still used by older WebEngine versions.
        # The values for the option were not expected to be prefixed with 'Relationship:', so make
        # sure to add the prefix for each value.
        set(extra_spdx_v2_relationships "")
        foreach(spdx_v2_relationship IN LISTS arg_SBOM_RELATIONSHIPS)
            if(NOT spdx_v2_relationship MATCHES "Relationship:")
                string(PREPEND spdx_v2_relationship "Relationship: ")
            endif()
            list(APPEND extra_spdx_v2_relationships "${spdx_v2_relationship}")
        endforeach()
        list(APPEND spdx_relationships ${extra_spdx_v2_relationships})
    endif()

    # Add default "contains" spdx relationship to the project package.
    _qt_internal_sbom_get_current_project_target(project_target)
    list(PREPEND sbom_relationship_entries
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                "${project_target}"
            SBOM_RELATIONSHIP_TYPE
                CONTAINS
            SBOM_RELATIONSHIP_TO
                "${target}"
    )

    set(${arg_OUT_VAR_SBOM_RELATIONSHIP_ENTRIES} "${sbom_relationship_entries}" PARENT_SCOPE)
    set(${arg_OUT_VAR_SPDX_V2_RELATIONSHIPS} "${spdx_relationships}" PARENT_SCOPE)
endfunction()

function(_qt_internal_sbom_get_current_project_relationships out_var)
    set(opt_args "")
    set(single_args "")
    set(multi_args
        SBOM_RELATIONSHIP_ENTRIES
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    # No validation on purpose, there might be more options set.

    set(values "")
    if(arg_SBOM_RELATIONSHIP_ENTRIES)
        set(values ${arg_SBOM_RELATIONSHIP_ENTRIES})
    endif()

    set(${out_var} "${values}" PARENT_SCOPE)
endfunction()

# Collects relationships for the currently active project, and serializes them for each supported
# format.
function(_qt_internal_sbom_handle_project_relationships)
    set(opt_args "")
    set(single_args
        OUTPUT_SBOM_FORMAT
        OUT_VAR_RELATIONSHIP_STRINGS
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUTPUT_SBOM_FORMAT)
        message(FATAL_ERROR "OUTPUT_SBOM_FORMAT is required.")
    endif()

    if(NOT arg_OUT_VAR_RELATIONSHIP_STRINGS)
        message(FATAL_ERROR "OUT_VAR_RELATIONSHIP_STRINGS is required.")
    endif()

    _qt_internal_sbom_get_current_project_target(project_target)

    get_target_property(project_finalize_args "${project_target}" _qt_finalize_sbom_args)
    if(NOT project_finalize_args)
        set(project_finalize_args "")
    endif()

    _qt_internal_sbom_get_current_project_relationships(entries
        ${project_finalize_args}
    )

    if(NOT entries)
        set(${arg_OUT_VAR_RELATIONSHIP_STRINGS} "" PARENT_SCOPE)
        return()
    endif()

    set(relationship_entries_args "")
    if(arg_OUTPUT_SBOM_FORMAT MATCHES "CYDX")
        list(APPEND relationship_entries_args CYDX_TOML_KEY "root_component.")
    endif()

    _qt_internal_sbom_serialize_relationship_entries(
        ${relationship_entries_args}
        OUTPUT_SBOM_FORMAT "${arg_OUTPUT_SBOM_FORMAT}"
        OUT_VAR_RELATIONSHIPS_STRINGS relationships_strings
        SBOM_RELATIONSHIP_ENTRIES ${entries}
    )

    set(${arg_OUT_VAR_RELATIONSHIP_STRINGS} "${relationships_strings}" PARENT_SCOPE)
endfunction()
