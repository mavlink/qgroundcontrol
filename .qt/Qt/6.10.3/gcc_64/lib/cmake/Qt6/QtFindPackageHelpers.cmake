# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# As an optimization when using -developer-build, qt_find_package records which
# packages were found during the initial configuration. Then on subsequent
# reconfigurations it skips looking for packages that were not found on the
# initial run.
# For the build system to pick up a newly added qt_find_package call, you need to:
# - Start with a clean build dir
# - Or remove the <builddir>/CMakeCache.txt file and configure from scratch
# - Or remove the QT_INTERNAL_PREVIOUSLY_FOUND_PACKAGES cache variable (by
#   editing CMakeCache.txt) and reconfigure.
macro(qt_find_package)
    # Get the target names we expect to be provided by the package.
    set(find_package_options CONFIG NO_MODULE MODULE REQUIRED)
    set(options ${find_package_options} MARK_OPTIONAL)
    set(oneValueArgs MODULE_NAME QMAKE_LIB)
    set(multiValueArgs PROVIDED_TARGETS COMPONENTS OPTIONAL_COMPONENTS)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # If some Qt internal project calls qt_find_package(WrapFreeType), but WrapFreeType was already
    # found as part of a find_dependency() call from a ModuleDependencies.cmake file (or similar),
    # and the provided target is also found, that means this might have been an unnecessary
    # qt_find_package() call, because the dependency was already found via some other transitive
    # dependency. Return early, so that CMake doesn't fail with an error with trying to promote the
    # targets to be global. This behavior is not enabled by default, because there are cases
    # when a regular find_package() (non qt_) can find a package (Freetype -> PNG), and a subsequent
    # qt_find_package(PNG PROVIDED_TARGET PNG::PNG) still needs to succeed and register the provided
    # targets. To enable the debugging behavior, set QT_DEBUG_QT_FIND_PACKAGE to 1.
    set(_qt_find_package_skip_find_package FALSE)

    # Skip looking for packages that were not found on initial configuration, because they likely
    # won't be found again, and only waste configuration time.
    # Speeds up reconfiguration configuration for certain platforms and repos.
    # Due to this behavior being different from what general CMake projects expect, it is only
    # done for -developer-builds.
    if(QT_INTERNAL_PREVIOUSLY_FOUND_PACKAGES AND
            NOT "${ARGV0}" IN_LIST QT_INTERNAL_PREVIOUSLY_FOUND_PACKAGES
            AND "${ARGV0}" IN_LIST QT_INTERNAL_PREVIOUSLY_SEARCHED_PACKAGES)
        set(_qt_find_package_skip_find_package TRUE)
    endif()

    set_property(GLOBAL APPEND PROPERTY _qt_previously_searched_packages "${ARGV0}")

    if(QT_DEBUG_QT_FIND_PACKAGE AND ${ARGV0}_FOUND AND arg_PROVIDED_TARGETS)
        set(_qt_find_package_skip_find_package TRUE)
        foreach(qt_find_package_target_name ${arg_PROVIDED_TARGETS})
            if(NOT TARGET ${qt_find_package_target_name})
                set(_qt_find_package_skip_find_package FALSE)
            endif()
        endforeach()

        if(_qt_find_package_skip_find_package)
            message(AUTHOR_WARNING "qt_find_package(${ARGV0}) called even though the package "
                   "was already found. Consider removing the call.")
        endif()
    endif()

    # When configure.cmake is included only to record summary entries, there's no point in looking
    # for the packages.
    if(__QtFeature_only_record_summary_entries)
        set(_qt_find_package_skip_find_package TRUE)
    endif()

    # Get the version if specified.
    set(package_version "")
    if(${ARGC} GREATER_EQUAL 2)
        if(${ARGV1} MATCHES "^[0-9\.]+$")
            set(package_version "${ARGV1}")
        endif()
    endif()

    if(arg_COMPONENTS)
        # Re-append components to forward them.
        list(APPEND arg_UNPARSED_ARGUMENTS "COMPONENTS;${arg_COMPONENTS}")
    endif()
    if(arg_OPTIONAL_COMPONENTS)
        # Re-append optional components to forward them.
        list(APPEND arg_UNPARSED_ARGUMENTS "OPTIONAL_COMPONENTS;${arg_OPTIONAL_COMPONENTS}")
    endif()

    # Don't look for packages in PATH if requested to.
    if(QT_NO_USE_FIND_PACKAGE_SYSTEM_ENVIRONMENT_PATH)
        set(_qt_find_package_use_system_env_backup "${CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH}")
        set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH "OFF")
    endif()

    if(NOT (arg_CONFIG OR arg_NO_MODULE OR arg_MODULE) AND NOT _qt_find_package_skip_find_package)
        # Try to find a config package first in quiet mode
        set(config_package_arg ${arg_UNPARSED_ARGUMENTS})
        list(APPEND config_package_arg "CONFIG;QUIET")
        find_package(${config_package_arg})

        # Double check that in config mode the targets become visible. Sometimes
        # only the module mode creates the targets. For example with vcpkg, the sqlite
        # package provides sqlite3-config.cmake, which offers multi-config targets but
        # in their own way. CMake has FindSQLite3.cmake and with the original
        # qt_find_package(SQLite3) call it is our intention to use the cmake package
        # in module mode.
        unset(_qt_any_target_found)
        unset(_qt_should_unset_found_var)
        if(${ARGV0}_FOUND AND arg_PROVIDED_TARGETS)
            foreach(expected_target ${arg_PROVIDED_TARGETS})
                if (TARGET ${expected_target})
                    set(_qt_any_target_found TRUE)
                    break()
                endif()
            endforeach()
            if(NOT _qt_any_target_found)
                set(_qt_should_unset_found_var TRUE)
            endif()
        endif()
        # If we consider the package not to be found, make sure to unset both regular
        # and CACHE vars, otherwise CMP0126 set to NEW might cause issues with
        # packages not being found correctly.
        if(NOT ${ARGV0}_FOUND OR _qt_should_unset_found_var)
            unset(${ARGV0}_FOUND)
            unset(${ARGV0}_FOUND CACHE)

            # Unset the NOTFOUND ${package}_DIR var that might have been set by the previous
            # find_package call, to get rid of "not found" messages in the feature summary
            # if the package is found by the next find_package call.
            if(DEFINED CACHE{${ARGV0}_DIR} AND NOT ${ARGV0}_DIR)
                unset(${ARGV0}_DIR CACHE)
            endif()
        endif()
    endif()

    # Ensure the options are back in the original unparsed arguments
    foreach(opt IN LISTS find_package_options)
        if(arg_${opt})
            list(APPEND arg_UNPARSED_ARGUMENTS ${opt})
        endif()
    endforeach()

    # TODO: Handle packages with components where a previous component is already found.
    # E.g. find_package(Qt6 COMPONENTS BuildInternals) followed by
    # qt_find_package(Qt6 COMPONENTS Core) doesn't end up calling find_package(Qt6Core).
    if ((NOT ${ARGV0}_FOUND OR arg_MODULE) AND NOT _qt_find_package_skip_find_package)
        # Call original function without our custom arguments.
        find_package(${arg_UNPARSED_ARGUMENTS})
    endif()

    if(QT_NO_USE_FIND_PACKAGE_SYSTEM_ENVIRONMENT_PATH)
        if("${_qt_find_package_use_system_env_backup}" STREQUAL "")
            unset(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH)
        else()
            set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH "${_qt_find_package_use_system_env_backup}")
        endif()
    endif()

    if(${ARGV0}_FOUND)
        # Record that the package was found, so that future reconfigurations can be sped up.
        set_property(GLOBAL APPEND PROPERTY _qt_previously_found_packages "${ARGV0}")
    endif()

    if(${ARGV0}_FOUND AND arg_PROVIDED_TARGETS AND NOT _qt_find_package_skip_find_package)
        # If package was found, associate each target with its package name. This will be used
        # later when creating Config files for Qt libraries, to generate correct find_dependency()
        # calls. Also make the provided targets global, so that the properties can be read in
        # all scopes.
        foreach(qt_find_package_target_name ${arg_PROVIDED_TARGETS})
            if(TARGET ${qt_find_package_target_name})
                # Allow usage of aliased targets by setting properties on the actual target
                _qt_internal_dealias_target(qt_find_package_target_name)

                if("${qt_find_package_target_name}" MATCHES "${QT_CMAKE_EXPORT_NAMESPACE}::"
                    AND QT_FEATURE_developer_build
                )
                    message(AUTHOR_WARNING
                        "qt_find_package() should NOT be used to look up Qt packages. "
                        "It should only be used to look up 3rd party packages. "
                        "Please remove the "
                        "qt_find_package(${ARGV0} PROVIDED_TARGETS "
                        "${qt_find_package_target_name}) call and contact the build tools team "
                        "in case the removal is causing issues."
                    )
                endif()

                set_target_properties(${qt_find_package_target_name} PROPERTIES
                    INTERFACE_QT_PACKAGE_NAME ${ARGV0}
                    INTERFACE_QT_PACKAGE_IS_OPTIONAL ${arg_MARK_OPTIONAL})
                if(package_version)
                    set_target_properties(${qt_find_package_target_name}
                                          PROPERTIES INTERFACE_QT_PACKAGE_VERSION ${ARGV1})
                endif()

                # Save the retrieved package version.
                set(_qt_find_package_found_version "")
                if(${ARGV0}_VERSION)
                    set(_qt_find_package_found_version "${${ARGV0}_VERSION}")
                    set_target_properties(${qt_find_package_target_name}
                        PROPERTIES
                            _qt_package_found_version "${_qt_find_package_found_version}")
                endif()

                if(arg_COMPONENTS)
                    string(REPLACE ";" " " components_as_string "${arg_COMPONENTS}")
                    set_property(TARGET ${qt_find_package_target_name}
                                 PROPERTY INTERFACE_QT_PACKAGE_COMPONENTS ${components_as_string})
                endif()

                if(arg_OPTIONAL_COMPONENTS)
                    string(REPLACE ";" " " components_as_string "${arg_OPTIONAL_COMPONENTS}")
                    set_property(TARGET ${qt_find_package_target_name}
                                 PROPERTY INTERFACE_QT_PACKAGE_OPTIONAL_COMPONENTS
                                 ${components_as_string})
                endif()

                # Work around: QTBUG-125371
                if(NOT "${ARGV0}" STREQUAL "Qt6")
                    # Record the package + component + optional component provided targets.
                    qt_internal_record_package_component_provided_targets(
                        PACKAGE_NAME "${ARGV0}"
                        ON_TARGET ${qt_find_package_target_name}
                        PROVIDED_TARGETS ${arg_PROVIDED_TARGETS}
                        COMPONENTS ${arg_COMPONENTS}
                        OPTIONAL_COMPONENTS ${arg_OPTIONAL_COMPONENTS}
                    )
                endif()

                _qt_internal_promote_3rd_party_provided_target_and_3rd_party_deps_to_global(
                    "${qt_find_package_target_name}")

                set(_qt_find_package_sbom_args "")

                if(_qt_find_package_found_version)
                    list(APPEND _qt_find_package_sbom_args
                        PACKAGE_VERSION "${_qt_find_package_found_version}"
                    )
                endif()

                # Work around: QTBUG-125371
                if(NOT "${ARGV0}" STREQUAL "Qt6")
                    _qt_internal_sbom_record_system_library_usage(
                        "${qt_find_package_target_name}"
                        SBOM_ENTITY_TYPE SYSTEM_LIBRARY
                        FRIENDLY_PACKAGE_NAME "${ARGV0}"
                        ${_qt_find_package_sbom_args}
                    )
                endif()
            endif()
        endforeach()

        if(arg_MODULE_NAME AND arg_QMAKE_LIB
           AND (NOT arg_QMAKE_LIB IN_LIST QT_QMAKE_LIBS_FOR_${arg_MODULE_NAME}))
            set(QT_QMAKE_LIBS_FOR_${arg_MODULE_NAME}
                ${QT_QMAKE_LIBS_FOR_${arg_MODULE_NAME}};${arg_QMAKE_LIB} CACHE INTERNAL "")
            set(${arg_QMAKE_LIB}_existing_targets "")
            foreach(provided_target ${arg_PROVIDED_TARGETS})
                if(TARGET ${provided_target})
                    list(APPEND ${arg_QMAKE_LIB}_existing_targets "${provided_target}")
                    set(QT_QMAKE_LIB_OF_TARGET_${provided_target}
                        ${arg_QMAKE_LIB} CACHE INTERNAL "")
                endif()
            endforeach()
            set(QT_TARGETS_OF_QMAKE_LIB_${arg_QMAKE_LIB}
                "${${arg_QMAKE_LIB}_existing_targets}" CACHE INTERNAL "")
        endif()
    endif()
