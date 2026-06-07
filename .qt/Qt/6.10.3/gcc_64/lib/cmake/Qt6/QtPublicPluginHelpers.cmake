# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Gets the qt plugin type of the given plugin into out_var_plugin_type.
# Also sets out_var_has_plugin_type to TRUE or FALSE depending on whether the plugin type was found.
function(__qt_internal_plugin_get_plugin_type
         plugin_target
         out_var_has_plugin_type
         out_var_plugin_type)
    set(has_plugin_type TRUE)

    set(plugin_target_versioned "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")

    get_target_property(_plugin_type "${plugin_target_versioned}" QT_PLUGIN_TYPE)
    if(NOT _plugin_type)
        message("Warning: plugin ${plugin_target_versioned} has no plugin type set, skipping.")
        set(has_plugin_type FALSE)
    else()
        set(${out_var_plugin_type} "${_plugin_type}" PARENT_SCOPE)
    endif()

    set(${out_var_has_plugin_type} "${has_plugin_type}" PARENT_SCOPE)
endfunction()

# Gets the qt plugin class name of the given target into out_var.
function(__qt_internal_plugin_has_class_name plugin_target out_var)
    set(plugin_target_versioned "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")

    get_target_property(classname "${plugin_target_versioned}" QT_PLUGIN_CLASS_NAME)
    if(NOT classname)
        message("Warning: plugin ${plugin_target_versioned} has no class name, skipping.")
    endif()

    # If unset, it will be -NOTFOUND and still evaluate to false.
    set(${out_var} "${classname}" PARENT_SCOPE)
endfunction()

# Constructs a generator expression which decides whether a plugin will be used.
#
# The conditions are based on the various properties set in qt_import_plugins.

# All the TARGET_PROPERTY genexes are evaluated in the context of the currently linked target,
# unless the TARGET argument is given.
#
# The genex is saved into out_var.
function(__qt_internal_get_static_plugin_condition_genex
         plugin_target_unprefixed
         out_var)
    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(plugin_target "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target_unprefixed}")
    set(plugin_target_versionless "Qt::${plugin_target_unprefixed}")

    get_target_property(_plugin_type "${plugin_target}" QT_PLUGIN_TYPE)

    set(target_infix "")
    if(arg_TARGET)
        set(target_infix "${arg_TARGET},")
    endif()

    set(_default_plugins_are_enabled
        "$<NOT:$<STREQUAL:$<GENEX_EVAL:$<TARGET_PROPERTY:${target_infix}QT_DEFAULT_PLUGINS>>,0>>")
    set(_manual_plugins_genex "$<GENEX_EVAL:$<TARGET_PROPERTY:${target_infix}QT_PLUGINS>>")
    set(_no_plugins_genex "$<GENEX_EVAL:$<TARGET_PROPERTY:${target_infix}QT_NO_PLUGINS>>")

    # Plugin genex marker for prl processing.
    set(_is_plugin_marker_genex "$<BOOL:QT_IS_PLUGIN_GENEX>")

    set(_plugin_is_default "$<TARGET_PROPERTY:${plugin_target},QT_DEFAULT_PLUGIN>")

    # The code in here uses the properties defined in qt_import_plugins (Qt6CoreMacros.cmake)

    # INCLUDE
    set(_plugin_is_whitelisted "$<IN_LIST:${plugin_target},${_manual_plugins_genex}>")
    set(_plugin_versionless_is_whitelisted
        "$<IN_LIST:${plugin_target_versionless},${_manual_plugins_genex}>")

    # Note: qt_import_plugins sets the QT_PLUGINS_${_plugin_type} to "-"
    # when excluding it with EXCLUDE_BY_TYPE,
    # which ensures that no plug-in will be supported unless explicitly re-added afterwards.
    string(CONCAT _plugin_is_not_blacklisted
        "$<AND:"
            "$<NOT:" # EXCLUDE
                "$<IN_LIST:${plugin_target},${_no_plugins_genex}>"
            ">,"
            "$<NOT:"
                "$<IN_LIST:${plugin_target_versionless},${_no_plugins_genex}>"
            ">,"
            # Excludes both plugins targeted by EXCLUDE_BY_TYPE and not included in
            # INCLUDE_BY_TYPE.
            "$<STREQUAL:,$<GENEX_EVAL:$<TARGET_PROPERTY:${target_infix}QT_PLUGINS_${_plugin_type}>>>"
        ">"
    )

    # Support INCLUDE_BY_TYPE
    string(CONCAT _plugin_is_in_type_whitelist
        "$<IN_LIST:"
            "${plugin_target},"
            "$<GENEX_EVAL:"
                "$<TARGET_PROPERTY:${target_infix}QT_PLUGINS_${_plugin_type}>"
            ">"
        ">"
    )
    string(CONCAT _plugin_versionless_is_in_type_whitelist
        "$<IN_LIST:"
            "${plugin_target_versionless},"
            "$<GENEX_EVAL:"
                "$<TARGET_PROPERTY:${target_infix}QT_PLUGINS_${_plugin_type}>"
            ">"
        ">"
    )

    # No point in linking the plugin initialization source file into static libraries. The
    # initialization symbol will be discarded by the linker when the static lib is linked into an
    # executable or shared library, because nothing is referencing the global static symbol.
    set(type_genex "$<TARGET_PROPERTY:${target_infix}TYPE>")
    set(no_static_genex "$<NOT:$<STREQUAL:${type_genex},STATIC_LIBRARY>>")

    # Complete condition that defines whether a static plugin is linked
    string(CONCAT _plugin_condition
        "$<BOOL:$<AND:"
            "${_is_plugin_marker_genex},"
            "${no_static_genex},"
            "$<OR:"
                "${_plugin_is_whitelisted},"
                "${_plugin_versionless_is_whitelisted},"
                "${_plugin_is_in_type_whitelist},"
                "${_plugin_versionless_is_in_type_whitelist},"
                "$<AND:"
                    "${_default_plugins_are_enabled},"
                    "${_plugin_is_default},"
                    "${_plugin_is_not_blacklisted}"
                ">"
            ">"
        ">>"
    )

    set(${out_var} "${_plugin_condition}" PARENT_SCOPE)
