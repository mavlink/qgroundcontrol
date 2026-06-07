# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Starts repo sbom generation.
# Should be called before any targets are added to the sbom.
#
# INSTALL_PREFIX should be passed a value like CMAKE_INSTALL_PREFIX or QT_STAGING_PREFIX.
# The default value is \${CMAKE_INSTALL_PREFIX}, which is evaluated at install time, not configure
# time.
# This default value is the /preferred/ value, to ensure using cmake --install . --prefix <path>
# works correctly for lookup of installed files during SBOM generation.
#
# INSTALL_SBOM_DIR should be passed a value like CMAKE_INSTALL_DATAROOTDIR or
#   Qt's INSTALL_SBOMDIR.
# The default value is "sbom".
#
# SUPPLIER, SUPPLIER_URL, DOCUMENT_NAMESPACE, COPYRIGHTS are self-explanatory.
function(_qt_internal_sbom_begin_project)
    # Allow opt out via an internal variable. Will be used in CI for repos like qtqa.
    if(QT_INTERNAL_FORCE_NO_GENERATE_SBOM)
        set(QT_GENERATE_SBOM OFF CACHE BOOL "Generate SBOM" FORCE)
    endif()

    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    _qt_internal_sbom_setup_fake_deterministic_build()

    set(opt_args
        USE_GIT_VERSION
        __QT_INTERNAL_HANDLE_QT_REPO
        NO_AUTO_DOCUMENT_NAMESPACE_INFIX
        NO_AUTO_SEARCH_EXTERNAL_DOCUMENTS_IN_CMAKE_PATHS
        NO_AUTO_SPDX_ID_SUFFIX
        NO_AUTO_ADD_BUILD_TOOLS
    )
    set(single_args
        INSTALL_PREFIX
        INSTALL_SBOM_DIR
        LICENSE_EXPRESSION
        SUPPLIER
        SUPPLIER_URL
        DOWNLOAD_LOCATION
        DOCUMENT_NAMESPACE
        DOCUMENT_NAMESPACE_INFIX
        DOCUMENT_NAMESPACE_SUFFIX
        DOCUMENT_NAMESPACE_URL_PREFIX
        SPDX_ID_SUFFIX
        SPDX_ID_SUFFIX_HASH_LENGTH
        VERSION
        SBOM_PROJECT_NAME
        QT_REPO_PROJECT_NAME
        CPE
        DOCUMENT_CREATOR_TOOL
    )
    set(multi_args
        COPYRIGHTS
        LICENSE_DIR_PATHS
        EXTERNAL_DOCUMENT_SEARCH_PATHS
    )

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(CMAKE_VERSION LESS_EQUAL "3.19")
        if(QT_IGNORE_MIN_CMAKE_VERSION_FOR_SBOM)
            message(STATUS
                "Using CMake version older than 3.19, and QT_IGNORE_MIN_CMAKE_VERSION_FOR_SBOM was "
                "set to ON. qt_attribution.json files will not be processed.")
        else()
            message(FATAL_ERROR
                "Generating an SBOM requires CMake version 3.19 or newer. You can pass "
                "-DQT_IGNORE_MIN_CMAKE_VERSION_FOR_SBOM=ON to try to generate the SBOM anyway, "
                "but it is not officially supported, and the SBOM might be incomplete.")
        endif()
    endif()

    # The ntia-conformance-checker insists that a SPDX document contain at least one
    # relationship that DESCRIBES a package, and that the package contains the string
    # "Package-" in the spdx id. boot2qt spdx seems to contain the same.

    if(arg_SBOM_PROJECT_NAME)
        _qt_internal_sbom_set_root_project_name("${arg_SBOM_PROJECT_NAME}")
    else()
        _qt_internal_sbom_set_root_project_name("${PROJECT_NAME}")
    endif()

    if(arg_QT_REPO_PROJECT_NAME)
        _qt_internal_sbom_set_qt_repo_project_name("${arg_QT_REPO_PROJECT_NAME}")
    else()
        _qt_internal_sbom_set_qt_repo_project_name("${PROJECT_NAME}")
    endif()

    _qt_internal_sbom_get_root_project_name_for_spdx_id(repo_project_name_for_spdx_id)
    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)

    set(begin_project_generate_args_spdx "")
    set(begin_project_generate_args_cydx "")

    if(arg_SUPPLIER_URL)
        set(repo_supplier_url "${arg_SUPPLIER_URL}")
    elseif(arg___QT_INTERNAL_HANDLE_QT_REPO)
        _qt_internal_sbom_get_default_supplier_url(repo_supplier_url)
    endif()
    if(repo_supplier_url)
        list(APPEND begin_project_generate_args_spdx SUPPLIER_URL "${repo_supplier_url}")
        list(APPEND begin_project_generate_args_cydx SUPPLIER_URL "${repo_supplier_url}")
    endif()

    set(sbom_project_version_args "")
    _qt_internal_forward_function_args(
        FORWARD_APPEND
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR sbom_project_version_args
        FORWARD_OPTIONS
            USE_GIT_VERSION
        FORWARD_SINGLE
            VERSION
    )
    _qt_internal_handle_sbom_project_version(${sbom_project_version_args})

    if(arg___QT_INTERNAL_HANDLE_QT_REPO)
        _qt_internal_sbom_compute_qt_uniqueish_document_namespace_infix(
            OUT_VAR_UUID_INFIX_MERGED document_namespace_infix
        )
        if(document_namespace_infix)
            set(arg_DOCUMENT_NAMESPACE_INFIX "-${document_namespace_infix}")
        endif()
    endif()

    if(arg_EXTERNAL_DOCUMENT_SEARCH_PATHS)
        set_property(GLOBAL APPEND PROPERTY _qt_internal_sbom_external_document_search_paths
            ${arg_EXTERNAL_DOCUMENT_SEARCH_PATHS})
    endif()

    if(arg_NO_AUTO_SEARCH_EXTERNAL_DOCUMENTS_IN_CMAKE_PATHS)
        set(auto_search_external_documents_in_cmake_paths FALSE)
    else()
        set(auto_search_external_documents_in_cmake_paths TRUE)
    endif()
    set_property(GLOBAL PROPERTY
        _qt_internal_sbom_auto_search_external_documents_in_paths
        "${auto_search_external_documents_in_cmake_paths}")

    if(arg_DOCUMENT_NAMESPACE)
        set(repo_spdx_namespace "${arg_DOCUMENT_NAMESPACE}")

        if(QT_SBOM_DOCUMENT_NAMESPACE_INFIX)
            string(APPEND repo_spdx_namespace "${QT_SBOM_DOCUMENT_NAMESPACE_INFIX}")
        elseif(arg_DOCUMENT_NAMESPACE_INFIX)
            string(APPEND repo_spdx_namespace "${arg_DOCUMENT_NAMESPACE_INFIX}")
        elseif(NOT arg_NO_AUTO_DOCUMENT_NAMESPACE_INFIX
                AND NOT QT_SBOM_NO_AUTO_DOCUMENT_NAMESPACE_INFIX)
            _qt_internal_sbom_compute_uniqueish_document_namespace_infix(
                OUT_VAR_UUID_INFIX_MERGED document_namespace_infix
            )
            string(APPEND repo_spdx_namespace "-${document_namespace_infix}")
        endif()

        if(QT_SBOM_DOCUMENT_NAMESPACE_SUFFIX)
            string(APPEND repo_spdx_namespace "${QT_SBOM_DOCUMENT_NAMESPACE_SUFFIX}")
        elseif(arg_DOCUMENT_NAMESPACE_SUFFIX)
            string(APPEND repo_spdx_namespace "${arg_DOCUMENT_NAMESPACE_SUFFIX}")
        endif()
    else()
        set(compute_project_namespace_args "")
        if(repo_supplier_url)
            list(APPEND compute_project_namespace_args SUPPLIER_URL "${repo_supplier_url}")
        endif()

        if(QT_SBOM_DOCUMENT_NAMESPACE_INFIX)
            list(APPEND compute_project_namespace_args
                DOCUMENT_NAMESPACE_INFIX "${QT_SBOM_DOCUMENT_NAMESPACE_INFIX}")
        elseif(arg_DOCUMENT_NAMESPACE_INFIX)
            list(APPEND compute_project_namespace_args
                DOCUMENT_NAMESPACE_INFIX "${arg_DOCUMENT_NAMESPACE_INFIX}")
        elseif(NOT arg_NO_AUTO_DOCUMENT_NAMESPACE_INFIX
                AND NOT QT_SBOM_NO_AUTO_DOCUMENT_NAMESPACE_INFIX)
            _qt_internal_sbom_compute_uniqueish_document_namespace_infix(
                OUT_VAR_UUID_INFIX_MERGED document_namespace_infix
            )
            list(APPEND compute_project_namespace_args
                DOCUMENT_NAMESPACE_INFIX "-${document_namespace_infix}")
        endif()

        if(QT_SBOM_DOCUMENT_NAMESPACE_SUFFIX)
            list(APPEND compute_project_namespace_args
                DOCUMENT_NAMESPACE_SUFFIX "${QT_SBOM_DOCUMENT_NAMESPACE_SUFFIX}")
        elseif(arg_DOCUMENT_NAMESPACE_SUFFIX)
            list(APPEND compute_project_namespace_args
                DOCUMENT_NAMESPACE_SUFFIX "${arg_DOCUMENT_NAMESPACE_SUFFIX}")
        endif()

        if(QT_SBOM_DOCUMENT_NAMESPACE_URL_PREFIX)
            list(APPEND compute_project_namespace_args
                DOCUMENT_NAMESPACE_URL_PREFIX "${QT_SBOM_DOCUMENT_NAMESPACE_URL_PREFIX}")
        elseif(arg_DOCUMENT_NAMESPACE_URL_PREFIX)
            list(APPEND compute_project_namespace_args
                DOCUMENT_NAMESPACE_URL_PREFIX "${arg_DOCUMENT_NAMESPACE_URL_PREFIX}")
        endif()

        _qt_internal_sbom_compute_project_namespace(repo_spdx_namespace
            PROJECT_NAME "${repo_project_name_lowercase}"
            ${compute_project_namespace_args}
        )
    endif()

    _qt_internal_sbom_get_cyclone_bom_serial_number(
        SPDX_NAMESPACE "${repo_spdx_namespace}"
        OUT_VAR_UUID cyclone_dx_bom_serial_number_uuid
    )
    list(APPEND begin_project_generate_args_cydx
        BOM_SERIAL_NUMBER_UUID "${cyclone_dx_bom_serial_number_uuid}")

    if(QT_SBOM_SPDX_ID_SUFFIX)
        set(spdx_id_unique_suffix "-${QT_SBOM_SPDX_ID_SUFFIX}")
    elseif(arg_SPDX_ID_SUFFIX)
        set(spdx_id_unique_suffix "-${arg_SPDX_ID_SUFFIX}")
    elseif(NOT arg_NO_AUTO_SPDX_ID_SUFFIX AND NOT QT_SBOM_NO_AUTO_SPDX_SUFFIX)
        set(compute_unique_spdx_id_suffix_args "")

        if(QT_SBOM_SPDX_ID_SUFFIX_HASH_LENGTH)
            list(APPEND compute_unique_spdx_id_suffix_args
                HASH_LENGTH "${QT_SBOM_SPDX_ID_SUFFIX_HASH_LENGTH}")
        elseif(arg_SPDX_ID_SUFFIX_HASH_LENGTH)
            list(APPEND compute_unique_spdx_id_suffix_args
                HASH_LENGTH "${arg_SPDX_ID_SUFFIX_HASH_LENGTH}")
        endif()

        _qt_internal_sbom_compute_uniqueish_spdx_id_suffix(
            SPDX_NAMESPACE "${repo_spdx_namespace}"
            OUT_VAR_UNIQUE_SUFFIX spdx_id_unique_suffix
            ${compute_unique_spdx_id_suffix_args}
        )
        string(PREPEND spdx_id_unique_suffix "-")
    else()
        set(spdx_id_unique_suffix "")
    endif()

    set_property(GLOBAL PROPERTY _qt_internal_sbom_repo_spdx_id_unique_suffix
        "${spdx_id_unique_suffix}")

    if(arg_INSTALL_SBOM_DIR)
        set(install_sbom_dir "${arg_INSTALL_SBOM_DIR}")
    elseif(INSTALL_SBOMDIR)
        set(install_sbom_dir "${INSTALL_SBOMDIR}")
    else()
        set(install_sbom_dir "sbom")
    endif()

    if(arg_INSTALL_PREFIX)
        set(install_prefix "${arg_INSTALL_PREFIX}")
    else()
        # The variable is escaped, so it is evaluated during cmake install time, so that the value
        # can be overridden with cmake --install . --prefix <path>.
        set(install_prefix "\${CMAKE_INSTALL_PREFIX}")
    endif()

    set(compute_project_file_name_args "")
    _qt_internal_sbom_get_project_explicit_version(explicit_version)
    if(explicit_version)
        list(APPEND compute_project_file_name_args VERSION_SUFFIX "${explicit_version}")
    endif()

    _qt_internal_sbom_compute_project_file_name(repo_project_file_name_spdx
        SPDX_TAG_VALUE
        PROJECT_NAME "${repo_project_name_lowercase}"
        ${compute_project_file_name_args}
    )

    _qt_internal_sbom_compute_project_file_name(repo_project_file_name_cydx
        CYCLONEDX_TOML
        PROJECT_NAME "${repo_project_name_lowercase}"
        ${compute_project_file_name_args}
    )

    _qt_internal_path_join(repo_spdx_relative_install_path_spdx
        "${install_sbom_dir}" "${repo_project_file_name_spdx}")

    # Currently only used for exporting as a target's property.
    _qt_internal_path_join(repo_spdx_relative_install_path_spdx_json
        "${install_sbom_dir}" "${repo_project_file_name_spdx}.json")

    # This is actually the path to the intermediate CycloneDX toml file.
    _qt_internal_path_join(repo_spdx_relative_install_path_cydx
        "${install_sbom_dir}" "${repo_project_file_name_cydx}")

    # Compute the relative path to the final cydx json file, to be exported in a target's property.
    get_filename_component(repo_project_file_name_cydx_without_ext
        "${repo_project_file_name_cydx}" NAME_WLE)

    _qt_internal_path_join(repo_spdx_relative_install_path_cydx_json
        "${install_sbom_dir}" "${repo_project_file_name_cydx_without_ext}.json")

    # Prepend DESTDIR, to allow relocating installed sbom. Needed for CI.
    _qt_internal_path_join(repo_spdx_install_path_spdx
        "\$ENV{DESTDIR}${install_prefix}" "${repo_spdx_relative_install_path_spdx}")
    _qt_internal_path_join(repo_spdx_install_path_cydx
        "\$ENV{DESTDIR}${install_prefix}" "${repo_spdx_relative_install_path_cydx}")

    if(arg_LICENSE_EXPRESSION)
        set(repo_license "${arg_LICENSE_EXPRESSION}")
    else()
        # Default to NOASSERTION for root repo SPDX packages, because we have some repos
        # with multiple licenses and AND-ing them together will create a giant unreadable list.
        # It's better to rely on the more granular package licenses.
        set(repo_license "")
    endif()
    if(repo_license)
        list(APPEND begin_project_generate_args_spdx LICENSE "${repo_license}")
        list(APPEND begin_project_generate_args_cydx LICENSE "${repo_license}")
    endif()

    if(arg_COPYRIGHTS)
        list(JOIN arg_COPYRIGHTS "\n" arg_COPYRIGHTS)
        set(repo_copyright "<text>${arg_COPYRIGHTS}</text>")
    elseif(arg___QT_INTERNAL_HANDLE_QT_REPO)
        _qt_internal_sbom_get_default_qt_copyright_header(repo_copyright)
    endif()
    if(repo_copyright)
        list(APPEND begin_project_generate_args_spdx COPYRIGHT "${repo_copyright}")
        list(APPEND begin_project_generate_args_cydx COPYRIGHT "${repo_copyright}")
    endif()

    if(arg_SUPPLIER)
        set(repo_supplier "${arg_SUPPLIER}")
    elseif(arg___QT_INTERNAL_HANDLE_QT_REPO)
        _qt_internal_sbom_get_default_supplier(repo_supplier)
    endif()
    if(repo_supplier)
        # This must not contain spaces!
        list(APPEND begin_project_generate_args_spdx SUPPLIER "${repo_supplier}")
        list(APPEND begin_project_generate_args_cydx SUPPLIER "${repo_supplier}")
    endif()

    if(arg_CPE)
        set(qt_cpe "${arg_CPE}")
    elseif(arg___QT_INTERNAL_HANDLE_QT_REPO)
        _qt_internal_sbom_get_cpe_qt_repo(qt_cpe)
    else()
        set(qt_cpe "")
    endif()
    if(qt_cpe)
        list(APPEND begin_project_generate_args_spdx CPE "${qt_cpe}")
        list(APPEND begin_project_generate_args_cydx CPE "${qt_cpe}")
    endif()

    if(arg_DOWNLOAD_LOCATION)
        set(download_location "${arg_DOWNLOAD_LOCATION}")
    elseif(arg___QT_INTERNAL_HANDLE_QT_REPO)
        _qt_internal_sbom_get_qt_repo_source_download_location(download_location)
    endif()
    if(download_location)
        list(APPEND begin_project_generate_args_spdx DOWNLOAD_LOCATION "${download_location}")
        list(APPEND begin_project_generate_args_cydx DOWNLOAD_LOCATION "${download_location}")
    endif()

    if(arg_DOCUMENT_CREATOR_TOOL)
        list(APPEND begin_project_generate_args_spdx
            DOCUMENT_CREATOR_TOOL "${arg_DOCUMENT_CREATOR_TOOL}")
    endif()

    set(project_comment "")

    if(arg___QT_INTERNAL_HANDLE_QT_REPO)
        _qt_internal_get_configure_line(configure_line)
        if(configure_line)
            set(configure_line_comment
                "\n${repo_project_name_lowercase} was configured with:\n    ${configure_line}\n")
            string(APPEND project_comment "${configure_line_comment}")
        endif()
    endif()

    if(project_comment)
        # Escape any potential semicolons.
        string(REPLACE ";" "\\;" project_comment "${project_comment}")
        set(project_comment PROJECT_COMMENT "${project_comment}")
    endif()

    if(QT_SBOM_GENERATE_SPDX_V2)
        _qt_internal_sbom_begin_project_generate(
            OUTPUT "${repo_spdx_install_path_spdx}"
            OUTPUT_RELATIVE_PATH "${repo_spdx_relative_install_path_spdx}"
            PROJECT "${repo_project_name_lowercase}"
            ${project_comment}
            PROJECT_FOR_SPDX_ID "${repo_project_name_for_spdx_id}"
            NAMESPACE "${repo_spdx_namespace}"
            ${begin_project_generate_args_spdx}
            OUT_VAR_PROJECT_SPDX_ID repo_project_spdx_id
        )
    endif()

    if(QT_SBOM_GENERATE_CYDX_V1_6)
        _qt_internal_sbom_begin_project_generate_cyclone(
            OUTPUT "${repo_spdx_install_path_cydx}"
            OUTPUT_RELATIVE_PATH "${repo_spdx_relative_install_path_cydx}"
            PROJECT "${repo_project_name_lowercase}"
            ${project_comment}
            PROJECT_FOR_SPDX_ID "${repo_project_name_for_spdx_id}"
            NAMESPACE "${repo_spdx_namespace}"
            ${begin_project_generate_args_cydx}
            OUT_VAR_PROJECT_SPDX_ID repo_project_spdx_id
        )
    endif()

    set_property(GLOBAL PROPERTY _qt_internal_project_attribution_files "")

    set_property(GLOBAL PROPERTY _qt_internal_sbom_repo_document_namespace
        "${repo_spdx_namespace}")

    set_property(GLOBAL PROPERTY _qt_internal_sbom_repo_cyclone_dx_bom_serial_number_uuid
        "${cyclone_dx_bom_serial_number_uuid}")

    # TODO: Figure out what needs to be done to fully port usage to the tag value var below,
    # taking into account compatibility for Qt Creator, etc.
    set_property(GLOBAL PROPERTY _qt_internal_sbom_relative_installed_repo_document_path
        "${repo_spdx_relative_install_path_spdx}")

    set_property(GLOBAL PROPERTY _qt_internal_sbom_document_spdx_v2_tag_value_relative_path
        "${repo_spdx_relative_install_path_spdx}")

    set_property(GLOBAL PROPERTY _qt_internal_sbom_document_spdx_v2_json_relative_path
        "${repo_spdx_relative_install_path_spdx_json}")

    set_property(GLOBAL PROPERTY _qt_internal_sbom_document_cydx_v1_6_json_relative_path
        "${repo_spdx_relative_install_path_cydx_json}")

    set_property(GLOBAL PROPERTY _qt_internal_sbom_repo_project_name_lowercase
        "${repo_project_name_lowercase}")

    set_property(GLOBAL PROPERTY _qt_internal_sbom_install_prefix
        "${arg_INSTALL_PREFIX}")

    set_property(GLOBAL PROPERTY _qt_internal_sbom_project_spdx_id
        "${repo_project_spdx_id}")

    _qt_internal_create_project_sbom_target()

    # Collect project licenses.
    set(license_dirs "")

    if(arg___QT_INTERNAL_HANDLE_QT_REPO AND EXISTS "${PROJECT_SOURCE_DIR}/LICENSES")
        list(APPEND license_dirs "${PROJECT_SOURCE_DIR}/LICENSES")
    endif()

    set(license_dir_candidates "")
    if(arg_LICENSE_DIR_PATHS)
        list(APPEND license_dir_candidates ${arg_LICENSE_DIR_PATHS})
    endif()

    # Allow specifying extra license dirs via a variable. Useful for standalone projects
    # like sqldrivers.
    # Kept for backwards compatibility, the new LICENSE_DIR_PATHS option should be preferred.
    if(QT_SBOM_LICENSE_DIRS)
        list(APPEND license_dir_candidates ${QT_SBOM_LICENSE_DIRS})
    endif()

    foreach(license_dir IN LISTS license_dir_candidates)
        if(EXISTS "${license_dir}")
            list(APPEND license_dirs "${license_dir}")
        endif()
    endforeach()

    list(REMOVE_DUPLICATES license_dirs)

    set(license_file_wildcard "LicenseRef-*.txt")
    list(TRANSFORM license_dirs APPEND "/${license_file_wildcard}" OUTPUT_VARIABLE license_globs)

    file(GLOB license_files ${license_globs})

    foreach(license_file IN LISTS license_files)
        get_filename_component(license_id "${license_file}" NAME_WLE)
        _qt_internal_sbom_add_license(
            LICENSE_ID "${license_id}"
            LICENSE_PATH "${license_file}"
            NO_LICENSE_REF_PREFIX
        )
    endforeach()

    # Make sure that any system library dependencies that have been found via qt_find_package or
    # _qt_internal_find_third_party_dependencies have their spdx id registered now.
    _qt_internal_sbom_record_system_library_spdx_ids()

    set_property(GLOBAL PROPERTY _qt_internal_sbom_repo_begin_called TRUE)

    _qt_internal_sbom_setup_project_ops()

    if(NOT arg_NO_AUTO_ADD_BUILD_TOOLS)
        _qt_internal_sbom_add_project_default_build_tools()
    endif()
