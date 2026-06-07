# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Walks a target's direct dependencies and assembles a list of relationships between the packages
# of the target dependencies.
# Currently handles various Qt targets and system libraries.
# Collects the relationships in OUT_SBOM_RELATIONSHIP_ENTRIES.
# If a CYDX dependency is in an external docuemnt, the dependency is added to
# OUT_EXTERNAL_TARGET_DEPENDENCIES as well.
function(_qt_internal_sbom_handle_target_dependencies target)
    set(opt_args "")
    set(single_args
        SPDX_ID
        OUT_SBOM_RELATIONSHIP_ENTRIES
        OUT_EXTERNAL_TARGET_DEPENDENCIES
    )
    set(multi_args
        LIBRARIES
        PUBLIC_LIBRARIES
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_SPDX_ID)
        message(FATAL_ERROR "SPDX_ID must be set")
    endif()
    set(package_spdx_id "${arg_SPDX_ID}")


    set(libraries "")
    if(arg_LIBRARIES)
        list(APPEND libraries "${arg_LIBRARIES}")
    endif()

    get_target_property(extend_libraries "${target}" _qt_extend_target_libraries)
    if(extend_libraries)
        list(APPEND libraries ${extend_libraries})
    endif()

    get_target_property(target_type ${target} TYPE)
    set(valid_target_types
        EXECUTABLE
        SHARED_LIBRARY
        MODULE_LIBRARY
        STATIC_LIBRARY
        OBJECT_LIBRARY
    )
    if(target_type IN_LIST valid_target_types)
        get_target_property(link_libraries "${target}" LINK_LIBRARIES)
        if(link_libraries)
            list(APPEND libraries ${link_libraries})
        endif()
    endif()

    set(public_libraries "")
    if(arg_PUBLIC_LIBRARIES)
        list(APPEND public_libraries "${arg_PUBLIC_LIBRARIES}")
    endif()

    get_target_property(extend_public_libraries "${target}" _qt_extend_target_public_libraries)
    if(extend_public_libraries)
        list(APPEND public_libraries ${extend_public_libraries})
    endif()

    set(sbom_dependencies "")
    if(arg_SBOM_DEPENDENCIES)
        list(APPEND sbom_dependencies "${arg_SBOM_DEPENDENCIES}")
    endif()

    get_target_property(extend_sbom_dependencies "${target}" _qt_extend_target_sbom_dependencies)
    if(extend_sbom_dependencies)
        list(APPEND sbom_dependencies ${extend_sbom_dependencies})
    endif()

    list(REMOVE_DUPLICATES libraries)
    list(REMOVE_DUPLICATES public_libraries)
    list(REMOVE_DUPLICATES sbom_dependencies)

    set(all_direct_libraries ${libraries} ${public_libraries} ${sbom_dependencies})
    list(REMOVE_DUPLICATES all_direct_libraries)

    set(regular_relationship_entries "")
    set(external_relationship_entries "")
    set(external_target_dependencies "")

    # Go through each direct linked lib.
    foreach(direct_lib IN LISTS all_direct_libraries)
        if(NOT TARGET "${direct_lib}")
            continue()
        endif()

        # Check for Qt-specific system library targets. These are marked via qt_find_package calls.
        get_target_property(is_system_library "${direct_lib}" _qt_internal_sbom_is_system_library)
        if(is_system_library)

            # We need to check if the dependency is a FindWrap dependency that points either to a
            # system library or a vendored / bundled library. We need to depend on whichever one
            # the FindWrap script points to.
            __qt_internal_walk_libs(
                "${direct_lib}"
                lib_walked_targets
                _discarded_out_var
                "sbom_targets"
                "collect_targets")

            # Detect if we are dealing with a vendored / bundled lib.
            set(bundled_targets_found FALSE)
            if(lib_walked_targets)
                foreach(lib_walked_target IN LISTS lib_walked_targets)
                    get_target_property(is_3rdparty_bundled_lib
                        "${lib_walked_target}" _qt_module_is_3rdparty_library)

                    # Add a dependency on the vendored lib instead of the Wrap target.
                    if(is_3rdparty_bundled_lib AND lib_spdx_id)
                        set(relationship_entry
                            SBOM_RELATIONSHIP_ENTRY
                                SBOM_RELATIONSHIP_FROM
                                    "${target}"
                                SBOM_RELATIONSHIP_TYPE
                                    DEPENDS_ON
                                SBOM_RELATIONSHIP_TO
                                    "${lib_walked_target}"
                        )
                        list(APPEND regular_relationship_entries ${relationship_entry})
                        set(bundled_targets_found TRUE)
                    endif()
                endforeach()

                if(bundled_targets_found)
                    # If we handled a bundled target, we can move on to process the next direct_lib.
                    continue()
                endif()
            endif()

            if(NOT bundled_targets_found)
                # If we haven't found a bundled target, then it's a regular system library
                # dependency. Make sure to mark the system library as consumed, so that we later
                # generate an sbom for it.
                # Also fall through to the code that actually adds the dependency on the target.
                _qt_internal_append_to_cmake_property_without_duplicates(
                    _qt_internal_sbom_consumed_system_library_targets
                    "${direct_lib}"
                )
            endif()
        endif()

        # Get the spdx id of the dependency.
        _qt_internal_sbom_get_spdx_id_for_target("${direct_lib}" lib_spdx_id)
        if(NOT lib_spdx_id)
            message(DEBUG "Could not add target dependency on target ${direct_lib} "
                "because no spdx id for it could be found.")
            continue()
        endif()

        # Check if the target sbom is defined in an external document.

        _qt_internal_sbom_is_external_target_dependency("${direct_lib}"
            OUT_VAR is_dependency_in_external_document
        )

        if(NOT is_dependency_in_external_document)
            # If the target is not in the external document, it must be one built as part of the
            # current project.
            set(relationship_entry
                SBOM_RELATIONSHIP_ENTRY
                    SBOM_RELATIONSHIP_FROM
                        "${target}"
                    SBOM_RELATIONSHIP_TYPE
                        DEPENDS_ON
                    SBOM_RELATIONSHIP_TO
                        "${direct_lib}"
            )
            list(APPEND regular_relationship_entries ${relationship_entry})
        else()
            # Refer to the package in the external document. This can be the case
            # in a top-level build, where a system library is reused across repos, or for any
            # regular dependency that was built as part of a different project.
            _qt_internal_sbom_add_external_target_dependency(
                TARGET "${target}"
                DEPENDENCY_TARGET "${direct_lib}"
                OUT_SBOM_RELATIONSHIP_ENTRIES extra_relationship_entries
                OUT_TARGET_DEPENDENCIES extra_target_dependencies
            )
            if(extra_relationship_entries)
                list(APPEND external_relationship_entries ${extra_relationship_entries})
                list(APPEND external_target_dependencies ${extra_target_dependencies})
            endif()
        endif()
    endforeach()

    set(all_relationship_entries ${external_relationship_entries} ${regular_relationship_entries})

    set(${arg_OUT_SBOM_RELATIONSHIP_ENTRIES} "${all_relationship_entries}" PARENT_SCOPE)
    set(${arg_OUT_EXTERNAL_TARGET_DEPENDENCIES} "${external_target_dependencies}" PARENT_SCOPE)