endfunction()

# Wraps the genex condition to evaluate to true only when using the regular plugin importing mode
# (not finalizer mode).
function(__qt_internal_get_plugin_condition_regular_mode plugin_condition out_var)
    set(not_finalizer_mode "$<NOT:$<BOOL:$<TARGET_PROPERTY:_qt_static_plugins_finalizer_mode>>>")
    set(full_plugin_condition "$<AND:${plugin_condition},${not_finalizer_mode}>")
    set(${out_var} "${full_plugin_condition}" PARENT_SCOPE)
endfunction()

# Link plugin via usage requirements of associated Qt module.
function(__qt_internal_add_static_plugin_linkage plugin_target qt_module_target)
    __qt_internal_get_static_plugin_condition_genex("${plugin_target}" plugin_condition)
    __qt_internal_get_plugin_condition_regular_mode("${plugin_condition}" full_plugin_condition)

    set(plugin_target "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")

    # If this condition is true, we link against the plug-in
    set(plugin_genex "$<${full_plugin_condition}:${plugin_target}>")
    target_link_libraries(${qt_module_target} INTERFACE "${plugin_genex}")
endfunction()

# Generates C++ import macro source code for given plugin
function(__qt_internal_get_plugin_import_macro plugin_target out_var)
    set(plugin_target_prefixed "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")

    # Query the class name of plugin targets prefixed with a Qt namespace and without, this is
    # needed to support plugin object initializers created by user projects.
    set(class_name"")
    set(class_name_prefixed "")

    if(TARGET ${plugin_target})
        get_target_property(class_name "${plugin_target}" QT_PLUGIN_CLASS_NAME)
    endif()

    if(TARGET ${plugin_target_prefixed})
        get_target_property(class_name_prefixed "${plugin_target_prefixed}" QT_PLUGIN_CLASS_NAME)
    endif()

    if(NOT class_name AND NOT class_name_prefixed)
        message(FATAL_ERROR "No QT_PLUGIN_CLASS_NAME value on target: '${plugin_target}'")
    endif()

    # Qt prefixed target takes priority.
    if(class_name_prefixed)
        set(class_name "${class_name_prefixed}")
    endif()

    set(${out_var} "Q_IMPORT_PLUGIN(${class_name})\n" PARENT_SCOPE)
endfunction()

function(__qt_internal_get_plugin_include_prelude out_var)
    set(${out_var} "#include <QtPlugin>\n" PARENT_SCOPE)