endfunction()

# Check various internal options to decide which sbom generation operations should be setup.
# Considered operations are generation of a JSON sbom, validation of the SBOM, NTIA checker, etc.
function(_qt_internal_sbom_setup_project_ops)
    set(options "")

    if(QT_SBOM_GENERATE_JSON
            OR QT_SBOM_GENERATE_SPDX_V2_JSON
            OR QT_INTERNAL_SBOM_GENERATE_JSON
            OR QT_INTERNAL_SBOM_DEFAULT_CHECKS)
        list(APPEND options GENERATE_JSON)
    endif()

    # Tring to generate the JSON might fail if the python dependencies are not available.
    # The user can explicitly request to fail the build if dependencies are not found.
    # error out. For internal options that the CI uses, we always want to fail the build if the
    # deps are not found.
    if(QT_SBOM_REQUIRE_GENERATE_JSON
            OR QT_SBOM_REQUIRE_GENERATE_SPDX_V2_JSON
            OR QT_INTERNAL_SBOM_GENERATE_JSON
            OR QT_INTERNAL_SBOM_DEFAULT_CHECKS)
        list(APPEND options GENERATE_JSON_REQUIRED)
    endif()

    if(QT_SBOM_VERIFY
            OR QT_SBOM_VERIFY_SPDX_V2
            OR QT_INTERNAL_SBOM_VERIFY
            OR QT_INTERNAL_SBOM_DEFAULT_CHECKS)
        list(APPEND options VERIFY_SBOM)
    endif()

    # Do the same requirement check for SBOM verification.
    if(QT_SBOM_REQUIRE_VERIFY
            OR QT_SBOM_REQUIRE_VERIFY_SPDX_V2
            OR QT_INTERNAL_SBOM_VERIFY
            OR QT_INTERNAL_SBOM_DEFAULT_CHECKS)
        list(APPEND options VERIFY_SBOM_REQUIRED)
    endif()

    if(QT_SBOM_GENERATE_CYDX_V1_6)
        list(APPEND options GENERATE_CYCLONE_DX_V1_6)
    endif()

    if(QT_SBOM_REQUIRE_GENERATE_CYDX_V1_6)
        list(APPEND options GENERATE_CYCLONE_DX_V1_6_REQUIRED)
    endif()

    if(QT_SBOM_VERIFY_CYDX_V1_6)
        list(APPEND options VERIFY_CYCLONE_DX_V1_6)
    endif()

    if(QT_SBOM_REQUIRE_VERIFY_CYDX_V1_6)
        list(APPEND options VERIFY_CYCLONE_DX_V1_6_REQUIRED)
    endif()

    if(QT_SBOM_VERBOSE_CYDX_V1_6)
        list(APPEND options VERBOSE_CYCLONE_DX_V1_6)
    endif()

    if(QT_SBOM_VERIFY_NTIA_COMPLIANT
            OR QT_INTERNAL_SBOM_VERIFY_NTIA_COMPLIANT OR QT_INTERNAL_SBOM_DEFAULT_CHECKS)
        list(APPEND options VERIFY_NTIA_COMPLIANT)
    endif()

    if(QT_SBOM_SHOW_TABLE OR QT_INTERNAL_SBOM_SHOW_TABLE OR QT_INTERNAL_SBOM_DEFAULT_CHECKS)
        list(APPEND options SHOW_TABLE)
    endif()

    if(QT_SBOM_AUDIT OR QT_INTERNAL_SBOM_AUDIT OR QT_INTERNAL_SBOM_AUDIT_NO_ERROR)
        list(APPEND options AUDIT)
    endif()

    if(QT_SBOM_AUDIT_NO_ERROR OR QT_INTERNAL_SBOM_AUDIT_NO_ERROR)
        list(APPEND options AUDIT_NO_ERROR)
    endif()

    if(QT_GENERATE_SOURCE_SBOM)
        list(APPEND options GENERATE_SOURCE_SBOM)
    endif()

    if(QT_LINT_SOURCE_SBOM)
        list(APPEND options LINT_SOURCE_SBOM)
    endif()

    if(QT_LINT_SOURCE_SBOM_NO_ERROR OR QT_INTERNAL_LINT_SOURCE_SBOM_NO_ERROR)
        list(APPEND options LINT_SOURCE_SBOM_NO_ERROR)
    endif()

    _qt_internal_sbom_setup_project_ops_generation(${options})
endfunction()