endmacro()

# Records information about a package's provided targets, given a specific list of components.
#
# A package might contain multiple components, and create only a subset of targets based on which
# components are looked for.
# This function computes a unique key / id using the package name and the components that are
# passed.
# Then it saves the id in a property on the ON_TARGET target. The ON_TARGET target is one
# of the provided targets for that package id. This allows us to create a relationship to find
# the package id, given a target.
# The function also appends the list of provided targets for that package id to a global property.
# This information will later be saved into the module Dependencies.cmake file.
function(qt_internal_record_package_component_provided_targets)
    set(opt_args "")
    set(single_args
        PACKAGE_NAME
        ON_TARGET
    )
    set(multi_args
        COMPONENTS
        OPTIONAL_COMPONENTS
        PROVIDED_TARGETS
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_PACKAGE_NAME)
        message(FATAL_ERROR "PACKAGE_NAME is required.")
    endif()

    if(NOT arg_ON_TARGET)
        message(FATAL_ERROR "ON_TARGET is required.")
    endif()

    _qt_internal_get_package_components_id(
        PACKAGE_NAME "${arg_PACKAGE_NAME}"
        COMPONENTS ${arg_COMPONENTS}
        OPTIONAL_COMPONENTS ${arg_OPTIONAL_COMPONENTS}
        OUT_VAR_KEY package_key
    )

    set_target_properties(${arg_ON_TARGET} PROPERTIES
        _qt_package_components_id "${package_key}"
    )

    _qt_internal_append_to_cmake_property_without_duplicates(
        _qt_find_package_${package_key}_provided_targets
        "${arg_PROVIDED_TARGETS}"
    )
