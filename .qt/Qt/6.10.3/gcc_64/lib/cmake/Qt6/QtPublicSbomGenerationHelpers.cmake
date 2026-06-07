# Copyright (C) 2024 The Qt Company Ltd.
# Copyright (C) 2023-2024 Jochem Rutgers
# SPDX-License-Identifier: MIT AND BSD-3-Clause

# Helper to set a single arg option to a default value if not set.
function(_qt_internal_sbom_set_default_option_value option_name default)
    if(NOT arg_${option_name})
        set(arg_${option_name} "${default}" PARENT_SCOPE)
    endif()
endfunction()

# Helper to set a single arg option to a default value if not set.
# Errors out if the end value is empty. Including if the default value was empty.
function(_qt_internal_sbom_set_default_option_value_and_error_if_empty option_name default)
    _qt_internal_sbom_set_default_option_value("${option_name}" "${default}")
    if(NOT arg_${option_name})
        message(FATAL_ERROR "Specifying a non-empty ${option_name} is required")
    endif()
endfunction()

# Helper that returns the relative sbom build dir.
# To accommodate multiple projects within a qt repo (like qtwebengine), we need to choose separate
# build dirs for each project.
function(_qt_internal_get_current_project_sbom_relative_dir out_var)
    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)
    _qt_internal_sbom_get_qt_repo_project_name_lower_case(real_qt_repo_project_name_lowercase)

    if(repo_project_name_lowercase STREQUAL real_qt_repo_project_name_lowercase)
        set(sbom_dir "qt_sbom")
    else()
        set(sbom_dir "qt_sbom/${repo_project_name_lowercase}")
    endif()
    set(${out_var} "${sbom_dir}" PARENT_SCOPE)
endfunction()

# Helper that returns the directory where the intermediate sbom files will be generated.
function(_qt_internal_get_current_project_sbom_dir out_var)
    _qt_internal_get_current_project_sbom_relative_dir(relative_dir)
    set(sbom_dir "${PROJECT_BINARY_DIR}/${relative_dir}")
    set(${out_var} "${sbom_dir}" PARENT_SCOPE)
endfunction()

# Helper to return the path to staging spdx file, where content will be incrementally appended to.
function(_qt_internal_get_staging_area_spdx_file_path out_var)
    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)
    set(staging_area_spdx_file "${sbom_dir}/staging-${repo_project_name_lowercase}.spdx.in")
    set(${out_var} "${staging_area_spdx_file}" PARENT_SCOPE)
endfunction()

