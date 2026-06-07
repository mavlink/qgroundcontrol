# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#
# QtDeclarative Specific extensions
#

include_guard(GLOBAL)

macro(qt_internal_get_internal_add_qml_module_keywords
        option_args single_args multi_args
        internal_option_args internal_single_args internal_multi_args)
    set(${option_args}
        DESIGNER_SUPPORTED
        FOLLOW_FOREIGN_VERSIONING
        NO_PLUGIN
        NO_PLUGIN_OPTIONAL
        NO_CREATE_PLUGIN_TARGET
        NO_GENERATE_PLUGIN_SOURCE
        NO_GENERATE_QMLTYPES
        NO_GENERATE_QMLDIR
        NO_GENERATE_EXTRA_QMLDIRS
        NO_LINT
        NO_CACHEGEN
        ENABLE_TYPE_COMPILER
        __QT_INTERNAL_STATIC_MODULE
        __QT_INTERNAL_SYSTEM_MODULE
    )
    set(${single_args}
        URI
        VERSION
        PLUGIN_TARGET
        TYPEINFO
        CLASS_NAME
        TYPE_COMPILER_NAMESPACE
    )
    set(${multi_args}
        QML_FILES
        RESOURCES
        IMPORTS
        IMPORT_PATH
        OPTIONAL_IMPORTS
        DEFAULT_IMPORTS
        DEPENDENCIES
        PAST_MAJOR_VERSIONS
    )
    # Args used by qt_internal_add_qml_module directly, which should not be passed to any other
    # functions.
    #
    # INSTALL_SOURCE_QMLTYPES takes a path to an existing plugins.qmltypes file that should be
    # installed.
    #
    # INSTALL_SOURCE_QMLDIR takes a path to an existing qmldir file that should be installed.
    set(${internal_option_args}
    )
    set(${internal_single_args}
        INSTALL_SOURCE_QMLTYPES
        INSTALL_SOURCE_QMLDIR
    )
    set(${internal_multi_args}
    )

endmacro()

