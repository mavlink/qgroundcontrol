# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(__qt_internal_strip_target_directory_scope_token target out_var)
    # In CMake versions earlier than CMake 3.18, a subdirectory scope id is appended to the
    # target name if the target is referenced in a target_link_libraries command from a
    # different directory scope than where the target was created.
    # Strip it.
    #
    # For informational purposes, in CMake 3.18, the target name looks as follows:
    # ::@(0x5604cb3f6b50);Threads::Threads;::@
    # This case doesn't have to be stripped (at least for now), because when we iterate over
    # link libraries, the tokens appear as separate target names.
    #
    # Example: Threads::Threads::@<0x5604cb3f6b50>
    # Output: Threads::Threads
    string(REGEX REPLACE "::@<.+>$" "" target "${target}")
    set("${out_var}" "${target}" PARENT_SCOPE)
endfunction()

# Work around AUTOGEN issue when a library is added as a dependency more than once, and the autogen
# library dependency results in being discarded. To mitigate that, add all autogen dependencies
# manually, based on the passed in dependencies.
# CMake 4.0+ has a fix, so we don't need the extra logic.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/26700
function(_qt_internal_work_around_autogen_discarded_dependencies target)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0
            OR QT_NO_AUTOGEN_DISCARDED_DEPENDENCIES_WORKAROUND)
        return()
    endif()

    set(libraries ${ARGN})
    set(final_libraries "")

    foreach(lib IN LISTS libraries)
        # Skip non-target dependencies.
        if(NOT TARGET "${lib}")
            continue()
        endif()

        # Resolve alias targets, because AUTOGEN_TARGET_DEPENDS doesn't seem to handle them.
        _qt_internal_dealias_target(lib)

        # Skip imported targets, they don't have sync_headers targets.
        get_target_property(imported "${lib}" IMPORTED)
        if(imported)
            continue()
        endif()

        # Resolve Qt private modules to their public counterparts.
        get_target_property(is_private_module "${lib}" _qt_is_private_module)
        get_target_property(public_module_target "${lib}" _qt_public_module_target_name)

        if(is_private_module AND public_module_target)
            set(lib "${public_module_target}")
        endif()

        # Another TARGET check, just in case.
        if(TARGET "${lib}")
            list(APPEND final_libraries "${lib}")
        endif()
    endforeach()
    if(final_libraries)
        _qt_internal_append_to_target_property_without_duplicates("${target}"
            AUTOGEN_TARGET_DEPENDS ${final_libraries})
    endif()
endfunction()

# This function is similar to _qt_internal_work_around_autogen_discarded_dependencies
# but it instead queries the libs to process from the target's LINK_LIBRARIES and
# INTERFACE_LINK_LIBRARIES.
# It only applies the logic while building Qt itself.
# It's meant to be used in public API like qt_finalize_target, so that the workaround is applied
# to examples that are built as part of the qt build tree.
function(_qt_internal_work_around_autogen_discarded_dependencies_from_target_libs target)
    if(NOT QT_BUILDING_QT)
        return()
    endif()

    set(libraries "")

    get_target_property(link_libs "${target}" LINK_LIBRARIES)
    if(link_libs)
        list(APPEND libraries "${link_libs}")
    endif()
    get_target_property(interface_link_libs "${target}" INTERFACE_LINK_LIBRARIES)

    if(interface_link_libs)
        list(APPEND libraries "${interface_link_libs}")
    endif()

    if(NOT libraries)
        return()
    endif()

    _qt_internal_work_around_autogen_discarded_dependencies("${target}" ${libraries})
endfunction()