# Sets up SBOM generation and verification options.
#
# By default, the main toggle for SBOM generation is disabled. The GENERATE_SBOM_DEFAULT option
# overrides that value and can be set by a project that sets up SBOM generation.
#
# If the main toggle gets enabled, we enable SPDX V2.3 tag:value generation, and try to enable
# CycloneDX V1.6 generation. Try, because CDX generation needs python dependencies. If they are
# not found, the generation is silently skipped.
#
# By default SPDX v2.3 JSON generation and verification is enabled, if the python dependencies
# are found. Otherwise they will be silently skipped.
# Unless the user explicitly requests to fail the build if the dependencies are not found.
# The same can be done for CycloneDX generation.
#
# Some older variables that were added pre-CycloneDX generation are deprecated.
function(_qt_internal_setup_sbom)
    set(opt_args "")
    set(single_args
        GENERATE_SBOM_DEFAULT
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(default_value "OFF")
    if(NOT "${arg_GENERATE_SBOM_DEFAULT}" STREQUAL "")
        set(default_value "${arg_GENERATE_SBOM_DEFAULT}")
    endif()

    # Main SBOM toggle. Used to be the toggle for SPDX v2.3 only, but now would also enable Cyclone
    # DX as well.
    set(sbom_help_string "Generate SBOM.")
    option(QT_GENERATE_SBOM "${sbom_help_string}" "${default_value}")


    # Toggle for SPDX V2.3 generation.
    set(spdx_v2_help_string "Generate SBOM documents in SPDX v2.3 tag:value format.")
    option(QT_SBOM_GENERATE_SPDX_V2 "${spdx_v2_help_string}" ON)


    # Toggles for CycloneDX V1.6 generation.
    set(cydx_help_string "Generate SBOM documents in CycloneDX v1.6 JSON format.")
    option(QT_SBOM_GENERATE_CYDX_V1_6 "${cydx_help_string}" ON)

    set(cydx_require_help_string
        "Error out if CycloneDX SBOM generation dependencies are not found.")
    option(QT_SBOM_REQUIRE_GENERATE_CYDX_V1_6 "${cydx_require_help_string}" OFF)


    # Options for SPDX v2.3 JSON generation and verification.

    string(CONCAT spdx_v23_json_help_string
        "Generate SBOM documents in SPDX v2.3 JSON format if required python dependency "
        "spdx-tools is available."
    )

    set(spdx_v23_json_require_help_string
        "Error out if JSON SBOM generation depdendency is not found.")

    set(spdx_v23_verify_help_string
        "Verify generated SBOM documents using python spdx-tools package.")

    set(spdx_v23_verify_require_help_string
        "Error out if SBOM verification dependencies are not found.")

    option(QT_SBOM_GENERATE_SPDX_V2_JSON "${spdx_v23_json_help_string}" ON)
    option(QT_SBOM_REQUIRE_GENERATE_SPDX_V2_JSON "${spdx_v23_json_require_help_string}" OFF)

    option(QT_SBOM_VERIFY_SPDX_V2 "${spdx_v23_verify_help_string}" ON)
    option(QT_SBOM_REQUIRE_VERIFY_SPDX_V2 "${spdx_v23_verify_require_help_string}" OFF)


    # Options for CycloneDX verification and verbosity.

    set(cydx_verify_help_string
        "Verify generated CycloneDX document against its json schema.")
    option(QT_SBOM_VERIFY_CYDX_V1_6 "${cydx_verify_help_string}" ON)

    set(cydx_verify_require_help_string
        "Error out if SBOM verification dependencies are not found.")
    option(QT_SBOM_REQUIRE_VERIFY_CYDX_V1_6 "${cydx_verify_require_help_string}" OFF)

    set(cydx_verbose_help_string
        "Enable verbose output for CycloneDX generation.")
    option(QT_SBOM_VERBOSE_CYDX_V1_6 "${cydx_verbose_help_string}" OFF)


    # Deprecated options, superseded by the options above.
    # Only add them if the values was previously defined, but update the doc string.

    if(DEFINED QT_SBOM_GENERATE_JSON)
        option(QT_SBOM_GENERATE_JSON "Deprecated: ${spdx_v23_json_help_string}" ON)
    endif()
    if(DEFINED QT_SBOM_REQUIRE_GENERATE_JSON)
        option(QT_SBOM_REQUIRE_GENERATE_JSON "Deprecated: ${spdx_v23_json_require_help_string}" OFF)
    endif()
    if(DEFINED QT_SBOM_VERIFY)
        option(QT_SBOM_VERIFY "Deprecated: ${spdx_v23_verify_help_string}" ON)
    endif()
    if(DEFINED QT_SBOM_REQUIRE_VERIFY)
        option(QT_SBOM_REQUIRE_VERIFY "Deprecated: ${spdx_v23_verify_require_help_string}" OFF)
    endif()

    # Semi-public, undocumented options to allow enabling all SBOM stuff, for easier testing.
    if(QT_SBOM_GENERATE_AND_VERIFY_ALL)
        set(QT_SBOM_GENERATE_ALL ON)
        set(QT_SBOM_GENERATE_REQUIRED_ALL ON)
        set(QT_SBOM_VERIFY_REQUIRED_ALL ON)
    endif()

    if(QT_SBOM_GENERATE_ALL)
        set(QT_GENERATE_SBOM ON CACHE BOOL "${sbom_help_string}" FORCE)
        set(QT_SBOM_GENERATE_SPDX_V2 ON CACHE BOOL "${spdx_v2_help_string}" FORCE)
        set(QT_SBOM_GENERATE_SPDX_V2_JSON ON CACHE BOOL "${spdx_v23_json_help_string}" FORCE)
        set(QT_SBOM_GENERATE_CYDX_V1_6 ON CACHE BOOL "${cydx_help_string}" FORCE)
        unset(QT_SBOM_GENERATE_ALL CACHE)
        unset(QT_SBOM_GENERATE_ALL)
    endif()

    if(QT_SBOM_GENERATE_REQUIRED_ALL)
        set(QT_SBOM_REQUIRE_GENERATE_SPDX_V2_JSON ON CACHE BOOL
            "${spdx_v23_json_require_help_string}" FORCE)
        set(QT_SBOM_REQUIRE_GENERATE_CYDX_V1_6 ON CACHE BOOL "${cydx_require_help_string}" FORCE)

        unset(QT_SBOM_GENERATE_REQUIRED_ALL CACHE)
        unset(QT_SBOM_GENERATE_REQUIRED_ALL)
    endif()

    if(QT_SBOM_VERIFY_REQUIRED_ALL)
        set(QT_SBOM_VERIFY_SPDX_V2 ON CACHE BOOL "${spdx_v23_verify_help_string}" FORCE)
        set(QT_SBOM_VERIFY_CYDX_V1_6 ON CACHE BOOL "${cydx_verify_help_string}" FORCE)

        set(QT_SBOM_REQUIRE_VERIFY_SPDX_V2 ON CACHE BOOL "${spdx_v23_verify_require_help_string}"
            FORCE)
        set(QT_SBOM_REQUIRE_VERIFY_CYDX_V1_6 ON CACHE BOOL "${cydx_verify_require_help_string}"
            FORCE)

        unset(QT_SBOM_VERIFY_ALL CACHE)
        unset(QT_SBOM_VERIFY_ALL)
    endif()

    # Various sanity checks.

    # Disable SPDX v2.3 JSON generation if tag:value generation is disabled.
    if(QT_GENERATE_SBOM
            AND QT_SBOM_GENERATE_SPDX_V2_JSON
            AND NOT QT_SBOM_GENERATE_SPDX_V2)
        if(NOT QT_NO_SBOM_INFORMATIONAL_MESSAGES)
            message(STATUS
                "Disabling SPDX v2.3 SBOM JSON generation because tag:value generation is "
                "disabled and that is a requirement for JSON generation.")
        endif()
        set(QT_SBOM_GENERATE_SPDX_V2_JSON OFF CACHE BOOL "${spdx_v23_json_help_string}" FORCE)
        set(QT_SBOM_VERIFY_SPDX_V2 OFF CACHE BOOL "${spdx_v23_verify_help_string}" FORCE)
    endif()

    # Disable CycloneDX generation if dependencies are not found and it wasn't required.
    if(QT_GENERATE_SBOM
            AND QT_SBOM_GENERATE_CYDX_V1_6
            AND NOT QT_SBOM_REQUIRE_GENERATE_CYDX_V1_6)
        _qt_internal_sbom_find_cydx_dependencies(OUT_VAR_DEPS_FOUND deps_found)
        if(NOT deps_found)
            if(NOT QT_NO_SBOM_INFORMATIONAL_MESSAGES)
                message(STATUS
                    "Disabling Cyclone DX SBOM generation because dependencies were not found, "
                    "and generation was not marked as required.")
            endif()
            set(QT_SBOM_GENERATE_CYDX_V1_6 OFF CACHE BOOL "${cydx_help_string}" FORCE)
        endif()
    endif()

    # Disable sbom generation if none of the formats are enabled. Failing to do so will cause
    # errors in _qt_internal_sbom_begin_project.
    if(QT_GENERATE_SBOM
            AND NOT QT_SBOM_GENERATE_SPDX_V2
            AND NOT QT_SBOM_GENERATE_CYDX_V1_6)
        if(NOT QT_NO_SBOM_INFORMATIONAL_MESSAGES)
            message(STATUS
                "Disabling SBOM generation because none of the supported formats were enabled.")
        endif()
        set(QT_GENERATE_SBOM OFF CACHE BOOL "${sbom_help_string}" FORCE)
    endif()
endfunction()

# Disable SBOM processing for targets created under tests/ or examples/.
# Some repos create mock qt modules under these directories like qtdeclarative and
# qtwebengine.
macro(_qt_internal_conditionally_disable_sbom_in_current_scope)
    set(QT_GENERATE_SBOM OFF)
endmacro()

# Ends repo sbom project generation.
# Should be called after all relevant targets are added to the sbom.
# Handles registering sbom info for recorded system libraries and then creates the sbom build
# and install rules.
function(_qt_internal_sbom_end_project)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    # Run sbom finalization for targets that had it scheduled, but haven't run yet.
    # This can happen when _qt_internal_sbom_end_project is called within the same
    # subdirectory scope as where the targets are meant to be finalized, but that would be too late
    # and the targets wouldn't be added to the sbom.
    # This can happen in user projects, but also it happens for qtbase with the Platform targets.
    # Check the list of targets to finalize in a loop, because finalizing a target can schedule
    # finalization of a different attribution target.
    set(targets_to_finalize "")
    get_cmake_property(new_targets _qt_internal_sbom_targets_waiting_for_finalization)
    if(new_targets)
        list(APPEND targets_to_finalize ${new_targets})
    endif()

    set(finalization_iterations 0)
    while(targets_to_finalize)
        # Make sure we don't accidentally create a never-ending loop.
        math(EXPR finalization_iterations "${finalization_iterations} + 1")
        if(finalization_iterations GREATER 500)
            message(WARNING
                "SBOM warning: Too many iterations while handling finalization of SBOM targets. "
                "Possible circular dependency. "
                "Please report to https://bugreports.qt.io with details "
                "about the project and a trace log. "
                "Targets left to finalize: '${targets_to_finalize}'"
            )
            break()
        endif()

        # Clear the list.
        set_property(GLOBAL PROPERTY _qt_internal_sbom_targets_waiting_for_finalization "")

        foreach(target IN LISTS targets_to_finalize)
            _qt_internal_finalize_sbom("${target}")
        endforeach()

        # Retrieve any new targets.
        set(targets_to_finalize "")
        get_cmake_property(new_targets _qt_internal_sbom_targets_waiting_for_finalization)
        if(new_targets)
            list(APPEND targets_to_finalize ${new_targets})
        endif()
    endwhile()

    # Now that we know which system libraries are linked against because we added all
    # subdirectories and finalized all targets, we can add the recorded system libs to the sbom.
    _qt_internal_sbom_add_recorded_system_libraries()

    # Add any external target dependencies, for CycloneDX generation.
    # E.g. For QtSvg, we need to create a QtCore component in the QtSvg document, so that we
    # can declare a dependency on it.
    if(QT_SBOM_GENERATE_CYDX_V1_6)
        _qt_internal_sbom_add_cydx_external_target_dependencies()
    endif()

    if(QT_SBOM_GENERATE_SPDX_V2)
        _qt_internal_sbom_end_project_generate()
    endif()

    if(QT_SBOM_GENERATE_CYDX_V1_6)
        _qt_internal_sbom_end_project_generate_cyclone()
    endif()

    # Clean up external document ref properties, because each repo needs to start from scratch
    # in a top-level build.
    get_cmake_property(known_external_documents _qt_known_external_documents)
    set_property(GLOBAL PROPERTY _qt_known_external_documents "")
    foreach(external_document IN LISTS known_external_documents)
        set_property(GLOBAL PROPERTY _qt_known_external_documents_${external_document} "")
        set_property(GLOBAL PROPERTY _qt_known_external_documents_${external_document}_target "")
    endforeach()

    set_property(GLOBAL PROPERTY _qt_internal_sbom_repo_begin_called FALSE)
    set_property(GLOBAL PROPERTY _qt_internal_sbom_repo_spdx_id_unique_suffix "")
    set_property(GLOBAL PROPERTY _qt_internal_sbom_external_document_search_paths "")
    set_property(GLOBAL PROPERTY _qt_internal_sbom_auto_search_external_documents_in_paths "")

    # Add configure-time dependency on project attribution files.
    get_property(attribution_files GLOBAL PROPERTY _qt_internal_project_attribution_files)
    _qt_internal_append_cmake_configure_depends(${attribution_files})
endfunction()

# Automatically begins sbom generation for a qt git repo unless QT_SKIP_SBOM_AUTO_PROJECT is TRUE.
function(_qt_internal_sbom_auto_begin_qt_repo_project)
    # Allow skipping auto generation of sbom project, in case it needs to be manually adjusted with
    # extra parameters.
    if(QT_SKIP_SBOM_AUTO_PROJECT)
        return()
    endif()

    _qt_internal_sbom_begin_qt_repo_project()
endfunction()

# Sets up sbom generation for a qt git repo or qt-git-repo-sub-project (e.g. qtpdf in qtwebengine).
#
# In the case of a qt-git-repo-sub-project, the function expects the following options:
# - SBOM_PROJECT_NAME (e.g. QtPdf)
# - QT_REPO_PROJECT_NAME (e.g. QtWebEngine)
#
# Expects the following variables to always be set before the function call:
# - QT_STAGING_PREFIX
# - INSTALL_SBOMDIR
function(_qt_internal_sbom_begin_qt_repo_project)
    set(opt_args "")
    set(single_args
        SBOM_PROJECT_NAME
        QT_REPO_PROJECT_NAME
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(sbom_project_args "")

    _qt_internal_forward_function_args(
        FORWARD_APPEND
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR sbom_project_args
        FORWARD_OPTIONS
            ${opt_args}
        FORWARD_SINGLE
            ${single_args}
        FORWARD_MULTI
            ${multi_args}
    )

    _qt_internal_sbom_begin_project(
        INSTALL_SBOM_DIR "${INSTALL_SBOMDIR}"
        USE_GIT_VERSION
        __QT_INTERNAL_HANDLE_QT_REPO
        ${sbom_project_args}
    )
endfunction()

# Automatically ends sbom generation for a qt git repo unless QT_SKIP_SBOM_AUTO_PROJECT is TRUE.
function(_qt_internal_sbom_auto_end_qt_repo_project)
    # Allow skipping auto generation of sbom project, in case it needs to be manually adjusted with
    # extra parameters.
    if(QT_SKIP_SBOM_AUTO_PROJECT)
        return()
    endif()

    _qt_internal_sbom_end_qt_repo_project()
endfunction()

# Ends sbom generation for a qt git repo or qt-git-repo-sub-project.
function(_qt_internal_sbom_end_qt_repo_project)
    _qt_internal_sbom_end_project()
endfunction()


# Enables a fake deterministic SBOM build, for easier inter-diffs between sbom files. Useful
# for local development.
function(_qt_internal_sbom_setup_fake_deterministic_build)
    if(NOT DEFINED QT_SBOM_FAKE_DETERMINISTIC_BUILD)
        return()
    endif()

    if(QT_SBOM_FAKE_DETERMINISTIC_BUILD)
        set(value "ON")
    else()
        set(value "OFF")
    endif()

    set(QT_SBOM_FAKE_GIT_VERSION "${value}" CACHE BOOL "SBOM fake git version")
    set(QT_SBOM_FAKE_TIMESTAMP "${value}" CACHE BOOL "SBOM fake timestamp")
    set(QT_SBOM_FAKE_CHECKSUM "${value}" CACHE BOOL "SBOM fake checksums")
endfunction()

# Helper to get purl entry-specific options.
macro(_qt_internal_get_sbom_purl_parsing_options opt_args single_args multi_args)
    set(${opt_args}
        NO_DEFAULT_QT_PURL
        PURL_USE_PACKAGE_VERSION
    )
    set(${single_args}
        PURL_ID
        PURL_TYPE
        PURL_NAMESPACE
        PURL_NAME
        PURL_VERSION
        PURL_SUBPATH
        PURL_VCS_URL
    )
    set(${multi_args}
        PURL_QUALIFIERS
    )
endmacro()

# Helper to get the purl options that should be recongized by sbom functions like
# _qt_internal_sbom_add_target.
macro(_qt_internal_get_sbom_purl_add_target_options opt_args single_args multi_args)
    set(${opt_args}
        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_PURL
    )
    set(${single_args} "")
    set(${multi_args}
        PURLS
        PURL_VALUES
    )
endmacro()

# Helper to get purl options that should be forwarded from _qt_internal_sbom_add_target to
# _qt_internal_sbom_handle_purl_values.
macro(_qt_internal_get_sbom_purl_handling_options opt_args single_args multi_args)
    set(${opt_args} "")
    set(${single_args}
        SUPPLIER
        SBOM_ENTITY_TYPE
        PACKAGE_VERSION
    )
    set(${multi_args} "")

    _qt_internal_get_sbom_purl_add_target_options(
        purl_add_target_opt_args purl_add_target_single_args purl_add_target_multi_args)
    list(APPEND ${opt_args} ${purl_add_target_opt_args})
    list(APPEND ${single_args} ${purl_add_target_single_args})
    list(APPEND ${multi_args} ${purl_add_target_multi_args})
endmacro()

# Helper to get the options that _qt_internal_sbom_add_target understands, but that are also
# a safe subset for qt_internal_add_module, qt_internal_extend_target, etc to understand.
macro(_qt_internal_get_sbom_add_target_common_options opt_args single_args multi_args)
    set(${opt_args}
        NO_CURRENT_DIR_ATTRIBUTION
        NO_ATTRIBUTION_LICENSE_ID
        NO_DEFAULT_QT_LICENSE
        NO_DEFAULT_QT_LICENSE_ID_LIBRARIES
        NO_DEFAULT_QT_LICENSE_ID_EXECUTABLES
        NO_DEFAULT_DIRECTORY_QT_LICENSE
        NO_DEFAULT_QT_COPYRIGHTS
        NO_DEFAULT_QT_PACKAGE_VERSION
        NO_DEFAULT_QT_SUPPLIER
        SBOM_INCOMPLETE_3RD_PARTY_DEPENDENCIES
        IS_QT_3RD_PARTY_HEADER_MODULE
        IS_EXTERNAL_SBOM_ENTITY
        USE_ATTRIBUTION_FILES
        CREATE_SBOM_FOR_EACH_ATTRIBUTION
        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_PACKAGE_VERSION
        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_SUPPLIER
        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_DOWNLOAD_LOCATION
        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_LICENSE
        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_COPYRIGHTS
        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_CPE
        __QT_INTERNAL_HANDLE_QT_ENTITY_ATTRIBUTION_FILES
    )
    set(${single_args}
        SPDX_ID
        EXTERNAL_SBOM_DOCUMENT_TARGET
        DEFAULT_SBOM_ENTITY_TYPE
        SBOM_ENTITY_TYPE
        PACKAGE_VERSION
        PACKAGE_SUMMARY
        FRIENDLY_PACKAGE_NAME
        SUPPLIER
        CPE_VENDOR
        CPE_PRODUCT
        LICENSE_EXPRESSION
        QT_LICENSE_ID
        DOWNLOAD_LOCATION
        ATTRIBUTION_ENTRY_INDEX
        ATTRIBUTION_PARENT_TARGET
        ATTRIBUTION_PARENT_TARGET_SBOM_ENTITY_TYPE
        SBOM_PACKAGE_COMMENT
    )
    set(${multi_args}
        COPYRIGHTS
        CPE
        SBOM_DEPENDENCIES
        ATTRIBUTION_FILE_PATHS
        ATTRIBUTION_FILE_DIR_PATHS
        ATTRIBUTION_IDS
        SBOM_RELATIONSHIP_ENTRIES
        # deprecated, previously used for SPDX v2 string-ified relationships
        # still used by WebEngine.
        SBOM_RELATIONSHIPS
    )

    _qt_internal_get_sbom_purl_add_target_options(
        purl_add_target_opt_args purl_add_target_single_args purl_add_target_multi_args)
    list(APPEND ${opt_args} ${purl_add_target_opt_args})
    list(APPEND ${single_args} ${purl_add_target_single_args})
    list(APPEND ${multi_args} ${purl_add_target_multi_args})
endmacro()

# Helper to get all known SBOM specific options, without the ones that qt_internal_add_module
# and similar functions understand, like LIBRARIES, INCLUDES, etc.
macro(_qt_internal_get_sbom_specific_options opt_args single_args multi_args)
    set(${opt_args} "")
    set(${single_args} "")
    set(${multi_args} "")

    _qt_internal_get_sbom_add_target_common_options(
        common_opt_args common_single_args common_multi_args)
    list(APPEND ${opt_args} ${common_opt_args})
    list(APPEND ${single_args} ${common_single_args})
    list(APPEND ${multi_args} ${common_multi_args})

    _qt_internal_sbom_get_multi_config_single_args(multi_config_single_args)
    list(APPEND ${single_args} ${multi_config_single_args})
endmacro()

# Helper to get the options that _qt_internal_sbom_add_target understands.
# Also used in qt_find_package_extend_sbom.
macro(_qt_internal_get_sbom_add_target_options opt_args single_args multi_args)
    set(${opt_args}
        NO_INSTALL
    )
    set(${single_args}
        TYPE # deprecated, use SBOM_ENTITY_TYPE or DEFAULT_SBOM_ENTITY_TYPE instead
    )
    set(${multi_args}
        LIBRARIES
        PUBLIC_LIBRARIES
    )

    _qt_internal_get_sbom_specific_options(
        specific_opt_args specific_single_args specific_multi_args)
    list(APPEND ${opt_args} ${specific_opt_args})
    list(APPEND ${single_args} ${specific_single_args})
    list(APPEND ${multi_args} ${specific_multi_args})
endmacro()

# Chooses between SBOM_ENTITY_TYPE, TYPE (deprecated) or DEFAULT_SBOM_ENTITY_TYPE and assigns it to
# out_var.
function(_qt_internal_map_sbom_entity_type out_var)
    set(opt_args "")
    set(single_args
        TYPE # deprecated
        SBOM_ENTITY_TYPE
        DEFAULT_SBOM_ENTITY_TYPE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")

    # Previously only TYPE existed, now there is also SBOM_ENTITY_TYPE and DEFAULT_SBOM_ENTITY_TYPE.
    # DEFAULT_SBOM_ENTITY_TYPE is the fallback value, TYPE is deprecated but left for compatibility
    # and SBOM_ENTITY_TYPE has the highest priority.
    if(arg_SBOM_ENTITY_TYPE)
        set(sbom_entity_type "${arg_SBOM_ENTITY_TYPE}")
    elseif(arg_TYPE)
        # deprecated code path
        if(QT_WARN_DEPRECATED_SBOM_TYPE_OPTION)
            message(DEPRECATION "TYPE option is deprecated, use SBOM_ENTITY_TYPE instead.")
        endif()
        set(sbom_entity_type "${arg_TYPE}")
    elseif(arg_DEFAULT_SBOM_ENTITY_TYPE)
        set(sbom_entity_type "${arg_DEFAULT_SBOM_ENTITY_TYPE}")
    else()
        set(sbom_entity_type "")
    endif()

    set(${out_var} "${sbom_entity_type}" PARENT_SCOPE)
endfunction()

# Generate sbom information for a given target.
# Creates:
# - a SPDX package for the target
# - zero or more SPDX file entries for each installed binary file
# - each binary file entry gets a list of 'generated from source files' section
# - dependency relationships to other target packages
# - other relevant information like licenses, copyright, etc.
# For licenses, copyrights, these can either be passed as options, or read from qt_attribution.json
# files.
# For dependencies, these are either specified via options, or read from properties set on the
# target by qt_internal_extend_target.
function(_qt_internal_sbom_add_target target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    _qt_internal_get_sbom_add_target_options(opt_args single_args multi_args)
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    get_target_property(target_type ${target} TYPE)

    _qt_internal_map_sbom_entity_type(sbom_entity_type ${ARGN})

    # Mark the target as a Qt module for sbom processing purposes.
    # Needed for non-standard targets like Bootstrap and QtLibraryInfo, that don't have a Qt::
    # namespace prefix.
    if(sbom_entity_type STREQUAL QT_MODULE)
        set_target_properties(${target} PROPERTIES _qt_sbom_is_qt_module TRUE)
    endif()

    set(project_package_options_spdx "")
    set(project_package_options_cydx "")

    if(arg_FRIENDLY_PACKAGE_NAME)
        set(package_name_for_spdx_id "${arg_FRIENDLY_PACKAGE_NAME}")
    else()
        set(package_name_for_spdx_id "${target}")
    endif()

    set(package_comment "")

    if(arg_SBOM_INCOMPLETE_3RD_PARTY_DEPENDENCIES)
        string(APPEND package_comment
            "Note: This package was marked as not listing all of its consumed 3rd party "
            "dependencies.\nThus the licensing and copyright information might be incomplete.\n")
    endif()

    if(arg_SBOM_PACKAGE_COMMENT)
        string(APPEND package_comment "${arg_SBOM_PACKAGE_COMMENT}\n")
    endif()

    string(APPEND package_comment "CMake target name: ${target}\n")

    get_target_property(qt_package_name "${target}" _qt_package_name)
    if(qt_package_name)
        get_target_property(qt_module_interface_name "${target}" _qt_module_interface_name)
        set(namespaced_target_name "${QT_CMAKE_EXPORT_NAMESPACE}::${qt_module_interface_name}")

        string(APPEND package_comment
            "CMake exported target name: ${namespaced_target_name}\n")
        string(APPEND package_comment "Contained in CMake package: ${qt_package_name}\n")
    endif()

    _qt_internal_sbom_get_spdx_id_for_target("${target}" package_spdx_id)

    if(arg_USE_ATTRIBUTION_FILES)
        set(attribution_args
            ATTRIBUTION_PARENT_TARGET "${target}"
            ATTRIBUTION_PARENT_TARGET_SBOM_ENTITY_TYPE "${sbom_entity_type}"
        )

        # Forward the sbom specific options when handling attribution files because those might
        # create other sbom targets that need to inherit the parent ones.
        _qt_internal_get_sbom_specific_options(sbom_opt_args sbom_single_args sbom_multi_args)

        _qt_internal_forward_function_args(
            FORWARD_APPEND
            FORWARD_PREFIX arg
            FORWARD_OUT_VAR attribution_args
            FORWARD_OPTIONS
                ${sbom_opt_args}
            FORWARD_SINGLE
                ${sbom_single_args}
            FORWARD_MULTI
                ${sbom_multi_args}
        )

        if(NOT arg_NO_CURRENT_DIR_ATTRIBUTION
                AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/qt_attribution.json")
            list(APPEND attribution_args
                ATTRIBUTION_FILE_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/qt_attribution.json"
            )
        endif()

        _qt_internal_sbom_handle_qt_attribution_files(qa ${attribution_args})
    endif()

    # Collect license expressions, but in most cases, each expression needs to be abided, so we
    # AND the accumulated license expressions.
    set(license_expression "")

    if(arg_LICENSE_EXPRESSION)
        set(license_expression "${arg_LICENSE_EXPRESSION}")
    endif()

    if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_LICENSE)
        _qt_internal_sbom_forward_sbom_add_target_options_modified(sbom_add_target_args)
        _qt_internal_sbom_handle_qt_entity_license_expression(${target} ${sbom_add_target_args}
            OUT_VAR qt_entity_license_expression)
        if(qt_entity_license_expression)
            _qt_internal_sbom_join_two_license_ids_with_op(
                "${license_expression}" "AND" "${qt_entity_license_expression}"
                license_expression)
        endif()
    endif()

    # Read a license expression from the attribution json file.
    if(arg_USE_ATTRIBUTION_FILES
            AND qa_license_id
            AND NOT arg_NO_ATTRIBUTION_LICENSE_ID)
        if(NOT qa_license_id MATCHES "urn:dje:license")
            _qt_internal_sbom_join_two_license_ids_with_op(
                "${license_expression}" "AND" "${qa_license_id}"
                license_expression)
        else()
            message(DEBUG
                "Attribution license id contains invalid spdx license reference: ${qa_license_id}")
            set(invalid_license_comment
                "    Attribution license ID with invalid spdx license reference: ")
            string(APPEND invalid_license_comment "${qa_license_id}\n")
            string(APPEND package_comment "${invalid_license_comment}")
        endif()
    endif()

    if(license_expression)
        list(APPEND project_package_options_spdx LICENSE_CONCLUDED "${license_expression}")
        list(APPEND project_package_options_cydx LICENSE_CONCLUDED "${license_expression}")
    endif()

    if(license_expression AND
            arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_LICENSE)
        _qt_internal_sbom_forward_sbom_add_target_options_modified(sbom_add_target_args)
        _qt_internal_sbom_handle_qt_entity_license_declared_expression(${target}
            ${sbom_add_target_args}
            LICENSE_CONCLUDED_EXPRESSION "${license_expression}"
            OUT_VAR qt_entity_license_declared_expression)
        if(qt_entity_license_declared_expression)
            list(APPEND project_package_options_spdx
                LICENSE_DECLARED "${qt_entity_license_declared_expression}")
            list(APPEND project_package_options_cydx
                LICENSE_DECLARED "${qt_entity_license_declared_expression}")
        endif()
    endif()

    # Copyrights are additive, so we collect them from all sources that were found.
    set(copyrights "")
    if(arg_COPYRIGHTS)
        list(APPEND copyrights "${arg_COPYRIGHTS}")
    endif()

    if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_COPYRIGHTS)
        _qt_internal_sbom_forward_sbom_add_target_options_modified(sbom_add_target_args)
        _qt_internal_sbom_handle_qt_entity_copyrights(${target} ${sbom_add_target_args}
            OUT_VAR qt_copyrights)
        if(qt_copyrights)
            list(APPEND copyrights ${qt_copyrights})
        endif()
    endif()

    if(arg_USE_ATTRIBUTION_FILES AND qa_copyrights)
        list(APPEND copyrights "${qa_copyrights}")
    endif()
    if(copyrights)
        list(JOIN copyrights "\n" copyrights)
        list(APPEND project_package_options_spdx COPYRIGHT "<text>${copyrights}</text>")
        list(APPEND project_package_options_cydx COPYRIGHT "${copyrights}")
    endif()

    set(package_version "")
    if(arg_PACKAGE_VERSION)
        set(package_version "${arg_PACKAGE_VERSION}")
    elseif(arg_USE_ATTRIBUTION_FILES AND qa_version)
        set(package_version "${qa_version}")
    endif()

    if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_PACKAGE_VERSION)
        _qt_internal_sbom_forward_sbom_add_target_options_modified(sbom_add_target_args)
        _qt_internal_sbom_handle_qt_entity_package_version(${target} ${sbom_add_target_args}
            OUT_VAR qt_entity_package_version)
        if(qt_entity_package_version)
            set(package_version "${qt_entity_package_version}")
        endif()
    endif()

    if(package_version)
        list(APPEND project_package_options_spdx VERSION "${package_version}")
        list(APPEND project_package_options_cydx VERSION "${package_version}")

        # Also export the value in a target property, to make it available for cydx generation.
        set_property(TARGET "${target}" PROPERTY _qt_sbom_package_version "${package_version}")
        set_property(TARGET "${target}" APPEND PROPERTY EXPORT_PROPERTIES _qt_sbom_package_version)
    endif()

    set(supplier "")
    if(arg_SUPPLIER)
        set(supplier "${arg_SUPPLIER}")
    endif()

    if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_SUPPLIER)
        _qt_internal_sbom_forward_sbom_add_target_options_modified(sbom_add_target_args)
        _qt_internal_sbom_handle_qt_entity_supplier(${target} ${sbom_add_target_args}
            OUT_VAR qt_entity_supplier)
        if(qt_entity_supplier)
            set(supplier "${qt_entity_supplier}")
        endif()
    endif()

    if(supplier)
        list(APPEND project_package_options_spdx SUPPLIER "Organization: ${supplier}")
        list(APPEND project_package_options_cydx CYDX_SUPPLIER "${supplier}")
    endif()

    set(download_location "")
    if(arg_DOWNLOAD_LOCATION)
        set(download_location "${arg_DOWNLOAD_LOCATION}")
    endif()

    if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_DOWNLOAD_LOCATION)
        _qt_internal_sbom_forward_sbom_add_target_options_modified(sbom_add_target_args)
        _qt_internal_sbom_handle_qt_entity_download_location(${target} ${sbom_add_target_args}
            OUT_VAR qt_entity_download_location)
        if(qt_entity_download_location)
            set(download_location "${qt_entity_download_location}")
        endif()
    endif()

    if(arg_USE_ATTRIBUTION_FILES)
        if(qa_download_location)
            set(download_location "${qa_download_location}")
        elseif(qa_homepage)
            set(download_location "${qa_homepage}")
        endif()
    endif()

    if(sbom_entity_type STREQUAL "SYSTEM_LIBRARY")
        # Try to get package url that was set using CMake's set_package_properties function.
        # Relies on querying the internal global property name that CMake sets in its
        # implementation.
        get_cmake_property(target_url _CMAKE_${package_name_for_spdx_id}_URL)
        if(target_url)
            set(download_location "${target_url}")
        endif()
        if(NOT download_location
                AND arg_USE_ATTRIBUTION_FILES
                AND qa_download_location)
            set(download_location "${qa_download_location}")
        endif()
    endif()

    if(download_location)
        set(placeholder_args "")
        if(package_version)
            list(APPEND placeholder_args VERSION "${package_version}")
        endif()
        _qt_internal_sbom_replace_qa_placeholders(
            VALUES "${download_location}"
            ${placeholder_args}
            OUT_VAR download_location_replaced
        )

        list(APPEND project_package_options_spdx DOWNLOAD_LOCATION "${download_location_replaced}")
        list(APPEND project_package_options_cydx DOWNLOAD_LOCATION "${download_location_replaced}")
    endif()

    _qt_internal_sbom_get_package_purpose("${sbom_entity_type}" package_purpose)
    list(APPEND project_package_options_spdx PURPOSE "${package_purpose}")

    set(cpe_values "")

    if(arg_CPE)
        list(APPEND cpe_values "${arg_CPE}")
    endif()

    if(arg_CPE_VENDOR AND arg_CPE_PRODUCT)
        _qt_internal_sbom_compute_security_cpe(custom_cpe
            VENDOR "${arg_CPE_VENDOR}"
            PRODUCT "${arg_CPE_PRODUCT}"
            VERSION "${package_version}")
        list(APPEND cpe_values "${custom_cpe}")
    endif()

    if(arg_USE_ATTRIBUTION_FILES AND qa_cpes)
        set(placeholder_args "")
        if(package_version)
            list(APPEND placeholder_args VERSION "${package_version}")
        endif()
        _qt_internal_sbom_replace_qa_placeholders(
            VALUES ${qa_cpes}
            ${placeholder_args}
            OUT_VAR qa_cpes_replaced
        )
        list(APPEND cpe_values "${qa_cpes_replaced}")
    endif()

    if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_CPE)
        _qt_internal_sbom_forward_sbom_add_target_options_modified(sbom_add_target_args)
        _qt_internal_sbom_handle_qt_entity_cpe(${target} ${sbom_add_target_args}
            CPE "${cpe_values}"
            OUT_VAR qt_cpe_list)
        if(qt_cpe_list)
            list(APPEND cpe_values ${qt_cpe_list})
        endif()
    endif()

    if(cpe_values)
        list(APPEND project_package_options_spdx CPE ${cpe_values})
        list(APPEND project_package_options_cydx CPE ${cpe_values})
    endif()

    # Assemble arguments to forward to the function that handles purl options.
    set(purl_args
        SBOM_ENTITY_TYPE ${sbom_entity_type}
    )

    _qt_internal_get_sbom_purl_add_target_options(purl_opt_args purl_single_args purl_multi_args)
    _qt_internal_forward_function_args(
        FORWARD_APPEND
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR purl_args
        FORWARD_OPTIONS
            ${purl_opt_args}
        FORWARD_SINGLE
            ${purl_single_args}
        FORWARD_MULTI
            ${purl_multi_args}
    )

    if(supplier)
        list(APPEND purl_args SUPPLIER "${supplier}")
    endif()

    if(package_version)
        list(APPEND purl_args PACKAGE_VERSION "${package_version}")
    endif()

    if(arg_USE_ATTRIBUTION_FILES AND qa_purls)
        set(placeholder_args "")
        if(package_version)
            list(APPEND placeholder_args VERSION "${package_version}")
        endif()
        _qt_internal_sbom_replace_qa_placeholders(
            VALUES ${qa_purls}
            ${placeholder_args}
            OUT_VAR qa_purls_replaced
        )

        list(APPEND purl_args PURL_VALUES ${qa_purls_replaced})
    endif()
    list(APPEND purl_args
        OUT_VAR_PURL_VALUES purl_values
        OUT_VAR_SPDX_EXT_REF_VALUES spdx_ext_ref_values
    )

    _qt_internal_sbom_handle_purl_values(${target} ${purl_args})

    if(spdx_ext_ref_values)
        list(APPEND project_package_options_spdx ${spdx_ext_ref_values})
        list(APPEND project_package_options_cydx PURL_VALUES ${purl_values})
    endif()

    if(arg_USE_ATTRIBUTION_FILES)
        if(qa_chosen_attribution_file_path)
            _qt_internal_sbom_map_path_to_reproducible_relative_path(relative_attribution_path
                PATH "${qa_chosen_attribution_file_path}"
            )
            string(APPEND package_comment
                "    Information extracted from:\n     ${relative_attribution_path}\n")
        endif()

        if(NOT "${qa_chosen_attribution_entry_index}" STREQUAL "")
            string(APPEND package_comment
                "    Entry index: ${qa_chosen_attribution_entry_index}\n")
        endif()

        if(qa_attribution_id)
            string(APPEND package_comment "    Id: ${qa_attribution_id}\n")
        endif()

        if(qa_attribution_name)
            string(APPEND package_comment "    Name: ${qa_attribution_name}\n")
        endif()

        if(qa_description)
            string(APPEND package_comment "    Description: ${qa_description}\n")
        endif()

        if(qa_qt_usage)
            string(APPEND package_comment "    Qt usage: ${qa_qt_usage}\n")
        endif()

        if(qa_license)
            string(APPEND package_comment "    License: ${qa_license}\n")
        endif()

        if(qa_license_file)
            string(APPEND package_comment "    License file: ${qa_license_file}\n")
        endif()
    endif()

    if(package_comment)
        list(APPEND project_package_options_spdx COMMENT "<text>\n${package_comment}</text>")
        list(APPEND project_package_options_cydx COMMENT "\n${package_comment}")
    endif()

    get_cmake_property(project_spdx_id _qt_internal_sbom_project_spdx_id)

    _qt_internal_forward_function_args(
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR relationship_args
        FORWARD_MULTI
            LIBRARIES
            PUBLIC_LIBRARIES
            SBOM_RELATIONSHIP_ENTRIES
            SBOM_RELATIONSHIPS # deprecated, still used by WebEngine
    )

    _qt_internal_sbom_handle_target_relationships("${target}"
        SPDX_ID "${package_spdx_id}"
        PROJECT_SPDX_ID "${project_spdx_id}"
        ${relationship_args}
        OUT_VAR_SBOM_RELATIONSHIP_ENTRIES sbom_relationship_entries
        OUT_VAR_SPDX_V2_RELATIONSHIPS spdx_relationships # deprecated, still used by WebEngine
    )
    if(sbom_relationship_entries)
        list(APPEND project_package_options_cydx
            SBOM_RELATIONSHIP_ENTRIES ${sbom_relationship_entries})
        list(APPEND project_package_options_spdx
            SBOM_RELATIONSHIP_ENTRIES ${sbom_relationship_entries})
    endif()
    if(spdx_relationships)
        list(APPEND project_package_options_spdx RELATIONSHIPS ${spdx_relationships})
    endif()

    if(arg_PACKAGE_SUMMARY)
        list(APPEND project_package_options_spdx PACKAGE_SUMMARY "${arg_PACKAGE_SUMMARY}")
    endif()

    if(QT_SBOM_GENERATE_SPDX_V2)
        _qt_internal_sbom_generate_add_package(
            PACKAGE "${package_name_for_spdx_id}"
            SPDXID "${package_spdx_id}"
            ${project_package_options_spdx}
        )
    endif()

    if(QT_SBOM_GENERATE_CYDX_V1_6)
        get_property(external_targets
            GLOBAL PROPERTY _qt_internal_sbom_external_target_dependencies)

        # Prevent against case when a system library is also an external target dependency,
        # which would lead to its creation twice, once via
        # _qt_internal_sbom_add_recorded_system_libraries
        # and second time via
        # _qt_internal_sbom_add_cydx_external_target_dependencies.
        # Skip the case when it's done via the first function, and only allow the second.
        # TODO: Can this be done better somehow?
        if(NOT target IN_LIST external_targets)
            _qt_internal_sbom_handle_qt_entity_cydx_properties(
                SBOM_ENTITY_TYPE "${sbom_entity_type}"
                OUT_CYDX_PROPERTIES cydx_properties
            )

            _qt_internal_sbom_generate_cyclone_add_package(
                PACKAGE "${package_name_for_spdx_id}"
                SPDXID "${package_spdx_id}"
                SBOM_ENTITY_TYPE "${sbom_entity_type}"
                ${project_package_options_cydx}
                CONTAINING_COMPONENT "${project_spdx_id}"
                CYDX_PROPERTIES ${cydx_properties}
            )
        endif()
    endif()

    set(no_install_option "")
    if(arg_NO_INSTALL)
        set(no_install_option NO_INSTALL)
    endif()

    set(framework_option "")
    if(APPLE AND NOT target_type STREQUAL "INTERFACE_LIBRARY")
        get_target_property(is_framework ${target} FRAMEWORK)
        if(is_framework)
            set(framework_option "FRAMEWORK")
        endif()
    endif()

    set(install_prefix_option "")
    get_cmake_property(install_prefix _qt_internal_sbom_install_prefix)
    if(install_prefix)
        set(install_prefix_option INSTALL_PREFIX "${install_prefix}")
    endif()

    _qt_internal_forward_function_args(
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR target_binary_multi_config_args
        FORWARD_SINGLE
            ${multi_config_single_args}
    )

    set(copyrights_option "")
    if(copyrights)
        set(copyrights_option COPYRIGHTS "${copyrights}")
    endif()

    set(license_option "")
    if(license_expression)
        set(license_option LICENSE_EXPRESSION "${license_expression}")
    endif()

    if(QT_SBOM_GENERATE_SPDX_V2)
        _qt_internal_sbom_handle_target_binary_files("${target}"
            ${no_install_option}
            ${framework_option}
            ${install_prefix_option}
            SBOM_ENTITY_TYPE "${sbom_entity_type}"
            ${target_binary_multi_config_args}
            SPDX_ID "${package_spdx_id}"
            ${copyrights_option}
            ${license_option}
        )

        _qt_internal_sbom_handle_target_custom_files("${target}"
            ${no_install_option}
            ${install_prefix_option}
            PACKAGE_TYPE "${sbom_entity_type}"
            PACKAGE_SPDX_ID "${package_spdx_id}"
            ${copyrights_option}
            ${license_option}
        )
    endif()
