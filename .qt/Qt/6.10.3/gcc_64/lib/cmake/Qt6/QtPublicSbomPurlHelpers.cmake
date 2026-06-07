# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Parse purl arguments for a specific purl entry.
# arguments_var_name is the variable name that contains the args.
# prefix is the prefix passed to cmake_parse_arguments.
macro(_qt_internal_sbom_parse_purl_entry_options prefix arguments_var_name)
    _qt_internal_get_sbom_purl_parsing_options(purl_opt_args purl_single_args purl_multi_args)

    cmake_parse_arguments(${prefix} "${purl_opt_args}" "${purl_single_args}" "${purl_multi_args}"
        ${${arguments_var_name}})
    _qt_internal_validate_all_args_are_parsed(${prefix})
endmacro()

# Helper macro to prepare forwarding all set purl options to some other function.
# Expects the options names to be set in the parent scope by calling
# _qt_internal_get_sbom_add_target_options(opt_args single_args multi_args)
macro(_qt_internal_sbom_forward_purl_handling_options args_var_name)
    if(NOT opt_args)
        message(FATAL_ERROR
            "Expected opt_args to be set by _qt_internal_get_sbom_purl_handling_options")
    endif()
    if(NOT single_args)
        message(FATAL_ERROR
            "Expected single_args to be set by _qt_internal_get_sbom_purl_handling_options")
    endif()
    if(NOT multi_args)
        message(FATAL_ERROR
            "Expected multi_args to be set by _qt_internal_get_sbom_purl_handling_options")
    endif()
    _qt_internal_forward_function_args(
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR ${args_var_name}
        FORWARD_OPTIONS
            ${opt_args}
        FORWARD_SINGLE
            ${single_args}
        FORWARD_MULTI
            ${multi_args}
    )
endmacro()