endfunction()

# Set up plugin initialization via usage requirements of associated Qt module.
#
# Adds the plugin init object library as an INTERFACE source of the plugin target.
# This is similar to how it was done before, except instead of generating a C++ file and compiling
# it as part of the user project, we just specify the pre-compiled object file as an INTERFACE
# source so that user projects don't have to compile it. User project builds will thus be shorter.
function(__qt_internal_add_static_plugin_import_macro
        plugin_target
        qt_module_target
        qt_module_unprefixed)

    __qt_internal_get_static_plugin_init_target_name("${plugin_target}" plugin_init_target)
    set(plugin_init_target_namespaced "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_init_target}")

    __qt_internal_propagate_object_library(
        "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}"
        "${plugin_init_target_namespaced}"
        EXTRA_CONDITIONS
            "$<NOT:$<BOOL:$<TARGET_PROPERTY:_qt_static_plugins_finalizer_mode>>>"
    )
endfunction()

# Get target name of object library which is used to initialize a qt plugin.
function(__qt_internal_get_static_plugin_init_target_name plugin_target out_var)
    # Keep the target name short, so we don't hit long path issues on Windows.
    set(plugin_init_target "${plugin_target}_init")

    set(${out_var} "${plugin_init_target}" PARENT_SCOPE)
endfunction()

# Create an object library that initializes a static qt plugin.
#
# The object library contains a single generated C++ file that calls Q_IMPORT_PLUGIN(plugin_class).
# The object library is exported as part of the Qt build and consumed by user applications
# that link to qt plugins.
#
# The created target name is assigned to 'out_var_plugin_init_target'.
function(__qt_internal_add_static_plugin_init_object_library
        plugin_target
        out_var_plugin_init_target)

    __qt_internal_get_plugin_import_macro(${plugin_target} import_macro)
    __qt_internal_get_plugin_include_prelude(include_prelude)
    set(import_content "${include_prelude}${import_macro}")

    string(MAKE_C_IDENTIFIER "${plugin_target}" plugin_target_identifier)
    set(generated_qt_plugin_file_name
        "${CMAKE_CURRENT_BINARY_DIR}/${plugin_target_identifier}_init.cpp")

    file(GENERATE
        OUTPUT "${generated_qt_plugin_file_name}"
        CONTENT "${import_content}"
    )

    _qt_internal_set_source_file_generated(SOURCES "${generated_qt_plugin_file_name}")

    __qt_internal_get_static_plugin_init_target_name("${plugin_target}" plugin_init_target)

    qt6_add_library("${plugin_init_target}" OBJECT "${generated_qt_plugin_file_name}")
    target_link_libraries(${plugin_init_target}
        PRIVATE

        # Core provides the symbols needed by Q_IMPORT_PLUGIN.
        ${QT_CMAKE_EXPORT_NAMESPACE}::Core
    )

    set_property(TARGET ${plugin_init_target} PROPERTY _is_qt_plugin_init_target TRUE)
    set_property(TARGET ${plugin_init_target} APPEND PROPERTY
        EXPORT_PROPERTIES _is_qt_plugin_init_target
    )

    set(${out_var_plugin_init_target} "${plugin_init_target}" PARENT_SCOPE)
endfunction()

# Collect a list of genexes to link plugin libraries.
function(__qt_internal_collect_plugin_libraries plugin_targets out_var)
    set(plugins_to_link "")

    foreach(plugin_target ${plugin_targets})
        set(plugin_target_versioned "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")
        get_target_property(type "${plugin_target_versioned}" TYPE)
        if(NOT type STREQUAL STATIC_LIBRARY)
            continue()
        endif()

        __qt_internal_get_static_plugin_condition_genex(
            "${plugin_target}"
            plugin_condition)

        list(APPEND plugins_to_link "$<${plugin_condition}:${plugin_target_versioned}>")
    endforeach()

    set("${out_var}" "${plugins_to_link}" PARENT_SCOPE)
endfunction()