endfunction()

# Save found packages in the cache. They will be read on next reconfiguration to skip looking
# for packages that were not previously found.
# Only applies to -developer-builds by default.
# Can also be opted in or opted out via QT_INTERNAL_SAVE_PREVIOUSLY_FOUND_PACKAGES.
# Opting out will need two reconfigurations to take effect.
function(qt_internal_save_previously_visited_packages)
    if(DEFINED QT_INTERNAL_SAVE_PREVIOUSLY_FOUND_PACKAGES)
        set(should_save "${QT_INTERNAL_SAVE_PREVIOUSLY_FOUND_PACKAGES}")
    else()
        if(FEATURE_developer_build OR QT_FEATURE_developer_build)
            set(should_save ON)
        else()
            set(should_save OFF)
        endif()
    endif()

    if(NOT should_save)
        # When the value is flipped to OFF, remove any previously saved packages.
        unset(QT_INTERNAL_PREVIOUSLY_FOUND_PACKAGES CACHE)
        unset(QT_INTERNAL_PREVIOUSLY_SEARCHED_PACKAGES CACHE)
        return()
    endif()

    get_property(_qt_previously_found_packages GLOBAL PROPERTY _qt_previously_found_packages)
    if(_qt_previously_found_packages)
        list(REMOVE_DUPLICATES _qt_previously_found_packages)
        set(QT_INTERNAL_PREVIOUSLY_FOUND_PACKAGES "${_qt_previously_found_packages}" CACHE INTERNAL
            "List of CMake packages found during configuration using qt_find_package.")
    endif()

    get_property(_qt_previously_searched_packages GLOBAL PROPERTY _qt_previously_searched_packages)
    if(_qt_previously_searched_packages)
        list(REMOVE_DUPLICATES _qt_previously_searched_packages)
        set(QT_INTERNAL_PREVIOUSLY_SEARCHED_PACKAGES
            "${_qt_previously_searched_packages}" CACHE INTERNAL
            "List of CMake packages searched during configuration using qt_find_package."
        )
    endif()