# Tests if linker could resolve circular dependencies between object files and static libraries.
function(__qt_internal_static_link_order_public_test result)
    # We could trust iOS linker
    if(IOS)
        set(QT_HAVE_LINK_ORDER_MATTERS "FALSE" CACHE INTERNAL "Link order matters")
    endif()

    if(DEFINED QT_HAVE_LINK_ORDER_MATTERS)
        set(${result} "${QT_HAVE_LINK_ORDER_MATTERS}" PARENT_SCOPE)
        return()
    endif()

    if(EXISTS "${QT_CMAKE_DIR}")
        set(test_source_basedir "${QT_CMAKE_DIR}/..")
    else()
        set(test_source_basedir "${_qt_cmake_dir}/${QT_CMAKE_EXPORT_NAMESPACE}")
    endif()

    try_compile(${result}
        "${CMAKE_CURRENT_BINARY_DIR}/config.tests/static_link_order"
        "${test_source_basedir}/config.tests/static_link_order"
        static_link_order_test
        static_link_order_test
    )
    message(STATUS "Check if linker can resolve circular dependencies - ${${result}}")

    # Invert the result
    if(${result})
        set(${result} FALSE)
    else()
        set(${result} TRUE)
    endif()

    set(QT_HAVE_LINK_ORDER_MATTERS "${${result}}" CACHE INTERNAL "Link order matters")

    set(${result} "${${result}}" PARENT_SCOPE)
endfunction()

# Sets _qt_link_order_matters flag for the target.
function(__qt_internal_set_link_order_matters target link_order_matters)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Unable to set _qt_link_order_matters flag. ${target} is not a target.")
    endif()

    _qt_internal_dealias_target(target)

    if(link_order_matters)
        set(link_order_matters TRUE)
    else()
        set(link_order_matters FALSE)
    endif()
    set_target_properties(${target} PROPERTIES _qt_link_order_matters "${link_order_matters}")
endfunction()

# Function combines __qt_internal_static_link_order_public_test and
# __qt_internal_set_link_order_matters calls on Qt::Platform target.
function(__qt_internal_check_link_order_matters)
    __qt_internal_static_link_order_public_test(
         link_order_matters
    )
    __qt_internal_set_link_order_matters(
        ${QT_CMAKE_EXPORT_NAMESPACE}::Platform "${link_order_matters}"
    )

    if("${ARGC}" GREATER "0" AND NOT ARGV0 STREQUAL "")
        set(${ARGV0} ${link_order_matters} PARENT_SCOPE)
    endif()
endfunction()

# Constructs a TARGET_POLICY genex expression if the policy is available.
function(__qt_internal_get_cmp0099_genex_check result)
    if(POLICY CMP0099)
        set(${result} "$<BOOL:$<TARGET_POLICY:CMP0099>>" PARENT_SCOPE)
    else()
        set(${result} "$<BOOL:FALSE>" PARENT_SCOPE)
    endif()
endfunction()

function(__qt_internal_check_cmp0099_available)
    set(platform_target ${QT_CMAKE_EXPORT_NAMESPACE}::Platform)
    _qt_internal_dealias_target(platform_target)

    __qt_internal_get_cmp0099_genex_check(cmp0099_check)
    set_target_properties(${platform_target} PROPERTIES
        _qt_cmp0099_policy_check "${cmp0099_check}"
    )

    set(result TRUE)
    if(NOT POLICY CMP0099)
        set(result FALSE)
    endif()

    if("${ARGC}" GREATER "0" AND NOT ARGV0 STREQUAL "")
        set(${ARGV0} ${result} PARENT_SCOPE)
    endif()
endfunction()

function(__qt_internal_process_dependency_object_libraries target)
    # The CMake versions greater than 3.21 take care about the order of object files in a
    # linker line, it's expected that all object files are located at the beginning of the linker
    # line.
    # So circular dependencies between static libraries and object files are resolved and no need
    # to call the finalizer code.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
        return()
    endif()
    get_target_property(processed ${target} _qt_object_libraries_finalizer_processed)
    if(processed)
        return()
    endif()
    set_target_properties(${target} PROPERTIES _qt_object_libraries_finalizer_processed TRUE)

    get_target_property(qt_link_order_matters
        ${QT_CMAKE_EXPORT_NAMESPACE}::Platform _qt_link_order_matters
    )
    __qt_internal_check_finalizer_mode(${target}
        use_finalizer_mode
        object_libraries
        DEFAULT_VALUE "${qt_link_order_matters}"
    )

    if(NOT use_finalizer_mode)
        return()
    endif()

    __qt_internal_collect_dependency_object_libraries(${target} objects)
    target_sources(${target} PRIVATE "${objects}")