# Collect a list of genexes to link plugin initializer object libraries.
#
# The object libraries are only linked if the associated plugins are linked.
function(__qt_internal_collect_plugin_init_libraries plugin_targets out_var)
    set(plugin_inits_to_link "")

    foreach(plugin_target ${plugin_targets})
        set(plugin_target_versioned "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")
        get_target_property(type "${plugin_target_versioned}" TYPE)
        if(NOT type STREQUAL STATIC_LIBRARY)
            continue()
        endif()

        __qt_internal_get_static_plugin_condition_genex(
            "${plugin_target}"
            plugin_condition)

        __qt_internal_get_static_plugin_init_target_name("${plugin_target}" plugin_init_target)
        set(plugin_init_target_versioned "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_init_target}")

        list(APPEND plugin_inits_to_link "$<${plugin_condition}:${plugin_init_target_versioned}>")
    endforeach()

    set("${out_var}" "${plugin_inits_to_link}" PARENT_SCOPE)
endfunction()

# Collect a list of genexes to deploy plugin libraries.
function(__qt_internal_collect_plugin_library_files target plugin_targets out_var)
    set(library_files "")

    foreach(plugin_target ${plugin_targets})
        set(plugin_target_versioned "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")
        __qt_internal_get_static_plugin_condition_genex(
            "${plugin_target}"
            plugin_condition
            TARGET ${target}
        )

        set(target_genex "$<${plugin_condition}:${plugin_target_versioned}>")
        list(APPEND library_files "$<$<BOOL:${target_genex}>:$<TARGET_FILE:${target_genex}>>")
    endforeach()

    set("${out_var}" "${library_files}" PARENT_SCOPE)
endfunction()

# Collects all plugin targets discovered by walking the dependencies of ${target}.
#
# Walks immediate dependencies and their transitive dependencies.
# Plugins are collected by inspecting the _qt_plugins property found on any dependency Qt target.
function(__qt_internal_collect_plugin_targets_from_dependencies target out_var)
    set(dep_targets "")

    __qt_internal_collect_all_target_dependencies("${target}" dep_targets)

    set(plugin_targets "")
    foreach(dep_target ${dep_targets})
        get_target_property(plugins ${dep_target} _qt_plugins)
        if(plugins)
            list(APPEND plugin_targets ${plugins})
        endif()
    endforeach()

    # Plugins that are specified via qt_import_plugin's INCLUDE or INCLUDE_BY_TYPE can have
    # dependencies on Qt modules. These modules in turn might bring in more default plugins to link
    # So it's recursive. Do only one pass for now. Try to extract the included static plugins, walk
    # their public and private dependencies, check if any of them are Qt modules that provide more
    # plugins and extract the target names of those plugins.
    __qt_internal_collect_plugin_targets_from_dependencies_of_plugins(
        "${target}" recursive_plugin_targets)
    if(recursive_plugin_targets)
        list(APPEND plugin_targets ${recursive_plugin_targets})
    endif()
    list(REMOVE_DUPLICATES plugin_targets)

    set("${out_var}" "${plugin_targets}" PARENT_SCOPE)
endfunction()

# Helper to collect plugin targets from encountered module dependencies as a result of walking
# dependencies. These module dependencies might expose additional plugins.
function(__qt_internal_collect_plugin_targets_from_dependencies_of_plugins target out_var)
    set(assigned_plugins_overall "")

    get_target_property(assigned_qt_plugins "${target}" QT_PLUGINS)

    if(assigned_qt_plugins)
        foreach(assigned_qt_plugin ${assigned_qt_plugins})
            if(TARGET "${assigned_qt_plugin}")
                list(APPEND assigned_plugins_overall "${assigned_qt_plugin}")
            endif()
        endforeach()
    endif()

    get_target_property(assigned_qt_plugins_by_type "${target}" _qt_plugins_by_type)

    if(assigned_qt_plugins_by_type)
        foreach(assigned_qt_plugin ${assigned_qt_plugins_by_type})
            if(TARGET "${assigned_qt_plugin}")
                list(APPEND assigned_plugins_overall "${assigned_qt_plugin}")
            endif()
        endforeach()
    endif()

    set(plugin_targets "")
    foreach(target ${assigned_plugins_overall})
        __qt_internal_walk_libs(
            "${target}"
            dep_targets
            _discarded_out_var
            "qt_private_link_library_targets"
            "collect_targets")

        foreach(dep_target ${dep_targets})
            get_target_property(plugins ${dep_target} _qt_plugins)
            if(plugins)
                list(APPEND plugin_targets ${plugins})
            endif()
        endforeach()
    endforeach()

    list(REMOVE_DUPLICATES plugin_targets)

    set("${out_var}" "${plugin_targets}" PARENT_SCOPE)
