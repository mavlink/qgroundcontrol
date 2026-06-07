# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Records information about a system library target, usually due to a qt_find_package call.
# This information is later used to generate packages for the system libraries, but only after
# confirming that the library was used (linked) into any of the Qt targets.
function(_qt_internal_sbom_record_system_library_usage target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args
        TYPE # deprecated
        SBOM_ENTITY_TYPE
        PACKAGE_VERSION
        FRIENDLY_PACKAGE_NAME
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_map_sbom_entity_type(sbom_entity_type ${ARGN})

    if(NOT sbom_entity_type)
        message(FATAL_ERROR "SBOM_ENTITY_TYPE is empty for target '${target}', but it must be set")
    endif()

    # A package might be looked up more than once, make sure to record it once.
    get_property(already_recorded GLOBAL PROPERTY
        _qt_internal_sbom_recorded_system_library_target_${target})

    if(already_recorded)
        return()
    endif()

    set_property(GLOBAL PROPERTY
        _qt_internal_sbom_recorded_system_library_target_${target} TRUE)

    # Defer spdx id creation until _qt_internal_sbom_begin_project is called, so we know the
    # project name. The project name is used in the package infix generation of the system library,
    # but _qt_internal_sbom_record_system_library_usage might be called before sbom generation
    # has started, e.g. during _qt_internal_find_third_party_dependencies.
    set(spdx_options
        ${target}
        SBOM_ENTITY_TYPE "${sbom_entity_type}"
        PACKAGE_NAME "${arg_FRIENDLY_PACKAGE_NAME}"
    )

    get_cmake_property(sbom_repo_begin_called _qt_internal_sbom_repo_begin_called)
    if(sbom_repo_begin_called AND TARGET "${target}")
        _qt_internal_sbom_record_system_library_spdx_id(${target} ${spdx_options})
    else()
        set_property(GLOBAL PROPERTY
            _qt_internal_sbom_recorded_system_library_spdx_options_${target} "${spdx_options}")
    endif()

    # Defer sbom info creation until we detect usage of the system library (whether the library is
    # linked into any other target).
    set_property(GLOBAL APPEND PROPERTY
        _qt_internal_sbom_recorded_system_library_targets "${target}")
    set_property(GLOBAL PROPERTY
        _qt_internal_sbom_recorded_system_library_options_${target} "${ARGN}")
endfunction()

# Helper to record spdx ids of all system library targets that were found so far.
function(_qt_internal_sbom_record_system_library_spdx_ids)
    get_property(recorded_targets GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_targets)

    if(NOT recorded_targets)
        return()
    endif()

    foreach(target IN LISTS recorded_targets)
        get_property(args GLOBAL PROPERTY
            _qt_internal_sbom_recorded_system_library_spdx_options_${target})

        # qt_find_package PROVIDED_TARGETS might refer to non-existent targets in certain cases,
        # like zstd::libzstd_shared for qt_find_package(WrapZSTD), because we are not sure what
        # kind of zstd build was done. Make sure to check if the target exists before recording it.
        if(TARGET "${target}")
            set(target_unaliased "${target}")
            _qt_internal_dealias_target(target_unaliased)

            _qt_internal_sbom_record_system_library_spdx_id(${target_unaliased} ${args})
        else()
            message(DEBUG
                "Skipping recording system library for SBOM because target does not exist: "
                " ${target}")
        endif()
    endforeach()
endfunction()

# Helper to record the spdx id of a system library target.
function(_qt_internal_sbom_record_system_library_spdx_id target)
    # Save the spdx id before the sbom info is added, so we can refer to it in relationships.
    _qt_internal_sbom_record_target_spdx_id(${ARGN} OUT_VAR package_spdx_id)

    if(NOT package_spdx_id)
        message(FATAL_ERROR "Could not generate spdx id for system library target: ${target}")
    endif()

    set_target_properties("${target}" PROPERTIES _qt_internal_sbom_is_system_library TRUE)

    set_property(GLOBAL PROPERTY
        _qt_internal_sbom_recorded_system_library_package_${target} "${package_spdx_id}")
endfunction()

# Goes through the list of consumed system libraries (those that were linked in) and creates
# sbom packages for them.
# Uses information from recorded system libraries (calls to qt_find_package).
function(_qt_internal_sbom_add_recorded_system_libraries)
    get_property(recorded_targets GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_targets)
    get_property(consumed_targets GLOBAL PROPERTY _qt_internal_sbom_consumed_system_library_targets)

    set(unconsumed_targets "${recorded_targets}")
    set(generated_package_names "")

    message(DEBUG
        "System libraries that were marked consumed "
        "(some target linked to them): ${consumed_targets}")
    message(DEBUG
        "System libraries that were recorded "
        "(they were marked with qt_find_package()): ${recorded_targets}")

    foreach(target IN LISTS consumed_targets)
        # Some system targets like qtspeech SpeechDispatcher::SpeechDispatcher might be aliased,
        # and we can't set properties on them, so unalias the target name.
        set(target_original "${target}")
        _qt_internal_dealias_target(target)

        get_property(args GLOBAL PROPERTY
            _qt_internal_sbom_recorded_system_library_options_${target})
        get_property(package_name GLOBAL PROPERTY
            _qt_internal_sbom_recorded_system_library_package_${target})

        set_property(GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_target_${target} "")
        set_property(GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_options_${target} "")
        set_property(GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_package_${target} "")

        # Guard against generating a package multiple times. Can happen when multiple targets belong
        # to the same package.
        if(sbom_generated_${package_name})
            continue()
        endif()

        # Automatic system library sbom recording happens at project root source dir scope, which
        # means it might accidentally pick up a qt_attribution.json file from the project root,
        # that is not intended to be use for system libraries.
        # For now, explicitly disable using the root attribution file.
        list(APPEND args NO_CURRENT_DIR_ATTRIBUTION)

        list(APPEND generated_package_names "${package_name}")
        set(sbom_generated_${package_name} TRUE)

        _qt_internal_extend_sbom(${target} ${args})
        _qt_internal_finalize_sbom(${target})

        list(REMOVE_ITEM unconsumed_targets "${target_original}")
    endforeach()

    message(DEBUG "System libraries that were recorded, but not consumed: ${unconsumed_targets}")
    message(DEBUG "Generated SBOMs for the following system packages: ${generated_package_names}")

    # Clean up, before configuring next repo project.
    set_property(GLOBAL PROPERTY _qt_internal_sbom_consumed_system_library_targets "")
    set_property(GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_targets "")
endfunction()