endfunction()

# Helper to add sbom information for a possibly non-existing target.
# This will defer the actual sbom generation until the end of the directory scope, unless
# immediate finalization was requested.
function(_qt_internal_add_sbom target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args "")
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    # No validation on purpose, the other options will be validated later.

    set(forward_args ${ARGN})

    # If a target doesn't exist we create it.
    if(NOT TARGET "${target}")
        _qt_internal_create_sbom_target("${target}" ${forward_args})
    endif()

    # Save the passed options.
    _qt_internal_extend_sbom("${target}" ${forward_args})
endfunction()

# Helper to add custom sbom information for some kind of dependency that is not backed by an
# existing target.
# Useful for cases like 3rd party dependencies not represented by an already existing imported
# target, or for 3rd party sources that get compiled into a regular Qt target (PCRE sources compiled
# into Bootstrap).
function(_qt_internal_create_sbom_target target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args "")
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    # No validation on purpose, the other options will be validated later.

    if(TARGET "${target}")
        message(FATAL_ERROR "The target ${target} already exists.")
    endif()

    add_library("${target}" INTERFACE IMPORTED)
    set_target_properties(${target} PROPERTIES
        _qt_sbom_is_custom_sbom_target "TRUE"
        IMPORTED_GLOBAL TRUE
    )