endfunction()

# Main logic of finalizer mode.
function(__qt_internal_apply_plugin_imports_finalizer_mode target)
    # Process a target only once.
    get_target_property(processed ${target} _qt_plugin_finalizer_imports_processed)
    if(processed)
        return()
    endif()

    # By default if the project hasn't explicitly opted in or out, use finalizer mode.
    # The precondition for this is that qt_finalize_target was called (either explicitly by the user
    # or auto-deferred by CMake 3.19+).
    __qt_internal_check_finalizer_mode("${target}"
        use_finalizer_mode
        static_plugins
        DEFAULT_VALUE "TRUE"
    )

    if(NOT use_finalizer_mode)
        return()
    endif()

    __qt_internal_collect_plugin_targets_from_dependencies("${target}" plugin_targets)
    __qt_internal_collect_plugin_init_libraries("${plugin_targets}" init_libraries)
    __qt_internal_collect_plugin_libraries("${plugin_targets}" plugin_libraries)

    target_link_libraries(${target} PRIVATE "${plugin_libraries}" "${init_libraries}")

    set_target_properties(${target} PROPERTIES _qt_plugin_finalizer_imports_processed TRUE)
endfunction()

# Adds the specific plugin target to the INTERFACE_QT_PLUGIN_TARGETS transitive compile property.
# The property is then propagated to all targets that link the plugin_module_target and
# can be accessed using $<TARGET_PROPERTY:tgt_name,QT_PLUGIN_TARGETS> genex.
#
# Note: this is only supported in CMake versions 3.30 and higher.
function(__qt_internal_add_interface_plugin_target plugin_module_target plugin_target)
    if(CMAKE_VERSION VERSION_LESS 3.30)
        return()
    endif()

    cmake_parse_arguments(arg "BUILD_ONLY" "" "" ${ARGN})
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    __qt_internal_get_static_plugin_condition_genex(${plugin_target} plugin_target_condition)
    string(JOIN "" plugin_target_name_wrapped
        "$<${plugin_target_condition}:"
            "$<TARGET_NAME:${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}>"
        ">"
    )

    if(arg_BUILD_ONLY)
        set(plugin_target_name_wrapped "$<BUILD_LOCAL_INTERFACE:${plugin_target_name_wrapped}>")
    endif()

    set_property(TARGET ${plugin_module_target}
        APPEND PROPERTY INTERFACE_QT_PLUGIN_TARGETS ${plugin_target_name_wrapped})
endfunction()

# TODO: Figure out how to do this more reliably, instead of parsing the file name to get
# the target name.
function(__qt_internal_get_target_name_from_plugin_config_file_name
        config_file_path
        package_prefix_regex
        out_var)
    string(REGEX REPLACE
        "^.*/${QT_CMAKE_EXPORT_NAMESPACE}(${package_prefix_regex})Config.cmake$"
        "\\1"
        target "${config_file_path}")

    set(${out_var} "${target}" PARENT_SCOPE)
endfunction()

