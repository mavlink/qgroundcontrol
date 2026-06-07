# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Gets the helper python script name and relative dir in the source dir.
function(_qt_internal_sbom_get_cyclone_dx_generator_script_name
        out_var_generator_name
        out_var_generator_relative_dir)
    set(generator_name "qt_cyclonedx_generator.py")

    _qt_internal_path_join(generator_relative_dir
        "util" "sbom" "cyclonedx" "qt_cyclonedx_generator")

    set(${out_var_generator_name} "${generator_name}" PARENT_SCOPE)
    set(${out_var_generator_relative_dir} "${generator_relative_dir}" PARENT_SCOPE)
endfunction()

# Ges the path to the helper python script, which should be used to generate CycloneDX document.
# Prefers the source path over the installed path, for easier development of the script.
function(_qt_internal_sbom_get_cyclone_dx_generator_path out_var)
    _qt_internal_sbom_get_cyclone_dx_generator_script_name(generator_name generator_relative_dir)

    _qt_internal_path_join(qtbase_script_path
        "${QT_SOURCE_TREE}" "${generator_relative_dir}" "${generator_name}")
    _qt_internal_path_join(installed_script_path
        "${QT6_INSTALL_PREFIX}" "${QT6_INSTALL_LIBEXECS}" "${generator_name}")

    # qtbase sources available, always use them, regardless if it's a prefix or non-prefix build.
    # Makes development easier.
    if(EXISTS "${qtbase_script_path}")
        set(script_path "${qtbase_script_path}")

    # qtbase sources unavailable, use installed files.
    elseif(EXISTS "${installed_script_path}")
        set(script_path "${installed_script_path}")
    else()
        message(FATAL_ERROR "Can't find ${generator_name} file.")
    endif()

    set(${out_var} "${script_path}" PARENT_SCOPE)
endfunction()