endfunction()

# Creates a custom target to represent a project's root SBOM package / component.
# For SPDX it represents what we consider the root package, although the spec doesn't define
# any such package.
# For CYDX it represents the root component, which is defined by the spec.
# The target is currently intended to be used in sbom relationship entries, but might be expanded
# with further information.
function(_qt_internal_create_project_sbom_target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args "")
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_sbom_get_current_project_target(target)
    if(TARGET "${target}")
        message(FATAL_ERROR "The target ${target} already exists.")
    endif()

    add_library("${target}" INTERFACE IMPORTED)

    get_property(project_spdx_id GLOBAL PROPERTY _qt_internal_sbom_project_spdx_id)
    if(NOT project_spdx_id)
        message(FATAL_ERROR "The global property _qt_internal_sbom_project_spdx_id was not set, "
            "which is required to create a project sbom target.")
    endif()

    get_property(repo_document_namespace
        GLOBAL PROPERTY _qt_internal_sbom_repo_document_namespace)

    if(NOT repo_document_namespace)
        message(FATAL_ERROR "The global property _qt_internal_sbom_repo_document_namespace was not"
            " set, which is required to create a project sbom target.")
    endif()

    get_property(bom_serial_number_uuid
        GLOBAL PROPERTY _qt_internal_sbom_repo_cyclone_dx_bom_serial_number_uuid)

    if(NOT bom_serial_number_uuid)
        message(FATAL_ERROR "The global property "
            "_qt_internal_sbom_repo_cyclone_dx_bom_serial_number_uuid"
            " was not set, which is required to create a project sbom target.")
    endif()

    _qt_internal_sbom_compute_external_document_ref_spdx_id(
        "${arg_PROJECT_NAME}" external_document_ref)

    set_target_properties("${target}" PROPERTIES
        IMPORTED_GLOBAL TRUE

        _qt_sbom_is_custom_sbom_target "TRUE"
        _qt_sbom_is_project_sbom_target "TRUE"

        _qt_sbom_spdx_id "${project_spdx_id}"
        _qt_sbom_entity_type "SBOM_PROJECT"
        _qt_sbom_spdx_repo_document_namespace "${repo_document_namespace}"
        _qt_sbom_spdx_v2_external_document_ref "${external_document_ref}"
        _qt_sbom_cydx_bom_serial_number_uuid "${bom_serial_number_uuid}"
        _qt_sbom_spdx_repo_project_name_lowercase "${project_name}"
    )

    # The computation for these relies on the previous ones being set.
    _qt_internal_sbom_compute_external_spdx_v2_id("${target}" external_spdx_v2_id)
    _qt_internal_sbom_get_cydx_external_bom_link("${target}" external_bom_link)

    set_target_properties("${target}" PROPERTIES
        _qt_sbom_spdx_v2_external_spdx_id "${external_spdx_v2_id}"
        _qt_sbom_cydx_external_bom_link "${external_bom_link}"
    )