endfunction()

function(__qt_internal_collect_dependency_object_libraries target out_var)
    set_property(GLOBAL PROPERTY _qt_processed_object_libraries "")

    __qt_internal_collect_object_libraries_recursively(object_libraries ${target} ${target})

    # Collect object libraries of plugins and plugin dependencies.
    __qt_internal_collect_plugin_targets_from_dependencies(${target} plugin_targets)
    __qt_internal_collect_dependency_plugin_object_libraries(${target}
        "${plugin_targets}"
        plugin_objects
    )

    set_property(GLOBAL PROPERTY _qt_processed_object_libraries "")
    __qt_internal_get_cmp0099_genex_check(cmp0099_check)

    list(REMOVE_DUPLICATES object_libraries)
    set(objects "")
    foreach(dep IN LISTS object_libraries)
        list(PREPEND objects "$<$<NOT:${cmp0099_check}>:$<TARGET_OBJECTS:${dep}>>")
    endforeach()

    set(${out_var} "${plugin_objects};${objects}" PARENT_SCOPE)
endfunction()

function(__qt_internal_collect_dependency_plugin_object_libraries target plugin_targets out_var)
    __qt_internal_get_cmp0099_genex_check(cmp0099_check)
    set(plugin_objects "")
    foreach(plugin_target IN LISTS plugin_targets)
        __qt_internal_collect_object_libraries_recursively(plugin_object_libraries
            "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}"
            ${target}
        )
        __qt_internal_get_static_plugin_condition_genex("${plugin_target}" plugin_condition)

        foreach(plugin_object_library IN LISTS plugin_object_libraries)
            string(JOIN "" plugin_objects_genex
                "$<"
                    "$<AND:"
                        "$<NOT:${cmp0099_check}>,"
                        "${plugin_condition}"
                    ">"
                    ":$<TARGET_OBJECTS:${plugin_object_library}>"
                ">"
            )
            list(APPEND plugin_objects "${plugin_objects_genex}")
        endforeach()
    endforeach()
    set(${out_var} "${plugin_objects}" PARENT_SCOPE)
endfunction()

function(__qt_internal_collect_object_libraries_recursively out_var target initial_target)
    get_property(processed_object_libraries GLOBAL PROPERTY _qt_processed_object_libraries)

    set(interface_libs "")
    set(libs "")
    if(NOT "${target}" STREQUAL "${initial_target}")
        get_target_property(interface_libs ${target} INTERFACE_LINK_LIBRARIES)
    endif()
    get_target_property(type ${target} TYPE)
    if(NOT type STREQUAL "INTERFACE_LIBRARY")
        get_target_property(libs ${target} LINK_LIBRARIES)
    endif()

    set(object_libraries "")
    foreach(lib IN LISTS libs interface_libs)
        # Extract possible target from exported LINK_ONLY dependencies.
        # This is super important for traversing backing library dependencies of qml plugins.
        if(lib MATCHES "^\\$<LINK_ONLY:(.*)>$")
            set(lib "${CMAKE_MATCH_1}")
        endif()
        if(TARGET ${lib})
            _qt_internal_dealias_target(lib)

            if(${lib} IN_LIST processed_object_libraries)
                continue()
            else()
                list(APPEND processed_object_libraries ${lib})
                set_property(GLOBAL APPEND PROPERTY _qt_processed_object_libraries ${lib})
            endif()

            get_target_property(is_qt_propagated_object_library ${lib}
                _is_qt_propagated_object_library
            )
            if(is_qt_propagated_object_library)
                list(APPEND object_libraries ${lib})
            else()
                __qt_internal_collect_object_libraries_recursively(next_level_object_libraries
                    ${lib}
                    ${initial_target}
                )
                list(APPEND object_libraries ${next_level_object_libraries})
            endif()
        endif()
    endforeach()
    set(${out_var} "${object_libraries}" PARENT_SCOPE)