# This function is essentially a wrapper around qt6_add_qml_module().
# It creates the targets explicitly and sets up internal properties before
# passing those targets to qt6_add_qml_module() for further updates.
# All keywords supported by qt_internal_add_module() can be used, as can most
# keywords for qt6_add_qml_module() except RESOURCE_PREFIX and
# OUTPUT_TARGETS.
#
# OUTPUT_DIRECTORY and INSTALL_DIRECTORY will be given more appropriate defaults
# if not provided by the caller. The defaults are usually what you want to use.
#
# - SOURCES is only passed through to qt_internal_add_plugin() or
#   qt_internal_add_module() but not to qt6_add_qml_module().
#
# See qt_internal_add_plugin() and qt6_add_qml_module() for the full set of
# supported keywords.
function(qt_internal_add_qml_module target)

    qt_internal_get_internal_add_module_keywords(
        module_option_args
        module_single_args
        module_multi_args
    )

    qt_internal_get_internal_add_qml_module_keywords(
        qml_module_option_args
        qml_module_single_args
        qml_module_multi_args
        qml_module_internal_option_args
        qml_module_internal_single_args
        qml_module_internal_multi_args
    )
    # TODO: Remove these once all repos have been updated to not use them
    set(ignore_option_args
        PLUGIN_OPTIONAL         # Now the default
        GENERATE_QMLTYPES       # Now the default
        INSTALL_QMLTYPES        # Now the default
    )

    set(option_args
        ${module_option_args}
        ${qml_module_option_args}
        ${ignore_option_args}
        ${qml_module_internal_option_args}
    )
    set(single_args
        ${module_single_args}
        ${qml_module_single_args}
        ${qml_module_internal_single_args}
    )
    set(multi_args
        ${module_multi_args}
        ${qml_module_multi_args}
        ${qml_module_internal_multi_args}
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${option_args}"
        "${single_args}"
        "${multi_args}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    set(QT_QML_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_QMLDIR}")
    string(REPLACE "." "/" target_path ${arg_URI})
    if(NOT arg_OUTPUT_DIRECTORY)
        set(arg_OUTPUT_DIRECTORY "${QT_QML_OUTPUT_DIRECTORY}/${target_path}")
    endif()
    if(NOT arg_INSTALL_DIRECTORY)
        set(arg_INSTALL_DIRECTORY "${INSTALL_QMLDIR}/${target_path}")
    endif()
    if(arg_NO_PLUGIN)
        unset(arg_PLUGIN_TARGET)
    elseif(NOT arg_PLUGIN_TARGET)
        set(arg_PLUGIN_TARGET ${target}plugin)
    endif()

    set(plugin_args "")
    if(arg_NO_PLUGIN OR NOT arg_PLUGIN_TARGET STREQUAL target)
        # Allow using an existing backing target.
        if(NOT TARGET ${target})
            # Create the backing target now to handle module-related things
            qt_remove_args(module_args
                ARGS_TO_REMOVE
                    ${ignore_option_args}
                    ${qml_module_option_args}
                    ${qml_module_single_args}
                    ${qml_module_multi_args}
                    ${qml_module_internal_option_args}
                    ${qml_module_internal_single_args}
                    ${qml_module_internal_multi_args}
                    OUTPUT_DIRECTORY
                    INSTALL_DIRECTORY
                ALL_ARGS
                    ${option_args}
                    ${single_args}
                    ${multi_args}
                ARGS
                    ${ARGN}
            )
            qt_internal_add_module(${target} ${module_args})
        elseif(arg_SOURCES)
            # If a module target was pre-created, we still need to pass the additional sources.
            target_sources(${target} PRIVATE ${arg_SOURCES})
        endif()
    else()
        # Since we are not creating a separate backing target, we have to pass
        # through the default args to the plugin target creation instead
        qt_internal_get_internal_add_plugin_keywords(
            plugin_option_args plugin_single_args plugin_multi_args
        )
        set(args_to_remove ${option_args} ${single_args} ${multi_args})
        list(REMOVE_ITEM args_to_remove
            ${plugin_option_args}
            ${plugin_single_args}
            ${plugin_multi_args}
        )
        qt_remove_args(plugin_args
            ARGS_TO_REMOVE
                ${args_to_remove}
                DEFAULT_IF
                OUTPUT_DIRECTORY
                INSTALL_DIRECTORY
                CLASS_NAME
            ALL_ARGS
                ${option_args}
                ${single_args}
                ${multi_args}
            ARGS
                ${ARGN}
        )
    endif()

    set(add_qml_module_args "")

    if(NOT arg_NO_PLUGIN AND NOT arg_NO_CREATE_PLUGIN_TARGET)
        # If the qt_internal_add_qml_module call didn't specify a CLASS_NAME, we need to pre-compute
        # it here and pass it along to qt_internal_add_plugin -> qt_add_plugin so that
        # qt_add_qml_plugin does not complain about differing class names (the default pre-computed
        # class name of a regular plugin and qml plugin are different).
        if(NOT arg_CLASS_NAME)
            _qt_internal_compute_qml_plugin_class_name_from_uri("${arg_URI}" arg_CLASS_NAME)
        endif()

        # If the module is explicitly marked as STATIC, the plugin should inherit that as well.
        set(static_plugin "")
        if(arg_STATIC)
            set(static_plugin "STATIC")
        endif()

        # Create plugin target now so we can set internal things
        list(APPEND plugin_args
            ${static_plugin}
            PLUGIN_TYPE qml_plugin
            DEFAULT_IF FALSE
            OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
            INSTALL_DIRECTORY ${arg_INSTALL_DIRECTORY}
            CLASS_NAME ${arg_CLASS_NAME}
        )

        qt_internal_add_plugin(${arg_PLUGIN_TARGET} ${plugin_args})

        # Use the plugin target name as the main part of the plugin basename.
        set(plugin_basename "${arg_PLUGIN_TARGET}")

        # If the target name already ends with a "plugin" suffix, remove it and re-add it to the end
        # of the base name after the infix.
        if(plugin_basename MATCHES "(.+)plugin$")
            set(plugin_basename "${CMAKE_MATCH_1}")
        endif()

        # Add a the infix if Qt was configured with one.
        if(QT_LIBINFIX)
            string(APPEND plugin_basename "${QT_LIBINFIX}")
        endif()

        # Add the "plugin" suffix after the infix.
        string(APPEND plugin_basename "plugin")

        # Lowercase the whole thing and use it as the basename of the plugin library.
        string(TOLOWER "${plugin_basename}" plugin_basename)
        set_target_properties(${arg_PLUGIN_TARGET} PROPERTIES
            OUTPUT_NAME "${plugin_basename}"
        )

        get_target_property(export_name ${arg_PLUGIN_TARGET} EXPORT_NAME)
        if(export_name)
            list(APPEND add_qml_module_args
                INSTALLED_PLUGIN_TARGET "${QT_CMAKE_EXPORT_NAMESPACE}::${export_name}"
            )
        else()
            list(APPEND add_qml_module_args
                INSTALLED_PLUGIN_TARGET "${QT_CMAKE_EXPORT_NAMESPACE}::${arg_PLUGIN_TARGET}"
                )
        endif()

        list(APPEND add_qml_module_args
            INSTALLED_BACKING_TARGET "${QT_CMAKE_EXPORT_NAMESPACE}::${target}"
        )

        if(NOT arg_PLUGIN_TARGET STREQUAL target)
            get_target_property(lib_type ${arg_PLUGIN_TARGET} TYPE)
            if(lib_type STREQUAL "STATIC_LIBRARY")
                # This is needed so that the dependency on the backing target
                # is included in the plugin's find_package() support.
                # The naming of backing targets and plugins don't typically
                # follow the pattern of other plugins with regard to Private
                # suffixes, so the dependency logic in qt_internal_add_plugin()
                # doesn't find these. For non-static builds, the private
                # dependency doesn't get exposed to find_package(), so we don't
                # have to make the dependency known for that case.
                set_property(TARGET ${arg_PLUGIN_TARGET} APPEND PROPERTY
                    _qt_target_deps "${INSTALL_CMAKE_NAMESPACE}${target}\;${PROJECT_VERSION}"
                )
            endif()
        endif()
    endif()

    # TODO: Check if we need arg_SOURCES in this condition
    if (arg_SOURCES AND NOT arg_TYPEINFO)
        set(arg_TYPEINFO "plugins.qmltypes")
    endif()

    # Pass through options if given (these are present/absent, not true/false)
    foreach(opt IN LISTS qml_module_option_args)
        if(arg_${opt})
            list(APPEND add_qml_module_args ${opt})
        endif()
    endforeach()

    # Pass through single and multi-value args as provided.
    foreach(arg IN LISTS qml_module_single_args qml_module_multi_args)
        if(DEFINED arg_${arg})
            list(APPEND add_qml_module_args ${arg} ${arg_${arg}})
        endif()
    endforeach()

    if (arg_FOLLOW_FOREIGN_VERSIONING)
        message(FATAL_ERROR "Do not set FOLLOW_FOREIGN_VERSIONING for module ${target}. It is already set by default for internal modules.")
    endif()

    get_target_property(qt_namespace ${QT_CMAKE_EXPORT_NAMESPACE}::Core QT_NAMESPACE)
    if(qt_namespace)
        list(APPEND add_qml_module_args NAMESPACE ${qt_namespace})
    endif()

    if (arg_ENABLE_TYPE_COMPILER AND NOT arg_TYPE_COMPILER_NAMESPACE)
        # if qmltc namespace is not specified explicitly, use Qt's namespace
        list(APPEND add_qml_module_args TYPE_COMPILER_NAMESPACE ${qt_namespace})
    endif()

    if((NOT arg_NO_GENERATE_QMLDIR) OR arg_INSTALL_SOURCE_QMLDIR)
        list(APPEND add_qml_module_args
            INSTALLED_QMLDIR_PATH
            "${arg_INSTALL_DIRECTORY}/qmldir")
    endif()

    # Update the backing and plugin targets with qml-specific things.
    qt6_add_qml_module(${target}
        ${add_qml_module_args}
        OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
        RESOURCE_PREFIX "/qt-project.org/imports"
        OUTPUT_TARGETS output_targets
        FOLLOW_FOREIGN_VERSIONING
    )

    if(TARGET "${target}" AND NOT target STREQUAL "Qml")
        qt_internal_add_autogen_sync_header_dependencies(${target} Qml)
    endif()
    if(TARGET "${arg_PLUGIN_TARGET}" AND NOT target STREQUAL arg_PLUGIN_TARGET)
        qt_internal_add_autogen_sync_header_dependencies(${arg_PLUGIN_TARGET} Qml)
    endif()

    if(output_targets)
        set(plugin_export_targets)
        set(backing_lib_export_targets)

        # Separate which output target should go to which export set.
        # The plugin initializer library should go to the plugin export set, the rest should go to
        # the backing lib export set.
        # In the case when the plugin target is the same as the backing lib target, all of the
        # output targets will go to the plugin export set.

        foreach(output_target IN LISTS output_targets)
            get_target_property(is_plugin_init ${output_target} _is_qt_plugin_init_target)
            if(is_plugin_init)
                list(APPEND plugin_export_targets ${output_target})

                # Plugin initializers associated with an internal module need the internal
                # platform flags.
                qt_internal_link_internal_platform_for_object_library("${output_target}"
                    PARENT_TARGET "${target}")
            else()
                list(APPEND backing_lib_export_targets ${output_target})
            endif()
        endforeach()

        if(backing_lib_export_targets)
            qt_install(TARGETS ${backing_lib_export_targets}
                EXPORT "${INSTALL_CMAKE_NAMESPACE}${target}Targets"
                DESTINATION "${arg_INSTALL_DIRECTORY}"
            )
            qt_internal_record_rcc_object_files(${target} "${backing_lib_export_targets}"
                INSTALL_DIRECTORY "${arg_INSTALL_DIRECTORY}"
            )

            qt_internal_add_targets_to_additional_targets_export_file(
                TARGETS ${backing_lib_export_targets}
                EXPORT_NAME_PREFIX "${INSTALL_CMAKE_NAMESPACE}${target}"
            )
        endif()

        if(arg_PLUGIN_TARGET AND plugin_export_targets)
            qt_install(TARGETS ${plugin_export_targets}
                EXPORT "${INSTALL_CMAKE_NAMESPACE}${arg_PLUGIN_TARGET}Targets"
                DESTINATION "${arg_INSTALL_DIRECTORY}"
            )

            qt_internal_add_targets_to_additional_targets_export_file(
                TARGETS ${plugin_export_targets}
                EXPORT_NAME_PREFIX "${INSTALL_CMAKE_NAMESPACE}${arg_PLUGIN_TARGET}"
            )
        endif()
    endif()

    if(DEFINED arg_QML_FILES OR DEFINED arg_RESOURCES)
        foreach(resource_file IN LISTS arg_QML_FILES arg_RESOURCES)
            __qt_get_relative_resource_path_for_file(file_resource_path ${resource_file})
            get_filename_component(resource_dir  ${file_resource_path} DIRECTORY)
            get_filename_component(resource_name ${file_resource_path} NAME)
            if(resource_dir)
                set(dest "${arg_INSTALL_DIRECTORY}/${resource_dir}")
            else()
                set(dest "${arg_INSTALL_DIRECTORY}")
            endif()
            qt_install(
                FILES ${resource_file}
                DESTINATION ${dest}
                RENAME ${resource_name}
            )
        endforeach()
    endif()

    if(NOT arg_NO_GENERATE_QMLTYPES)
        qt_install(
            FILES ${arg_OUTPUT_DIRECTORY}/$<TARGET_PROPERTY:${target},QT_QML_MODULE_TYPEINFO>
            DESTINATION "${arg_INSTALL_DIRECTORY}"
        )

        # Assign the install time metatypes file of the backing library to the plugin.
        # Only do it if the backing library is different from the plugin and we do generate
        # qml types.
        # The install time metatypes only apply to Qt's own qml plugins, not to user project
        # qml plugins.
        if(arg_PLUGIN_TARGET AND
                TARGET "${arg_PLUGIN_TARGET}" AND NOT target STREQUAL arg_PLUGIN_TARGET)
            _qt_internal_get_metatypes_install_dir(
                ""
                "${INSTALL_ARCHDATADIR}"
                install_dir
            )

            _qt_internal_assign_install_metatypes_files_and_properties(
                "${arg_PLUGIN_TARGET}"
                INSTALL_DIR "${install_dir}"
            )
        endif()
    endif()

    if(NOT arg_NO_GENERATE_QMLDIR)
        qt_install(
            FILES ${arg_OUTPUT_DIRECTORY}/qmldir
            DESTINATION "${arg_INSTALL_DIRECTORY}"
        )

        get_target_property(extra_qmldirs ${target} _qt_internal_extra_qmldirs)
        if(extra_qmldirs)
            foreach(extra_qmldir IN LISTS extra_qmldirs)
                __qt_get_relative_resource_path_for_file(qmldir_resource_path ${extra_qmldir})
                get_filename_component(qmldir_dir ${qmldir_resource_path} DIRECTORY)
                qt_install(
                    FILES ${extra_qmldir}
                    DESTINATION "${arg_INSTALL_DIRECTORY}/${qmldir_dir}"
                )
            endforeach()
        endif()
    endif()

    if(arg_INSTALL_SOURCE_QMLTYPES)
        message(AUTHOR_WARNING
            "INSTALL_SOURCE_QMLTYPES option is deprecated and should not be used. "
            "Please port your module to use declarative type registration.")

        set(files ${arg_INSTALL_SOURCE_QMLTYPES})
        if(QT_WILL_INSTALL)
            install(
                FILES ${files}
                DESTINATION "${arg_INSTALL_DIRECTORY}"
            )
        else()
            file(
                COPY ${files}
                DESTINATION "${arg_OUTPUT_DIRECTORY}"
            )
        endif()
    endif()

    if(arg_INSTALL_SOURCE_QMLDIR)
        message(AUTHOR_WARNING
            "INSTALL_SOURCE_QMLDIR option is deprecated and should not be used. "
            "Please port your module to use declarative type registration.")

        set(files ${arg_INSTALL_SOURCE_QMLDIR})
        if(QT_WILL_INSTALL)
            install(
                FILES ${files}
                DESTINATION "${arg_INSTALL_DIRECTORY}"
            )
        else()
            file(
                COPY ${files}
                DESTINATION "${arg_OUTPUT_DIRECTORY}"
            )
        endif()
    endif()
endfunction()