endfunction()

# Returns the target name representing the current sbom project.
# When SBOM generation is disable, returns an empty string.
function(_qt_internal_sbom_get_current_project_target out_var)
    if(NOT QT_GENERATE_SBOM)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    _qt_internal_sbom_get_root_project_name_lower_case(project_name)

    set(target "${project_name}ProjectSbom")
    set(${out_var} "${target}" PARENT_SCOPE)
endfunction()

# Helper to add additional sbom information for an existing target.
# Just appends the options to the target's sbom args property, which will will be evaluated
# during finalization.
# For external sbom targets there is no finalization, so the relevant info is recorded
# immediately.
function(_qt_internal_extend_sbom target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    if(NOT TARGET "${target}")
        message(FATAL_ERROR
            "The target ${target} does not exist, use qt_internal_add_sbom to create "
            "a target first, or call the function on any other exsiting target.")
    endif()

    set(opt_args
        NO_FINALIZATION
        IMMEDIATE_FINALIZATION
        IS_EXTERNAL_SBOM_ENTITY
    )
    set(single_args
        TYPE # deprecated
        SBOM_ENTITY_TYPE
        DEFAULT_SBOM_ENTITY_TYPE
        FRIENDLY_PACKAGE_NAME
        SPDX_ID
        EXTERNAL_SBOM_DOCUMENT_TARGET
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    # No validation on purpose, the other options will be validated later.

    set(forward_args ${ARGN})

    # Remove NO_FINALIZATION, IMMEDIATE_FINALIZATION from the forwarded args.
    list(REMOVE_ITEM forward_args
        NO_FINALIZATION
        IMMEDIATE_FINALIZATION
    )

    _qt_internal_map_sbom_entity_type(sbom_entity_type ${ARGN})

    # Make sure the spdx id and sbom entity type is recorded for the target right now, so it is
    # "known" when handling relationships for other targets, even if the target was not yet
    # finalized.
    # The SBOM entity type is expected to be passed when creating a new SBOM target, or extending
    # an existing target for the first time with SBOM info.
    _qt_internal_sbom_get_spdx_id_for_target("${target}" spdx_id)
    if(NOT spdx_id)
        set(record_spdx_id_args "")

        # Isn't allowed to be empty when creating the spdx id.
        if(sbom_entity_type)
            list(APPEND record_spdx_id_args SBOM_ENTITY_TYPE "${sbom_entity_type}")
        else()
            message(FATAL_ERROR
                "Target '${target}' is missing an SBOM_ENTITY_TYPE value. "
                "Make sure to pass it when creating new SBOM target or extending an existing "
                "target with SBOM infromation for the first time.")
        endif()

        # Friendly package name is allowed to be empty.
        if(arg_FRIENDLY_PACKAGE_NAME)
            list(APPEND record_spdx_id_args PACKAGE_NAME "${arg_FRIENDLY_PACKAGE_NAME}")
        endif()

        # Allow specifying a custom spdx id, even for non-external targets.
        if(arg_SPDX_ID)
            list(APPEND record_spdx_id_args SPDX_ID "${arg_SPDX_ID}")
        endif()

        if(arg_IS_EXTERNAL_SBOM_ENTITY)
            list(APPEND record_spdx_id_args IS_EXTERNAL_SBOM_ENTITY)
        endif()

        if(arg_EXTERNAL_SBOM_DOCUMENT_TARGET)
            list(APPEND record_spdx_id_args
                EXTERNAL_SBOM_DOCUMENT_TARGET ${arg_EXTERNAL_SBOM_DOCUMENT_TARGET})
        endif()

        _qt_internal_sbom_record_target_spdx_id(${target} ${record_spdx_id_args})
    endif()

    get_target_property(is_system_library "${target}" _qt_internal_sbom_is_system_library)
    get_target_property(is_project_sbom_target "${target}" _qt_sbom_is_project_sbom_target)

    set_property(TARGET ${target} APPEND PROPERTY _qt_finalize_sbom_args "${forward_args}")

    # Record the target's relevant properties immediately after they are saved above, so we can
    # refer to it in relationship entries.
    if(arg_IS_EXTERNAL_SBOM_ENTITY AND QT_SBOM_GENERATE_CYDX_V1_6)
        _qt_internal_sbom_record_external_target_dependecies(TARGETS "${target}")
    endif()

    # If requested via NO_FINALIZATION or the target is a system library, don't run finalization.
    # This is necessary for system libraries because those are handled in special code path just
    # before finishing project sbom generation, and finalizing them would cause issues because they
    # don't actually have a TYPE until a later point in time.
    # Also skip regular processing / finalization for external targets, because their handling
    # is different, and we don't create a regular package for them.
    if(NOT arg_NO_FINALIZATION
            AND NOT is_system_library
            AND NOT is_project_sbom_target
            AND NOT arg_IS_EXTERNAL_SBOM_ENTITY
        )
        # Defer finalization. In case it was already deferred, it will be a no-op.
        # Some targets need immediate finalization, like the PlatformInternal ones,
        # because otherwise they would be finalized after the sbom was already generated.
        set(immediate_finalization "")
        if(arg_IMMEDIATE_FINALIZATION)
            set(immediate_finalization IMMEDIATE_FINALIZATION)
        endif()
        _qt_internal_defer_sbom_finalization("${target}" ${immediate_finalization})
    endif()
endfunction()

# Helper to add additional sbom information to targets created by qt_find_package.
# If the package was not found, and the targets were not created, the functions does nothing.
# This is similar to _qt_internal_extend_sbom, but is explicit in the fact that the targets might
# not exist.
function(_qt_find_package_extend_sbom)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    _qt_internal_get_sbom_add_target_options(sbom_opt_args sbom_single_args sbom_multi_args)

    set(opt_args
        ${sbom_opt_args}
    )
    set(single_args
        ${sbom_single_args}
    )
    set(multi_args
        TARGETS
        ${sbom_multi_args}
    )

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    # Make sure not to forward TARGETS.
    # Also don't enable finalization, because system libraries are handled specially in a different
    # code path.
    set(sbom_args
        NO_FINALIZATION
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

    foreach(target IN LISTS arg_TARGETS)
        if(TARGET "${target}")
            _qt_internal_extend_sbom("${target}" ${sbom_args})
        else()
            message(DEBUG "The target ${target} does not exist, skipping extending the sbom info.")
        endif()
    endforeach()
endfunction()

# Helper to defer adding sbom information for a target, at the end of the directory scope.
function(_qt_internal_defer_sbom_finalization target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args
        IMMEDIATE_FINALIZATION
    )
    set(single_args "")
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    get_target_property(sbom_finalization_requested ${target} _qt_sbom_finalization_requested)
    if(sbom_finalization_requested)
        # Already requested, nothing to do.
        return()
    endif()
    set_target_properties(${target} PROPERTIES _qt_sbom_finalization_requested TRUE)

    _qt_internal_append_to_cmake_property_without_duplicates(
        _qt_internal_sbom_targets_waiting_for_finalization
        "${target}"
    )

    set(func "_qt_internal_finalize_sbom")

    if(arg_IMMEDIATE_FINALIZATION)
        _qt_internal_finalize_sbom(${target})
    elseif(QT_BUILDING_QT)
        qt_add_list_file_finalizer("${func}" "${target}")
    elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
        cmake_language(EVAL CODE "cmake_language(DEFER CALL \"${func}\" \"${target}\")")
    else()
        message(FATAL_ERROR "Defer adding a sbom target requires CMake version 3.19")
    endif()
endfunction()

# Finalizer to add sbom information for the target.
# Expects the target to exist.
function(_qt_internal_finalize_sbom target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    get_target_property(sbom_finalization_done ${target} _qt_sbom_finalization_done)
    if(sbom_finalization_done)
        # Already done, nothing to do.
        return()
    endif()
    set_target_properties(${target} PROPERTIES _qt_sbom_finalization_done TRUE)

    get_target_property(sbom_args ${target} _qt_finalize_sbom_args)
    if(NOT sbom_args)
        set(sbom_args "")
    endif()
    _qt_internal_sbom_add_target(${target} ${sbom_args})
endfunction()

# Extends the list of targets that are considered dependencies for target.
function(_qt_internal_extend_sbom_dependencies target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args "")
    set(multi_args
        SBOM_DEPENDENCIES
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT TARGET "${target}")
        message(FATAL_ERROR "The target ${target} does not exist.")
    endif()

    _qt_internal_append_to_target_property_without_duplicates(${target}
        _qt_extend_target_sbom_dependencies "${arg_SBOM_DEPENDENCIES}"
    )
endfunction()

# Sets the sbom project name for the root project.
function(_qt_internal_sbom_set_root_project_name project_name)
    set_property(GLOBAL PROPERTY _qt_internal_sbom_repo_project_name "${project_name}")
endfunction()

# Sets the real qt repo project name for a given project (e.g. set QtWebEngine for project QtPdf).
# This is needed to be able to extract the qt repo dependencies in a top-level build.
function(_qt_internal_sbom_set_qt_repo_project_name project_name)
    set_property(GLOBAL PROPERTY _qt_internal_sbom_qt_repo_project_name "${project_name}")
endfunction()

# Get repo project_name spdx id reference, needs to start with Package- to be NTIA compliant.
function(_qt_internal_sbom_get_root_project_name_for_spdx_id out_var)
    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)
    set(sbom_repo_project_name "Package-${repo_project_name_lowercase}")
    set(${out_var} "${sbom_repo_project_name}" PARENT_SCOPE)
endfunction()

# Returns the lower case sbom project name.
function(_qt_internal_sbom_get_root_project_name_lower_case out_var)
    get_cmake_property(project_name _qt_internal_sbom_repo_project_name)

    if(NOT project_name)
        message(FATAL_ERROR "The current active SBOM project name was not found.")
    endif()

    string(TOLOWER "${project_name}" repo_project_name_lowercase)
    set(${out_var} "${repo_project_name_lowercase}" PARENT_SCOPE)
endfunction()

# Returns the lower case real qt repo project name (e.g. returns 'qtwebengine' when building the
# project qtpdf).
function(_qt_internal_sbom_get_qt_repo_project_name_lower_case out_var)
    get_cmake_property(project_name _qt_internal_sbom_qt_repo_project_name)

    if(NOT project_name)
        message(FATAL_ERROR "No real Qt repo SBOM project name was set.")
    endif()

    string(TOLOWER "${project_name}" repo_project_name_lowercase)
    set(${out_var} "${repo_project_name_lowercase}" PARENT_SCOPE)
endfunction()

# Compute a SPDX v2.3 DocumentRef ID, to reference the current project's SPDX document as an
# external document in another project.
#
# This is only meant to be used for exporting the value as part of the target's properties, to be
# used in a different project.
#
# It should NOT be used to refer to already exported targets.
#
# For querying the DocumentRef of an exported target, use
# _qt_internal_sbom_get_external_document_ref_spdx_id_from_sbom_target.
function(_qt_internal_sbom_compute_external_document_ref_spdx_id repo_name out_var)
    _qt_internal_sbom_get_spdx_id_unique_suffix(spdx_id_unique_suffix)
    set(${out_var} "DocumentRef-${repo_name}${spdx_id_unique_suffix}" PARENT_SCOPE)
endfunction()

# Older deprecated function name of the function above, kept for compatibility in case it is used.
function(_qt_internal_sbom_get_external_document_ref_spdx_id repo_name out_var)
    if(NOT QT_NO_DEPRECATED_GET_EXTERNAL_DOCUMENT_REF_SPDX_ID)
        message(DEPRECATION
            "This function is deprecated. "
            "Please use _qt_internal_sbom_compute_external_document_ref_spdx_id() instead."
            "To silence this deprecation, pass "
            "-DQT_NO_DEPRECATED_GET_EXTERNAL_DOCUMENT_REF_SPDX_ID=ON "
            "when configuring the project."
        )
    endif()

    _qt_internal_sbom_compute_external_document_ref_spdx_id("${repo_name}" external_document_ref)

    set(${out_var} "${external_document_ref}" PARENT_SCOPE)
endfunction()

# Query the external reference DocumentRef ID from the given SBOM target.
#
# If CREATE_TEMPORARY_REF_WHEN_MISSING is set, it instead of erroring out when the document ref
# isn't found, create one and assign it to the to the usual target property.
# This allows referring to older targets that were exported without the necessary properties.
function(_qt_internal_sbom_get_external_document_ref_spdx_id_from_sbom_target)
    if(NOT QT_GENERATE_SBOM)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    set(opt_args
        CREATE_TEMPORARY_REF_WHEN_MISSING
    )
    set(single_args
        TARGET
        OUT_VAR
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_TARGET)
        message(FATAL_ERROR "TARGET argument is required.")
    endif()

    if(NOT arg_OUT_VAR)
        message(FATAL_ERROR "OUT_VAR argument is required.")
    endif()

    set(target_unaliased "${arg_TARGET}")
    _qt_internal_dealias_target(target_unaliased)

    get_target_property(external_document_ref
        "${target_unaliased}" _qt_sbom_spdx_v2_external_document_ref)

    if(NOT external_document_ref)
        if(NOT arg_CREATE_TEMPORARY_REF_WHEN_MISSING
                AND NOT QT_SBOM_NO_AUTO_CREATE_DOCUMENT_REF_WHEN_MISSING)
            message(FATAL_ERROR
                "The target '${arg_TARGET}' does not have a recorded SPDX v2.3 external document "
                "DocumentRef value recorded.")
        else()
            string(MAKE_C_IDENTIFIER "${arg_TARGET}" target_c_identifier)
            string(REPLACE "_" "-" target_c_identifier "${target_c_identifier}")
            _qt_internal_sbom_compute_external_document_ref_spdx_id("${target_c_identifier}"
                external_document_ref
            )
            set_target_properties("${target_unaliased}" PROPERTIES
                _qt_sbom_spdx_v2_external_document_ref "${external_document_ref}")

            message(DEBUG "Did not find external document ref for target '${arg_TARGET}'. "
                "Created one and set it to '${external_document_ref}' in the target's properties.")
        endif()
    endif()

    set(${arg_OUT_VAR} "${external_document_ref}" PARENT_SCOPE)
endfunction()

# Computes a spdx id to reference the target's spdx v2 package via a SPDX v2 external document ref.
# To be used for exporting the value as part of the target's metadata.
function(_qt_internal_sbom_compute_external_spdx_v2_id target out_var)
    get_target_property(project_name_lowercase "${target}"
        _qt_sbom_spdx_repo_project_name_lowercase)

    _qt_internal_sbom_compute_external_document_ref_spdx_id(
        "${project_name_lowercase}" external_document_ref)

    _qt_internal_sbom_get_spdx_id_for_target("${target}" spdx_id)

    set(ext_spdx_id "${external_document_ref}:${spdx_id}")
    set(${out_var} "${ext_spdx_id}" PARENT_SCOPE)
endfunction()

# Sanitize a given value to be used as a SPDX id.
function(_qt_internal_sbom_get_sanitized_spdx_id out_var hint)
    # Only allow alphanumeric characters and dashes.
    string(REGEX REPLACE "[^a-zA-Z0-9]+" "-" spdx_id "${hint}")

    # Remove all trailing dashes.
    string(REGEX REPLACE "-+$" "" spdx_id "${spdx_id}")

    set(${out_var} "${spdx_id}" PARENT_SCOPE)
endfunction()

# Generates a spdx id for a target and saves it, the passed in SBOM_ENTITY_TYPE value, and a lot
# of other project specific properties like the spdx document namespace, cyclone dx document serial
# number, etc, in its target properties.
# A SBOM_ENTITY_TYPE value is required when saving the spdx id.

# Exits early and returns the spdx id if it was already created earlier.
function(_qt_internal_sbom_record_target_spdx_id target)
    set(opt_args
        IS_EXTERNAL_SBOM_ENTITY
    )
    set(single_args
        SPDX_ID
        PACKAGE_NAME
        SBOM_ENTITY_TYPE
        EXTERNAL_SBOM_DOCUMENT_TARGET
        OUT_VAR
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_sbom_get_spdx_id_for_target("${target}" spdx_id)

    if(spdx_id)
        # Return early if the target was already recorded and has a spdx id.
        if(arg_OUT_VAR)
            set(${arg_OUT_VAR} "${spdx_id}" PARENT_SCOPE)
        endif()
        return()
    endif()

    if(arg_PACKAGE_NAME)
        set(package_name_for_spdx_id "${arg_PACKAGE_NAME}")
    else()
        set(package_name_for_spdx_id "${target}")
    endif()

    # If an explicit SPDX_ID is set, use it rather than generating one.
    if(arg_SPDX_ID)
        set(package_spdx_id "${arg_SPDX_ID}")
    else()
        _qt_internal_sbom_generate_target_package_spdx_id(package_spdx_id
            SBOM_ENTITY_TYPE "${arg_SBOM_ENTITY_TYPE}"
            PACKAGE_NAME "${package_name_for_spdx_id}"
        )
    endif()

    set(save_spdx_id_args "")

    if(arg_IS_EXTERNAL_SBOM_ENTITY)
        list(APPEND save_spdx_id_args IS_EXTERNAL_SBOM_ENTITY)
    endif()

    if(arg_EXTERNAL_SBOM_DOCUMENT_TARGET)
        list(APPEND save_spdx_id_args
            EXTERNAL_SBOM_DOCUMENT_TARGET ${arg_EXTERNAL_SBOM_DOCUMENT_TARGET})
    endif()

    _qt_internal_sbom_save_spdx_id_for_target("${target}"
        SPDX_ID "${package_spdx_id}"
        PACKAGE_NAME "${package_name_for_spdx_id}"
        SBOM_ENTITY_TYPE "${arg_SBOM_ENTITY_TYPE}"
        ${save_spdx_id_args}
    )

    _qt_internal_sbom_is_qt_entity_type("${arg_SBOM_ENTITY_TYPE}" is_qt_entity_type)
    _qt_internal_sbom_save_spdx_id_for_qt_entity_type(
        "${target}" "${is_qt_entity_type}" "${package_spdx_id}")

    if(arg_OUT_VAR)
        set(${arg_OUT_VAR} "${package_spdx_id}" PARENT_SCOPE)
    endif()
endfunction()

# Generates a sanitized spdx id for a target (package) of a specific type.
function(_qt_internal_sbom_generate_target_package_spdx_id out_var)
    set(opt_args "")
    set(single_args
        PACKAGE_NAME
        SBOM_ENTITY_TYPE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_PACKAGE_NAME)
        message(FATAL_ERROR "PACKAGE_NAME must be set")
    endif()
    if(NOT arg_SBOM_ENTITY_TYPE)
        message(FATAL_ERROR "SBOM_ENTITY_TYPE must be set")
    endif()

    _qt_internal_sbom_get_root_project_name_for_spdx_id(repo_project_name_spdx_id)
    _qt_internal_sbom_get_package_infix("${arg_SBOM_ENTITY_TYPE}" package_infix)
    _qt_internal_sbom_get_spdx_id_unique_suffix(spdx_id_unique_suffix)

    string(CONCAT spdx_id_input
        "SPDXRef-${repo_project_name_spdx_id}-${package_infix}-${arg_PACKAGE_NAME}"
        "${spdx_id_unique_suffix}"
    )
    _qt_internal_sbom_get_sanitized_spdx_id(spdx_id "${spdx_id_input}")

    set(${out_var} "${spdx_id}" PARENT_SCOPE)
endfunction()

# Save a spdx id for a target inside its target properties.
# These are used when generating a SPDX external document reference for exported targets, to
# include them in relationships.
# Also saves the repo document namespace and relative installed repo document path.
# Also saves the sbom entity type and package name, because it's needed when creating CycloneDX
# components where the target is in an external document.
function(_qt_internal_sbom_save_spdx_id_for_target target)
    set(opt_args
        IS_EXTERNAL_SBOM_ENTITY
    )
    set(single_args
        SPDX_ID
        PACKAGE_NAME
        SBOM_ENTITY_TYPE
        EXTERNAL_SBOM_DOCUMENT_TARGET
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_SPDX_ID)
        message(FATAL_ERROR "arg_SPDX_ID must be set")
    endif()

    if(NOT arg_PACKAGE_NAME)
        message(FATAL_ERROR "PACKAGE_NAME must be set")
    endif()

    if(NOT arg_SBOM_ENTITY_TYPE)
        message(FATAL_ERROR "SBOM_ENTITY_TYPE must be set")
    endif()

    if(arg_IS_EXTERNAL_SBOM_ENTITY AND NOT arg_EXTERNAL_SBOM_DOCUMENT_TARGET)
        message(FATAL_ERROR
            "Target '${target}' is marked as external with the IS_EXTERNAL_SBOM_ENTITY option "
            "but no EXTERNAL_SBOM_DOCUMENT_TARGET was provided. Make sure to pass in a "
            "an external sbom document target created by "
            "_qt_internal_sbom_add_external_reference_document() to the mentioned option.")
    endif()

    if(arg_EXTERNAL_SBOM_DOCUMENT_TARGET AND NOT TARGET "${arg_EXTERNAL_SBOM_DOCUMENT_TARGET}")
        message(FATAL_ERROR
            "The target '${arg_EXTERNAL_SBOM_DOCUMENT_TARGET}' passed to "
            "EXTERNAL_SBOM_DOCUMENT_TARGET does not exist.")
    endif()

    if(arg_EXTERNAL_SBOM_DOCUMENT_TARGET)
        get_target_property(is_external_project_sbom_target "${arg_EXTERNAL_SBOM_DOCUMENT_TARGET}"
            _qt_sbom_is_external_project_sbom_target)
        if(NOT is_external_project_sbom_target)
            message(FATAL_ERROR
                "The target '${arg_EXTERNAL_SBOM_DOCUMENT_TARGET}' passed to "
                "EXTERNAL_SBOM_DOCUMENT_TARGET is not marked as an external project sbom target. "
                "Make sure to create the target with "
                "_qt_internal_sbom_add_external_reference_document().")
        endif()
    endif()

    set(spdx_id "${arg_SPDX_ID}")

    message(DEBUG "Saving spdx id for target ${target}: ${spdx_id}")

    set(target_unaliased "${target}")
    _qt_internal_dealias_target(target_unaliased)

    set_target_properties(${target_unaliased} PROPERTIES
        _qt_sbom_spdx_id "${spdx_id}")

    # Retrieve some of the repo / project specific properties.
    if(arg_IS_EXTERNAL_SBOM_ENTITY)
        get_target_property(repo_document_namespace
            "${arg_EXTERNAL_SBOM_DOCUMENT_TARGET}" _qt_sbom_spdx_repo_document_namespace)
    else()
        get_property(repo_document_namespace
            GLOBAL PROPERTY _qt_internal_sbom_repo_document_namespace)
    endif()

    if(arg_IS_EXTERNAL_SBOM_ENTITY)
        get_target_property(bom_serial_number_uuid
            "${arg_EXTERNAL_SBOM_DOCUMENT_TARGET}" _qt_sbom_cydx_bom_serial_number_uuid)
    else()
        get_property(bom_serial_number_uuid
            GLOBAL PROPERTY _qt_internal_sbom_repo_cyclone_dx_bom_serial_number_uuid)
    endif()

    if(arg_IS_EXTERNAL_SBOM_ENTITY)
        get_target_property(relative_installed_repo_document_path
            "${arg_EXTERNAL_SBOM_DOCUMENT_TARGET}"
            _qt_sbom_spdx_relative_installed_repo_document_path)
    else()
        get_property(relative_installed_repo_document_path
            GLOBAL PROPERTY _qt_internal_sbom_relative_installed_repo_document_path)
    endif()

    if(NOT arg_IS_EXTERNAL_SBOM_ENTITY)
        get_property(document_spdx_v2_tag_value_relative_path
            GLOBAL PROPERTY _qt_internal_sbom_document_spdx_v2_tag_value_relative_path)

        get_property(document_spdx_v2_json_relative_path
            GLOBAL PROPERTY _qt_internal_sbom_document_spdx_v2_json_relative_path)

        get_property(document_cydx_v1_6_json_relative_path
            GLOBAL PROPERTY _qt_internal_sbom_document_cydx_v1_6_json_relative_path)
    endif()

    if(arg_IS_EXTERNAL_SBOM_ENTITY)
        get_target_property(project_name_lowercase
            "${arg_EXTERNAL_SBOM_DOCUMENT_TARGET}" _qt_sbom_spdx_repo_project_name_lowercase)
    else()
        get_property(project_name_lowercase
            GLOBAL PROPERTY _qt_internal_sbom_repo_project_name_lowercase)
    endif()

    # And save them on the target.
    set_property(TARGET ${target_unaliased} PROPERTY
        _qt_sbom_spdx_repo_document_namespace
        "${repo_document_namespace}")

    set_property(TARGET ${target_unaliased} PROPERTY
        _qt_sbom_cydx_bom_serial_number_uuid
        "${bom_serial_number_uuid}")

    if(NOT arg_IS_EXTERNAL_SBOM_ENTITY)
        set_property(TARGET ${target_unaliased} PROPERTY
            _qt_sbom_spdx_v2_document_tag_value_relative_path
            "${document_spdx_v2_tag_value_relative_path}")

        set_property(TARGET ${target_unaliased} PROPERTY
            _qt_sbom_spdx_v2_document_json_relative_path
            "${document_spdx_v2_json_relative_path}")

        set_property(TARGET ${target_unaliased} PROPERTY
            _qt_sbom_cydx_v1_6_document_json_relative_path
            "${document_cydx_v1_6_json_relative_path}")
    endif()

    set_property(TARGET ${target_unaliased} PROPERTY
        _qt_sbom_spdx_relative_installed_repo_document_path
        "${relative_installed_repo_document_path}")

    set_property(TARGET ${target_unaliased} PROPERTY
        _qt_sbom_spdx_repo_project_name_lowercase
        "${project_name_lowercase}")

    set_property(TARGET ${target_unaliased} PROPERTY
        _qt_sbom_package_name
        "${arg_PACKAGE_NAME}")

    set_property(TARGET ${target_unaliased} PROPERTY
        _qt_sbom_entity_type
        "${arg_SBOM_ENTITY_TYPE}")

    if(arg_IS_EXTERNAL_SBOM_ENTITY)
        get_target_property(external_document_ref
            "${arg_EXTERNAL_SBOM_DOCUMENT_TARGET}" _qt_sbom_spdx_v2_external_document_ref)
    else()
        _qt_internal_sbom_compute_external_document_ref_spdx_id(
            "${project_name_lowercase}" external_document_ref)
    endif()

    set_property(TARGET "${target_unaliased}" PROPERTY
        _qt_sbom_spdx_v2_external_document_ref "${external_document_ref}")

    _qt_internal_sbom_compute_external_spdx_v2_id("${target_unaliased}" external_spdx_v2_id)

    set_property(TARGET "${target_unaliased}" PROPERTY
        _qt_sbom_spdx_v2_external_spdx_id "${external_spdx_v2_id}")

    _qt_internal_sbom_get_cydx_external_bom_link("${target_unaliased}" external_bom_link)
    set_property(TARGET "${target_unaliased}" PROPERTY
        _qt_sbom_cydx_external_bom_link "${external_bom_link}")

    _qt_internal_sbom_get_cydx_external_urn_bom_version("${target_unaliased}"
        external_urn_bom_version)
    set_property(TARGET "${target_unaliased}" PROPERTY
        _qt_sbom_cydx_external_urn_bom_version "${external_urn_bom_version}")

    if(NOT arg_IS_EXTERNAL_SBOM_ENTITY)
        # Export the properties, so they can be queried by other repos.
        # We also do it for versionless targets.
        set(export_properties
            _qt_sbom_entity_type
            _qt_sbom_package_name
            _qt_sbom_spdx_id
            _qt_sbom_spdx_repo_document_namespace
            _qt_sbom_spdx_v2_external_document_ref
            _qt_sbom_spdx_v2_external_spdx_id
            _qt_sbom_spdx_v2_document_tag_value_relative_path
            _qt_sbom_spdx_v2_document_json_relative_path
            _qt_sbom_cydx_bom_serial_number_uuid
            _qt_sbom_cydx_external_bom_link
            _qt_sbom_cydx_external_urn_bom_version
            _qt_sbom_cydx_v1_6_document_json_relative_path
            _qt_sbom_spdx_relative_installed_repo_document_path
            _qt_sbom_spdx_repo_project_name_lowercase
        )
        set_property(TARGET "${target_unaliased}" APPEND PROPERTY
            EXPORT_PROPERTIES "${export_properties}")
    endif()
endfunction()

# Returns whether the given sbom type is considered to be a Qt type like a module or a tool.
function(_qt_internal_sbom_is_qt_entity_type sbom_type out_var)
    set(qt_entity_types
        QT_MODULE
        QT_PLUGIN
        QT_APP
        QT_TOOL
        QT_TRANSLATIONS
        QT_RESOURCES
        QT_CUSTOM
        QT_CUSTOM_NO_INFIX
    )

    set(is_qt_entity_type FALSE)
    if(sbom_type IN_LIST qt_entity_types)
        set(is_qt_entity_type TRUE)
    endif()

    set(${out_var} ${is_qt_entity_type} PARENT_SCOPE)
endfunction()

# Returns whether the given sbom type is considered to a Qt 3rd party entity type.
function(_qt_internal_sbom_is_qt_3rd_party_entity_type sbom_type out_var)
    set(entity_types
        QT_THIRD_PARTY_MODULE
        QT_THIRD_PARTY_SOURCES
    )

    set(is_qt_third_party_entity_type FALSE)
    if(sbom_type IN_LIST entity_types)
        set(is_qt_third_party_entity_type TRUE)
    endif()

    set(${out_var} ${is_qt_third_party_entity_type} PARENT_SCOPE)
endfunction()

# Save a spdx id for all known related target names of a given Qt target.
# Related being the namespaced and versionless variants of a Qt target.
# All the related targets will contain the same spdx id.
# So Core, CorePrivate, Qt6::Core, Qt6::CorePrivate, Qt::Core, Qt::CorePrivate will all be
# referred to by the same spdx id.
function(_qt_internal_sbom_save_spdx_id_for_qt_entity_type target is_qt_entity_type package_spdx_id)
    # Assign the spdx id to all known related target names of given the given Qt target.
    set(target_names "")

    if(is_qt_entity_type)
        set(namespaced_target "${QT_CMAKE_EXPORT_NAMESPACE}::${target}")
        set(namespaced_private_target "${QT_CMAKE_EXPORT_NAMESPACE}::${target}Private")
        set(versionless_target "Qt::${target}")
        set(versionless_private_target "Qt::${target}Private")

        list(APPEND target_names
            namespaced_target
            namespaced_private_target
            versionless_target
            versionless_private_target
        )

        get_property(package_name TARGET "${target}" PROPERTY _qt_sbom_package_name)
        get_property(sbom_entity_type TARGET "${target}" PROPERTY _qt_sbom_entity_type)
    endif()

    foreach(target_name IN LISTS ${target_names})
        if(TARGET "${target_name}")
            _qt_internal_sbom_save_spdx_id_for_target("${target_name}"
                SPDX_ID "${package_spdx_id}"
                PACKAGE_NAME "${package_name}"
                SBOM_ENTITY_TYPE "${sbom_entity_type}"
            )
        endif()
    endforeach()
endfunction()

# Retrieves a saved spdx id from the target. Might be empty.
function(_qt_internal_sbom_get_spdx_id_for_target target out_var)
    get_target_property(spdx_id ${target} _qt_sbom_spdx_id)
    set(${out_var} "${spdx_id}" PARENT_SCOPE)
endfunction()

# Retrieves a saved spdx id for the current project. Might be empty.
function(_qt_internal_sbom_get_current_project_spdx_id out_var)
    get_cmake_property(spdx_id _qt_internal_sbom_project_spdx_id)
    set(${out_var} "${spdx_id}" PARENT_SCOPE)
endfunction()

# Returns a package infix for a given target sbom type to be used in spdx package id generation.
function(_qt_internal_sbom_get_package_infix type out_infix)
    if(type STREQUAL "QT_MODULE")
        set(package_infix "qt-module")
    elseif(type STREQUAL "QT_PLUGIN")
        set(package_infix "qt-plugin")
    elseif(type STREQUAL "QML_PLUGIN")
        set(package_infix "qt-qml-plugin") # not used at the moment
    elseif(type STREQUAL "QT_TOOL")
        set(package_infix "qt-tool")
    elseif(type STREQUAL "QT_APP")
        set(package_infix "qt-app")
    elseif(type STREQUAL "QT_THIRD_PARTY_MODULE")
        set(package_infix "qt-bundled-3rdparty-module")
    elseif(type STREQUAL "QT_THIRD_PARTY_SOURCES")
        set(package_infix "qt-3rdparty-sources")
    elseif(type STREQUAL "QT_TRANSLATIONS")
        set(package_infix "qt-translation")
    elseif(type STREQUAL "QT_RESOURCES")
        set(package_infix "qt-resource")
    elseif(type STREQUAL "QT_CUSTOM")
        set(package_infix "qt-custom")
    elseif(type STREQUAL "QT_CUSTOM_NO_INFIX")
        set(package_infix "qt")
    elseif(type STREQUAL "SYSTEM_LIBRARY")
        set(package_infix "system-3rdparty")
    elseif(type STREQUAL "EXECUTABLE")
        set(package_infix "executable")
    elseif(type STREQUAL "LIBRARY")
        set(package_infix "library")
    elseif(type STREQUAL "THIRD_PARTY_LIBRARY")
        set(package_infix "3rdparty-library")
    elseif(type STREQUAL "THIRD_PARTY_LIBRARY_WITH_FILES")
        set(package_infix "3rdparty-library-with-files")
    elseif(type STREQUAL "THIRD_PARTY_SOURCES")
        set(package_infix "3rdparty-sources")
    elseif(type STREQUAL "SBOM_PROJECT")
        set(package_infix "sbom-project")
    elseif(type STREQUAL "TRANSLATIONS")
        set(package_infix "translations")
    elseif(type STREQUAL "RESOURCES")
        set(package_infix "resource")
    elseif(type STREQUAL "BUILD_TOOL")
        set(package_infix "build-tool")
    elseif(type STREQUAL "CUSTOM")
        set(package_infix "custom")
    elseif(type STREQUAL "CUSTOM_NO_INFIX")
        set(package_infix "")
    else()
        message(DEBUG "No package infix due to unknown type: ${type}")
        set(package_infix "")
    endif()
    set(${out_infix} "${package_infix}" PARENT_SCOPE)
endfunction()

# Returns a package purpose for a given target sbom type.
function(_qt_internal_sbom_get_package_purpose type out_purpose)
    if(type STREQUAL "QT_MODULE")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "QT_PLUGIN")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "QML_PLUGIN")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "QT_TOOL")
        set(package_purpose "APPLICATION")
    elseif(type STREQUAL "QT_APP")
        set(package_purpose "APPLICATION")
    elseif(type STREQUAL "QT_THIRD_PARTY_MODULE")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "QT_THIRD_PARTY_SOURCES")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "QT_TRANSLATIONS")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "QT_RESOURCES")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "QT_CUSTOM")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "QT_CUSTOM_NO_INFIX")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "SYSTEM_LIBRARY")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "EXECUTABLE")
        set(package_purpose "APPLICATION")
    elseif(type STREQUAL "LIBRARY")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "THIRD_PARTY_LIBRARY")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "THIRD_PARTY_LIBRARY_WITH_FILES")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "THIRD_PARTY_SOURCES")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "SBOM_PROJECT")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "TRANSLATIONS")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "RESOURCES")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "BUILD_TOOL")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "CUSTOM")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "CUSTOM_NO_INFIX")
        set(package_purpose "OTHER")
    else()
        set(package_purpose "OTHER")
    endif()
    set(${out_purpose} "${package_purpose}" PARENT_SCOPE)