# Parses the options for a single CYDX_PROPERTY_ENTRY, and creates a toml snippet to add a
# CycloneDX property to the final toml document.
function(_qt_internal_sbom_parse_cydx_property_entry_options)
    set(opt_args "")
    set(single_args
        CYDX_PROPERTY_NAME
        CYDX_PROPERTY_VALUE
        OUT_VAR
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_CYDX_PROPERTY_NAME)
        message(FATAL_ERROR "CYDX_PROPERTY_NAME is required.")
    endif()

    if(NOT arg_CYDX_PROPERTY_VALUE)
        message(FATAL_ERROR "CYDX_PROPERTY_VALUE is required.")
    endif()

    if(NOT arg_OUT_VAR)
        message(FATAL_ERROR "OUT_VAR is required.")
    endif()

    set(${arg_OUT_VAR} "
[[components.properties]]
name = \\\"${arg_CYDX_PROPERTY_NAME}\\\"
value = \\\"${arg_CYDX_PROPERTY_VALUE}\\\"
" PARENT_SCOPE)
endfunction()

# Processes a list of CycloneDX property entries, and creates their toml representation as output.
function(_qt_internal_sbom_handle_cydx_properties)
    set(opt_args "")
    set(single_args
        OUT_VAR_CYDX_PROPERTIES_STRING
    )
    set(multi_args
        CYDX_PROPERTIES
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR_CYDX_PROPERTIES_STRING)
        message(FATAL_ERROR "OUT_VAR_CYDX_PROPERTIES_STRING is required.")
    endif()

    # Collect each CYDX_PROPERTY_ENTRY args into a separate variable.
    set(prop_idx -1)
    set(prop_entry_indices "")

    foreach(prop_arg IN LISTS arg_CYDX_PROPERTIES)
        if(prop_arg STREQUAL "CYDX_PROPERTY_ENTRY")
            math(EXPR prop_idx "${prop_idx}+1")
            list(APPEND prop_entry_indices "${prop_idx}")
        elseif(prop_idx GREATER_EQUAL 0)
            list(APPEND prop_${prop_idx}_args "${prop_arg}")
        else()
            message(FATAL_ERROR "Missing CYDX_PROPERTY_ENTRY keyword.")
        endif()
    endforeach()

    set(properties_string "")

    foreach(prop_idx IN LISTS prop_entry_indices)
        _qt_internal_sbom_parse_cydx_property_entry_options(
            ${prop_${prop_idx}_args}
            OUT_VAR property_tuple
        )

        string(APPEND properties_string "${property_tuple}")
    endforeach()

    set(${arg_OUT_VAR_CYDX_PROPERTIES_STRING} "${properties_string}" PARENT_SCOPE)
endfunction()

# Outputs extra Cyclone DX properties based on the sbom entity type.
function(_qt_internal_sbom_handle_qt_entity_cydx_properties)
    set(opt_args "")
    set(single_args
        SBOM_ENTITY_TYPE
        OUT_CYDX_PROPERTIES
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_SBOM_ENTITY_TYPE)
        message(FATAL_ERROR "SBOM_ENTITY_TYPE is required.")
    endif()

    if(NOT arg_OUT_CYDX_PROPERTIES)
        message(FATAL_ERROR "OUT_CYDX_PROPERTIES is required.")
    endif()

    set(cydx_properties "")
    list(APPEND cydx_properties
            CYDX_PROPERTY_ENTRY
                CYDX_PROPERTY_NAME "qt:sbom:entity_type"
                CYDX_PROPERTY_VALUE "${arg_SBOM_ENTITY_TYPE}"
    )

    _qt_internal_sbom_is_qt_entity_type("${arg_SBOM_ENTITY_TYPE}" is_qt_entity_type)
    if(is_qt_entity_type)
        list(APPEND cydx_properties
                CYDX_PROPERTY_ENTRY
                    CYDX_PROPERTY_NAME "qt:sbom:is_qt_entity_type"
                    CYDX_PROPERTY_VALUE "true"
        )
    endif()
    _qt_internal_sbom_is_qt_3rd_party_entity_type("${arg_SBOM_ENTITY_TYPE}"
        is_qt_3rd_party_entity_type)
    if(is_qt_3rd_party_entity_type)
        list(APPEND cydx_properties
                CYDX_PROPERTY_ENTRY
                    CYDX_PROPERTY_NAME "qt:sbom:is_qt_3rd_party_entity_type"
                    CYDX_PROPERTY_VALUE "true"
        )
    endif()

    set(${arg_OUT_CYDX_PROPERTIES} "${cydx_properties}" PARENT_SCOPE)
endfunction()

# Maps an sbom entity type to a cyclone dx component type.
function(_qt_internal_sbom_get_cyclone_component_type out_var sbom_entity_type)
    set(library_types
        "QT_MODULE"
        "QT_PLUGIN"
        "QML_PLUGIN"
        "QT_THIRD_PARTY_MODULE"
        "QT_THIRD_PARTY_SOURCES"
        "SYSTEM_LIBRARY"
        "LIBRARY"
        "THIRD_PARTY_LIBRARY"
        "THIRD_PARTY_LIBRARY_WITH_FILES"
        "THIRD_PARTY_SOURCES"
    )

    set(application_types
        "QT_TOOL"
        "QT_APP"
        "EXECUTABLE"
    )

    if(sbom_entity_type IN_LIST library_types)
        set(component_type "library")
    elseif(sbom_entity_type IN_LIST application_types)
        set(component_type "application")
    else()
        # Default to library for now, because it's unclear what would be a better default.
        set(component_type "library")
    endif()

    set(${out_var} "${component_type}" PARENT_SCOPE)
endfunction()

# Generates a pseudo-unique serial number for a CycloneDX sbom document.
#
# The spec says that a BOM serial number must conform to RFC 4122, but doesn't specify which
# kind of uuid version should be generated.
# The upstream python library generates a version 4 uuid, which is fully random.
# CMake can only generate version 3 and 5 uuids, which are fully deterministic based on the given
# NAMESPACE and NAME values.
# Generating a fully random uuid prevents build reproducibility. The maintainer of the Cyclone DX
# spec even mentions that here:
# https://github.com/CycloneDX/specification/issues/97#issuecomment-955904904
# And yet to to do component-wise inter-document linking using the BOM-Link mechanism, you have to
# use serial numbers.
#
# Because the spec doesn't explicitly prohibit it, we will generate a version 5 uuid based on the
# SPDX_NAMESPACE passed to the function, which is supposed to be unique enough, because it contains
# the project / document name (e.g. qtbase) and its version or git version.
# This should alleviate the reproducibility problem as well.
function(_qt_internal_sbom_get_cyclone_bom_serial_number)
    set(opt_args "")
    set(single_args
        SPDX_NAMESPACE
        OUT_VAR_UUID
        OUT_VAR_SERIAL_NUMBER
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_sbom_set_default_option_value_and_error_if_empty(SPDX_NAMESPACE "")

    _qt_internal_sbom_get_document_namespace_uuid_namespace(uuid_namespace)

    string(UUID uuid NAMESPACE "${uuid_namespace}" NAME "${arg_SPDX_NAMESPACE}" TYPE SHA1)
    set(cyclone_dx_serial_number "urn:cdx:${uuid}")

    if(arg_OUT_VAR_UUID)
        set("${arg_OUT_VAR_UUID}" "${uuid}" PARENT_SCOPE)
    endif()
    if(arg_OUT_VAR_SERIAL_NUMBER)
        set("${arg_OUT_VAR_SERIAL_NUMBER}" "${cyclone_dx_serial_number}" PARENT_SCOPE)
    endif()
endfunction()

# Returns the bom version field that is part of a bom-link to refer to the target in an
# external CycloneDX document.
function(_qt_internal_sbom_get_cydx_external_urn_bom_version target out_var)
    # Currently 1, but we might change it to be configurable in the future.
    set(bom_version "1")
    set(${out_var} "${bom_version}" PARENT_SCOPE)
endfunction()

# See https://github.com/CycloneDX/guides/blob/main/SBOM/en/0x52-Linking.md
function(_qt_internal_sbom_get_cydx_external_bom_link target out_var)
    get_target_property(spdx_id "${target}" _qt_sbom_spdx_id)
    get_target_property(bom_serial_number "${target}" _qt_sbom_cydx_bom_serial_number_uuid)

    _qt_internal_sbom_get_cydx_external_urn_bom_version("${target}" bom_version)
    set(bom_link "urn:cdx:${bom_serial_number}/${bom_version}#${spdx_id}")

    set(${out_var} "${bom_link}" PARENT_SCOPE)
endfunction()

# Records necessary details of external target dependencies in global properties, to later create
# the CycloneDX packages for them. The info collection needs to be done immediately in the directory
# scope where the targets were found, because they might not be global, and thus can't be accessed
# later.
function(_qt_internal_sbom_record_external_target_dependecies)
    set(opt_args "")
    set(single_args "")
    set(multi_args
        TARGETS
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_TARGETS)
        return()
    endif()

    get_property(existing_ids GLOBAL PROPERTY _qt_internal_sbom_external_target_dep_ids)
    if(NOT existing_ids)
        set(existing_ids "")
    endif()

    foreach(target IN LISTS arg_TARGETS)
        # Use the full spdx id (one prefixed with the containing DocumentRef-) because that's what
        # our spdx dependency relationships use at the moment.
        # Both Foo and FooPrivate map to the same spdx_id, so we need to avoid duplicates on spdx id
        # level.
        get_target_property(spdx_id "${target}" _qt_sbom_spdx_id)

        if(spdx_id IN_LIST existing_ids)
            continue()
        endif()

        list(APPEND existing_ids "${spdx_id}")
        set_property(GLOBAL APPEND PROPERTY _qt_internal_sbom_external_target_dep_ids "${spdx_id}")

        # This is checked in _qt_internal_sbom_add_target, to prevent duplicate creation of
        # system library targets.
        set_property(GLOBAL APPEND PROPERTY _qt_internal_sbom_external_target_dependencies
            "${target}")

        get_target_property(package_name "${target}" _qt_sbom_package_name)
        get_target_property(sbom_entity_type "${target}" _qt_sbom_entity_type)
        get_target_property(package_version "${target}" _qt_sbom_package_version)
        _qt_internal_sbom_get_cydx_external_bom_link("${target}" external_bom_link)

        if(NOT package_name)
            message(FATAL_ERROR "Target '${target}' does not provide a SBOM package name")
        endif()

        set_property(GLOBAL
            PROPERTY "_qt_internal_sbom_external_target_dep_${spdx_id}_target"
            "${target}")
        set_property(GLOBAL
            PROPERTY "_qt_internal_sbom_external_target_dep_${spdx_id}_package_name"
            "${package_name}")
        set_property(GLOBAL
            PROPERTY "_qt_internal_sbom_external_target_dep_${spdx_id}_sbom_entity_type"
            "${sbom_entity_type}")
        set_property(GLOBAL
            PROPERTY "_qt_internal_sbom_external_target_dep_${spdx_id}_package_version"
            "${package_version}")
        set_property(GLOBAL
            PROPERTY "_qt_internal_sbom_external_target_dep_${spdx_id}_external_bom_link"
            "${external_bom_link}")
    endforeach()
endfunction()

# Goes through the list of recorded external target dependencies collected during target
# dependency analysis, and adds them as CycloneDX packages to the CycloneDX document.
# This is different from SPDX v2.3, which doesn't require creating a package for dependencies that
# are defined in a different document.
function(_qt_internal_sbom_add_cydx_external_target_dependencies)
    get_property(spdx_ids GLOBAL PROPERTY _qt_internal_sbom_external_target_dep_ids)
    if(NOT spdx_ids)
        # Clean up external target dependencies, before configuring next repo project.
        set_property(GLOBAL PROPERTY _qt_internal_sbom_external_target_dep_ids "")
        set_property(GLOBAL PROPERTY _qt_internal_sbom_external_target_dependencies "")
        return()
    endif()

    # Just in case, don't add duplicates.
    set(visited_spdx_ids "")

    foreach(spdx_id IN LISTS spdx_ids)
        if(spdx_id IN_LIST visited_spdx_ids)
            continue()
        endif()

        get_cmake_property(package_name
            "_qt_internal_sbom_external_target_dep_${spdx_id}_package_name")
        get_cmake_property(sbom_entity_type
            "_qt_internal_sbom_external_target_dep_${spdx_id}_sbom_entity_type")
        get_cmake_property(package_version
            "_qt_internal_sbom_external_target_dep_${spdx_id}_package_version")
        get_cmake_property(external_bom_link
            "_qt_internal_sbom_external_target_dep_${spdx_id}_external_bom_link")

        _qt_internal_sbom_generate_cyclone_add_package(
            PACKAGE "${package_name}"
            SPDXID "${spdx_id}"
            SBOM_ENTITY_TYPE "${sbom_entity_type}"
            VERSION "${package_version}"
            EXTERNAL_BOM_LINK "${external_bom_link}"
        )

        list(APPEND visited_spdx_ids "${spdx_id}")
    endforeach()

    # Clean up external target dependencies, before configuring next repo project.
    set_property(GLOBAL PROPERTY _qt_internal_sbom_external_target_dep_ids "")
    set_property(GLOBAL PROPERTY _qt_internal_sbom_external_target_dependencies "")
endfunction()

# Records a license id and its text in global properties, to be added to the CycloneDX document
# later.
function(_qt_internal_sbom_record_license_cydx)
    set(opt_args "")
    set(single_args
        LICENSE_ID
        EXTRACTED_TEXT
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set_property(GLOBAL APPEND PROPERTY
        _qt_internal_sbom_cydx_licenses "${arg_LICENSE_ID}")
    set_property(GLOBAL PROPERTY
        _qt_internal_sbom_cydx_licenses_${arg_LICENSE_ID}_text "${arg_EXTRACTED_TEXT}"
    )
endfunction()