endfunction()

# Return qmake library name for the given target, e.g. return "vulkan" for "Vulkan::Vulkan".
function(qt_internal_map_target_to_qmake_lib target out_var)
    set(${out_var} "${QT_QMAKE_LIB_OF_TARGET_${target}}" PARENT_SCOPE)
endfunction()

# This function records a dependency between ${main_target_name} and ${dep_package_name}.
# at the CMake package level.
# E.g. The Tools package that provides the qtwaylandscanner target
# needs to call find_package(WaylandScanner) (non-qt-package).
# main_target_name = qtwaylandscanner
# dep_package_name = WaylandScanner
function(qt_record_extra_package_dependency main_target_name dep_package_name dep_package_version)
    if(NOT TARGET "${main_target_name}")
        qt_get_tool_target_name(main_target_name "${main_target_name}")
    endif()
    if (TARGET "${main_target_name}")
        get_target_property(extra_packages "${main_target_name}" QT_EXTRA_PACKAGE_DEPENDENCIES)
        if(NOT extra_packages)
            set(extra_packages "")
        endif()

        list(APPEND extra_packages "${dep_package_name}\;${dep_package_version}")
        set_target_properties("${main_target_name}" PROPERTIES QT_EXTRA_PACKAGE_DEPENDENCIES
                                                               "${extra_packages}")
    endif()
