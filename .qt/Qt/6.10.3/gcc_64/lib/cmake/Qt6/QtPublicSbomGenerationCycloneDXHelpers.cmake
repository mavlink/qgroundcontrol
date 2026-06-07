# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Helper to return the path to staging cydx file, where content will be incrementally appended to.
function(_qt_internal_get_staging_area_cydx_file_path out_var)
    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)
    set(staging_area_cydx_file "${sbom_dir}/staging-${repo_project_name_lowercase}.cdx.in.toml")
    set(${out_var} "${staging_area_cydx_file}" PARENT_SCOPE)
endfunction()

# Starts recording information for the generation of a CycloneDX sbom for a project.
# Similar to _qt_internal_sbom_begin_project_generate which is the SPDX variant.
function(_qt_internal_sbom_begin_project_generate_cyclone)
    set(opt_args "")
    set(single_args
        OUTPUT
        OUTPUT_RELATIVE_PATH
        LICENSE
        COPYRIGHT
        DOWNLOAD_LOCATION
        PROJECT
        PROJECT_COMMENT
        PROJECT_FOR_SPDX_ID
        SUPPLIER
        SUPPLIER_URL
        NAMESPACE
        BOM_SERIAL_NUMBER_UUID
        CPE
        OUT_VAR_PROJECT_SPDX_ID
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_forward_function_args(
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR common_project_args
        FORWARD_SINGLE
            ${single_args}
    )

    _qt_internal_sbom_get_common_project_variables(
        ${common_project_args}
        OUT_VAR_PROJECT_NAME arg_PROJECT
        OUT_VAR_CURRENT_UTC current_utc
        OUT_VAR_CURRENT_YEAR current_year
        DEFAULT_SBOM_FILE_NAME_EXTENSION "cdx"
        OUT_VAR_OUTPUT arg_OUTPUT
        OUT_VAR_OUTPUT_RELATIVE_PATH arg_OUTPUT_RELATIVE_PATH
        OUT_VAR_PROJECT_FOR_SPDX_ID project_spdx_id
        OUT_VAR_COPYRIGHT arg_COPYRIGHT
        OUT_VAR_SUPPLIER arg_SUPPLIER
        OUT_VAR_SUPPLIER_URL arg_SUPPLIER_URL
        OUT_VAR_DEFAULT_PROJECT_COMMENT project_comment
    )
    if(arg_OUT_VAR_PROJECT_SPDX_ID)
        set(${arg_OUT_VAR_PROJECT_SPDX_ID} "${project_spdx_id}" PARENT_SCOPE)
    endif()

    _qt_internal_sbom_get_git_version_vars()

    _qt_internal_sbom_set_default_option_value_and_error_if_empty(BOM_SERIAL_NUMBER_UUID "")

    if(arg_PROJECT_COMMENT)
        string(APPEND project_comment "${arg_PROJECT_COMMENT}")
    endif()

    _qt_internal_sbom_set_default_option_value(DOWNLOAD_LOCATION "")
    set(download_location_field "")
    if(arg_DOWNLOAD_LOCATION)
        set(download_location_field "download_location = \"${arg_DOWNLOAD_LOCATION}\"")
    endif()

    set(content "
[root_component]
name = \"${arg_PROJECT}\" # Required
spdx_id = \"${project_spdx_id}\" # Required
version = \"${QT_SBOM_GIT_VERSION}\" # Required
supplier = '${arg_SUPPLIER}' # Required
supplier_url = \"${arg_SUPPLIER_URL}\" # Required
${download_location_field}
build_date = \"${current_utc}\"
serial_number_uuid = \"${arg_BOM_SERIAL_NUMBER_UUID}\" # Required
description = '''${project_comment}
'''

<PROJECT_RELATIONSHIP_PLACEHOLDER>

[[project_build_tools]]
name = \"cmake\"
version = \"${CMAKE_VERSION}\"
component_type = \"application\"
description = \"Build system tool used to build the project.\"
")

    set(sbom_format "CYDX_V1_6")
    _qt_internal_sbom_save_intro_content(
        SBOM_FORMAT "${sbom_format}"
        CONTENT "${content}")

    _qt_internal_sbom_create_sbom_staging_dir(OUT_VAR_SBOM_DIR sbom_dir)

    _qt_internal_sbom_save_project_info_in_global_properties(
        SUPPLIER "${arg_SUPPLIER}"
        SUPPLIER_URL "${arg_SUPPLIER_URL}"
        NAMESPACE "${arg_NAMESPACE}"
        PROJECT "${arg_PROJECT}"
        PROJECT_SPDX_ID "${project_spdx_id}"
    )

    _qt_internal_sbom_save_common_path_variables_in_global_properties(
        OUTPUT "${arg_OUTPUT}"
        OUTPUT_RELATIVE_PATH "${arg_OUTPUT_RELATIVE_PATH}"
        SBOM_DIR "${sbom_dir}"
        SBOM_FORMAT "${sbom_format}"
    )
endfunction()

# Finalizes the CycloneDX sbom generation for a project.
function(_qt_internal_sbom_end_project_generate_cyclone)
    set(sbom_format "CYDX_V1_6")

    _qt_internal_sbom_get_common_path_variables_from_global_properties(
        SBOM_FORMAT "${sbom_format}"
        OUT_VAR_SBOM_BUILD_OUTPUT_PATH sbom_build_output_path
        OUT_VAR_SBOM_BUILD_OUTPUT_PATH_WITHOUT_EXT sbom_build_output_path_without_ext
        OUT_VAR_SBOM_BUILD_OUTPUT_DIR sbom_build_output_dir
        OUT_VAR_SBOM_INSTALL_OUTPUT_PATH sbom_install_output_path
        OUT_VAR_SBOM_INSTALL_OUTPUT_PATH_WITHOUT_EXT sbom_install_output_path_without_ext
        OUT_VAR_SBOM_INSTALL_OUTPUT_DIR sbom_install_output_dir
    )

    if(NOT sbom_build_output_path)
        message(FATAL_ERROR "Call _qt_internal_sbom_begin_project() first")
    endif()

    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)
    _qt_internal_sbom_get_qt_repo_project_name_lower_case(real_qt_repo_project_name_lowercase)
    _qt_internal_get_current_project_sbom_dir(sbom_dir)

    _qt_internal_sbom_handle_project_relationships(
        OUTPUT_SBOM_FORMAT "${sbom_format}"
        OUT_VAR_RELATIONSHIP_STRINGS relationship_strings
    )

    set(staging_file_args "")
    if(relationship_strings)
        list(APPEND staging_file_args RELATIONSHIP_STRINGS "${relationship_strings}")
    endif()

    _qt_internal_sbom_create_sbom_staging_file(
        SBOM_FORMAT "${sbom_format}"
        SBOM_DIR "${sbom_dir}"
        REPO_PROJECT_NAME_LOWERCASE "${repo_project_name_lowercase}"
        ${staging_file_args}
    )

    # Process licenses before getting the includes.
    _qt_internal_sbom_add_recorded_licenses_cydx()

    _qt_internal_sbom_get_cmake_include_files(
        SBOM_FORMAT "${sbom_format}"
        OUT_VAR_INCLUDES includes
        OUT_VAR_POST_GENERATION_INCLUDES post_generation_includes
    )

    set(build_time_args "")
    if(includes)
        list(APPEND build_time_args INCLUDES "${includes}")
    endif()
    if(post_generation_includes)
        list(APPEND build_time_args POST_GENERATION_INCLUDES "${post_generation_includes}")
    endif()
    _qt_internal_sbom_create_build_time_sbom_targets(
        SBOM_FORMAT "${sbom_format}"
        REPO_PROJECT_NAME_LOWERCASE "${repo_project_name_lowercase}"
        REAL_QT_REPO_PROJECT_NAME_LOWERCASE "${real_qt_repo_project_name_lowercase}"
        SBOM_BUILD_OUTPUT_PATH "${sbom_build_output_path}"
        SBOM_BUILD_OUTPUT_PATH_WITHOUT_EXT "${sbom_build_output_path_without_ext}"
        SBOM_BUILD_OUTPUT_DIR "${sbom_build_output_dir}"
        OUT_VAR_ASSEMBLE_SBOM_INCLUDE_PATH assemble_sbom
        ${build_time_args}
    )

    _qt_internal_sbom_setup_multi_config_install_markers(
        SBOM_DIR "${sbom_dir}"
        SBOM_FORMAT "${sbom_format}"
        REPO_PROJECT_NAME_LOWERCASE "${repo_project_name_lowercase}"
        OUT_VAR_EXTRA_CODE_BEGIN extra_code_begin
        OUT_VAR_EXTRA_CODE_INNER_END extra_code_inner_end
    )

    _qt_internal_sbom_setup_fake_checksum(
        OUT_VAR_FAKE_CHECKSUM_CODE extra_code_begin_fake_checksum
    )
    if(extra_code_begin_fake_checksum)
        string(APPEND extra_code_begin "${extra_code_begin_fake_checksum}")
    endif()

    set(setup_sbom_install_args "")
    if(extra_code_begin)
        list(APPEND setup_sbom_install_args EXTRA_CODE_BEGIN "${extra_code_begin}")
    endif()
    if(extra_code_inner_end)
        list(APPEND setup_sbom_install_args EXTRA_CODE_INNER_END "${extra_code_inner_end}")
    endif()
    if(before_checksum_includes)
        list(APPEND setup_sbom_install_args BEFORE_CHECKSUM_INCLUDES "${before_checksum_includes}")
    endif()
    if(after_checksum_includes)
        list(APPEND setup_sbom_install_args AFTER_CHECKSUM_INCLUDES "${after_checksum_includes}")
    endif()
    if(post_generation_includes)
        list(APPEND setup_sbom_install_args POST_GENERATION_INCLUDES "${post_generation_includes}")
    endif()
    if(verify_includes)
        list(APPEND setup_sbom_install_args VERIFY_INCLUDES "${verify_includes}")
    endif()

    _qt_internal_sbom_setup_sbom_install_code(
        SBOM_FORMAT "${sbom_format}"
        REPO_PROJECT_NAME_LOWERCASE "${repo_project_name_lowercase}"
        SBOM_INSTALL_OUTPUT_PATH "${sbom_install_output_path}"
        SBOM_INSTALL_OUTPUT_PATH_WITHOUT_EXT "${sbom_install_output_path_without_ext}"
        SBOM_INSTALL_OUTPUT_DIR "${sbom_install_output_dir}"
        ASSEMBLE_SBOM_INCLUDE_PATH "${assemble_sbom}"
        ${setup_sbom_install_args}
    )

    _qt_internal_sbom_clear_cmake_include_files(
        SBOM_FORMAT "${sbom_format}"
    )
endfunction()

# Helper to add info about a package to the sbom. Usually a package is a mapping to a cmake target.
function(_qt_internal_sbom_generate_cyclone_add_package)
    set(opt_args "")
    set(single_args
        PACKAGE
        VERSION
        LICENSE_DECLARED
        LICENSE_CONCLUDED
        COPYRIGHT
        DOWNLOAD_LOCATION
        SPDXID
        COMMENT

        # Additions compared to spdx function signature.
        CYDX_SUPPLIER
        SBOM_ENTITY_TYPE
        CONTAINING_COMPONENT
        EXTERNAL_BOM_LINK
    )
    set(multi_args
        CPE

        # Additions compared to spdx function signature.
        PURL_VALUES
        CYDX_PROPERTIES
        SBOM_RELATIONSHIP_ENTRIES
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_sbom_set_default_option_value_and_error_if_empty(PACKAGE "")

    set(check_option "")
    if(arg_SPDXID)
        set(check_option "CHECK" "${arg_SPDXID}")
    endif()

    _qt_internal_sbom_get_and_check_spdx_id(
        VARIABLE arg_SPDXID
        ${check_option}
        HINTS "SPDXRef-${arg_PACKAGE}"
    )

    _qt_internal_sbom_set_default_option_value(DOWNLOAD_LOCATION "")
    _qt_internal_sbom_set_default_option_value(VERSION "")
    _qt_internal_sbom_set_default_option_value(SUPPLIER "")
    _qt_internal_sbom_set_default_option_value(LICENSE_CONCLUDED "")
    _qt_internal_sbom_set_default_option_value(COPYRIGHT "")

    set(cpe_field "")
    set(cpe_list ${arg_CPE})
    if(cpe_list)
        # Wrap values in double quotes.
        list(TRANSFORM cpe_list PREPEND "\\\"")
        list(TRANSFORM cpe_list APPEND "\\\"")
        list(JOIN cpe_list ", " cpe_string)
        set(cpe_field "cpe_list = [${cpe_string}]")
    endif()

    set(purl_field "")
    set(purl_list ${arg_PURL_VALUES})
    if(purl_list)
        # Wrap values in double quotes.
        list(TRANSFORM purl_list PREPEND "\\\"")
        list(TRANSFORM purl_list APPEND "\\\"")
        list(JOIN purl_list ", " purl_string)
        set(purl_field "purl_list = [${purl_string}]")
    endif()

    set(name_field "name = \\\"${arg_PACKAGE}\\\"")
    set(spdx_id_field "spdx_id = \\\"${arg_SPDXID}\\\"")
    set(sbom_entity_type_field "sbom_entity_type = \\\"${arg_SBOM_ENTITY_TYPE}\\\"")

    _qt_internal_sbom_get_cyclone_component_type(component_type "${arg_SBOM_ENTITY_TYPE}")
    set(component_type_field "component_type = \\\"${component_type}\\\"")

    set(version_field "")
    if(arg_VERSION)
        set(version_field "version = \\\"${arg_VERSION}\\\"")
    endif()

    set(relationships_field "")
    if(arg_SBOM_RELATIONSHIP_ENTRIES)
        _qt_internal_sbom_serialize_relationship_entries(
            OUTPUT_SBOM_FORMAT "CYDX_V1_6"
            CYDX_TOML_KEY "components."
            ESCAPE_CYDX_QUOTES
            OUT_VAR_RELATIONSHIPS_STRINGS entries_relationships_strings
            SBOM_RELATIONSHIP_ENTRIES ${arg_SBOM_RELATIONSHIP_ENTRIES}
        )
        if(entries_relationships_strings)
            # Remove duplicates, because apparently we sometimes get them for some system libraries.
            list(REMOVE_DUPLICATES entries_relationships_strings)

            list(JOIN entries_relationships_strings "\n" relationships_field)
        endif()
    endif()

    set(copyright_field "")
    if(arg_COPYRIGHT)
        set(copyright_field "copyright = '''${arg_COPYRIGHT}'''")
    endif()

    set(download_location_field "")
    if(arg_DOWNLOAD_LOCATION)
        set(download_location_field "download_location = \\\"${arg_DOWNLOAD_LOCATION}\\\"")
    endif()

    set(license_concluded_field "")
    if(arg_LICENSE_CONCLUDED)
        set(license_concluded_field
            "license_concluded_expression = \\\"${arg_LICENSE_CONCLUDED}\\\"")
    endif()

    set(supplier_field "")
    if(arg_CYDX_SUPPLIER)
        set(supplier_field "supplier = '${arg_CYDX_SUPPLIER}'")
    endif()

    set(containing_component_field "")
    if(arg_CONTAINING_COMPONENT)
        set(containing_component_field "containing_component = \\\"${arg_CONTAINING_COMPONENT}\\\"")
    endif()

    set(external_bom_link_field "")
    if(arg_EXTERNAL_BOM_LINK)
        set(external_bom_link_field "external_bom_link = \\\"${arg_EXTERNAL_BOM_LINK}\\\"")
    endif()

    set(properties_field "")
    if(arg_CYDX_PROPERTIES)
        _qt_internal_sbom_handle_cydx_properties(
            CYDX_PROPERTIES ${arg_CYDX_PROPERTIES}
            OUT_VAR_CYDX_PROPERTIES_STRING properties_field
        )
    endif()

    _qt_internal_get_staging_area_cydx_file_path(staging_area_spdx_file)

    set(content "
file(APPEND \"${staging_area_spdx_file}\"
\"

[[components]]
${name_field}
${spdx_id_field}
${sbom_entity_type_field}
${component_type_field}
${version_field}
${download_location_field}
${copyright_field}
${supplier_field}
${containing_component_field}
${external_bom_link_field}
${cpe_field}
${purl_field}
${license_concluded_field}
${properties_field}
${relationships_field}
\"
)
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(package_sbom "${sbom_dir}/cydx_${arg_SPDXID}.cmake")
    file(GENERATE OUTPUT "${package_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_include_files_cydx "${package_sbom}")
endfunction()

# Adds all recorded custom licenses to the toml file.
function(_qt_internal_sbom_add_recorded_licenses_cydx)
    get_property(license_ids GLOBAL PROPERTY _qt_internal_sbom_cydx_licenses)
    if(NOT license_ids)
        return()
    endif()

    set(content "")

    foreach(license_id IN LISTS license_ids)
        get_property(license_text GLOBAL PROPERTY
            _qt_internal_sbom_cydx_licenses_${license_id}_text)

        string(APPEND content "
[[licenses]]
license_id = \\\"${license_id}\\\"
text = '''${license_text}
'''
")
    endforeach()

    _qt_internal_get_staging_area_cydx_file_path(staging_area_spdx_file)

    set(content "
file(APPEND \"${staging_area_spdx_file}\"
\"
${content}
\"
)
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(snippet "${sbom_dir}/cydx_license_info.cmake")
    file(GENERATE OUTPUT "${snippet}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_end_include_files_cydx "${snippet}")

    # Clean up before configuring next repo project.
    set_property(GLOBAL PROPERTY _qt_internal_sbom_cydx_licenses "")
    foreach(license_id IN LISTS license_ids)
        set_property(GLOBAL PROPERTY _qt_internal_sbom_cydx_licenses_${license_id}_text "")
    endforeach()
endfunction()
