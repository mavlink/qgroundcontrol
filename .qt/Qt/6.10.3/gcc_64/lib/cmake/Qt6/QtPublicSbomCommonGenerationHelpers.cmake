# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(CMAKE_VERSION VERSION_LESS 3.17.0)
    set(CMAKE_CURRENT_FUNCTION_LIST_DIR "${CMAKE_CURRENT_LIST_DIR}")
endif()

# Helper function to get common project related variables for SBOM generation for both SPDX and
# CycloneDX formats.
function(_qt_internal_sbom_get_common_project_variables)
    set(opt_args "")
    set(single_args
        # Forwarded
        OUTPUT
        OUTPUT_RELATIVE_PATH
        COPYRIGHT
        PROJECT
        PROJECT_FOR_SPDX_ID
        SUPPLIER
        SUPPLIER_URL

        # Custom inputs
        DEFAULT_SBOM_FILE_NAME_EXTENSION

        # Out vars
        OUT_VAR_PROJECT_NAME
        OUT_VAR_CURRENT_UTC
        OUT_VAR_CURRENT_YEAR
        OUT_VAR_OUTPUT
        OUT_VAR_OUTPUT_RELATIVE_PATH
        OUT_VAR_PROJECT_FOR_SPDX_ID
        OUT_VAR_COPYRIGHT
        OUT_VAR_SUPPLIER
        OUT_VAR_SUPPLIER_URL
        OUT_VAR_DEFAULT_PROJECT_COMMENT
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")

    if(QT_SBOM_FAKE_TIMESTAMP)
        set(current_utc "2590-01-01T11:33:55Z")
        set(current_year "2590")
    else()
        string(TIMESTAMP current_utc UTC)
        string(TIMESTAMP current_year "%Y" UTC)
    endif()

    set(${arg_OUT_VAR_CURRENT_UTC} "${current_utc}" PARENT_SCOPE)
    set(${arg_OUT_VAR_CURRENT_YEAR} "${current_year}" PARENT_SCOPE)

    _qt_internal_sbom_set_default_option_value(PROJECT "${PROJECT_NAME}")
    set(${arg_OUT_VAR_PROJECT_NAME} "${arg_PROJECT}" PARENT_SCOPE)

    _qt_internal_sbom_get_git_version_vars()

    _qt_internal_path_join(default_sbom_file_name
        "${arg_PROJECT}"
        "${arg_PROJECT}-sbom-${QT_SBOM_GIT_VERSION_PATH}.${arg_DEFAULT_SBOM_FILE_NAME_EXTENSION}")
    _qt_internal_path_join(default_install_sbom_path
        "\${CMAKE_INSTALL_PREFIX}/" "${CMAKE_INSTALL_DATAROOTDIR}" "${default_sbom_file_name}"
    )

    _qt_internal_sbom_set_default_option_value(OUTPUT "${default_install_sbom_path}")
    _qt_internal_sbom_set_default_option_value(OUTPUT_RELATIVE_PATH
        "${default_sbom_file_name}")

    set(${arg_OUT_VAR_OUTPUT} "${arg_OUTPUT}" PARENT_SCOPE)
    set(${arg_OUT_VAR_OUTPUT_RELATIVE_PATH} "${arg_OUTPUT_RELATIVE_PATH}" PARENT_SCOPE)

    _qt_internal_sbom_set_default_option_value(PROJECT_FOR_SPDX_ID "Package-${arg_PROJECT}")
    string(REGEX REPLACE "[^A-Za-z0-9.]+" "-" arg_PROJECT_FOR_SPDX_ID "${arg_PROJECT_FOR_SPDX_ID}")
    string(REGEX REPLACE "-+$" "" arg_PROJECT_FOR_SPDX_ID "${arg_PROJECT_FOR_SPDX_ID}")

    # Prevent collision with other generated SPDXID with -[0-9]+ suffix.
    string(REGEX REPLACE "-([0-9]+)$" "\\1" arg_PROJECT_FOR_SPDX_ID "${arg_PROJECT_FOR_SPDX_ID}")

    _qt_internal_sbom_get_spdx_id_unique_suffix(spdx_id_unique_suffix)
    set(project_spdx_id "SPDXRef-${arg_PROJECT_FOR_SPDX_ID}${spdx_id_unique_suffix}")

    set(${arg_OUT_VAR_PROJECT_FOR_SPDX_ID} "${project_spdx_id}" PARENT_SCOPE)

    _qt_internal_sbom_set_default_option_value_and_error_if_empty(SUPPLIER "")
    set(${arg_OUT_VAR_SUPPLIER} "${arg_SUPPLIER}" PARENT_SCOPE)

    _qt_internal_sbom_set_default_option_value_and_error_if_empty(SUPPLIER_URL
        "${PROJECT_HOMEPAGE_URL}")
    set(${arg_OUT_VAR_SUPPLIER_URL} "${arg_SUPPLIER_URL}" PARENT_SCOPE)

    _qt_internal_sbom_set_default_option_value(COPYRIGHT "${current_year} ${arg_SUPPLIER}")
    set(${arg_OUT_VAR_COPYRIGHT} "${arg_COPYRIGHT}" PARENT_SCOPE)

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(cmake_configs "${CMAKE_CONFIGURATION_TYPES}")
    else()
        set(cmake_configs "${CMAKE_BUILD_TYPE}")
    endif()

    set(cmake_version "Built by CMake ${CMAKE_VERSION}")
    set(system_name_and_processor "${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})")
    set(default_project_comment
        "${cmake_version} with ${cmake_configs} configuration for ${system_name_and_processor}")
    set(${arg_OUT_VAR_DEFAULT_PROJECT_COMMENT} "${default_project_comment}" PARENT_SCOPE)
endfunction()

# Helper function to save SBOM project path values like relative build and install dirs,
# in global properties.
function(_qt_internal_sbom_save_common_path_variables_in_global_properties)
    set(opt_args "")
    set(single_args
        OUTPUT
        OUTPUT_RELATIVE_PATH
        SBOM_DIR
        SBOM_FORMAT
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    get_filename_component(output_file_name_without_ext "${arg_OUTPUT}" NAME_WLE)
    get_filename_component(output_file_ext "${arg_OUTPUT}" LAST_EXT)

    set(computed_sbom_file_name_without_ext "${output_file_name_without_ext}${multi_config_suffix}")
    set(computed_sbom_file_name "${output_file_name_without_ext}${output_file_ext}")

    # In a super build and in a no-prefix build, put all the build time sboms into the same dir in,
    # in the qtbase build dir.
    if(QT_BUILDING_QT AND (QT_SUPERBUILD OR (NOT QT_WILL_INSTALL)))
        set(build_sbom_root_dir "${QT_BUILD_DIR}")
    else()
        set(build_sbom_root_dir "${arg_SBOM_DIR}")
    endif()

    get_filename_component(output_relative_dir "${arg_OUTPUT_RELATIVE_PATH}" DIRECTORY)

    set(build_sbom_dir "${build_sbom_root_dir}/${output_relative_dir}")
    set(build_sbom_path "${build_sbom_dir}/${computed_sbom_file_name}")
    set(build_sbom_path_without_ext
        "${build_sbom_dir}/${computed_sbom_file_name_without_ext}")

    set(install_sbom_path "${arg_OUTPUT}")

    get_filename_component(install_sbom_dir "${install_sbom_path}" DIRECTORY)
    set(install_sbom_path_without_ext "${install_sbom_dir}/${output_file_name_without_ext}")

    if(arg_SBOM_FORMAT STREQUAL "SPDX_V2")
        set(suffix "")
    elseif(arg_SBOM_FORMAT STREQUAL "CYDX_V1_6")
        set(suffix "_cydx")
    else()
        message(FATAL_ERROR "Unknown SBOM_FORMAT: ${arg_SBOM_FORMAT}")
    endif()

    set_property(GLOBAL PROPERTY _qt_sbom_build_output_path${suffix} "${build_sbom_path}")
    set_property(GLOBAL PROPERTY _qt_sbom_build_output_path_without_ext${suffix}
        "${build_sbom_path_without_ext}")
    set_property(GLOBAL PROPERTY _qt_sbom_build_output_dir${suffix} "${build_sbom_dir}")

    set_property(GLOBAL PROPERTY _qt_sbom_install_output_path${suffix} "${install_sbom_path}")
    set_property(GLOBAL PROPERTY _qt_sbom_install_output_path_without_ext${suffix}
        "${install_sbom_path_without_ext}")
    set_property(GLOBAL PROPERTY _qt_sbom_install_output_dir${suffix} "${install_sbom_dir}")
endfunction()

# Helper function to get SBOM project path values like relative build and install dirs,
function(_qt_internal_sbom_get_common_path_variables_from_global_properties)
    set(opt_args "")
    set(single_args
        SBOM_FORMAT
        OUT_VAR_SBOM_BUILD_OUTPUT_PATH
        OUT_VAR_SBOM_BUILD_OUTPUT_PATH_WITHOUT_EXT
        OUT_VAR_SBOM_BUILD_OUTPUT_DIR
        OUT_VAR_SBOM_INSTALL_OUTPUT_PATH
        OUT_VAR_SBOM_INSTALL_OUTPUT_PATH_WITHOUT_EXT
        OUT_VAR_SBOM_INSTALL_OUTPUT_DIR
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(arg_SBOM_FORMAT STREQUAL "SPDX_V2")
        set(suffix "")
    elseif(arg_SBOM_FORMAT STREQUAL "CYDX_V1_6")
        set(suffix "_cydx")
    endif()

    get_property(sbom_build_output_path GLOBAL PROPERTY _qt_sbom_build_output_path${suffix})
    get_property(sbom_build_output_path_without_ext GLOBAL PROPERTY
        _qt_sbom_build_output_path_without_ext${suffix})
    get_property(sbom_build_output_dir GLOBAL PROPERTY _qt_sbom_build_output_dir${suffix})

    get_property(sbom_install_output_path GLOBAL PROPERTY _qt_sbom_install_output_path${suffix})
    get_property(sbom_install_output_path_without_ext GLOBAL PROPERTY
        _qt_sbom_install_output_path_without_ext${suffix})
    get_property(sbom_install_output_dir GLOBAL PROPERTY _qt_sbom_install_output_dir${suffix})

    if(arg_OUT_VAR_SBOM_BUILD_OUTPUT_PATH)
        set(${arg_OUT_VAR_SBOM_BUILD_OUTPUT_PATH} "${sbom_build_output_path}" PARENT_SCOPE)
    endif()
    if(arg_OUT_VAR_SBOM_BUILD_OUTPUT_PATH_WITHOUT_EXT)
        set(${arg_OUT_VAR_SBOM_BUILD_OUTPUT_PATH_WITHOUT_EXT}
            "${sbom_build_output_path_without_ext}" PARENT_SCOPE)
    endif()
    if(arg_OUT_VAR_SBOM_BUILD_OUTPUT_DIR)
        set(${arg_OUT_VAR_SBOM_BUILD_OUTPUT_DIR} "${sbom_build_output_dir}" PARENT_SCOPE)
    endif()
    if(arg_OUT_VAR_SBOM_INSTALL_OUTPUT_PATH)
        set(${arg_OUT_VAR_SBOM_INSTALL_OUTPUT_PATH} "${sbom_install_output_path}" PARENT_SCOPE)
    endif()
    if(arg_OUT_VAR_SBOM_INSTALL_OUTPUT_PATH_WITHOUT_EXT)
        set(${arg_OUT_VAR_SBOM_INSTALL_OUTPUT_PATH_WITHOUT_EXT}
            "${sbom_install_output_path_without_ext}" PARENT_SCOPE)
    endif()
    if(arg_OUT_VAR_SBOM_INSTALL_OUTPUT_DIR)
        set(${arg_OUT_VAR_SBOM_INSTALL_OUTPUT_DIR} "${sbom_install_output_dir}" PARENT_SCOPE)
    endif()
endfunction()

# Create the build directory that will contain all the intermediate generated sbom files.
function(_qt_internal_sbom_create_sbom_staging_dir)
    set(opt_args "")
    set(single_args
        OUT_VAR_SBOM_DIR
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR_SBOM_DIR)
        message(FATAL_ERROR "OUT_VAR_SBOM_DIR argument is required.")
    endif()

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    file(MAKE_DIRECTORY "${sbom_dir}")
    set(${arg_OUT_VAR_SBOM_DIR} "${sbom_dir}" PARENT_SCOPE)
endfunction()

# Helper function to create a staging file for SBOM generation.
# It is the file that will be incrementally assembled by having content appended to it.
# Also creates the intro file that will add the assembled content for the SBOM project, aka for the
# main SPDX package or root CycloneDX component.
function(_qt_internal_sbom_create_sbom_staging_file)
    set(opt_args "")
    set(single_args
        SBOM_FORMAT
        REPO_PROJECT_NAME_LOWERCASE
        SBOM_DIR
    )
    set(multi_args
        RELATIONSHIP_STRINGS
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_SBOM_DIR)
        message(FATAL_ERROR "SBOM_DIR argument is required")
    endif()
    set(sbom_dir "${arg_SBOM_DIR}")

    if(arg_SBOM_FORMAT STREQUAL "SPDX_V2")
        set(doc_base_name "SPDXRef-DOCUMENT")
        set(doc_extension "spdx.in")
        set(suffix "")
        set(extra_content "
        set(QT_SBOM_EXTERNAL_DOC_REFS \"\")
")
        _qt_internal_get_staging_area_spdx_file_path(staging_area_file)
        set(starting_message "Starting SPDX SBOM generation in build dir: ${staging_area_file}")
        set(extra_intro_content "")
        set(project_relationship_strings "")
        if(arg_RELATIONSHIP_STRINGS)
            list(JOIN arg_RELATIONSHIP_STRINGS "\n" arg_RELATIONSHIP_STRINGS)

            # Prepend a newline to separate from the very first always there relationship.
            string(APPEND extra_intro_content "\n${arg_RELATIONSHIP_STRINGS}")
        endif()
    elseif(arg_SBOM_FORMAT STREQUAL "CYDX_V1_6")
        set(doc_base_name "cydx-document")
        set(doc_extension "cdx.in.toml")
        set(suffix "_cydx")
        set(extra_content "")
        _qt_internal_get_staging_area_cydx_file_path(staging_area_file)
        set(starting_message
            "Starting CycloneDX SBOM TOML file generation in build dir: ${staging_area_file}")
        set(extra_intro_content "")

        if(arg_RELATIONSHIP_STRINGS)
            list(JOIN arg_RELATIONSHIP_STRINGS "\n" arg_RELATIONSHIP_STRINGS)
            set(project_relationship_strings "${arg_RELATIONSHIP_STRINGS}")
        else()
            set(project_relationship_strings "")
        endif()
    endif()

    # Assemble final intro content.
    get_property(intro_content GLOBAL PROPERTY _qt_sbom_project_intro_content${suffix})
    if(NOT intro_content)
        message(FATAL_ERROR "Missing intro content for SBOM generation.")
    endif()

    if(arg_SBOM_FORMAT STREQUAL "CYDX_V1_6")
        string(REPLACE "<PROJECT_RELATIONSHIP_PLACEHOLDER>" "${project_relationship_strings}"
            intro_content "${intro_content}")
    endif()

    if(extra_intro_content)
        string(APPEND intro_content "${extra_intro_content}")
    endif()

    # Final new line to separate the main package from rest of info.
    if(arg_SBOM_FORMAT STREQUAL "SPDX_V2")
        string(APPEND intro_content "\n")
    endif()

    # Generate project document intro file.
    set(document_intro_file_name
        "${sbom_dir}/${doc_base_name}-${arg_REPO_PROJECT_NAME_LOWERCASE}.${doc_extension}")
    file(GENERATE OUTPUT "${document_intro_file_name}" CONTENT "${intro_content}")

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(multi_config_suffix "-$<CONFIG>")
    else()
        set(multi_config_suffix "")
    endif()

    # Create cmake file to append the document intro to the staging file.
    set(create_staging_file
        "${sbom_dir}/append_document_to_staging${suffix}${multi_config_suffix}.cmake")

    set(content "
        cmake_minimum_required(VERSION 3.16)
        message(STATUS \"${starting_message}\")
        ${extra_content}
        file(READ \"${document_intro_file_name}\" content)
        # Override any previous file because we're starting from scratch.
        file(WRITE \"${staging_area_file}\" \"\${content}\")
")
    file(GENERATE OUTPUT "${create_staging_file}" CONTENT "${content}")
    set_property(GLOBAL PROPERTY _qt_sbom_cmake_intro_file${suffix} "${create_staging_file}")
endfunction()

# Helper function to save common project info like supplier, project name, spdx id in global
# properties.
function(_qt_internal_sbom_save_project_info_in_global_properties)
    set(opt_args "")
    set(single_args
        SUPPLIER
        SUPPLIER_URL
        NAMESPACE
        PROJECT
        PROJECT_SPDX_ID
    )
    set(multi_args
        EXTERNAL_REFERENCE_SBOM_DIRS
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set_property(GLOBAL PROPERTY _qt_sbom_project_supplier "${arg_SUPPLIER}")
    set_property(GLOBAL PROPERTY _qt_sbom_project_supplier_url "${arg_SUPPLIER_URL}")
    set_property(GLOBAL PROPERTY _qt_sbom_project_namespace "${arg_NAMESPACE}")

    set_property(GLOBAL PROPERTY _qt_sbom_project_name "${arg_PROJECT}")
    set_property(GLOBAL PROPERTY _qt_sbom_project_spdx_id "${arg_PROJECT_SPDX_ID}")

    # Used for finding external reference spdx documents.
    if(arg_EXTERNAL_REFERENCE_SBOM_DIRS)
        set_property(GLOBAL APPEND PROPERTY _qt_internal_sbom_dirs
            ${arg_EXTERNAL_REFERENCE_SBOM_DIRS})
    endif()
endfunction()

# Helper function to save sbom intro content (per sbom format) in a global property.
function(_qt_internal_sbom_save_intro_content)
    set(opt_args "")
    set(single_args
        SBOM_FORMAT
        CONTENT
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(arg_SBOM_FORMAT STREQUAL "SPDX_V2")
        set(suffix "")
    elseif(arg_SBOM_FORMAT STREQUAL "CYDX_V1_6")
        set(suffix "_cydx")
    else()
        message(FATAL_ERROR "Unknown SBOM_FORMAT: ${arg_SBOM_FORMAT}")
    endif()

    set_property(GLOBAL PROPERTY _qt_sbom_project_intro_content${suffix} "${content}")
endfunction()

# Helper function to get cmake include files for SBOM generation from global properties.
function(_qt_internal_sbom_get_cmake_include_files)
    set(opt_args "")
    set(single_args
        SBOM_FORMAT
        OUT_VAR_INCLUDES
        OUT_VAR_BEFORE_CHECKSUM_INCLUDES
        OUT_VAR_AFTER_CHECKSUM_INCLUDES
        OUT_VAR_POST_GENERATION_INCLUDES
        OUT_VAR_VERIFY_INCLUDES
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(arg_SBOM_FORMAT STREQUAL "SPDX_V2")
        set(suffix "")
    elseif(arg_SBOM_FORMAT STREQUAL "CYDX_V1_6")
        set(suffix "_cydx")
    endif()

    _qt_internal_sbom_collect_cmake_include_files(includes
        JOIN_WITH_NEWLINES
        PROPERTIES
            _qt_sbom_cmake_intro_file${suffix}
            _qt_sbom_cmake_include_files${suffix}
            _qt_sbom_cmake_end_include_files${suffix}
    )

    # Before checksum includes are included after the verification codes have been collected
    # and before their merged checksum(s) has been computed.
    _qt_internal_sbom_collect_cmake_include_files(before_checksum_includes
        JOIN_WITH_NEWLINES
        PROPERTIES _qt_sbom_cmake_before_checksum_include_files${suffix}
    )

    # After checksum includes are included after the checksum has been computed and written to the
    # QT_SBOM_VERIFICATION_CODE variable.
    _qt_internal_sbom_collect_cmake_include_files(after_checksum_includes
        JOIN_WITH_NEWLINES
        PROPERTIES _qt_sbom_cmake_after_checksum_include_files${suffix}
    )

    # Post generation includes are included for both build and install time sboms, after
    # sbom generation has finished.
    _qt_internal_sbom_collect_cmake_include_files(post_generation_includes
        JOIN_WITH_NEWLINES
        PROPERTIES _qt_sbom_cmake_post_generation_include_files${suffix}
    )

    # Verification only makes sense on installation, where the checksums are present.
    _qt_internal_sbom_collect_cmake_include_files(verify_includes
        JOIN_WITH_NEWLINES
        PROPERTIES _qt_sbom_cmake_verify_include_files${suffix}
    )

    if(arg_OUT_VAR_INCLUDES)
        set(${arg_OUT_VAR_INCLUDES} "${includes}" PARENT_SCOPE)
    endif()
    if(arg_OUT_VAR_INCLUDES)
        set(${arg_OUT_VAR_BEFORE_CHECKSUM_INCLUDES} "${before_checksum_includes}" PARENT_SCOPE)
    endif()
    if(arg_OUT_VAR_INCLUDES)
        set(${arg_OUT_VAR_AFTER_CHECKSUM_INCLUDES} "${after_checksum_includes}" PARENT_SCOPE)
    endif()
    if(arg_OUT_VAR_INCLUDES)
        set(${arg_OUT_VAR_POST_GENERATION_INCLUDES} "${post_generation_includes}" PARENT_SCOPE)
    endif()
    if(arg_OUT_VAR_INCLUDES)
        set(${arg_OUT_VAR_VERIFY_INCLUDES} "${verify_includes}" PARENT_SCOPE)
    endif()
endfunction()

# Clears cmake include files for the current project from the global properties.
function(_qt_internal_sbom_clear_cmake_include_files)
    set(opt_args "")
    set(single_args
        SBOM_FORMAT
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(arg_SBOM_FORMAT STREQUAL "SPDX_V2")
        set(suffix "")
    elseif(arg_SBOM_FORMAT STREQUAL "CYDX_V1_6")
        set(suffix "_cydx")
    endif()

    # Clean up properties, so that they are empty for possible next repo in a top-level build.
    set_property(GLOBAL PROPERTY _qt_sbom_cmake_include_files${suffix} "")
    set_property(GLOBAL PROPERTY _qt_sbom_cmake_end_include_files${suffix} "")
    set_property(GLOBAL PROPERTY _qt_sbom_cmake_before_checksum_include_files${suffix} "")
    set_property(GLOBAL PROPERTY _qt_sbom_cmake_after_checksum_include_files${suffix} "")
    set_property(GLOBAL PROPERTY _qt_sbom_cmake_post_generation_include_files${suffix} "")
    set_property(GLOBAL PROPERTY _qt_sbom_cmake_verify_include_files${suffix} "")
endfunction()

# Creates cmake build targets to create build-time SBOMs (for testing purposes only, because they
# lack checksums for installed files).
# Also creates the assemble_sbom cmake file that is used by both build-time and install-time
# sbom generation.
function(_qt_internal_sbom_create_build_time_sbom_targets)
    set(opt_args "")
    set(single_args
        SBOM_FORMAT
        REPO_PROJECT_NAME_LOWERCASE
        REAL_QT_REPO_PROJECT_NAME_LOWERCASE
        SBOM_BUILD_OUTPUT_PATH
        SBOM_BUILD_OUTPUT_PATH_WITHOUT_EXT
        SBOM_BUILD_OUTPUT_DIR
        INCLUDES
        POST_GENERATION_INCLUDES
        OUT_VAR_ASSEMBLE_SBOM_INCLUDE_PATH
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(arg_SBOM_FORMAT STREQUAL "SPDX_V2")
        set(suffix "")
        set(extra_content "
        set(QT_SBOM_PACKAGES \"\")
        set(QT_SBOM_PACKAGES_WITH_VERIFICATION_CODES \"\")
")
        _qt_internal_get_staging_area_spdx_file_path(staging_area_file)
        set(final_message "Finalizing SPDX SBOM generation in build dir")
        set(build_comment "SPDX document")
    elseif(arg_SBOM_FORMAT STREQUAL "CYDX_V1_6")
        set(suffix "_cydx")
        set(extra_content "")
        _qt_internal_get_staging_area_cydx_file_path(staging_area_file)
        set(final_message "Finalizing Cyclone DX SBOM TOML generation in build dir")
        set(build_comment "Cyclone DX document")
    endif()

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(multi_config_suffix "-$<CONFIG>")
    else()
        set(multi_config_suffix "")
    endif()

    if(QT_BUILD_DIR)
        set(build_sbom_install_prefix "${QT_BUILD_DIR}")
    else()
        set(build_sbom_install_prefix "${CMAKE_BINARY_DIR}")
    endif()

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(content "
        # Include helpers functions.
        include(\"${CMAKE_CURRENT_FUNCTION_LIST_DIR}/QtPublicCMakeHelpers.cmake\")
        include(\"${CMAKE_CURRENT_FUNCTION_LIST_DIR}/QtPublicSbomExternalReferenceHelpers.cmake\")

        # QT_SBOM_BUILD_TIME be set to FALSE at install time, so don't override if it's set.
        # This allows reusing the same cmake file for both build and install.
        if(NOT DEFINED QT_SBOM_BUILD_TIME)
            set(QT_SBOM_BUILD_TIME TRUE)

            # Set CMAKE_INSTALL_PREFIX for build time SBOMs to the build dir,
            # because a repo's SBOM generation might refer to it, and when it is empty,
            # it will try to write into a subdirectory of the root dir, e.g. '/sbom' dir,
            # failing due to lack of permissions.
            if(NOT DEFINED CMAKE_INSTALL_PREFIX OR NOT CMAKE_INSTALL_PREFIX)
                set(QT_SBOM_BUILD_SBOM_INSTALL_PREFIX \"${build_sbom_install_prefix}\")
                set(CMAKE_INSTALL_PREFIX \"\${QT_SBOM_BUILD_SBOM_INSTALL_PREFIX}\")
            endif()
        endif()
        if(NOT QT_SBOM_OUTPUT_PATH)
            set(QT_SBOM_OUTPUT_DIR \"${arg_SBOM_BUILD_OUTPUT_DIR}\")
            set(QT_SBOM_OUTPUT_PATH \"${arg_SBOM_BUILD_OUTPUT_PATH}\")
            set(QT_SBOM_OUTPUT_PATH_WITHOUT_EXT \"${arg_SBOM_BUILD_OUTPUT_PATH_WITHOUT_EXT}\")
            file(MAKE_DIRECTORY \"${arg_SBOM_BUILD_OUTPUT_DIR}\")
        endif()
        ${extra_content}
        ${arg_INCLUDES}
        if(QT_SBOM_BUILD_TIME)
            message(STATUS \"${final_message}: \${QT_SBOM_OUTPUT_PATH}\")
            configure_file(\"${staging_area_file}\" \"\${QT_SBOM_OUTPUT_PATH}\")
            ${arg_POST_GENERATION_INCLUDES}
        endif()
")
    set(assemble_sbom "${sbom_dir}/assemble_sbom${suffix}${multi_config_suffix}.cmake")
    file(GENERATE OUTPUT "${assemble_sbom}" CONTENT "${content}")

    if(NOT TARGET sbom)
        add_custom_target(sbom)
    endif()

    # Create a build target to create a build-time sbom (no verification codes or sha1s).
    set(repo_sbom_target "sbom_${arg_REPO_PROJECT_NAME_LOWERCASE}${suffix}")
    set(comment "")
    string(APPEND comment "Assembling build time ${build_comment} without checksums for "
        "${arg_REPO_PROJECT_NAME_LOWERCASE}. Just for testing.")
    add_custom_target(${repo_sbom_target}
        COMMAND "${CMAKE_COMMAND}" -P "${assemble_sbom}"
        COMMENT "${comment}"
        VERBATIM
        USES_TERMINAL # To avoid running two configs of the command in parallel
    )

    get_cmake_property(qt_repo_deps _qt_repo_deps_${arg_REAL_QT_REPO_PROJECT_NAME_LOWERCASE})
    if(qt_repo_deps)
        foreach(repo_dep IN LISTS qt_repo_deps)
            set(repo_dep_sbom "sbom_${repo_dep}${suffix}")
            if(TARGET "${repo_dep_sbom}")
                add_dependencies(${repo_sbom_target} ${repo_dep_sbom})
            endif()
        endforeach()
    endif()

    add_dependencies(sbom ${repo_sbom_target})

    set(${arg_OUT_VAR_ASSEMBLE_SBOM_INCLUDE_PATH} "${assemble_sbom}" PARENT_SCOPE)
endfunction()

# Helper function to setup install markers for multi-config generators.
# Makes sure to wait for all configurations to finish installation before actually generating
# the SBOM and removing the markers.
function(_qt_internal_sbom_setup_multi_config_install_markers)
    set(opt_args "")
    set(single_args
        SBOM_DIR
        SBOM_FORMAT
        REPO_PROJECT_NAME_LOWERCASE
        OUT_VAR_EXTRA_CODE_BEGIN
        OUT_VAR_EXTRA_CODE_INNER_END
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(extra_code_begin "")
    set(extra_code_inner_end "")

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(NOT is_multi_config)
        set(${arg_OUT_VAR_EXTRA_CODE_BEGIN} "${extra_code_begin}" PARENT_SCOPE)
        set(${arg_OUT_VAR_EXTRA_CODE_INNER_END} "${extra_code_inner_end}" PARENT_SCOPE)
        return()
    endif()

    if(arg_SBOM_FORMAT STREQUAL "SPDX_V2")
        set(suffix "_spdx")
    elseif(arg_SBOM_FORMAT STREQUAL "CYDX_V1_6")
        set(suffix "_cydx")
    endif()

    set(configs ${CMAKE_CONFIGURATION_TYPES})

    set(install_markers_dir "${arg_SBOM_DIR}")
    set(install_marker_path "${install_markers_dir}/finished_install${suffix}-$<CONFIG>.cmake")

    set(install_marker_code "
        message(STATUS \"Writing install marker for config $<CONFIG>: ${install_marker_path} \")
        file(WRITE \"${install_marker_path}\" \"\")
")

    install(CODE "${install_marker_code}" COMPONENT sbom)
    if(QT_SUPERBUILD)
        install(CODE "${install_marker_code}"
            COMPONENT "sbom_${arg_REPO_PROJECT_NAME_LOWERCASE}${suffix}"
            EXCLUDE_FROM_ALL)
    endif()

    set(install_markers "")
    foreach(config IN LISTS configs)
        set(marker_path "${install_markers_dir}/finished_install${suffix}-${config}.cmake")
        list(APPEND install_markers "${marker_path}")
        # Remove the markers on reconfiguration, just in case there are stale ones.
        if(EXISTS "${marker_path}")
            file(REMOVE "${marker_path}")
        endif()
    endforeach()

    # Escape the semicolons in install_makers, so they don't break argument parsing in
    # _qt_internal_sbom_setup_sbom_install_code when they are forwarded there.
    string(REPLACE ";" "\\;" install_markers "${install_markers}")

    set(extra_code_begin "
    set(QT_SBOM_INSTALL_MARKERS${suffix} \"${install_markers}\")
    foreach(QT_SBOM_INSTALL_MARKER IN LISTS QT_SBOM_INSTALL_MARKERS${suffix})
        if(NOT EXISTS \"\${QT_SBOM_INSTALL_MARKER}\")
            set(QT_SBOM_INSTALLED_ALL_CONFIGS${suffix} FALSE)
        endif()
    endforeach()
")
    set(extra_code_inner_end "
        foreach(QT_SBOM_INSTALL_MARKER IN LISTS QT_SBOM_INSTALL_MARKERS${suffix})
            message(STATUS
                \"Removing install marker: \${QT_SBOM_INSTALL_MARKER} \")
            file(REMOVE \"\${QT_SBOM_INSTALL_MARKER}\")
        endforeach()
")

    set(${arg_OUT_VAR_EXTRA_CODE_BEGIN} "${extra_code_begin}" PARENT_SCOPE)
    set(${arg_OUT_VAR_EXTRA_CODE_INNER_END} "${extra_code_inner_end}" PARENT_SCOPE)
endfunction()

# Helper function to setup the fake checksum code snippet.
function(_qt_internal_sbom_setup_fake_checksum)
    set(opt_args "")
    set(single_args
        OUT_VAR_FAKE_CHECKSUM_CODE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    # Allow skipping checksum computation for testing purposes, while installing just the sbom
    # documents, without requiring to build and install all the actual files.
    set(extra_code "")
    if(QT_SBOM_FAKE_CHECKSUM)
        string(APPEND extra_code "
            set(QT_SBOM_FAKE_CHECKSUM TRUE)")
    endif()

    set(${arg_OUT_VAR_FAKE_CHECKSUM_CODE} "${extra_code}" PARENT_SCOPE)
endfunction()

# Helper function to setup the install-time SBOM generation code.
function(_qt_internal_sbom_setup_sbom_install_code)
    set(opt_args "")
    set(single_args
        SBOM_FORMAT
        REPO_PROJECT_NAME_LOWERCASE

        SBOM_INSTALL_OUTPUT_PATH
        SBOM_INSTALL_OUTPUT_PATH_WITHOUT_EXT
        SBOM_INSTALL_OUTPUT_DIR

        ASSEMBLE_SBOM_INCLUDE_PATH

        EXTRA_CODE_BEGIN
        EXTRA_CODE_INNER_END
        PROCESS_VERIFICATION_CODES

        BEFORE_CHECKSUM_INCLUDES
        AFTER_CHECKSUM_INCLUDES
        POST_GENERATION_INCLUDES
        VERIFY_INCLUDES
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(arg_SBOM_FORMAT STREQUAL "SPDX_V2")
        set(suffix "_spdx")
        _qt_internal_get_staging_area_spdx_file_path(staging_area_file)
        set(final_message "Finalizing SBOM generation in install dir")
        set(process_verification_codes "
            include(\"${arg_PROCESS_VERIFICATION_CODES}\")
")
    elseif(arg_SBOM_FORMAT STREQUAL "CYDX_V1_6")
        set(suffix "_cydx")
        set(final_message "Finalizing intermediate TOML generation in install dir")

        set(process_verification_codes "")
        _qt_internal_get_staging_area_cydx_file_path(staging_area_file)
    endif()

    set(assemble_sbom_install "
        set(QT_SBOM_INSTALLED_ALL_CONFIGS${suffix} TRUE)
        ${arg_EXTRA_CODE_BEGIN}
        if(QT_SBOM_INSTALLED_ALL_CONFIGS${suffix})
            set(QT_SBOM_BUILD_TIME FALSE)
            set(QT_SBOM_OUTPUT_DIR \"${arg_SBOM_INSTALL_OUTPUT_DIR}\")
            set(QT_SBOM_OUTPUT_PATH \"${arg_SBOM_INSTALL_OUTPUT_PATH}\")
            set(QT_SBOM_OUTPUT_PATH_WITHOUT_EXT \"${arg_SBOM_INSTALL_OUTPUT_PATH_WITHOUT_EXT}\")
            file(MAKE_DIRECTORY \"${arg_SBOM_INSTALL_OUTPUT_DIR}\")
            include(\"${arg_ASSEMBLE_SBOM_INCLUDE_PATH}\")
            ${arg_BEFORE_CHECKSUM_INCLUDES}
            ${process_verification_codes}
            ${arg_AFTER_CHECKSUM_INCLUDES}
            message(STATUS \"${final_message}: \${QT_SBOM_OUTPUT_PATH}\")
            configure_file(\"${staging_area_file}\" \"\${QT_SBOM_OUTPUT_PATH}\")
            ${arg_POST_GENERATION_INCLUDES}
            ${arg_VERIFY_INCLUDES}
            ${arg_EXTRA_CODE_INNER_END}
        else()
            message(STATUS \"Skipping SBOM finalization because not all configs were installed.\")
        endif()
")

    install(CODE "${assemble_sbom_install}" COMPONENT sbom)
    if(QT_SUPERBUILD)
        install(CODE "${assemble_sbom_install}" COMPONENT "sbom_${arg_REPO_PROJECT_NAME_LOWERCASE}"
            EXCLUDE_FROM_ALL)
    endif()
endfunction()