endfunction()

# This function records a dependency between ${main_target_name} and ${dep_target_name}
# at the CMake package level.
# E.g. Qt6CoreConfig.cmake needs to find_package(Qt6EntryPointPrivate).
# main_target_name = Core
# dep_target_name = EntryPointPrivate
# This is just a convenience function that deals with Qt targets and their associated packages
# instead of raw package names.
#
# Deprecated since 6.9.
function(qt_record_extra_qt_package_dependency main_target_name dep_target_name
                                                                dep_package_version)
    # EntryPointPrivate -> Qt6EntryPointPrivate.
    qt_internal_qtfy_target(qtfied_target_name "${dep_target_name}")
    qt_record_extra_package_dependency("${main_target_name}"
        "${qtfied_target_name_versioned}" "${dep_package_version}")
endfunction()

# This function records a 'QtFooTools' package dependency for the ${main_target_name} target
# onto the ${dep_package_name} tools package.
# E.g. The QtWaylandCompositor package needs to call find_package(QtWaylandScannerTools).
# main_target_name = WaylandCompositor
# dep_package_name = Qt6WaylandScannerTools
function(qt_record_extra_main_tools_package_dependency
         main_target_name dep_package_name dep_package_version)
    if(NOT TARGET "${main_target_name}")
        qt_get_tool_target_name(main_target_name "${main_target_name}")
    endif()
    if (TARGET "${main_target_name}")
        get_target_property(extra_packages "${main_target_name}"
                            QT_EXTRA_TOOLS_PACKAGE_DEPENDENCIES)
        if(NOT extra_packages)
            set(extra_packages "")
        endif()

        list(APPEND extra_packages "${dep_package_name}\;${dep_package_version}")
        set_target_properties("${main_target_name}" PROPERTIES QT_EXTRA_TOOLS_PACKAGE_DEPENDENCIES
                                                               "${extra_packages}")
    endif()
endfunction()

# This function records a 'QtFooTools' package dependency for the ${main_target_name} target
# onto the ${dep_non_versioned_package_name} Tools package.
# main_target_name = WaylandCompositor
# dep_non_versioned_package_name = WaylandScannerTools
# This is just a convenience function to avoid hardcoding the qtified version in the dep package
# name.
function(qt_record_extra_qt_main_tools_package_dependency main_target_name
                                                          dep_non_versioned_package_name
                                                          dep_package_version)
    # WaylandScannerTools -> Qt6WaylandScannerTools.
    qt_internal_qtfy_target(qtfied_package_name "${dep_non_versioned_package_name}")
    qt_record_extra_main_tools_package_dependency(
        "${main_target_name}" "${qtfied_package_name_versioned}" "${dep_package_version}")
endfunction()

# Record an extra 3rd party target as a dependency for ${main_target_name}.
#
# Adds a find_package(${dep_target_package_name}) in ${main_target_name}Dependencies.cmake.
#
# Needed to record a dependency on the package that provides WrapVulkanHeaders::WrapVulkanHeaders.
# The package version, components, whether the package is optional, etc, are queried from the
# ${dep_target} target properties.
# Usually these are set at the qt_find_package() call site of a configure.cmake file e.g. using
# Qt's MARK_OPTIONAL option.
function(qt_record_extra_third_party_dependency main_target_name dep_target)
    if(NOT TARGET "${main_target_name}")
        qt_get_tool_target_name(main_target_name "${main_target_name}")
    endif()
    if(TARGET "${main_target_name}")
        get_target_property(extra_deps "${main_target_name}" _qt_extra_third_party_dep_targets)
        if(NOT extra_deps)
            set(extra_deps "")
        endif()

        list(APPEND extra_deps "${dep_target}")
        set_target_properties("${main_target_name}" PROPERTIES _qt_extra_third_party_dep_targets
                                                               "${extra_deps}")
    endif()