endfunction()

# Sets out_var to to TRUE if the target was marked to not be promoted to global scope.
function(_qt_internal_should_not_promote_package_target_to_global target out_var)
    get_property(should_not_promote TARGET "${target}" PROPERTY _qt_no_promote_global)
    set("${out_var}" "${should_not_promote}" PARENT_SCOPE)
endfunction()

# This function recursively walks transitive link libraries of the given target
# and promotes encountered 3rd party targets to be IMPORTED_GLOBAL if they are not.
#
# This is required for .prl file generation in top-level builds, to make sure that imported 3rd
# party library targets in any repo are made global, so there are no scoping issues.
#
# The promotion needs to happen in the same directory scope where the imported target is
# first created.
#
# Uses __qt_internal_walk_libs.
function(_qt_internal_promote_3rd_party_link_targets_to_global target)
    __qt_internal_walk_libs("${target}" _discarded_out_var _discarded_out_var_2
                            "qt_find_package_targets_dict" "promote_3rd_party_global")
endfunction()

# Check if a target is an internal target (one added by qt_internal_* API, executables, libraries,
# etc).
function(_qt_internal_is_internal_target target out_var)
    get_target_property(is_internal ${target} _qt_is_internal_target)
    if(is_internal)
        set(value TRUE)
    else()
        set(value FALSE)
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

# Check if a target should never be promoted to global.
# Some targets like the Platform target is public, and can't have _qt_is_internal_target set.
# But we still want to avoid promoting it to global. Setting this property achieves that.
function(_qt_internal_should_skip_3rd_party_global_promotion target out_var)
    get_target_property(should_skip ${target} _qt_should_skip_3rd_party_global_promotion)
    if(should_skip)
        set(value TRUE)
    else()
        set(value FALSE)
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

# Tries to promote any non-global imported target to global scope.
function(__qt_internal_promote_target_to_global target)
    get_property(is_global TARGET ${target} PROPERTY IMPORTED_GLOBAL)
    if(NOT is_global)
        message(DEBUG "Promoting target to global: '${target}'")
        set_property(TARGET ${target} PROPERTY IMPORTED_GLOBAL TRUE)
    endif()
endfunction()

# Promotes a 3rd party provided target to global, which was found by qt_find_package or
# _qt_internal_find_third_party_dependencies.
# Only does it when building Qt, but not when building user projects.
function(_qt_internal_promote_3rd_party_provided_target_and_3rd_party_deps_to_global target)
    # Return early if building a user project, and not Qt.
    # QT_BUILDING_QT is set when building a qt repo, but we also check for QT_REPO_MODULE_VERSION,
    # which is set in .cmake.conf, because _qt_internal_find_third_party_dependencies is called
    # before QT_BUILDING_QT is set.
    if(NOT (QT_BUILDING_QT OR QT_REPO_MODULE_VERSION))
        return()
    endif()

    # Return early if the provided target does not exist, which can happen in the case of zstd,
    # where we list multiple possible target names, but only some will be available.
    if(NOT TARGET "${target}")
        return()
    endif()

    get_property(is_global TARGET "${target}" PROPERTY IMPORTED_GLOBAL)
    _qt_internal_should_not_promote_package_target_to_global("${target}" should_not_promote)
    if(NOT is_global AND NOT should_not_promote)
        _qt_internal_promote_3rd_party_target_to_global(${target})
        _qt_internal_promote_3rd_party_link_targets_to_global("${target}")
    endif()