# Handles purl arguments specified to functions like qt_internal_add_sbom.
#
# Synopsis
#
# qt_internal_add_sbom(<target>
#     PURLS
#         [[PURL_ENTRY
#             PURL_ID <id>
#             PURL_TYPE <type>
#             PURL_NAMESPACE <namespace>
#             PURL_NAME <name>
#             PURL_VERSION <version>]...]
#     PURL_VALUES
#         [purl-string...]
# )
#
# Example
#
# qt_internal_add_sbom(<target>
#     PURLS
#         PURL_ENTRY
#             PURL_ID "UPSTREAM"
#             PURL_TYPE "github"
#             PURL_NAMESPACE "harfbuzz"
#             PURL_NAME "harfbuzz"
#             PURL_VERSION "v8.5.0"
#         PURL_ENTRY
#             PURL_ID "MIRROR"
#             PURL_TYPE "git"
#             PURL_NAMESPACE "harfbuzz"
#             PURL_NAME "harfbuzz"
#             PURL_QUALIFIERS "vcs_url=https://code.qt.io/qt/qtbase"
#         ....
#     PURL_VALUES
#         pkg:git/harfbuzz/harfbuzz@v8.5.0
#         pkg:github/harfbuzz/harfbuzz@v8.5.0
#         ....
#
#
# PURLS accepts multiple purl entries, each starting with the PURL_ENTRY keyword.
# PURL_VALUES takes a list of pre-built purl strings.
#
# If no arguments are specified, for qt entity types (e.g. libraries built as part of Qt repos),
# default purls will be generated.
#
# There is no limit to the number of purls that can be added to a target.
# The created purls are saved in:
# - OUT_VAR_PURL_VALUES as plain purl values, to be used for CycloneDX genereation.
# - OUT_VAR_SPDX_EXT_REF_VALUES as SPDX ExtRef entries, to be used for SPDX v2.3 generation.
function(_qt_internal_sbom_handle_purl_values target)
    _qt_internal_get_sbom_purl_handling_options(opt_args single_args multi_args)
    list(APPEND single_args
        OUT_VAR_SPDX_EXT_REF_VALUES
        OUT_VAR_PURL_VALUES
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR_PURL_VALUES)
        message(FATAL_ERROR "OUT_VAR_PURL_VALUES must be set")
    endif()

    if(NOT arg_OUT_VAR_SPDX_EXT_REF_VALUES)
        message(FATAL_ERROR "OUT_VAR_SPDX_EXT_REF_VALUES must be set")
    endif()

    _qt_internal_get_sbom_purl_parsing_options(purl_opt_args purl_single_args purl_multi_args)

    set(purl_values "")
    set(spdx_ext_ref_values "")

    # Collect each PURL_ENTRY args into a separate variable.
    set(purl_idx -1)
    set(purl_entry_indices "")
    foreach(purl_arg IN LISTS arg_PURLS)
        if(purl_arg STREQUAL "PURL_ENTRY")
            math(EXPR purl_idx "${purl_idx}+1")
            list(APPEND purl_entry_indices "${purl_idx}")
        elseif(purl_idx GREATER_EQUAL 0)
            list(APPEND purl_${purl_idx}_args "${purl_arg}")
        else()
            message(FATAL_ERROR "Missing PURL_ENTRY keyword.")
        endif()
    endforeach()

    # Validate the args for each collected entry.
    foreach(purl_idx IN LISTS purl_entry_indices)
        list(LENGTH purl_${purl_idx}_args num_args)
        if(num_args LESS 1)
            message(FATAL_ERROR "Empty PURL_ENTRY encountered.")
        endif()
        _qt_internal_sbom_parse_purl_entry_options(arg purl_${purl_idx}_args)
    endforeach()

    # Append qt specific placeholder entries when handling Qt entities.
    if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_PURL)
        _qt_internal_sbom_forward_purl_handling_options(purl_handling_args)
        _qt_internal_sbom_handle_qt_entity_purl_entries(${purl_handling_args}
            OUT_VAR_IDS qt_purl_ids
        )
        if(qt_purl_ids)
            # Create purl placeholder indices for each qt purl id.
            foreach(qt_purl_id IN LISTS qt_purl_ids)
                math(EXPR purl_idx "${purl_idx}+1")
                list(APPEND purl_entry_indices "${purl_idx}")
                list(APPEND purl_${purl_idx}_args PURL_ID "${qt_purl_id}")
            endforeach()
        endif()
    endif()

    set(qt_entity_cydx_purl_values "")
    set(qt_entity_spdx_purl_ext_refs "")

    # When generating purls for Qt entities targeting Cyclone DX, we prefer the generic purl first,
    # otherwise DependencyTrack gets confused with lots of components having the same purls,
    # because it doesn't take into account the '#' part of the purl.
    # Keep these separate and append them in the right order later.
    set(qt_entity_cydx_purl_for_github_id "")
    set(qt_entity_cydx_purl_for_generic_id "")

    foreach(purl_idx IN LISTS purl_entry_indices)
        # Clear previous values.
        foreach(option_name IN LISTS purl_opt_args purl_single_args purl_multi_args)
            unset(arg_${option_name})
        endforeach()

        _qt_internal_sbom_parse_purl_entry_options(arg purl_${purl_idx}_args)

        set(purl_args "")

        # Override the purl version with the package version.
        if(arg_PURL_USE_PACKAGE_VERSION AND arg_PACKAGE_VERSION)
            set(arg_PURL_VERSION "${arg_PACKAGE_VERSION}")
        endif()

        # Append a vcs_url to the qualifiers if specified.
        if(arg_PURL_VCS_URL)
            list(APPEND arg_PURL_QUALIFIERS "vcs_url=${arg_PURL_VCS_URL}")
        endif()

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

        set(is_qt_entity_purl FALSE)
        # Qt entity types get special treatment to gather the required args.
        if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_PURL
                AND arg_PURL_ID
                AND arg_PURL_ID IN_LIST qt_purl_ids)

            set(is_qt_entity_purl TRUE)
            _qt_internal_sbom_handle_qt_entity_purl("${target}"
                ${purl_handling_args}
                PURL_ID "${arg_PURL_ID}"
                OUT_PURL_ARGS qt_purl_args
            )
            if(qt_purl_args)
                list(APPEND purl_args "${qt_purl_args}")
            endif()
        endif()

        _qt_internal_sbom_assemble_purl(${target}
            ${purl_args}
            OUT_VAR purl_bare
            OUT_VAR_SPDX_EXT_REF package_manager_external_ref_purl
        )

        if(is_qt_entity_purl)
            if(arg_PURL_ID STREQUAL "GENERIC")
                set(qt_entity_cydx_purl_for_generic_id "${purl_bare}")
            elseif(arg_PURL_ID STREQUAL "GITHUB")
                set(qt_entity_cydx_purl_for_github_id "${purl_bare}")
            else()
                list(APPEND purl_values "${purl_bare}")
            endif()
        else()
            list(APPEND purl_values "${purl_bare}")
        endif()

        list(APPEND spdx_ext_ref_values ${package_manager_external_ref_purl})
    endforeach()

    # Add the custom qt entity purls at the front in the right order for CycloneDX.
    # If they are empty (for non-Qt entities), nothing will be prepended.
    set(qt_entity_cydx_purl_values
        ${qt_entity_cydx_purl_for_generic_id}
        ${qt_entity_cydx_purl_for_github_id}
    )
    list(PREPEND purl_values ${qt_entity_cydx_purl_values})

    foreach(purl_value IN LISTS arg_PURL_VALUES)
        _qt_internal_sbom_get_purl_value_extref(VALUE "${purl_value}"
            OUT_VAR package_manager_external_ref_purl)

        # The order in which the purls are generated, matters for tools that consume the SBOM.
        # Some tools can only handle one PURL per package, so the first one should be the
        # important one.
        # For now, I deem that the directly specified ones (probably via a qt_attribution.json
        # file) are the more important ones. So we prepend them.
        list(PREPEND purl_values ${purl_value})
        list(PREPEND spdx_ext_ref_values ${package_manager_external_ref_purl})
    endforeach()

    set(${arg_OUT_VAR_PURL_VALUES} "${purl_values}" PARENT_SCOPE)
    set(${arg_OUT_VAR_SPDX_EXT_REF_VALUES} "${spdx_ext_ref_values}" PARENT_SCOPE)