endfunction()

# Sets out_var to TRUE if the non-namespaced ${lib} target is exported as part of Qt6Targets.cmake.
function(qt_internal_is_lib_part_of_qt6_package lib out_var)
    if (lib STREQUAL "Platform"
            OR lib STREQUAL "GlobalConfig"
            OR lib STREQUAL "GlobalConfigPrivate"
            OR lib STREQUAL "PlatformModuleInternal"
            OR lib STREQUAL "PlatformPluginInternal"
            OR lib STREQUAL "PlatformToolInternal"
            OR lib STREQUAL "PlatformCommonInternal"
            OR lib STREQUAL "PlatformExampleInternal"
    )
        set(${out_var} "TRUE" PARENT_SCOPE)
    else()
        set(${out_var} "FALSE" PARENT_SCOPE)
    endif()
endfunction()

# Try to get the CMake package version of a Qt target.
#
# Query the target's _qt_package_version property, or try to read it from the CMake package version
# variable set from calling find_package(Qt6${target}).
# Not all targets will have a find_package _VERSION variable, for example if the target is an
# executable.
# A heuristic is used to handle QtFooPrivate module targets.
# If no version can be found, fall back to ${PROJECT_VERSION} and issue a warning.
function(qt_internal_get_package_version_of_target target package_version_out_var)
    qt_internal_is_lib_part_of_qt6_package("${target}" is_part_of_qt6)

    if(is_part_of_qt6)
        # When building qtbase, Qt6_VERSION is not set (unless examples are built in-tree,
        # non-ExternalProject). Use the Platform target's version instead which would be the
        # equivalent.
        if(TARGET "${QT_CMAKE_EXPORT_NAMESPACE}::Platform")
            get_target_property(package_version
                "${QT_CMAKE_EXPORT_NAMESPACE}::Platform" _qt_package_version)
        endif()
        if(NOT package_version)
            set(package_version "${${QT_CMAKE_EXPORT_NAMESPACE}_VERSION}")
        endif()
    else()
        # Try to get the version from the target.
        # Try the Private target first and if it doesn't exist, try the non-Private target later.
        if(TARGET "${QT_CMAKE_EXPORT_NAMESPACE}::${target}")
            get_target_property(package_version
                "${QT_CMAKE_EXPORT_NAMESPACE}::${target}" _qt_package_version)
        endif()

        # Try to get the version from the corresponding package version variable.
        if(NOT package_version)
            set(package_version "${${QT_CMAKE_EXPORT_NAMESPACE}${target}_VERSION}")
        endif()

        # Try non-Private target.
        if(NOT package_version AND target MATCHES "(.*)Private$")
            set(target "${CMAKE_MATCH_1}")
        endif()

        if(NOT package_version AND TARGET "${QT_CMAKE_EXPORT_NAMESPACE}::${target}")
            get_target_property(package_version
                "${QT_CMAKE_EXPORT_NAMESPACE}::${target}" _qt_package_version)
        endif()

        if(NOT package_version)
            set(package_version "${${QT_CMAKE_EXPORT_NAMESPACE}${target}_VERSION}")
        endif()
    endif()

    if(NOT package_version)
        set(package_version "${PROJECT_VERSION}")
        if(FEATURE_developer_build)
            message(WARNING
                "Could not determine package version of target ${target}. "
                "Defaulting to project version ${PROJECT_VERSION}.")
        endif()
    endif()

    set(${package_version_out_var} "${package_version}" PARENT_SCOPE)
endfunction()