endfunction()

# Tries to promote a non-global imported 3rd party target to global scope.
# 3rd party targets are usually system library targets.
# - targets that were not created by qt_internal_add_foo commands
# - targets that don't have the should_skip_global_promotion property
function(_qt_internal_promote_3rd_party_target_to_global target)
    get_property(is_global TARGET ${target} PROPERTY IMPORTED_GLOBAL)

    if(NOT is_global)
        _qt_internal_is_internal_target("${target}" is_internal)
        _qt_internal_should_skip_3rd_party_global_promotion("${target}" should_skip)
        if(NOT is_internal AND NOT should_skip)
            __qt_internal_promote_target_to_global("${target}")
        endif()
    endif()
endfunction()

function(__qt_internal_promote_target_to_global_checked target)
    # With CMake version 3.21 we use a different mechanism that allows us to promote all targets
    # within a scope.
    if(QT_PROMOTE_TO_GLOBAL_TARGETS AND CMAKE_VERSION VERSION_LESS 3.21)
        __qt_internal_promote_target_to_global(${target})
    endif()
endfunction()

function(__qt_internal_promote_targets_in_dir_scope_to_global)
    # IMPORTED_TARGETS got added in 3.21.
    if(CMAKE_VERSION VERSION_LESS 3.21)
        return()
    endif()

    get_directory_property(targets IMPORTED_TARGETS)
    foreach(target IN LISTS targets)
        __qt_internal_promote_target_to_global(${target})
    endforeach()
endfunction()

function(__qt_internal_promote_targets_in_dir_scope_to_global_checked)
    if(QT_PROMOTE_TO_GLOBAL_TARGETS)
        __qt_internal_promote_targets_in_dir_scope_to_global()
    endif()
endfunction()

# This function ends up being called multiple times as part of a find_package(Qt6Foo) call,
# due sub-packages depending on the Qt6 package. Ensure the finalizer is ran only once per
# directory scope.
function(__qt_internal_defer_promote_targets_in_dir_scope_to_global)
    get_directory_property(is_deferred _qt_promote_targets_is_deferred)
    if(NOT is_deferred)
        set_property(DIRECTORY PROPERTY _qt_promote_targets_is_deferred TRUE)

        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
            cmake_language(DEFER CALL __qt_internal_promote_targets_in_dir_scope_to_global_checked)
        endif()
    endif()
endfunction()