endfunction()

# Determines various version vars for the root SBOM package.
# Options:
#   QT_SBOM_GIT_VERSION - tries to query version info from the git repo that is in the current
#     working dir.
#   VERSION - overrides the git version info with the provided value.
#
# Stores the version vars in global properties. The following vars are set:
#  QT_SBOM_GIT_VERSION - the version extracted from git, or the explicit version specified via
#    VERSION. The git version is usually a tag or short sha1 + a branch + dirty flag.
#  QT_SBOM_GIT_VERSION_PATH - the same as above, but the value is sanitized to be path safe
#  QT_SBOM_GIT_HASH - the full git commit sha
#  QT_SBOM_GIT_HASH_SHORT - the short git commit sha
#  QT_SBOM_EXPLICIT_VERSION - the explicit version provided via VERSION or QT_SBOM_VERSION_OVERRIDE
#    if set.
#
# TODO: Consider renaming QT_SBOM_GIT_VERSION and friends to QT_SBOM_VERSION, because the version
# might not come from git, but from VERSION option, so the current naming is somewhat misleading.
function(_qt_internal_handle_sbom_project_version)
    set(opt_args
        USE_GIT_VERSION
    )
    set(single_args
        VERSION
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    # Query git version info if requested.
    # The git version might not be available for multiple reasons even if requsted, due to e.g. the
    # current working dir not being a git repo, or git not being installed, etc.
    # Some of the values might be overridden further down even.
    if(arg_USE_GIT_VERSION)
        _qt_internal_find_git_package()
        _qt_internal_query_git_version(
            EMPTY_VALUE_WHEN_NOT_GIT_REPO
            OUT_VAR_PREFIX __sbom_
        )
        set(QT_SBOM_GIT_VERSION "${__sbom_git_version}")
        set(QT_SBOM_GIT_VERSION_PATH "${__sbom_git_version_path}")
        set(QT_SBOM_GIT_HASH "${__sbom_git_hash}")
        set(QT_SBOM_GIT_HASH_SHORT "${__sbom_git_hash_short}")
    else()
        # To be clean, set the variables to an empty value rather than keeping them undefined.
        set(QT_SBOM_GIT_VERSION "")
        set(QT_SBOM_GIT_VERSION_PATH "")
        set(QT_SBOM_GIT_HASH "")
        set(QT_SBOM_GIT_HASH_SHORT "")
    endif()

    # If an explicit version was passed, override the git version and the path-safe git version
    # with the provided value.
    if(arg_VERSION)
        set(QT_SBOM_GIT_VERSION "${arg_VERSION}")
        set(QT_SBOM_GIT_VERSION_PATH "${arg_VERSION}")
    endif()

    # Store the explicit version, aka the non-git version, if provided, in a separate variable.
    # Allow overriding with the QT_SBOM_VERSION_OVERRIDE variable just in case, even if it's empty.
    # In case of a qt build, for compatibility we still read the QT_REPO_MODULE_VERSION variable
    if(DEFINED QT_SBOM_VERSION_OVERRIDE)
        set(explicit_version "${QT_SBOM_VERSION_OVERRIDE}")
    elseif(arg_VERSION)
        set(explicit_version "${arg_VERSION}")
    elseif(QT_REPO_MODULE_VERSION)
        set(explicit_version "${QT_REPO_MODULE_VERSION}")
    else()
        set(explicit_version "")
    endif()

    # If the git version hasn't been set yet, set it to the explicit version, if available.
    if(NOT QT_SBOM_GIT_VERSION)
        set(QT_SBOM_GIT_VERSION "${explicit_version}")
    endif()
    if(NOT QT_SBOM_GIT_VERSION_PATH)
        set(QT_SBOM_GIT_VERSION_PATH "${explicit_version}")
    endif()

    # Save the variables in a global property to later query them in other functions.
    set_property(GLOBAL PROPERTY QT_SBOM_GIT_VERSION "${QT_SBOM_GIT_VERSION}")
    set_property(GLOBAL PROPERTY QT_SBOM_GIT_VERSION_PATH "${QT_SBOM_GIT_VERSION_PATH}")
    set_property(GLOBAL PROPERTY QT_SBOM_GIT_HASH "${QT_SBOM_GIT_HASH}")
    set_property(GLOBAL PROPERTY QT_SBOM_GIT_HASH_SHORT "${QT_SBOM_GIT_HASH_SHORT}")
    set_property(GLOBAL PROPERTY QT_SBOM_EXPLICIT_VERSION "${explicit_version}")
endfunction()

# Queries the current project git version variables and sets them in the parent scope.
function(_qt_internal_sbom_get_git_version_vars)
    get_cmake_property(QT_SBOM_GIT_VERSION QT_SBOM_GIT_VERSION)
    get_cmake_property(QT_SBOM_GIT_VERSION_PATH QT_SBOM_GIT_VERSION_PATH)
    get_cmake_property(QT_SBOM_GIT_HASH QT_SBOM_GIT_HASH)
    get_cmake_property(QT_SBOM_GIT_HASH_SHORT QT_SBOM_GIT_HASH_SHORT)
    get_cmake_property(QT_SBOM_EXPLICIT_VERSION QT_SBOM_EXPLICIT_VERSION)

    set(QT_SBOM_GIT_VERSION "${QT_SBOM_GIT_VERSION}" PARENT_SCOPE)
    set(QT_SBOM_GIT_VERSION_PATH "${QT_SBOM_GIT_VERSION_PATH}" PARENT_SCOPE)
    set(QT_SBOM_GIT_HASH "${QT_SBOM_GIT_HASH}" PARENT_SCOPE)
    set(QT_SBOM_GIT_HASH_SHORT "${QT_SBOM_GIT_HASH_SHORT}" PARENT_SCOPE)
    set(QT_SBOM_EXPLICIT_VERSION "${QT_SBOM_EXPLICIT_VERSION}" PARENT_SCOPE)
endfunction()

# Queries the current project git version variables and sets them in the parent scope.
function(_qt_internal_sbom_get_project_explicit_version out_var)
    get_cmake_property(explicit_version QT_SBOM_EXPLICIT_VERSION)
    set(${out_var} "${explicit_version}" PARENT_SCOPE)
endfunction()

# Returns the configure line used to configure the current repo or top-level build, by reading
# the config.opt file that the configure script writes out.
# Returns an empty string if configure was not called, but CMake was called directly.
# If the build is reconfigured with bare CMake, the config.opt remains untouched, and thus
# the previous contents is returned.
function(_qt_internal_get_configure_line out_var)
    set(content "")

    if(QT_SUPERBUILD OR PROJECT_NAME STREQUAL "QtBase")
        set(configure_script_name "qt6/configure")
    elseif(PROJECT_NAME STREQUAL "QtBase")
        set(configure_script_name "qtbase/configure")
    else()
        _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)
        set(configure_script_name "qt-configure-module <sources>/${repo_project_name_lowercase}")
    endif()

    if(QT_SUPERBUILD)
        set(config_opt_path "${PROJECT_BINARY_DIR}/../config.opt")
    else()
        set(config_opt_path "${PROJECT_BINARY_DIR}/config.opt")
    endif()

    if(NOT EXISTS "${config_opt_path}")
        message(DEBUG "Couldn't find config.opt file in ${config_opt} for argument extraction.")
        set(${out_var} "${content}" PARENT_SCOPE)
        return()
    endif()

    file(STRINGS "${config_opt_path}" args)
    list(JOIN args " " joined_args)

    set(content "${configure_script_name} ${joined_args}")
    string(STRIP "${content}" content)

    set(${out_var} "${content}" PARENT_SCOPE)