# Get the CMake package name that contains / exported the Qt module target.
function(qt_internal_get_package_name_of_target target package_name_out_var)
    qt_internal_is_lib_part_of_qt6_package("${target}" is_part_of_qt6)

    if(is_part_of_qt6)
        set(package_name "${INSTALL_CMAKE_NAMESPACE}")
    else()
        # Get the package name from the module's target property.
        # If not set, fallback to a name based on the target name.
        #
        # TODO: Remove fallback once sufficient time has passed, aka all developers updated
        # their builds not to contain stale FooDependencies.cmakes files without the
        # _qt_package_name property.
        set(package_name "")
        set(package_name_default "${INSTALL_CMAKE_NAMESPACE}${target}")
        set(target_namespaced "${QT_CMAKE_EXPORT_NAMESPACE}::${target}")
        if(TARGET "${target_namespaced}")
            get_target_property(package_name_from_prop "${target_namespaced}" _qt_package_name)
            if(package_name_from_prop)
                set(package_name "${package_name_from_prop}")
            endif()
        endif()
        if(NOT package_name)
            message(WARNING
                "Could not find target ${target_namespaced} to query its package name. "
                "Defaulting to package name ${package_name_default}. Consider re-arranging the "
                "project structure to ensure the target exists by this point."
            )
            set(package_name "${package_name_default}")
        endif()
    endif()

    set(${package_name_out_var} "${package_name}" PARENT_SCOPE)
endfunction()

# This function collects the list of Qt targets a library depend on,
# along with their version info, for usage in ${target}Dependencies.cmake file
# Multi-value Arguments:
#   PUBLIC
#     public dependencies
#   PRIVATE
#     private dependencies
function(qt_internal_register_target_dependencies target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "PUBLIC;PRIVATE")
    get_target_property(target_deps "${target}" _qt_target_deps)
    if(NOT target_deps)
        set(target_deps "")
    endif()

    set(lib_list "")
    if(arg_PUBLIC)
        set(lib_list "${arg_PUBLIC}")
    endif()

    get_target_property(target_type ${target} TYPE)
    set(target_is_shared FALSE)
    set(target_is_static FALSE)
    if(target_type STREQUAL "SHARED_LIBRARY")
        set(target_is_shared TRUE)
    elseif(target_type STREQUAL "STATIC_LIBRARY")
        set(target_is_static TRUE)
    endif()

    # Record 'Qt::Foo'-like private dependencies of static library targets, this will be used to
    # generate find_dependency() calls.
    #
    # Private static library dependencies will become $<LINK_ONLY:> dependencies in
    # INTERFACE_LINK_LIBRARIES.
    if(target_is_static AND arg_PRIVATE)
        list(APPEND lib_list ${arg_PRIVATE})
    endif()

    foreach(lib IN LISTS lib_list)
        if("${lib}" MATCHES "^Qt::(.*)")
            set(lib "${CMAKE_MATCH_1}")
        elseif("${lib}" MATCHES "^${QT_CMAKE_EXPORT_NAMESPACE}::(.*)")
            set(lib "${CMAKE_MATCH_1}")
        else()
            set(lib "")
        endif()

        if(lib)
            qt_internal_get_package_name_of_target("${lib}" package_name)
            qt_internal_get_package_version_of_target("${lib}" package_version)
            list(APPEND target_deps "${package_name}\;${package_version}")
        endif()
    endforeach()

    # Record 'Qt::Foo'-like shared private dependencies of shared library targets.
    #
    # Private shared library dependencies are listed in the target's
    # IMPORTED_LINK_DEPENDENT_LIBRARIES and used in rpath-link calculation.
    # See QTBUG-86533 for some details.
    # We filter out static libraries and common platform targets, but include both SHARED and
    # INTERFACE libraries. INTERFACE libraries in most cases will be FooPrivate libraries.
    if(target_is_shared AND arg_PRIVATE)
        foreach(lib IN LISTS arg_PRIVATE)
            set(lib_namespaced "${lib}")
            if("${lib}" MATCHES "^Qt::(.*)")
                set(lib "${CMAKE_MATCH_1}")
            elseif("${lib}" MATCHES "^${QT_CMAKE_EXPORT_NAMESPACE}::(.*)")
                set(lib "${CMAKE_MATCH_1}")
            else()
                set(lib "")
            endif()

            if(lib)
                set(lib "${CMAKE_MATCH_1}")

                qt_internal_is_lib_part_of_qt6_package("${lib}" is_part_of_qt6)
                get_target_property(lib_type "${lib_namespaced}" TYPE)
                if(NOT lib_type STREQUAL "STATIC_LIBRARY" AND NOT is_part_of_qt6)
                    qt_internal_get_package_name_of_target("${lib}" package_name)
                    qt_internal_get_package_version_of_target("${lib}" package_version)
                    list(APPEND target_deps "${package_name}\;${package_version}")
                endif()
            endif()
        endforeach()
    endif()

    set_target_properties("${target}" PROPERTIES _qt_target_deps "${target_deps}")
endfunction()