# Include CMake plugin packages that belong to the Qt module ${target} and initialize automatic
# linkage of the plugins in static builds.
# The variables inside the macro have to be named unique to the module because an included Plugin
# file might look up another module dependency that calls the same macro before the first one
# has finished processing, which can silently override the values if the variables are not unique.
macro(__qt_internal_include_plugin_packages target)
    set(__qt_${target}_plugin_module_target "${QT_CMAKE_EXPORT_NAMESPACE}::${target}")
    set(__qt_${target}_plugins "")

    # Properties can't be set on aliased targets, so make sure to unalias the target. This is needed
    # when Qt examples are built as part of the Qt build itself.
    _qt_internal_dealias_target(__qt_${target}_plugin_module_target)

    # Ensure that QT_PLUGIN_TARGETS is a known transitive compile property. Works with CMake
    # versions >= 3.30.
    _qt_internal_add_transitive_property(${__qt_${target}_plugin_module_target}
        COMPILE QT_PLUGIN_TARGETS)

    # Include all PluginConfig.cmake files and update the _qt_plugins and QT_PLUGINS property of
    # the module. The underscored version is the one we will use going forward to have compatibility
    # with INTERFACE libraries. QT_PLUGINS is now deprecated and only kept so that we don't break
    # existing projects using it (like CMake itself).
    file(GLOB __qt_${target}_plugin_config_files
        "${CMAKE_CURRENT_LIST_DIR}/${QT_CMAKE_EXPORT_NAMESPACE}*PluginConfig.cmake")
    foreach(__qt_${target}_plugin_config_file ${__qt_${target}_plugin_config_files})
        include("${__qt_${target}_plugin_config_file}")

        __qt_internal_get_target_name_from_plugin_config_file_name(
            "${__qt_${target}_plugin_config_file}"
            "(.*Plugin)"
            __qt_${target}_qt_plugin
        )

        if(TARGET "${QT_CMAKE_EXPORT_NAMESPACE}::${__qt_${target}_qt_plugin}")
            list(APPEND __qt_${target}_plugins ${__qt_${target}_qt_plugin})
            __qt_internal_add_interface_plugin_target(${__qt_${target}_plugin_module_target}
                ${__qt_${target}_qt_plugin})
        endif()
    endforeach()
    set_property(TARGET ${__qt_${target}_plugin_module_target}
                 PROPERTY _qt_plugins ${__qt_${target}_plugins})

    # TODO: Deprecated. Remove in Qt 7.
    set_property(TARGET ${__qt_${target}_plugin_module_target}
                 PROPERTY QT_PLUGINS ${__qt_${target}_plugins})

    get_target_property(__qt_${target}_have_added_plugins_already
        ${__qt_${target}_plugin_module_target} __qt_internal_plugins_added)
    if(__qt_${target}_have_added_plugins_already)
        return()
    endif()

    foreach(plugin_target ${__qt_${target}_plugins})
        __qt_internal_plugin_get_plugin_type("${plugin_target}" __has_plugin_type __plugin_type)
        if(NOT __has_plugin_type)
            continue()
        endif()

        __qt_internal_plugin_has_class_name("${plugin_target}" __has_class_name)
        if(NOT __has_class_name)
            continue()
        endif()

        set(plugin_target_versioned "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")

        if(NOT "${plugin_target}"
                IN_LIST QT_ALL_PLUGINS_FOUND_VIA_FIND_PACKAGE)

            # Old compatibility name.
            # TODO: Remove once all usages are ported.
            list(APPEND QT_ALL_PLUGINS_FOUND_BY_FIND_PACKAGE "${plugin_target}")

            # New name consistent with other such variables.
            list(APPEND QT_ALL_PLUGINS_FOUND_VIA_FIND_PACKAGE "${plugin_target}")
            list(APPEND QT_ALL_PLUGINS_VERSIONED_FOUND_VIA_FIND_PACKAGE
                "${plugin_target_versioned}")
        endif()

        if(NOT "${plugin_target}" IN_LIST QT_ALL_PLUGINS_FOUND_VIA_FIND_PACKAGE_${__plugin_type})
            # Old compatibility name.
            # TODO: Remove once all usages are ported.
            list(APPEND QT_ALL_PLUGINS_FOUND_BY_FIND_PACKAGE_${__plugin_type} "${plugin_target}")

            # New name consistent with other such variables.
            list(APPEND QT_ALL_PLUGINS_FOUND_VIA_FIND_PACKAGE_${__plugin_type} "${plugin_target}")
            list(APPEND
                QT_ALL_PLUGINS_VERSIONED_FOUND_VIA_FIND_PACKAGE_${__plugin_type}
                "${plugin_target_versioned}")
        endif()

        # Auto-linkage should be set up only for static plugins.
        get_target_property(type "${plugin_target_versioned}" TYPE)
        if(type STREQUAL STATIC_LIBRARY)
            __qt_internal_add_static_plugin_linkage(
                "${plugin_target}" "${__qt_${target}_plugin_module_target}")
            __qt_internal_add_static_plugin_import_macro(
                "${plugin_target}" ${__qt_${target}_plugin_module_target} "${target}")
        endif()
    endforeach()

    set_target_properties(
        ${__qt_${target}_plugin_module_target} PROPERTIES __qt_internal_plugins_added TRUE)
endmacro()

