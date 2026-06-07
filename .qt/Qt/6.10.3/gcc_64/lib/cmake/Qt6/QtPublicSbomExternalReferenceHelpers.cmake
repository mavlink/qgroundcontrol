# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Records a reference to an SBOM external document in the given 'target' for the currently active
# SBOM project.
#
# External SBOM targets can then reference this document target, to allow creating relationships
# bewteen current project and external SBOM entities.
#
# Currently the following SBOM_FORMAT formats are supported:
# - 'SPDX_V2_TAG_VALUE'
# - 'SPDX_V2_JSON'
# - 'CYDX_V1_JSON'
#
# For the SPDX format, the SPDX_V2_DOCUMENT_REF_ID option is required, which is used to identify
# the external document within the current project SBOM.
#
# In addition, we need the SPDX document namespace and file sha1 checksum. These can be passed
# explcitily, or parsed from the external document.
#
# 1) To parse them from the document, the relative or absolute document path should be passed via
# the EXTERNAL_DOCUMENT_FILE_PATH option.
# If the passed path is relative, it gets searched in 'EXTERNAL_DOCUMENT_SEARCH_PATHS'.
# The file must exist at the point when the API is called.
#
# 2) The values can be specified via the SPDX_V2_DOCUMENT_NAMESPACE and SPDX_V2_DOCUMENT_SHA1
# options.
#
# For CycloneDX the CYDX_URN_SERIAL_NUMBER and CYDX_URN_BOM_VERSION options should be passed.
# Or they can be parsed via EXTERNAL_DOCUMENT_FILE_PATH.
function(_qt_internal_sbom_add_external_reference_document target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args
        NO_SCAN_FILE_AT_CONFIGURE_TIME
    )
    set(single_args
        SBOM_FORMAT
        EXTERNAL_DOCUMENT_FILE_PATH

        SPDX_V2_DOCUMENT_REF_ID
        SPDX_V2_DOCUMENT_NAMESPACE
        SPDX_V2_DOCUMENT_SHA1

        CYDX_URN_SERIAL_NUMBER
        CYDX_URN_BOM_VERSION
    )
    set(multi_args
        EXTERNAL_DOCUMENT_SEARCH_PATHS
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    # We don't support SPDX v3 json at the moment.
    set(supported_sbom_formats
        CYDX_V1_JSON
        SPDX_V2_TAG_VALUE
        SPDX_V2_JSON
    )

    if(NOT arg_SBOM_FORMAT)
        message(FATAL_ERROR "SBOM_FORMAT must be specified.")
    endif()

    if(NOT arg_SBOM_FORMAT IN_LIST supported_sbom_formats)
        message(FATAL_ERROR "Unsupported SBOM_FORMAT value: ${arg_SBOM_FORMAT}. "
            "Currently supported values are: ${supported_sbom_formats}")
    endif()

    if(NOT TARGET "${target}")
        add_library("${target}" INTERFACE IMPORTED)
        set_target_properties("${target}" PROPERTIES IMPORTED_GLOBAL TRUE)
    endif()

    get_target_property(is_external_project_sbom_target "${target}"
        _qt_sbom_is_external_project_sbom_target)
    if(NOT is_external_project_sbom_target)
        set_target_properties("${target}" PROPERTIES
            _qt_sbom_is_custom_sbom_target "TRUE"
            _qt_sbom_is_external_project_sbom_target "TRUE"

            # These aren't real values, but it's what we use to determine external packages
            # during dependency handling.
            _qt_sbom_spdx_repo_project_name_lowercase "fake_external_project_name_${target}"
            _qt_sbom_spdx_relative_installed_repo_document_path "/fake_external_doc_path_${target}"
        )
    endif()

    # Return early gracefully when we're requested to add a format that SBOM generation isn't
    # enabled for.
    if((arg_SBOM_FORMAT STREQUAL "SPDX_V2_TAG_VALUE" OR arg_SBOM_FORMAT STREQUAL "SPDX_V2_JSON")
            AND NOT QT_SBOM_GENERATE_SPDX_V2)
        message(DEBUG "SPDX v2 SBOM generation is not enabled, skipping adding external reference "
            "for target '${target}'."
            "\nSPDX_V2_DOCUMENT_REF_ID: '${arg_SPDX_V2_DOCUMENT_REF_ID}'"
            "\nSPDX_V2_DOCUMENT_NAMESPACE: '${arg_SPDX_V2_DOCUMENT_NAMESPACE}'"
            "\nEXTERNAL_DOCUMENT_FILE_PATH: '${arg_EXTERNAL_DOCUMENT_FILE_PATH}'"
        )
        return()
    endif()

    if(arg_SBOM_FORMAT STREQUAL "CYDX_V1_JSON" AND NOT QT_SBOM_GENERATE_CYDX_V1_6)
        message(DEBUG
            "CycloneDX v1 SBOM generation is not enabled, skipping adding external reference "
            "for target '${target}'."
            "\nCYDX_URN_SERIAL_NUMBER: '${arg_CYDX_URN_SERIAL_NUMBER}'"
            "\nEXTERNAL_DOCUMENT_FILE_PATH: '${arg_EXTERNAL_DOCUMENT_FILE_PATH}'"
        )
        return()
    endif()

    # If an document file path is given and configure time scan opt out is not passed, search for
    # the file and prefer the values extracted from it, rather than what can be passed to the
    # function.
    # If configure time scanning is opted out, we will require some of the information to be passed
    # at explicitly.
    if(arg_EXTERNAL_DOCUMENT_FILE_PATH AND NOT arg_NO_SCAN_FILE_AT_CONFIGURE_TIME)
        set(document_search_path_args "")
        if(arg_EXTERNAL_DOCUMENT_SEARCH_PATHS)
            list(APPEND document_search_path_args
                EXTERNAL_DOCUMENT_SEARCH_PATHS ${arg_EXTERNAL_DOCUMENT_SEARCH_PATHS})
        endif()
        _qt_internal_sbom_get_external_reference_search_paths(document_search_paths
            ${document_search_path_args}
        )

        set(find_file_args "")
        if(document_search_paths)
            list(APPEND find_file_args
                EXTERNAL_DOCUMENT_SEARCH_PATHS ${document_search_paths})
        endif()
        _qt_internal_sbom_find_external_reference_document(document_file_path
            EXTERNAL_DOCUMENT_FILE_PATH "${arg_EXTERNAL_DOCUMENT_FILE_PATH}"
            ${find_file_args}
        )
    elseif(arg_EXTERNAL_DOCUMENT_FILE_PATH)
        # If we don't scan at configure time, we still need to pass the document along at least
        # for spdx v2.
        set(document_file_path "${arg_EXTERNAL_DOCUMENT_FILE_PATH}")
    endif()

    if(arg_SBOM_FORMAT STREQUAL "SPDX_V2_TAG_VALUE" OR arg_SBOM_FORMAT STREQUAL "SPDX_V2_JSON")
        _qt_internal_forward_function_args(
            FORWARD_PREFIX arg
            FORWARD_OUT_VAR external_document_args
            FORWARD_OPTIONS
                NO_SCAN_FILE_AT_CONFIGURE_TIME
            FORWARD_SINGLE
                SBOM_FORMAT
                SPDX_V2_DOCUMENT_REF_ID
                SPDX_V2_DOCUMENT_NAMESPACE
                SPDX_V2_DOCUMENT_SHA1
            FORWARD_MULTI
                EXTERNAL_DOCUMENT_SEARCH_PATHS
        )
        if(document_file_path)
            list(APPEND external_document_args EXTERNAL_DOCUMENT_FILE_PATH "${document_file_path}")
        endif()
        _qt_internal_sbom_add_external_reference_document_spdx_v2(${target}
            ${external_document_args}
        )
    endif()

    if(arg_SBOM_FORMAT STREQUAL "CYDX_V1_JSON")
        _qt_internal_forward_function_args(
            FORWARD_PREFIX arg
            FORWARD_OUT_VAR external_document_args
            FORWARD_OPTIONS
                NO_SCAN_FILE_AT_CONFIGURE_TIME
            FORWARD_SINGLE
                CYDX_URN_SERIAL_NUMBER
                CYDX_URN_BOM_VERSION
        )
        if(document_file_path)
            list(APPEND external_document_args EXTERNAL_DOCUMENT_FILE_PATH "${document_file_path}")
        endif()
        _qt_internal_sbom_add_external_reference_document_cydx_v1(${target}
            ${external_document_args}
        )
    endif()
endfunction()

# Handles adding an SPDX v2 external document reference, either in tag:value or json format.
# NO_SCAN_FILE_AT_CONFIGURE_TIME is an internal option, only used by the Qt build system
# to reference external documents that do not exist yet, but will exist at installation time.
function(_qt_internal_sbom_add_external_reference_document_spdx_v2 target)
    set(opt_args
        NO_SCAN_FILE_AT_CONFIGURE_TIME
    )
    set(single_args
        SBOM_FORMAT
        EXTERNAL_DOCUMENT_FILE_PATH

        SPDX_V2_DOCUMENT_REF_ID
        SPDX_V2_DOCUMENT_NAMESPACE
        SPDX_V2_DOCUMENT_SHA1
    )
    set(multi_args
        EXTERNAL_DOCUMENT_SEARCH_PATHS
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_SPDX_V2_DOCUMENT_REF_ID)
        message(FATAL_ERROR "SPDX_V2_DOCUMENT_REF_ID must be specified.")
    endif()

    get_target_property(exising_external_document_ref "${target}"
        _qt_sbom_spdx_v2_external_document_ref)

    if(exising_external_document_ref
            AND NOT exising_external_document_ref STREQUAL arg_SPDX_V2_DOCUMENT_REF_ID)
        message(FATAL_ERROR
            "Target '${target}' already has an associated SPDX v2 external document reference "
            "'${exising_external_document_ref}' set, but a new value "
            "'${arg_SPDX_V2_DOCUMENT_REF_ID}' is being set. A target can't have more than one "
            "value for the same SBOM format.")
    endif()

    # Used later.
    if(arg_SBOM_FORMAT STREQUAL "SPDX_V2_TAG_VALUE")
        set(document_path_property_name "_qt_sbom_spdx_v2_document_tag_value_relative_path")
    elseif(arg_SBOM_FORMAT STREQUAL "SPDX_V2_JSON")
        set(document_path_property_name "_qt_sbom_spdx_v2_document_json_relative_path")
    endif()

    # Check that the document ref id is unique for the current project. Error out if it is not
    # unique, otherwise add to the known list. But allow for the use case of adding one of the
    # missing formats, either a tag:value or a json spdx v2 document for the same target,
    # as long as the same ref id is used.
    get_cmake_property(known_external_document_target
        _qt_known_external_documents_${arg_SPDX_V2_DOCUMENT_REF_ID}_target)

    set(should_add_new_ref_id TRUE)
    if(known_external_document_target)
        get_target_property(document_relative_path "${target}" "${document_path_property_name}")

        # If the property is already set, treat it as a duplication error.
        if(document_relative_path)
            message(FATAL_ERROR
                "SPDX v2 external document reference '${arg_SPDX_V2_DOCUMENT_REF_ID}' was already "
                "recorded when processing target '${known_external_document_target}', but is now "
                "being added again to target '${target}', format '${arg_SBOM_FORMAT}'. "
                "A SPDX document reference ID can only be recorded once for the current project "
                "SPDX document.")
        else()
            # We are just updating a new format's relative file path.
            set(should_add_new_ref_id FALSE)
        endif()
    endif()

    if(should_add_new_ref_id)
        set_property(GLOBAL PROPERTY
            _qt_known_external_documents_${arg_SPDX_V2_DOCUMENT_REF_ID} TRUE)

        set_property(GLOBAL PROPERTY
            _qt_known_external_documents_${arg_SPDX_V2_DOCUMENT_REF_ID}_target "${target}")

        set_property(GLOBAL APPEND PROPERTY
            _qt_known_external_documents "${arg_SPDX_V2_DOCUMENT_REF_ID}")

        set_target_properties("${target}" PROPERTIES
            _qt_sbom_spdx_v2_external_document_ref "${arg_SPDX_V2_DOCUMENT_REF_ID}"
        )
    endif()

    if(arg_EXTERNAL_DOCUMENT_FILE_PATH AND NOT arg_NO_SCAN_FILE_AT_CONFIGURE_TIME)
        _qt_internal_sbom_parse_spdx_v2_document_namespace(
            EXTERNAL_DOCUMENT_FILE_PATH "${arg_EXTERNAL_DOCUMENT_FILE_PATH}"
            SBOM_FORMAT "${arg_SBOM_FORMAT}"
            OUT_VAR_DOCUMENT_NAMESPACE document_namespace
        )

        if(NOT document_namespace)
            message(FATAL_ERROR
                "Failed to extract DocumentNamespace from SPDX v2 external document: "
                "'${arg_EXTERNAL_DOCUMENT_FILE_PATH}'")
        endif()
    else()
        if(NOT arg_SPDX_V2_DOCUMENT_NAMESPACE)
            message(FATAL_ERROR
                "SPDX_V2_DOCUMENT_NAMESPACE must be specified for target '${target}' if "
                "EXTERNAL_DOCUMENT_FILE_PATH is not given or NO_SCAN_FILE_AT_CONFIGURE_TIME is "
                "set.")
        else()
            set(document_namespace "${arg_SPDX_V2_DOCUMENT_NAMESPACE}")
        endif()
    endif()

    get_target_property(existing_document_namespace "${target}"
        _qt_sbom_spdx_repo_document_namespace)

    if(existing_document_namespace
            AND NOT existing_document_namespace STREQUAL document_namespace)
        message(FATAL_ERROR
            "Target '${target}' already has an associated SPDX v2 external document namespace "
            "'${existing_document_namespace}' set, but a new value "
            "'${document_namespace}' is being set. A target can't have more than one "
            "value for the same SBOM format.")
    endif()

    if(should_add_new_ref_id)
        set_target_properties("${target}" PROPERTIES
            _qt_sbom_spdx_repo_document_namespace "${document_namespace}"
        )
    endif()

    # Save the document file path for querying if available.
    if(arg_EXTERNAL_DOCUMENT_FILE_PATH)
        set_target_properties("${target}" PROPERTIES
            "${document_path_property_name}" "${arg_EXTERNAL_DOCUMENT_FILE_PATH}"
        )
    endif()

    # SPDX v2 compared to CycloneDX also requires adding an explicit external document ref to the
    # current's project SBOM document, which needs a namespace and a sha1 value.
    set(forward_args "")
    if(arg_EXTERNAL_DOCUMENT_FILE_PATH)
        list(APPEND forward_args
            EXTERNAL_DOCUMENT_FILE_PATH "${arg_EXTERNAL_DOCUMENT_FILE_PATH}")
    endif()

    if(arg_EXTERNAL_DOCUMENT_SEARCH_PATHS)
        list(APPEND forward_args
            EXTERNAL_DOCUMENT_INSTALL_PREFIXES ${arg_EXTERNAL_DOCUMENT_SEARCH_PATHS})
    endif()

    if(arg_SPDX_V2_DOCUMENT_NAMESPACE)
        list(APPEND forward_args
            EXTERNAL_DOCUMENT_NAMESPACE "${arg_SPDX_V2_DOCUMENT_NAMESPACE}")
    endif()

    if(arg_SPDX_V2_DOCUMENT_SHA1)
        list(APPEND forward_args
            EXTERNAL_DOCUMENT_SHA1 "${arg_SPDX_V2_DOCUMENT_SHA1}")
    endif()

    if(should_add_new_ref_id)
        _qt_internal_sbom_generate_add_external_reference(
            EXTERNAL_DOCUMENT_SPDX_ID "${arg_SPDX_V2_DOCUMENT_REF_ID}"
            SBOM_FORMAT "${arg_SBOM_FORMAT}"
            ${forward_args}
        )
    endif()
endfunction()

# Parses out the SPDX v2 document namespace value out of an external SPDX document.
# Support SPDX v2.3 tag value and json formats for the file path.
# Errors out when the document namespace can't be found.
function(_qt_internal_sbom_parse_spdx_v2_document_namespace)
    set(opt_args "")
    set(single_args
        SBOM_FORMAT
        EXTERNAL_DOCUMENT_FILE_PATH
        OUT_VAR_DOCUMENT_NAMESPACE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_EXTERNAL_DOCUMENT_FILE_PATH)
        message(FATAL_ERROR "EXTERNAL_DOCUMENT_FILE_PATH must be specified.")
    endif()

    if(NOT EXISTS "${arg_EXTERNAL_DOCUMENT_FILE_PATH}")
        message(FATAL_ERROR
            "EXTERNAL_DOCUMENT_FILE_PATH '${arg_EXTERNAL_DOCUMENT_FILE_PATH}' does not exist.")
    endif()

    if(NOT arg_OUT_VAR_DOCUMENT_NAMESPACE)
        message(FATAL_ERROR "OUT_VAR_DOCUMENT_NAMESPACE must be specified.")
    endif()

    if(NOT arg_SBOM_FORMAT)
        message(FATAL_ERROR "SBOM_FORMAT must be specified.")
    endif()

    file(READ "${arg_EXTERNAL_DOCUMENT_FILE_PATH}" ext_content)

    set(extra_error_message "")

    # Support extracting the namespace from both a tag:value and json document.
    if(arg_SBOM_FORMAT STREQUAL "SPDX_V2_TAG_VALUE")
        string(REGEX REPLACE "^.*[\r\n]DocumentNamespace:[ \t]*([^#\r\n]*).*$"
                "\\1" document_namespace "${ext_content}")
    elseif(arg_SBOM_FORMAT STREQUAL "SPDX_V2_JSON")
        string(JSON document_namespace ERROR_VARIABLE document_namespace_error
            GET "${ext_content}" "documentNamespace")
        if(document_namespace_error)
            set(extra_error_message "${document_namespace_error}")
        endif()
    endif()

    if(NOT document_namespace)
        message(FATAL_ERROR
            "Failed to extract DocumentNamespace from SPDX v2 external document: "
            "'${arg_EXTERNAL_DOCUMENT_FILE_PATH}'")
    endif()

    set(${arg_OUT_VAR_DOCUMENT_NAMESPACE} "${document_namespace}" PARENT_SCOPE)
endfunction()

# Handles adding a CycloneDX v1 external document reference in json format.
function(_qt_internal_sbom_add_external_reference_document_cydx_v1 target)
    set(opt_args
        NO_SCAN_FILE_AT_CONFIGURE_TIME
    )
    set(single_args
        EXTERNAL_DOCUMENT_FILE_PATH

        CYDX_URN_SERIAL_NUMBER
        CYDX_URN_BOM_VERSION
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(arg_EXTERNAL_DOCUMENT_FILE_PATH AND NOT arg_NO_SCAN_FILE_AT_CONFIGURE_TIME)
        file(READ "${arg_EXTERNAL_DOCUMENT_FILE_PATH}" ext_content)

        string(JSON serial_number ERROR_VARIABLE serial_number_error GET "${ext_content}"
            "serialNumber")
        if(serial_number_error)
            message(FATAL_ERROR
                "Failed to extract CycloneDX serialNumber from external document: "
                "'${arg_EXTERNAL_DOCUMENT_FILE_PATH}'")
        endif()

        string(JSON urn_bom_version ERROR_VARIABLE bom_version_error GET "${ext_content}"
            "version")
        if(serial_number_error)
            message(FATAL_ERROR
                "Failed to extract CycloneDX bom version from external document: "
                "'${arg_EXTERNAL_DOCUMENT_FILE_PATH}'")
        endif()
    else()
        if(NOT arg_CYDX_URN_SERIAL_NUMBER)
            message(FATAL_ERROR "CYDX_URN_SERIAL_NUMBER must be specified for target '${target}' "
                "if EXTERNAL_DOCUMENT_FILE_PATH is not given or NO_SCAN_FILE_AT_CONFIGURE_TIME is "
                "set.")
        else()
            set(serial_number "${arg_CYDX_URN_SERIAL_NUMBER}")
        endif()

        if(arg_CYDX_URN_BOM_VERSION)
            set(urn_bom_version "${arg_CYDX_URN_BOM_VERSION}")
        else()
            set(urn_bom_version "1")
        endif()
    endif()

    get_target_property(existing_bom_serial_number "${target}"
        _qt_sbom_cydx_bom_serial_number_uuid)

    if(existing_bom_serial_number)
        message(FATAL_ERROR "existing")
    endif()

    if(existing_bom_serial_number AND NOT existing_bom_serial_number STREQUAL serial_number)
        message(FATAL_ERROR
            "Target '${target}' already has an associated CycloneDX external document serial "
            "number '${existing_bom_serial_number}' set, but a new value "
            "'${serial_number}' is being set. A target can't have more than one "
            "value for the same SBOM format.")
    endif()

    set_target_properties("${target}" PROPERTIES
        _qt_sbom_cydx_bom_serial_number_uuid "${serial_number}"
        _qt_sbom_cydx_urn_bom_version "${urn_bom_version}"
    )

    # Save the document file path for querying if available.
    if(arg_EXTERNAL_DOCUMENT_FILE_PATH)
        set_target_properties("${target}" PROPERTIES
            "_qt_sbom_cydx_v1_6_document_json_relative_path" "${arg_EXTERNAL_DOCUMENT_FILE_PATH}"
        )
    endif()
endfunction()

# Returns a list of external document search paths, based on confguration options passed.
function(_qt_internal_sbom_get_external_reference_search_paths out_var)
    if(NOT QT_GENERATE_SBOM)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    set(opt_args
        SEARCH_IN_BUILD_SBOM_DIRS
        SEARCH_IN_QT_PREFIXES
        SEARCH_IN_DESTDIR_INSTALL_PREFIX_AT_INSTALL_TIME
    )
    set(single_args "")
    set(multi_args
        EXTERNAL_DOCUMENT_SEARCH_PATHS
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(search_paths "")

    # Add the current sbom build dirs as install prefixes, so that we can use ninja 'sbom'
    # in top-level builds. This is needed because the external references will point
    # to sbom docs in different build dirs, not just one.
    # We also need it in case we are converting a json document to a tag/value format in the
    # current build dir of the project, and want it to be found.
    if(arg_SEARCH_IN_BUILD_SBOM_DIRS)
        get_cmake_property(build_sbom_dirs _qt_internal_sbom_dirs)
        if(build_sbom_dirs)
            foreach(build_sbom_dir IN LISTS build_sbom_dirs)
                list(APPEND search_paths "${build_sbom_dir}")
            endforeach()
        endif()
    endif()

    get_cmake_property(project_search_paths _qt_internal_sbom_external_document_search_paths)
    if(project_search_paths)
        list(APPEND search_paths ${project_search_paths})
    endif()

    get_cmake_property(search_in_cmake_paths
        _qt_internal_sbom_auto_search_external_documents_in_paths)
    if(search_in_cmake_paths)
        if(CMAKE_PREFIX_PATH)
            list(APPEND search_paths ${CMAKE_PREFIX_PATH})
        endif()
        if(CMAKE_FRAMEWORK_PATH)
            list(APPEND search_paths ${CMAKE_FRAMEWORK_PATH})
        endif()
    endif()

    # Always append the install time install prefix.
    # The variable is escaped, so it is evaluated during cmake install time, so that the value
    # can be overridden with cmake --install . --prefix <path>.
    if(arg_SEARCH_IN_DESTDIR_INSTALL_PREFIX_AT_INSTALL_TIME)
        list(APPEND search_paths "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}")
    endif()

    if(arg_EXTERNAL_DOCUMENT_SEARCH_PATHS)
        list(APPEND search_paths ${arg_EXTERNAL_DOCUMENT_SEARCH_PATHS})
    endif()

    if(arg_SEARCH_IN_QT_PREFIXES AND QT6_INSTALL_PREFIX)
        list(APPEND search_paths ${QT6_INSTALL_PREFIX})
    endif()

    if(arg_SEARCH_IN_QT_PREFIXES AND QT_ADDITIONAL_PACKAGES_PREFIX_PATH)
        list(APPEND search_paths ${QT_ADDITIONAL_PACKAGES_PREFIX_PATH})
    endif()

    if(QT_ADDITIONAL_SBOM_DOCUMENT_PATHS)
        list(APPEND search_paths ${QT_ADDITIONAL_SBOM_DOCUMENT_PATHS})
    endif()

    list(REMOVE_DUPLICATES search_paths)

    set(${out_var} "${search_paths}" PARENT_SCOPE)
endfunction()

# Given an absolute or relative path for an external document, searches for it in the given
# EXTERNAL_DOCUMENT_SEARCH_PATHS and returns the found path, or an empty string.
function(_qt_internal_sbom_find_external_reference_document out_var)
    if(NOT QT_GENERATE_SBOM)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    set(opt_args "")
    set(single_args
        EXTERNAL_DOCUMENT_FILE_PATH
    )
    set(multi_args
        EXTERNAL_DOCUMENT_SEARCH_PATHS
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(document_file_path "")

    set(maybe_external_document_file_path "${arg_EXTERNAL_DOCUMENT_FILE_PATH}")

    if(IS_ABSOLUTE "${maybe_external_document_file_path}"
            AND EXISTS "${maybe_external_document_file_path}")
        set(document_file_path "${maybe_external_document_file_path}")
        set(${out_var} "${document_file_path}" PARENT_SCOPE)
        return()
    endif()

    set(relative_file_name "${maybe_external_document_file_path}")
    list(JOIN arg_EXTERNAL_DOCUMENT_SEARCH_PATHS "\n" document_dir_paths_per_line)

    foreach(document_dir_path IN LISTS arg_EXTERNAL_DOCUMENT_SEARCH_PATHS)
        set(maybe_document_file_path "${document_dir_path}/${relative_file_name}")
        if(EXISTS "${maybe_document_file_path}")
            set(document_file_path "${maybe_document_file_path}")
            break()
        endif()
    endforeach()

    if(NOT document_file_path OR NOT EXISTS "${document_file_path}")
        message(FATAL_ERROR "Could not find external SBOM document "
            "${maybe_external_document_file_path}"
            " in any of the document search paths: ${document_dir_paths_per_line} "
        )
    endif()

    set(${out_var} "${document_file_path}" PARENT_SCOPE)
endfunction()

# Queries information about the external reference document associated with the given target, and
# sets the values in the parent scope.
function(_qt_internal_sbom_query_external_reference_document target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args
        OUT_VAR_PROJECT_NAME_LOWERCASE

        OUT_VAR_RELATIVE_SPDX_V2_TAG_VALUE_DOCUMENT_PATH
        OUT_VAR_RELATIVE_SPDX_V2_JSON_DOCUMENT_PATH
        OUT_VAR_RELATIVE_CYDX_V1_6_JSON_DOCUMENT_PATH

        OUT_VAR_SPDX_V2_DOCUMENT_REF_ID
        OUT_VAR_SPDX_V2_DOCUMENT_NAMESPACE

        OUT_VAR_CYDX_URN_SERIAL_NUMBER
        OUT_VAR_CYDX_URN_BOM_VERSION
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT TARGET "${target}")
        message(FATAL_ERROR "Target '${target}' does not exist.")
    endif()

    get_target_property(is_external_project_sbom_target
        "${target}" _qt_sbom_is_external_project_sbom_target)

    if(NOT is_external_project_sbom_target)
        message(FATAL_ERROR "Target '${target}' is not an external project SBOM target.")
    endif()

    if(arg_OUT_VAR_PROJECT_NAME_LOWERCASE)
        get_target_property(project_name_lowecase "${target}"
            _qt_sbom_spdx_repo_project_name_lowercase)
        set(${arg_OUT_VAR_PROJECT_NAME_LOWERCASE} "${project_name_lowecase}" PARENT_SCOPE)
    endif()

    if(arg_OUT_VAR_RELATIVE_SPDX_V2_TAG_VALUE_DOCUMENT_PATH)
        get_target_property(document_spdx_v2_tag_value_relative_path "${target}"
            _qt_sbom_spdx_v2_document_tag_value_relative_path)
        set(${arg_OUT_VAR_RELATIVE_SPDX_V2_TAG_VALUE_DOCUMENT_PATH}
            "${document_spdx_v2_tag_value_relative_path}" PARENT_SCOPE)
    endif()

    if(arg_OUT_VAR_RELATIVE_SPDX_V2_JSON_DOCUMENT_PATH)
        get_target_property(document_spdx_v2_json_relative_path "${target}"
            _qt_sbom_spdx_v2_document_json_relative_path)
        set(${arg_OUT_VAR_RELATIVE_SPDX_V2_JSON_DOCUMENT_PATH}
            "${document_spdx_v2_json_relative_path}" PARENT_SCOPE)
    endif()

    if(arg_OUT_VAR_RELATIVE_CYDX_V1_6_JSON_DOCUMENT_PATH)
        get_target_property(document_cydx_v1_6_json_relative_path "${target}"
            _qt_sbom_cydx_v1_6_document_json_relative_path)
        set(${arg_OUT_VAR_RELATIVE_CYDX_V1_6_JSON_DOCUMENT_PATH}
            "${document_cydx_v1_6_json_relative_path}" PARENT_SCOPE)
    endif()

    if(arg_OUT_VAR_SPDX_V2_DOCUMENT_REF_ID)
        get_target_property(spdx_external_document_ref "${target}"
            _qt_sbom_spdx_v2_external_document_ref)
        set(${arg_OUT_VAR_SPDX_V2_DOCUMENT_REF_ID} "${spdx_external_document_ref}" PARENT_SCOPE)
    endif()

    if(arg_OUT_VAR_SPDX_V2_DOCUMENT_NAMESPACE)
        get_target_property(spdx_document_namespace "${target}"
            _qt_sbom_spdx_repo_document_namespace)
        set(${arg_OUT_VAR_SPDX_V2_DOCUMENT_NAMESPACE} "${spdx_document_namespace}" PARENT_SCOPE)
    endif()

    if(arg_OUT_VAR_CYDX_URN_SERIAL_NUMBER)
        get_target_property(cydx_bom_serial_number "${target}" _qt_sbom_cydx_bom_serial_number_uuid)
        set(${arg_OUT_VAR_CYDX_URN_SERIAL_NUMBER} "${cydx_bom_serial_number}" PARENT_SCOPE)
    endif()

    if(arg_OUT_VAR_CYDX_URN_BOM_VERSION)
        get_target_property(cydx_urn_bom_version "${target}" _qt_sbom_cydx_urn_bom_version)
        set(${arg_OUT_VAR_CYDX_URN_BOM_VERSION} "${cydx_urn_bom_version}" PARENT_SCOPE)
    endif()
endfunction()