function(_qt_internal_set_up_static_runtime_library target)
    if(QT_FEATURE_static_runtime)
        if(MSVC)
            set_property(TARGET ${target} PROPERTY
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        elseif(MINGW)
            get_target_property(target_type ${target} TYPE)
            if(target_type STREQUAL "EXECUTABLE")
                set(link_option PRIVATE)
            else()
                set(link_option INTERFACE)
            endif()
            if(CLANG)
                target_link_options(${target} ${link_option} "LINKER:-Bstatic")
            else()
                target_link_options(${target} ${link_option} "-static")
            endif()
        endif()
    endif()
endfunction()

# Controls the QT_SKIP_WARNINGS_ARE_ERRORS property for the given target.
function(_qt_internal_set_skip_warnings_are_errors target value)
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()
    set_target_properties("${target}" PROPERTIES QT_SKIP_WARNINGS_ARE_ERRORS ${value})
endfunction()

function(_qt_internal_warn_about_example_add_subdirectory)
    # This is set by qt_build_repo_impl_examples() in QtBuildRepoHelpers.cmake, only for developer
    # builds, to catch examples that are added via add_subdirectory instead of via
    # qt_internal_add_example.
    if(QT_WARN_ABOUT_EXAMPLE_ADD_SUBDIRECTORY)
        get_filename_component(dir_name "${PROJECT_SOURCE_DIR}" NAME)
        message(AUTHOR_WARNING
            "It looks like this example project was added via add_subdirectory instead of via "
            "qt_internal_add_example. This causes issues in certain build configurations. Please "
            "change the code to use\n  qt_internal_add_example(${dir_name})\ninstead."
        )
    endif()
endfunction()

# Mark source files as generated.
#
# This sets `GENERATED` property to TRUE, along with other Qt relevant properties,
# e.g. `SKIP_LINTING`.
#
# Synopsis
#
#   _qt_internal_set_source_file_generated(SOURCE <src1> ...
#       [CONFIGURE_GENERATED]
#       [SKIP_AUTOGEN]
#       [DIRECTORY <dirs> ...]
#       [TARGET_DIRECTORY <targets> ...]
#   )
#
# Arguments
#
# `SOURCES`
#   Source files that are generated.
#
#   Equivalent to `set_source_files_properties(<files>)`.
#
# `DIRECTORY`
#   Equivalent to `set_source_files_properties(DIRECTORY)`.
#
# `TARGET_DIRECTORY`
#   Equivalent to `set_source_files_properties(TARGET_DIRECTORY)`.
#
# `SKIP_AUTOGEN`
#   Set SKIP_AUTOGEN property to True as well.
#
# `CONFIGURE_GENERATED`
#   Files are generated with `configure_file`.
#   Does not set `GENERATED TRUE` property. This is needed to avoid removing the file when
#   running the clean target.
function(_qt_internal_set_source_file_generated)
    set(option_args
        SKIP_AUTOGEN
        CONFIGURE_GENERATED
    )
    set(single_args "")
    set(multi_args
        SOURCES
        DIRECTORY
        TARGET_DIRECTORY
    )

    cmake_parse_arguments(PARSE_ARGV 0 arg
            "${option_args}" "${single_args}" "${multi_args}"
    )
    # Parse required variables
    if(NOT arg_SOURCES AND QT_FEATURE_developer_build)
        message(WARNING
            "Unexpected call _qt_internal_set_source_file_generated with empty `SOURCES`."
        )
    endif()
    # Prepend again the appropriate keywords to pass to `set_source_files_properties`
    if(arg_DIRECTORY)
        list(PREPEND arg_DIRECTORY DIRECTORY)
    endif()
    if(arg_TARGET_DIRECTORY)
        list(PREPEND arg_TARGET_DIRECTORY TARGET_DIRECTORY)
    endif()

    # Construct the properties list
    set(properties "")
    if(NOT arg_CONFIGURE_GENERATED)
        list(APPEND properties
            GENERATED TRUE
        )
    endif()
    if(arg_SKIP_AUTOGEN)
        list(APPEND properties
            SKIP_AUTOGEN TRUE
        )
    endif()
    # Add SKIP_LINTING if possible. We do not add it unconditionally here to avoid
    # confusion when CMake ignores this variable.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.27" AND NOT QT_FEATURE_lint_generated_code)
        list(APPEND properties
            SKIP_LINTING TRUE
        )
    endif()

    set_source_files_properties(${arg_SOURCES}
        ${arg_DIRECTORY}
        ${arg_TARGET_DIRECTORY}
        PROPERTIES ${properties}
    )
endfunction()

# Get the real target checking for ALIASED_TARGET
function(_qt_internal_get_real_target out_var target)
    get_target_property(aliased_target "${target}" ALIASED_TARGET)
    if(aliased_target)
        set(${out_var} "${aliased_target}" PARENT_SCOPE)
    else()
        set(${out_var} "${target}" PARENT_SCOPE)
    endif()
endfunction()

# Helpful shortcut to `_qt_internal_get_real_target` if we just need to dealias
function(_qt_internal_dealias_target target_var)
    _qt_internal_get_real_target(${target_var} ${${target_var}})
    set(${target_var} "${${target_var}}" PARENT_SCOPE)
endfunction()