# Starts recording information for the generation of an sbom for a project.
# The intermediate files that generate the sbom are generated at cmake generation time, but are only
# actually run at build time or install time.
# The files are tracked in cmake global properties.
function(_qt_internal_sbom_begin_project_generate)
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
        CPE
        DOCUMENT_CREATOR_TOOL
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
        DEFAULT_SBOM_FILE_NAME_EXTENSION "spdx"
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

    _qt_internal_sbom_set_default_option_value(NAMESPACE
        "${arg_SUPPLIER}/spdxdocs/${arg_PROJECT}-${QT_SBOM_GIT_VERSION}")
    _qt_internal_sbom_set_default_option_value(LICENSE "NOASSERTION")

    _qt_internal_sbom_set_default_option_value(DOCUMENT_CREATOR_TOOL "Qt Build System")
    if(arg_DOCUMENT_CREATOR_TOOL)
        string(PREPEND arg_DOCUMENT_CREATOR_TOOL "Creator: Tool: ")
    endif()

    set(document_fields "")
    if(arg_DOCUMENT_CREATOR_TOOL)
        set(document_fields "${document_fields}
${arg_DOCUMENT_CREATOR_TOOL}")
    endif()

    set(fields "")
    if(arg_CPE)
        set(fields "${fields}
ExternalRef: SECURITY cpe23Type ${arg_CPE}")
    endif()

    set(purl_generic_id "pkg:generic/${arg_SUPPLIER}/${arg_PROJECT}@${QT_SBOM_GIT_VERSION}")
    set(fields "${fields}
ExternalRef: PACKAGE-MANAGER purl ${purl_generic_id}")

    if(QT_SBOM_GIT_VERSION)
        set(fields "${fields}
PackageVersion: ${QT_SBOM_GIT_VERSION}")
    endif()

    get_filename_component(doc_name "${arg_OUTPUT}" NAME_WLE)

    _qt_internal_sbom_set_default_option_value(DOWNLOAD_LOCATION "NOASSERTION")

    if(arg_PROJECT_COMMENT)
        string(APPEND project_comment "${arg_PROJECT_COMMENT}")
    endif()

    set(project_comment "<text>${project_comment}</text>")

    _qt_internal_sbom_get_spdx_id_unique_suffix(spdx_id_unique_suffix)

    set(content
        "SPDXVersion: SPDX-2.3
DataLicense: CC0-1.0
SPDXID: SPDXRef-DOCUMENT
DocumentName: ${doc_name}
DocumentNamespace: ${arg_NAMESPACE}
Creator: Organization: ${arg_SUPPLIER}${document_fields}
CreatorComment: <text>This SPDX document was created from CMake ${CMAKE_VERSION}, using the qt
build system from https://code.qt.io/cgit/qt/qtbase.git/tree/cmake/QtPublicSbomHelpers.cmake</text>
Created: ${current_utc}\${QT_SBOM_EXTERNAL_DOC_REFS}

PackageName: ${arg_PROJECT}
SPDXID: ${project_spdx_id}${fields}
ExternalRef: PACKAGE-MANAGER purl pkg:generic/${arg_SUPPLIER}/${arg_PROJECT}@${QT_SBOM_GIT_VERSION}
PackageSupplier: Organization: ${arg_SUPPLIER}
PackageDownloadLocation: ${arg_DOWNLOAD_LOCATION}
PackageLicenseConcluded: ${arg_LICENSE}
PackageLicenseDeclared: ${arg_LICENSE}
PackageCopyrightText: ${arg_COPYRIGHT}
PackageHomePage: ${arg_SUPPLIER_URL}
PackageComment: ${project_comment}
FilesAnalyzed: false
BuiltDate: ${current_utc}
Relationship: SPDXRef-DOCUMENT DESCRIBES ${project_spdx_id}")

    set(sbom_format "SPDX_V2")
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
        EXTERNAL_REFERENCE_SBOM_DIRS "${sbom_dir}"
    )

    _qt_internal_sbom_save_common_path_variables_in_global_properties(
        OUTPUT "${arg_OUTPUT}"
        OUTPUT_RELATIVE_PATH "${arg_OUTPUT_RELATIVE_PATH}"
        SBOM_DIR "${sbom_dir}"
        SBOM_FORMAT "${sbom_format}"
    )

    set_property(GLOBAL PROPERTY _qt_sbom_spdx_id_count 0)
    set_property(GLOBAL PROPERTY _qt_sbom_relationship_counter 0)
endfunction()

# Signals the end of recording sbom information for a project.
# Creates an 'sbom' custom target to generate an incomplete sbom at build time (no checksums).
# Creates install rules to install a complete (with checksums) sbom.
function(_qt_internal_sbom_end_project_generate)
    set(sbom_format "SPDX_V2")
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

    _qt_internal_sbom_get_cmake_include_files(
        SBOM_FORMAT "${sbom_format}"
        OUT_VAR_INCLUDES includes
        OUT_VAR_BEFORE_CHECKSUM_INCLUDES before_checksum_includes
        OUT_VAR_AFTER_CHECKSUM_INCLUDES after_checksum_includes
        OUT_VAR_POST_GENERATION_INCLUDES post_generation_includes
        OUT_VAR_VERIFY_INCLUDES verify_includes
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

    # Add 'reuse lint' per-repo custom targets.
    if(arg_LINT_SOURCE_SBOM AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        if(NOT TARGET reuse_lint)
            add_custom_target(reuse_lint)
        endif()

        set(comment "Running 'reuse lint' for '${repo_project_name_lowercase}'.")
        add_custom_target(${repo_sbom_target}_reuse_lint
            COMMAND "${CMAKE_COMMAND}" -P "${reuse_lint_script}"
            COMMENT "${comment}"
            VERBATIM
            USES_TERMINAL # To avoid running multiple lints in parallel
        )
        add_dependencies(reuse_lint ${repo_sbom_target}_reuse_lint)
    endif()

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

    set(verification_codes_content "
list(REMOVE_DUPLICATES QT_SBOM_PACKAGES_WITH_VERIFICATION_CODES)
# Go through each package that has verification codes (a code for each file that is part of a
# package), sort them, concatenate them, and calculate the sha1.
# Prepend the value with the PackageVerificationCode: prefix, so it can be directly evaluated
# in the spdx.in file via configure_file.
foreach(_sbom_package IN LISTS QT_SBOM_PACKAGES_WITH_VERIFICATION_CODES)
    set(_codes \${QT_SBOM_PACKAGES_WITH_VERIFICATION_CODES_\${_sbom_package}})
    list(SORT _codes)
    string(REPLACE \";\" \"\" _codes \"\${_codes}\")
    string(SHA1 _verification_code \"\${_codes}\")
    set(QT_SBOM_VERIFICATION_CODE_\${_sbom_package} \"
PackageVerificationCode: \${_verification_code}\")
endforeach()
unset(_sbom_package)
unset(_codes)
unset(_verification_code)
")
    set(process_verification_codes "${sbom_dir}/process_verification_codes.cmake")
    file(GENERATE OUTPUT "${process_verification_codes}" CONTENT "${verification_codes_content}")

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
        PROCESS_VERIFICATION_CODES "${process_verification_codes}"
        ${setup_sbom_install_args}
    )

    _qt_internal_sbom_clear_cmake_include_files(
        SBOM_FORMAT "${sbom_format}"
    )
endfunction()

# Gets a list of cmake include file paths, joins them as include() statements and returns the
# output.
function(_qt_internal_sbom_collect_cmake_include_files out_var)
    set(opt_args
        JOIN_WITH_NEWLINES
    )
    set(single_args "")
    set(multi_args
        PROPERTIES
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_PROPERTIES)
        message(FATAL_ERROR "PROPERTIES is required")
    endif()

    set(includes "")

    foreach(property IN LISTS arg_PROPERTIES)
        get_cmake_property(include_files "${property}")

        if(include_files)
            foreach(include_file IN LISTS include_files)
                list(APPEND includes "include(\"${include_file}\")")
            endforeach()
        endif()
    endforeach()

    if(arg_JOIN_WITH_NEWLINES)
        list(JOIN includes "\n" includes)
    endif()

    set(${out_var} "${includes}" PARENT_SCOPE)
endfunction()

# Helper to add info about a file to the sbom.
# Targets are backed by multiple files in multi-config builds. To support multi-config,
# we generate a -$<CONFIG> file for each config, but we only include / install the one that is
# specified via the CONFIG option.
# For build time sboms, we skip checking file existence and sha1 computation, because the files
# are not installed yet.
function(_qt_internal_sbom_generate_add_file)
    set(opt_args
        OPTIONAL
    )
    set(single_args
        FILENAME
        FILETYPE
        RELATIONSHIP
        PARENT_PACKAGE_SPDXID
        SPDXID
        CONFIG
        LICENSE
        COPYRIGHT
        INSTALL_PREFIX
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_sbom_set_default_option_value_and_error_if_empty(FILENAME "")
    _qt_internal_sbom_set_default_option_value_and_error_if_empty(FILETYPE "")

    set(check_option "")
    if(arg_SPDXID)
        set(check_option "CHECK" "${arg_SPDXID}")
    endif()

    _qt_internal_sbom_get_and_check_spdx_id(
        VARIABLE arg_SPDXID
        ${check_option}
        HINTS "SPDXRef-${arg_FILENAME}"
    )

    _qt_internal_sbom_set_default_option_value_and_error_if_empty(PARENT_PACKAGE_SPDXID "")

    _qt_internal_sbom_set_default_option_value(LICENSE "NOASSERTION")
    _qt_internal_sbom_set_default_option_value(COPYRIGHT "NOASSERTION")

    get_property(sbom_project_spdx_id GLOBAL PROPERTY _qt_sbom_project_spdx_id)
    if(NOT sbom_project_spdx_id)
        message(FATAL_ERROR "Call _qt_internal_sbom_begin_project() first")
    endif()
    if(NOT arg_RELATIONSHIP)
        set(arg_RELATIONSHIP "${sbom_project_spdx_id} CONTAINS ${arg_SPDXID}")
    else()
        string(REPLACE
            "@QT_SBOM_LAST_SPDXID@" "${arg_SPDXID}" arg_RELATIONSHIP "${arg_RELATIONSHIP}")
    endif()

    set(fields "")

    if(arg_LICENSE)
        set(fields "${fields}
LicenseConcluded: ${arg_LICENSE}"
        )
    else()
        set(fields "${fields}
LicenseConcluded: NOASSERTION"
        )
    endif()

    if(arg_COPYRIGHT)
        set(fields "${fields}
FileCopyrightText: ${arg_COPYRIGHT}"
        )
    else()
        set(fields "${fields}
FileCopyrightText: NOASSERTION"
        )
    endif()

    set(file_suffix_to_generate "")
    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(file_suffix_to_generate "-$<CONFIG>")
    endif()

    if(arg_CONFIG)
        set(file_suffix_to_install "-${arg_CONFIG}")
    else()
        set(file_suffix_to_install "")
    endif()

    _qt_internal_get_staging_area_spdx_file_path(staging_area_spdx_file)

    if(arg_INSTALL_PREFIX)
        set(install_prefix "${arg_INSTALL_PREFIX}")
    else()
        # The variable is escaped, so it is evaluated during cmake install time, so that the value
        # can be overridden with cmake --install . --prefix <path>.
        set(install_prefix "\${CMAKE_INSTALL_PREFIX}")
    endif()

    set(content "
        if(NOT EXISTS \"\$ENV{DESTDIR}${install_prefix}/${arg_FILENAME}\"
                AND NOT QT_SBOM_BUILD_TIME AND NOT QT_SBOM_FAKE_CHECKSUM)
            if(NOT ${arg_OPTIONAL})
                message(FATAL_ERROR \"Cannot find '${arg_FILENAME}' to compute its checksum. \"
                    \"Expected to find it at '\$ENV{DESTDIR}${install_prefix}/${arg_FILENAME}' \")
            endif()
        else()
            if(NOT QT_SBOM_BUILD_TIME)
                if(QT_SBOM_FAKE_CHECKSUM)
                    set(sha1 \"158942a783ee1095eafacaffd93de73edeadbeef\")
                else()
                    file(SHA1 \"\$ENV{DESTDIR}${install_prefix}/${arg_FILENAME}\" sha1)
                endif()

                set(\"QT_SBOM_PACKAGE_HAS_FILES_${arg_PARENT_PACKAGE_SPDXID}\" true)

                list(APPEND QT_SBOM_PACKAGES_WITH_VERIFICATION_CODES
                    \"${arg_PARENT_PACKAGE_SPDXID}\")
                list(APPEND
                    \"QT_SBOM_PACKAGES_WITH_VERIFICATION_CODES_${arg_PARENT_PACKAGE_SPDXID}\"
                    \"\${sha1}\")
            endif()
            file(APPEND \"${staging_area_spdx_file}\"
\"
FileName: ./${arg_FILENAME}
SPDXID: ${arg_SPDXID}
FileType: ${arg_FILETYPE}
FileChecksum: SHA1: \${sha1}${fields}
LicenseInfoInFile: NOASSERTION
Relationship: ${arg_RELATIONSHIP}
\"
                )
        endif()
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(file_sbom "${sbom_dir}/${arg_SPDXID}${file_suffix_to_generate}.cmake")
    file(GENERATE OUTPUT "${file_sbom}" CONTENT "${content}")

    set(file_sbom_to_install "${sbom_dir}/${arg_SPDXID}${file_suffix_to_install}.cmake")
    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_include_files "${file_sbom_to_install}")
endfunction()

# Helper to add a reference to an external SPDX v2 document.
#
# EXTERNAL_DOCUMENT_SPDX_ID: The spdx id by which the external document should be referenced in
# the current project SPDX document. This semantically serves as a pointer to the external document
# URI.
# e.g. DocumentRef-qtbase.
#
# SBOM_FORMAT: Type of SPDX file, SPDX_V2_TAG_VALUE or SPDX_V2_JSON.
#
# EXTERNAL_DOCUMENT_NAMESPACE: Explicit SPDX namespace to embed, instead of parsing the file passed
# in EXTERNAL_DOCUMENT_FILE_PATH.
#
# EXTERNAL_DOCUMENT_SHA1: Explicit sha1 to embed, instead of calculating it from the file passed
# in EXTERNAL_DOCUMENT_FILE_PATH.
#
# EXTERNAL_DOCUMENT_FILE_PATH: An absolute or relative file path of the external sbom document.
# In case of a relative file path, it will be searched for in the directories specified by the
# EXTERNAL_DOCUMENT_INSTALL_PREFIXES option.
# e.g. "sbom/qtbase-6.9.0.spdx"
# Can contain generator expressions.
# The file path is NOT embedded into the current project spdx document. Only the document ref id,
# spdx namespace and sha1 is embedded.
# The namespace is extracted from the contents of the referenced file.
#
# EXTERNAL_DOCUMENT_INSTALL_PREFIXES: A list of directories where the external document file path
# is searched for. The first existing file is used. Additionally the following locations are
# searched:
# - QT6_INSTALL_PREFIX
# - QT_ADDITIONAL_PACKAGES_PREFIX_PATH
# - QT_ADDITIONAL_SBOM_DOCUMENT_PATHS
#
# EXTERNAL_PACKAGE_SPDX_ID: If set, and no RELATIONSHIP_STRING is provided, an automatic DEPENDS_ON
# relationship is added from the current project spdx id to the package identified by
# $EXTERNAL_DOCUMENT_SPDX_ID:$EXTERNAL_PACKAGE_SPDX_ID options.
# This is mostly a beginner convenience.
#
# RELATIONSHIP_STRING: If set, it is used as the relationship string to add to the current project
# relationships.
function(_qt_internal_sbom_generate_add_external_reference)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args
        EXTERNAL_DOCUMENT_FILE_PATH
        EXTERNAL_DOCUMENT_SPDX_ID
        EXTERNAL_PACKAGE_SPDX_ID
        RELATIONSHIP_STRING

        EXTERNAL_DOCUMENT_NAMESPACE
        EXTERNAL_DOCUMENT_SHA1
        SBOM_FORMAT
    )
    set(multi_args
        EXTERNAL_DOCUMENT_INSTALL_PREFIXES
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    # Default to tag:value if nothing is passed, because that was the previous behavior before this
    # option got introduced.
    if(NOT arg_SBOM_FORMAT)
        set(arg_SBOM_FORMAT "SPDX_V2_TAG_VALUE")
    endif()

    if(NOT arg_EXTERNAL_DOCUMENT_FILE_PATH AND NOT arg_EXTERNAL_DOCUMENT_NAMESPACE)
        message(FATAL_ERROR "Either EXTERNAL_DOCUMENT_FILE_PATH or "
            "EXTERNAL_DOCUMENT_NAMESPACE together with EXTERNAL_DOCUMENT_SHA1 must be specified "
            "to be able to add an external reference for an external SPDX v2 tag:value document.")
    endif()

    set(debug_var_assignments "
        EXTERNAL_DOCUMENT_FILE_PATH: '${arg_EXTERNAL_DOCUMENT_FILE_PATH}'
        EXTERNAL_DOCUMENT_NAMESPACE: '${arg_EXTERNAL_DOCUMENT_NAMESPACE}'
        EXTERNAL_DOCUMENT_SHA1: '${arg_EXTERNAL_DOCUMENT_SHA1}'
        EXTERNAL_DOCUMENT_SPDX_ID: '${arg_EXTERNAL_DOCUMENT_SPDX_ID}'
")

    if(NOT arg_EXTERNAL_DOCUMENT_FILE_PATH
            AND arg_EXTERNAL_DOCUMENT_NAMESPACE
            AND NOT arg_EXTERNAL_DOCUMENT_SHA1)
        message(FATAL_ERROR
            "EXTERNAL_DOCUMENT_SHA1 must be specified if EXTERNAL_DOCUMENT_NAMESPACE is specified "
            "and EXTERNAL_DOCUMENT_FILE_PATH is not specified.\n${debug_var_assignments}")
    endif()

    if(NOT arg_EXTERNAL_DOCUMENT_FILE_PATH
            AND NOT arg_EXTERNAL_DOCUMENT_NAMESPACE
            AND arg_EXTERNAL_DOCUMENT_SHA1)
        message(FATAL_ERROR
            "EXTERNAL_DOCUMENT_NAMESPACE must be specified if EXTERNAL_DOCUMENT_SHA1 is specified "
            "and EXTERNAL_DOCUMENT_FILE_PATH is not specified.\n${debug_var_assignments}")
    endif()

    if(arg_EXTERNAL_DOCUMENT_FILE_PATH)
        set(find_external_document TRUE)
    else()
        set(find_external_document FALSE)
    endif()

    if(NOT arg_EXTERNAL_DOCUMENT_SPDX_ID)
        get_property(spdx_id_count GLOBAL PROPERTY _qt_sbom_spdx_id_count)
        set(arg_EXTERNAL_DOCUMENT_SPDX_ID "DocumentRef-${spdx_id_count}")
        math(EXPR spdx_id_count "${spdx_id_count} + 1")
        set_property(GLOBAL PROPERTY _qt_sbom_spdx_id_count "${spdx_id_count}")
    elseif(NOT "${arg_EXTERNAL_DOCUMENT_SPDX_ID}" MATCHES
            "^DocumentRef-[a-zA-Z0-9]+[-a-zA-Z0-9]+$")
        message(FATAL_ERROR "Invalid DocumentRef \"${arg_EXTERNAL_DOCUMENT_SPDX_ID}\"")
    endif()

    get_property(sbom_project_spdx_id GLOBAL PROPERTY _qt_sbom_project_spdx_id)
    if(NOT sbom_project_spdx_id)
        message(FATAL_ERROR "Call _qt_internal_sbom_begin_project() first")
    endif()

    if(NOT arg_RELATIONSHIP_STRING)
        if(arg_EXTERNAL_PACKAGE_SPDX_ID)
            set(external_package "${arg_EXTERNAL_DOCUMENT_SPDX_ID}:${arg_EXTERNAL_PACKAGE_SPDX_ID}")
            set(arg_RELATIONSHIP_STRING
                "${sbom_project_spdx_id} DEPENDS_ON ${external_package}")
        endif()
    else()
        string(REPLACE
            "@QT_SBOM_LAST_SPDXID@" "${arg_EXTERNAL_DOCUMENT_SPDX_ID}"
            arg_RELATIONSHIP_STRING "${arg_RELATIONSHIP_STRING}")
    endif()

    _qt_internal_get_staging_area_spdx_file_path(staging_area_spdx_file)

    set(document_search_paths "")
    if(find_external_document)
        set(search_path_args "")
        if(arg_EXTERNAL_DOCUMENT_INSTALL_PREFIXES)
            list(APPEND search_path_args
                    EXTERNAL_DOCUMENT_SEARCH_PATHS ${arg_EXTERNAL_DOCUMENT_INSTALL_PREFIXES}
            )
        endif()

        _qt_internal_sbom_get_external_reference_search_paths(document_search_paths
            SEARCH_IN_BUILD_SBOM_DIRS
            SEARCH_IN_QT_PREFIXES
            SEARCH_IN_DESTDIR_INSTALL_PREFIX_AT_INSTALL_TIME
            ${search_path_args}
        )
    endif()

    set(relationship_content "")
    if(arg_RELATIONSHIP_STRING)
        set(relationship_content "
        file(APPEND \"${staging_area_spdx_file}\"
    \"Relationship: ${arg_RELATIONSHIP_STRING}\")
")
    endif()

    # File path may not exist yet, and it could be a generator expression.
    set(content "
        set(find_external_document \"${find_external_document}\")
        set(document_search_paths ${document_search_paths})
        set(maybe_external_document_file_path \"${arg_EXTERNAL_DOCUMENT_FILE_PATH}\")
        set(sbom_format \"${arg_SBOM_FORMAT}\")

        set(explicit_external_document_namespace \"${arg_EXTERNAL_DOCUMENT_NAMESPACE}\")
        set(explicit_external_document_sha1 \"${arg_EXTERNAL_DOCUMENT_SHA1}\")

        # The helper functions below are used both during configure time and SBOM generation
        # time. During configure time, QT_GENERATE_SBOM is checked to return early gracefully,
        # to allow projects to skip SBOM generation.
        # During SBOM generation time, we need to ensure these functions run properly.
        # Backup the var instead of setting it directly, because some of the testing infrastructure
        # does checks of the variable at the end of sbom generation.
        set(backup_qt_generate_sbom \"${QT_GENERATE_SBOM}\")
        set(QT_GENERATE_SBOM ON)
        if(find_external_document)
            _qt_internal_sbom_find_external_reference_document(document_file_path
                EXTERNAL_DOCUMENT_FILE_PATH \"\${maybe_external_document_file_path}\"
                EXTERNAL_DOCUMENT_SEARCH_PATHS \${document_search_paths}
            )

            _qt_internal_sbom_parse_spdx_v2_document_namespace(
                EXTERNAL_DOCUMENT_FILE_PATH \"\${document_file_path}\"
                SBOM_FORMAT \"\${sbom_format}\"
                OUT_VAR_DOCUMENT_NAMESPACE ext_ns
            )

            file(SHA1 \"\${document_file_path}\" ext_sha1)
        else()
            set(ext_ns \"\${explicit_external_document_namespace}\")
            set(ext_sha1 \"\${explicit_external_document_sha1}\")
        endif()
        set(QT_GENERATE_SBOM \${backup_qt_generate_sbom})

        string(APPEND QT_SBOM_EXTERNAL_DOC_REFS \"
ExternalDocumentRef: ${arg_EXTERNAL_DOCUMENT_SPDX_ID} \${ext_ns} SHA1: \${ext_sha1}\")

        ${relationship_content}
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(ext_ref_sbom "${sbom_dir}/${arg_EXTERNAL_DOCUMENT_SPDX_ID}.cmake")
    file(GENERATE OUTPUT "${ext_ref_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_end_include_files "${ext_ref_sbom}")
endfunction()

# Helper to add info about a package to the sbom. Usually a package is a mapping to a cmake target.
function(_qt_internal_sbom_generate_add_package)
    set(opt_args
        ""
    )
    set(single_args
        PACKAGE
        PACKAGE_SUMMARY
        VERSION
        LICENSE_DECLARED
        LICENSE_CONCLUDED
        COPYRIGHT
        DOWNLOAD_LOCATION
        SPDXID
        SUPPLIER
        PURPOSE
        COMMENT
    )
    set(multi_args
        EXTREF
        CPE
        RELATIONSHIPS # Deprecated, SBOM_RELATIONSHIP_ENTRIES is preferred
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

    _qt_internal_sbom_set_default_option_value(DOWNLOAD_LOCATION "NOASSERTION")
    _qt_internal_sbom_set_default_option_value(VERSION "unknown")
    _qt_internal_sbom_set_default_option_value(SUPPLIER "Person: Anonymous")
    _qt_internal_sbom_set_default_option_value(LICENSE_DECLARED "NOASSERTION")
    _qt_internal_sbom_set_default_option_value(LICENSE_CONCLUDED "NOASSERTION")
    _qt_internal_sbom_set_default_option_value(COPYRIGHT "NOASSERTION")
    _qt_internal_sbom_set_default_option_value(PURPOSE "OTHER")

    set(fields "")

    if(arg_LICENSE_CONCLUDED)
        set(fields "${fields}
PackageLicenseConcluded: ${arg_LICENSE_CONCLUDED}"
        )
    else()
        set(fields "${fields}
PackageLicenseConcluded: NOASSERTION"
        )
    endif()

    if(arg_LICENSE_DECLARED)
        set(fields "${fields}
PackageLicenseDeclared: ${arg_LICENSE_DECLARED}"
        )
    else()
        set(fields "${fields}
PackageLicenseDeclared: NOASSERTION"
        )
    endif()

    foreach(ext_ref IN LISTS arg_EXTREF)
        set(fields "${fields}
ExternalRef: ${ext_ref}"
        )
    endforeach()

    if(arg_COPYRIGHT)
        set(fields "${fields}
PackageCopyrightText: ${arg_COPYRIGHT}"
        )
    else()
        set(fields "${fields}
PackageCopyrightText: NOASSERTION"
        )
    endif()

    if(arg_PACKAGE_SUMMARY)
        set(fields "${fields}
PackageSummary: <text>${arg_PACKAGE_SUMMARY}</text>"
        )
    endif()

    if(arg_PURPOSE)
        set(fields "${fields}
PrimaryPackagePurpose: ${arg_PURPOSE}"
        )
    else()
        set(fields "${fields}
PrimaryPackagePurpose: OTHER"
        )
    endif()

    if(arg_COMMENT)
        set(fields "${fields}
PackageComment: ${arg_COMMENT}"
        )
    endif()

    foreach(cpe IN LISTS arg_CPE)
        set(fields "${fields}
ExternalRef: SECURITY cpe23Type ${cpe}"
        )
    endforeach()

    get_property(sbom_project_spdx_id GLOBAL PROPERTY _qt_sbom_project_spdx_id)
    if(NOT sbom_project_spdx_id)
        message(FATAL_ERROR "Call _qt_internal_sbom_begin_project() first")
    endif()

    set(relationship_strings "")
    if(arg_SBOM_RELATIONSHIP_ENTRIES)
        _qt_internal_sbom_serialize_relationship_entries(
            OUTPUT_SBOM_FORMAT "SPDX_V2"
            OUT_VAR_RELATIONSHIPS_STRINGS entries_relationships_strings
            SBOM_RELATIONSHIP_ENTRIES ${arg_SBOM_RELATIONSHIP_ENTRIES}
        )
        if(entries_relationships_strings)
            list(APPEND relationship_strings ${entries_relationships_strings})
        endif()
    endif()

    # TODO: This is deprecated, SBOM_RELATIONSHIP_ENTRIES is preferred, remove it once qtwebengine
    # is ported over.
    if(arg_RELATIONSHIPS)
        string(REPLACE
            "@QT_SBOM_LAST_SPDXID@" "${arg_SPDXID}" arg_RELATIONSHIPS "${arg_RELATIONSHIPS}")
        list(APPEND relationship_strings ${arg_RELATIONSHIPS})
    endif()

    # Remove duplicates, because apparently we sometimes get them for some system libraries.
    list(REMOVE_DUPLICATES relationship_strings)

    list(JOIN relationship_strings "\n" relationships_str)
    string(PREPEND relationships_str "\n")

    set(fields "${fields}\\\${QT_SBOM_VERIFICATION_CODE_${arg_SPDXID}}")

    _qt_internal_get_staging_area_spdx_file_path(staging_area_spdx_file)

    # QT_SBOM_PACKAGE_HAS_FILES_ gets overriden by any added file to 'true'.
    set(content "
        list(APPEND QT_SBOM_PACKAGES \"${arg_SPDXID}\")
        set(\"QT_SBOM_PACKAGE_HAS_FILES_${arg_SPDXID}\" false)

        file(APPEND \"${staging_area_spdx_file}\"
\"
PackageName: ${arg_PACKAGE}
SPDXID: ${arg_SPDXID}
PackageDownloadLocation: ${arg_DOWNLOAD_LOCATION}
PackageVersion: ${arg_VERSION}
PackageSupplier: ${arg_SUPPLIER}${fields}
FilesAnalyzed: \\\${QT_SBOM_PACKAGE_HAS_FILES_${arg_SPDXID}}${relationships_str}
\"
        )
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(package_sbom "${sbom_dir}/${arg_SPDXID}.cmake")
    file(GENERATE OUTPUT "${package_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_include_files "${package_sbom}")
endfunction()

# Helper to add relationship entries to the current project SBOM document package.
#
# RELATIONSHIPS: A list of relationship strings to add to the current project relationships.
#
# Care must be taken to call the function right after project creation, before other targets are
# created, otherwise the relationship strings might be added to the wrong package.
# It doesn't seem to cause tooling to fail, but it's something to look out for.
function(_qt_internal_sbom_generate_add_project_relationship)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args "")
    set(multi_args
        RELATIONSHIPS
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_sbom_set_default_option_value_and_error_if_empty(RELATIONSHIPS "")

    _qt_internal_get_staging_area_spdx_file_path(staging_area_spdx_file)

    get_property(counter GLOBAL PROPERTY _qt_sbom_relationship_counter)
    set(current_counter "${counter}")
    math(EXPR counter "${counter} + 1")
    set_property(GLOBAL PROPERTY _qt_sbom_relationship_counter "${counter}")

    set(relationships "${arg_RELATIONSHIPS}")
    list(REMOVE_DUPLICATES relationships)
    list(JOIN relationships "\nRelationship: " relationships)

    set(content "
        # Custom relationship index: ${current_counter}
        file(APPEND \"${staging_area_spdx_file}\"
    \"
Relationship: ${relationships}\")
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(ext_ref_sbom "${sbom_dir}/relationship_${counter}.cmake")
    file(GENERATE OUTPUT "${ext_ref_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_include_files "${ext_ref_sbom}")
endfunction()

# Adds a cmake include file to the sbom generation process at a specific step.
# INCLUDE_PATH - path to the cmake file to include.
# STEP - one of
#        BEGIN
#        END
#        POST_GENERATION
#        VERIFY
#
# BEGIN includes are included after the spdx staging file is created.
#
# END includes are included after the all the BEGIN ones are included.
#
# BEFORE_CHECKSUM includes are included after the verification codes have been collected
# and before their merged checksum(s) has been computed. Only relevant for install time sboms.
#
# AFTER_CHECKSUM includes are included after the checksum has been computed and written to the
# QT_SBOM_VERIFICATION_CODE variable. Only relevant for install time sboms.
#
# POST_GENERATION includes are included for both build and install time sboms, after
# sbom generation has finished.
# Currently used for adding reuse lint and reuse source steps, before VERIFY include are run.
#
# VERIFY includes only make sense on installation, where the checksums are present, so they are
# only included during install time.
# Used for generating a json sbom from the tag value one, for running ntia compliance check, etc.
function(_qt_internal_sbom_add_cmake_include_step)
    set(opt_args "")
    set(single_args
        STEP
        INCLUDE_PATH
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")

    if(NOT arg_STEP)
        message(FATAL_ERROR "STEP is required")
    endif()

    if(NOT arg_INCLUDE_PATH)
        message(FATAL_ERROR "INCLUDE_PATH is required")
    endif()

    set(step "${arg_STEP}")

    if(step STREQUAL "BEGIN")
        set(property "_qt_sbom_cmake_include_files")
    elseif(step STREQUAL "END")
        set(property "_qt_sbom_cmake_end_include_files")
    elseif(step STREQUAL "BEFORE_CHECKSUM")
        set(property "_qt_sbom_cmake_before_checksum_include_files")
    elseif(step STREQUAL "AFTER_CHECKSUM")
        set(property "_qt_sbom_cmake_after_checksum_include_files")
    elseif(step STREQUAL "POST_GENERATION")
        set(property "_qt_sbom_cmake_post_generation_include_files")
    elseif(step STREQUAL "VERIFY")
        set(property "_qt_sbom_cmake_verify_include_files")
    else()
        message(FATAL_ERROR "Invalid SBOM cmake include step name: ${step}")
    endif()

    set_property(GLOBAL APPEND PROPERTY "${property}" "${arg_INCLUDE_PATH}")
endfunction()

# Helper to add a license text from a file or text into the sbom document.
function(_qt_internal_sbom_generate_add_license)
    set(opt_args "")
    set(single_args
        LICENSE_ID
        EXTRACTED_TEXT
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_sbom_set_default_option_value_and_error_if_empty(LICENSE_ID "")

    set(check_option "")
    if(arg_SPDXID)
        set(check_option "CHECK" "${arg_SPDXID}")
    endif()

    _qt_internal_sbom_get_and_check_spdx_id(
        VARIABLE arg_SPDXID
        ${check_option}
        HINTS "SPDXRef-${arg_LICENSE_ID}"
    )

    if(NOT arg_EXTRACTED_TEXT)
        set(licenses_dir "${PROJECT_SOURCE_DIR}/LICENSES")
        file(READ "${licenses_dir}/${arg_LICENSE_ID}.txt" arg_EXTRACTED_TEXT)
        string(PREPEND arg_EXTRACTED_TEXT "<text>")
        string(APPEND arg_EXTRACTED_TEXT "</text>")
    endif()

    _qt_internal_get_staging_area_spdx_file_path(staging_area_spdx_file)

    set(content "
        file(APPEND \"${staging_area_spdx_file}\"
\"
LicenseID: ${arg_LICENSE_ID}
ExtractedText: ${arg_EXTRACTED_TEXT}
\"
        )
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(license_sbom "${sbom_dir}/${arg_SPDXID}.cmake")
    file(GENERATE OUTPUT "${license_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_end_include_files "${license_sbom}")
endfunction()

# Helper to retrieve a valid spdx id, given some hints.
# HINTS can be a list of values, one of which will be sanitized and used as the spdx id.
# CHECK is expected to be a valid spdx id.
function(_qt_internal_sbom_get_and_check_spdx_id)
    set(opt_args "")
    set(single_args
        VARIABLE
        CHECK
    )
    set(multi_args
        HINTS
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_sbom_set_default_option_value_and_error_if_empty(VARIABLE "")

    if(NOT arg_CHECK)
        get_property(spdx_id_count GLOBAL PROPERTY _qt_sbom_spdx_id_count)
        set(suffix "-${spdx_id_count}")
        math(EXPR spdx_id_count "${spdx_id_count} + 1")
        set_property(GLOBAL PROPERTY _qt_sbom_spdx_id_count "${spdx_id_count}")

        foreach(hint IN LISTS arg_HINTS)
            _qt_internal_sbom_get_sanitized_spdx_id(id "${hint}")
            if(id)
                set(id "${id}${suffix}")
                break()
            endif()
        endforeach()

        if(NOT id)
            set(id "SPDXRef${suffix}")
        endif()
    else()
        set(id "${arg_CHECK}")
    endif()

    if("${id}" MATCHES "^SPDXRef-[-]+$"
        OR (NOT "${id}" MATCHES "^SPDXRef-[-a-zA-Z0-9]+$"))
        message(FATAL_ERROR "Invalid SPDXID \"${id}\"")
    endif()

    set(${arg_VARIABLE} "${id}" PARENT_SCOPE)
endfunction()