endfunction()

# Assembles an external reference purl identifier.
#
# PURL_TYPE and PURL_NAME are required.
#
# Stores the bare purl in the OUT_VAR.
# Stores the SPDX External Reference purl in the OUT_VAR_SPDX_EXT_REF.
#
# Accepted options:
#    PURL_TYPE
#    PURL_NAME
#    PURL_NAMESPACE
#    PURL_VERSION
#    PURL_SUBPATH
#    PURL_QUALIFIERS
function(_qt_internal_sbom_assemble_purl target)
    set(opt_args "")
    set(single_args
        OUT_VAR
        OUT_VAR_SPDX_EXT_REF
    )
    set(multi_args "")

    _qt_internal_get_sbom_purl_parsing_options(purl_opt_args purl_single_args purl_multi_args)
    list(APPEND opt_args ${purl_opt_args})
    list(APPEND single_args ${purl_single_args})
    list(APPEND multi_args ${purl_multi_args})

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(purl_scheme "pkg")

    if(NOT arg_PURL_TYPE)
        message(FATAL_ERROR "PURL_TYPE must be set")
    endif()

    if(NOT arg_PURL_NAME)
        message(FATAL_ERROR "PURL_NAME must be set")
    endif()

    if(NOT arg_OUT_VAR)
        message(FATAL_ERROR "OUT_VAR must be set")
    endif()

    # https://github.com/package-url/purl-spec
    # Spec is 'scheme:type/namespace/name@version?qualifiers#subpath'
    set(purl "${purl_scheme}:${arg_PURL_TYPE}")

    if(arg_PURL_NAMESPACE)
        string(APPEND purl "/${arg_PURL_NAMESPACE}")
    endif()

    string(APPEND purl "/${arg_PURL_NAME}")

    if(arg_PURL_VERSION)
        string(APPEND purl "@${arg_PURL_VERSION}")
    endif()

    if(arg_PURL_QUALIFIERS)
        # TODO: Note that the qualifiers are expected to be URL encoded, which this implementation
        # is not doing at the moment.
        list(JOIN arg_PURL_QUALIFIERS "&" qualifiers)
        string(APPEND purl "?${qualifiers}")
    endif()

    if(arg_PURL_SUBPATH)
        string(APPEND purl "#${arg_PURL_SUBPATH}")
    endif()

    _qt_internal_sbom_get_purl_value_extref(VALUE "${purl}" OUT_VAR ext_ref_result)

    set(${arg_OUT_VAR} "${purl}" PARENT_SCOPE)
    set(${arg_OUT_VAR_SPDX_EXT_REF} "${ext_ref_result}" PARENT_SCOPE)
endfunction()

# Takes a PURL VALUE and returns an SBOM purl external reference in OUT_VAR.
function(_qt_internal_sbom_get_purl_value_extref)
    set(opt_args "")
    set(single_args
        OUT_VAR
        VALUE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR)
        message(FATAL_ERROR "OUT_VAR must be set")
    endif()

    if(NOT arg_VALUE)
        message(FATAL_ERROR "VALUE must be set")
    endif()

    # SPDX SBOM External reference type.
    set(ext_ref_prefix "PACKAGE-MANAGER purl")
    set(external_ref "${ext_ref_prefix} ${arg_VALUE}")
    set(result "EXTREF" "${external_ref}")
    set(${arg_OUT_VAR} "${result}" PARENT_SCOPE)
endfunction()