# Include Qt Qml plugin CMake packages that are present under the Qml package directory.
# TODO: Consider moving this to qtdeclarative somehow.
macro(__qt_internal_include_qml_plugin_packages)
    # Qml plugin targets might have dependencies on other qml plugin targets, but the Targets.cmake
    # files are included in the order that file(GLOB) returns, which means certain targets that are
    # referenced might not have been created yet, and ${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
    # might be set to a message saying those targets don't exist.
    #
    # Postpone checking of which targets don't exist until all Qml PluginConfig.cmake files have
    # been included, by including all the files one more time and checking for errors at each step.
    #
    # TODO: Find a better way to deal with this, perhaps by using find_package() instead of include
    # for the Qml PluginConfig.cmake files.

    # Distributions should probably change this default.
    if(NOT DEFINED QT_SKIP_AUTO_QML_PLUGIN_INCLUSION)
        set(QT_SKIP_AUTO_QML_PLUGIN_INCLUSION OFF)
    endif()

    set(__qt_qml_plugins_config_file_list "")
    set(__qt_qml_plugins_glob_prefixes "${CMAKE_CURRENT_LIST_DIR}")

    # Allow passing additional prefixes where we will glob for PluginConfig.cmake files.
    if(QT_ADDITIONAL_QML_PLUGIN_GLOB_PREFIXES)
        foreach(__qt_qml_plugin_glob_prefix IN LISTS QT_ADDITIONAL_QML_PLUGIN_GLOB_PREFIXES)
            if(__qt_qml_plugin_glob_prefix)
                list(APPEND __qt_qml_plugins_glob_prefixes "${__qt_qml_plugin_glob_prefix}")
            endif()
        endforeach()
    endif()

    list(REMOVE_DUPLICATES __qt_qml_plugins_glob_prefixes)

    foreach(__qt_qml_plugin_glob_prefix IN LISTS __qt_qml_plugins_glob_prefixes)
        file(GLOB __qt_qml_plugins_glob_config_file_list
            "${__qt_qml_plugin_glob_prefix}/QmlPlugins/${INSTALL_CMAKE_NAMESPACE}*Config.cmake")
        if(__qt_qml_plugins_glob_config_file_list)
            list(APPEND __qt_qml_plugins_config_file_list ${__qt_qml_plugins_glob_config_file_list})
        endif()
    endforeach()

    if (__qt_qml_plugins_config_file_list AND NOT QT_SKIP_AUTO_QML_PLUGIN_INCLUSION)
        # First round of inclusions ensure all qml plugin targets are brought into scope.
        foreach(__qt_qml_plugin_config_file ${__qt_qml_plugins_config_file_list})
            include(${__qt_qml_plugin_config_file})

            # Temporarily unset any failure markers and mark the Qml package as found.
            unset(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE)
            set(${CMAKE_FIND_PACKAGE_NAME}_FOUND TRUE)
        endforeach()

        # For the second round of inclusions, check and bail out early if there are errors.
        foreach(__qt_qml_plugin_config_file ${__qt_qml_plugins_config_file_list})
            include(${__qt_qml_plugin_config_file})

            __qt_internal_get_target_name_from_plugin_config_file_name(
                "${__qt_qml_plugin_config_file}"
                "(.*)"
                __qt_qml_plugin_target
            )
            set(__qt_qml_plugin_target_versioned
                "${QT_CMAKE_EXPORT_NAMESPACE}::${__qt_qml_plugin_target}")

            if(TARGET "${__qt_qml_plugin_target_versioned}"
                AND NOT "${__qt_qml_plugin_target}"
                    IN_LIST QT_ALL_QML_PLUGINS_FOUND_VIA_FIND_PACKAGE)
                list(APPEND QT_ALL_QML_PLUGINS_FOUND_VIA_FIND_PACKAGE "${__qt_qml_plugin_target}")
                list(APPEND QT_ALL_QML_PLUGINS_VERSIONED_FOUND_VIA_FIND_PACKAGE
                    "${__qt_qml_plugin_target_versioned}")
            endif()
            unset(__qt_qml_plugin_target)
            unset(__qt_qml_plugin_target_versioned)

            if(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE)
                string(APPEND ${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
                    "\nThe message was set in ${__qt_qml_plugin_config_file} ")
                set(${CMAKE_FIND_PACKAGE_NAME}_FOUND FALSE)
                return()
            endif()
        endforeach()
    endif()
endmacro()