endfunction()

function(_qt_internal_sbom_compute_project_file_name out_var)
    set(opt_args
        SPDX_TAG_VALUE
        SPDX_JSON
        CYCLONEDX_JSON
        CYCLONEDX_TOML

        EXTENSION_JSON # deprecated, used by WebEngine
    )
    set(single_args
        PROJECT_NAME
        VERSION_SUFFIX
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_PROJECT_NAME)
        message(FATAL_ERROR "PROJECT_NAME must be set")
    endif()

    if(NOT arg_SPDX_JSON
            AND NOT arg_SPDX_TAG_VALUE
            AND NOT arg_CYCLONEDX_TOML
            AND NOT arg_CYCLONEDX_JSON
            AND NOT arg_EXTENSION_JSON
        )
        message(FATAL_ERROR "One of the following options should be set: "
            "SPDX_TAG_VALUE, SPDX_JSON, CYCLONEDX_JSON, CYCLONEDX_TOML")
    endif()

    string(TOLOWER "${arg_PROJECT_NAME}" project_name_lowercase)

    set(version_suffix "")

    if(arg_VERSION_SUFFIX)
        set(version_suffix "-${arg_VERSION_SUFFIX}")
    elseif(QT_REPO_MODULE_VERSION)
        set(version_suffix "-${QT_REPO_MODULE_VERSION}")
    endif()

    if(arg_SPDX_TAG_VALUE)
        set(extension "spdx")
    elseif(arg_SPDX_JSON OR arg_EXTENSION_JSON)
        set(extension "spdx.json")
    elseif(arg_CYCLONEDX_TOML)
        set(extension "cdx.toml")
    elseif(arg_CYCLONEDX_JSON)
        set(extension "cdx.json")
    else()
        message(FATAL_ERROR "Unknown file extension for SBOM generation.")
    endif()

    set(result
        "${project_name_lowercase}${version_suffix}.${extension}")

    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()