endfunction()

# Checks whether the current target will have its sbom generated into the current repo sbom
# document, or whether it is present in an external sbom document.
function(_qt_internal_sbom_is_external_target_dependency target)
    set(opt_args
        SYSTEM_LIBRARY
    )
    set(single_args
        OUT_VAR
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    get_target_property(is_imported "${target}" IMPORTED)
    get_target_property(is_custom_sbom_target "${target}" _qt_sbom_is_custom_sbom_target)

    _qt_internal_sbom_get_root_project_name_lower_case(current_repo_project_name)
    get_property(target_repo_project_name TARGET ${target}
        PROPERTY _qt_sbom_spdx_repo_project_name_lowercase)

    if(NOT "${target_repo_project_name}" STREQUAL ""
            AND NOT "${target_repo_project_name}" STREQUAL "${current_repo_project_name}")
        set(part_of_other_repo TRUE)
    else()
        set(part_of_other_repo FALSE)
    endif()

    set(${arg_OUT_VAR} "${part_of_other_repo}" PARENT_SCOPE)
endfunction()

# Handles generating an external document reference SDPX element for each ${arg_DEPENDENCY_TARGET}
# package that is located in a different spdx document, and collects the relationship in
# OUT_SBOM_RELATIONSHIP_ENTRIES.
# In case of CycloneDX, we also collect the dependency target name in OUT_TARGET_DEPENDENCIES.
function(_qt_internal_sbom_add_external_target_dependency)
    set(opt_args "")
    set(single_args
        TARGET
        DEPENDENCY_TARGET
        OUT_SBOM_RELATIONSHIP_ENTRIES
        OUT_TARGET_DEPENDENCIES
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_TARGET)
        message(FATAL_ERROR "TARGET is required.")
    endif()
    if(NOT arg_DEPENDENCY_TARGET)
        message(FATAL_ERROR "DEPENDENCY_TARGET is required.")
    endif()
    if(NOT arg_OUT_SBOM_RELATIONSHIP_ENTRIES)
        message(FATAL_ERROR "OUT_SBOM_RELATIONSHIP_ENTRIES is required.")
    endif()
    if(NOT arg_OUT_TARGET_DEPENDENCIES)
        message(FATAL_ERROR "OUT_TARGET_DEPENDENCIES is required.")
    endif()

    _qt_internal_sbom_get_spdx_id_for_target("${arg_DEPENDENCY_TARGET}" dep_spdx_id)

    if(NOT dep_spdx_id)
        message(DEBUG "Could not add external target dependency on ${arg_DEPENDENCY_TARGET} "
            "because no spdx id could be found")
        set(${arg_OUT_CYDX_DEPENDENCIES} "" PARENT_SCOPE)
        set(${arg_OUT_SPDX_DEPENDENCIES} "" PARENT_SCOPE)
        set(${arg_OUT_TARGET_DEPENDENCIES} "" PARENT_SCOPE)
        return()
    endif()

    set(relationship_entries "")
    set(target_depdendencies "")

    # Get the external document path and the repo it belongs to for the given target,
    # as well as the spdx document namespace, and cydx serial number.
    get_property(relative_installed_repo_document_path TARGET "${arg_DEPENDENCY_TARGET}"
        PROPERTY _qt_sbom_spdx_relative_installed_repo_document_path)

    get_property(project_name_lowercase TARGET "${arg_DEPENDENCY_TARGET}"
        PROPERTY _qt_sbom_spdx_repo_project_name_lowercase)

    get_property(spdx_document_namespace TARGET "${arg_DEPENDENCY_TARGET}"
        PROPERTY _qt_sbom_spdx_repo_document_namespace)

    get_property(bom_serial_number_uuid TARGET "${arg_DEPENDENCY_TARGET}"
        PROPERTY _qt_sbom_cydx_bom_serial_number_uuid)

    if(relative_installed_repo_document_path AND project_name_lowercase)
        _qt_internal_sbom_get_external_document_ref_spdx_id_from_sbom_target(
            TARGET "${arg_DEPENDENCY_TARGET}"
            OUT_VAR external_document_ref
            CREATE_TEMPORARY_REF_WHEN_MISSING
        )

        get_cmake_property(known_external_document
            _qt_known_external_documents_${external_document_ref})

        set(relationship_entry
            SBOM_RELATIONSHIP_ENTRY
                SBOM_RELATIONSHIP_FROM
                    "${arg_TARGET}"
                SBOM_RELATIONSHIP_TYPE
                    DEPENDS_ON
                SBOM_RELATIONSHIP_TO
                    "${arg_DEPENDENCY_TARGET}"
        )

        list(APPEND relationship_entries "${relationship_entry}")
        list(APPEND target_depdendencies "${arg_DEPENDENCY_TARGET}")

        # Only add a reference to the external document package, if we haven't done so already.
        if(NOT known_external_document)
            set(install_prefixes "")

            get_cmake_property(install_prefix _qt_internal_sbom_install_prefix)
            list(APPEND install_prefixes "${install_prefix}")

            set(external_document "${relative_installed_repo_document_path}")

            # We need to make the external document target name unique to the currently active
            # project, otherwise we risk trying to override the document details of an existing
            # external document target when processing multiple SBOM projects within a single
            # cmake invocation.
            _qt_internal_sbom_get_root_project_name_lower_case(current_project_name)
            set(external_document_target
                "ExternalDocumentProject-${project_name_lowercase}-for-${current_project_name}")

            if(QT_SBOM_GENERATE_SPDX_V2)
                _qt_internal_sbom_add_external_reference_document("${external_document_target}"
                    NO_SCAN_FILE_AT_CONFIGURE_TIME
                    SBOM_FORMAT SPDX_V2_TAG_VALUE
                    SPDX_V2_DOCUMENT_REF_ID "${external_document_ref}"
                    SPDX_V2_DOCUMENT_NAMESPACE "${spdx_document_namespace}"
                    EXTERNAL_DOCUMENT_FILE_PATH "${external_document}"
                    EXTERNAL_DOCUMENT_SEARCH_PATHS ${install_prefixes}
                )
            endif()

            if(QT_SBOM_GENERATE_CYDX_V1_6)
                _qt_internal_sbom_add_external_reference_document("${external_document_target}"
                    NO_SCAN_FILE_AT_CONFIGURE_TIME
                    SBOM_FORMAT CYDX_V1_JSON
                    CYDX_URN_SERIAL_NUMBER "${bom_serial_number_uuid}"
                )
            endif()
        endif()
    else()
        message(AUTHOR_WARNING
            "Missing spdx document path for external target dependency: ${arg_DEPENDENCY_TARGET}")
    endif()

    set(${arg_OUT_SBOM_RELATIONSHIP_ENTRIES} "${relationship_entries}" PARENT_SCOPE)
    set(${arg_OUT_TARGET_DEPENDENCIES} "${target_depdendencies}" PARENT_SCOPE)
endfunction()
