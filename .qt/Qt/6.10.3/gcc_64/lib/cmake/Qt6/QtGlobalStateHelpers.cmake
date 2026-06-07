# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_internal_clear_qt_repo_known_modules)
    set(QT_REPO_KNOWN_MODULES "" CACHE INTERNAL "Known current repo Qt modules" FORCE)
endfunction()

function(qt_internal_add_qt_repo_known_module)
    set(QT_REPO_KNOWN_MODULES ${QT_REPO_KNOWN_MODULES} ${ARGN}
        CACHE INTERNAL "Known current repo Qt modules" FORCE)
endfunction()

function(qt_internal_get_qt_repo_known_modules out_var)
    set("${out_var}" "${QT_REPO_KNOWN_MODULES}" PARENT_SCOPE)
endfunction()

# Gets the list of all known Qt modules both found and that were built as part of the
# current project.
function(qt_internal_get_qt_all_known_modules out_var)
    qt_internal_get_qt_repo_known_modules(repo_known_modules)
    set(known_modules ${QT_ALL_MODULES_FOUND_VIA_FIND_PACKAGE} ${repo_known_modules})
    list(REMOVE_DUPLICATES known_modules)
    set("${out_var}" "${known_modules}" PARENT_SCOPE)
endfunction()

macro(qt_internal_set_qt_known_plugins)
    set(QT_KNOWN_PLUGINS ${ARGN} CACHE INTERNAL "Known Qt plugins" FORCE)
endmacro()

### Global plug-in types handling ###
# QT_REPO_KNOWN_PLUGIN_TYPES and QT_ALL_PLUGIN_TYPES_FOUND_VIA_FIND_PACKAGE
# hold a list of plug-in types (e.G. "imageformats;bearer")
function(qt_internal_clear_qt_repo_known_plugin_types)
    set(QT_REPO_KNOWN_PLUGIN_TYPES "" CACHE INTERNAL "Known current repo Qt plug-in types" FORCE)
endfunction()

function(qt_internal_add_plugin_types target plugin_types)
    # Update the variable containing the list of plugins for the given plugin type
    foreach(plugin_type ${plugin_types})
        qt_get_sanitized_plugin_type("${plugin_type}" plugin_type)
        set_property(TARGET "${target}" APPEND PROPERTY MODULE_PLUGIN_TYPES "${plugin_type}")
        qt_internal_add_qt_repo_known_plugin_types("${plugin_type}")
    endforeach()

    # Save the non-sanitized plugin type values for qmake consumption via .pri files.
    set_property(TARGET "${target}"
                 APPEND PROPERTY QMAKE_MODULE_PLUGIN_TYPES "${plugin_types}")

    # Export the plugin types.
    get_property(export_properties TARGET ${target} PROPERTY EXPORT_PROPERTIES)
    if(NOT MODULE_PLUGIN_TYPES IN_LIST export_properties)
        set_property(TARGET ${target} APPEND PROPERTY
            EXPORT_PROPERTIES MODULE_PLUGIN_TYPES)
    endif()
endfunction()

function(qt_internal_add_qt_repo_known_plugin_types)
    set(QT_REPO_KNOWN_PLUGIN_TYPES ${QT_REPO_KNOWN_PLUGIN_TYPES} ${ARGN}
        CACHE INTERNAL "Known current repo Qt plug-in types" FORCE)
endfunction()

function(qt_internal_get_qt_repo_known_plugin_types out_var)
    set("${out_var}" "${QT_REPO_KNOWN_PLUGIN_TYPES}" PARENT_SCOPE)
endfunction()

function(qt_internal_get_qt_all_known_plugin_types out_var)
    qt_internal_get_qt_repo_known_plugin_types(repo_known_plugin_types)
    set(known ${QT_ALL_PLUGIN_TYPES_FOUND_VIA_FIND_PACKAGE} ${repo_known_plugin_types})
    list(REMOVE_DUPLICATES known)
    set("${out_var}" "${known}" PARENT_SCOPE)
endfunction()

macro(qt_internal_append_known_modules_with_tools module)
    if(NOT ${module} IN_LIST QT_KNOWN_MODULES_WITH_TOOLS)
        set(QT_KNOWN_MODULES_WITH_TOOLS "${QT_KNOWN_MODULES_WITH_TOOLS};${module}"
            CACHE INTERNAL "Known Qt modules with tools" FORCE)
        set(QT_KNOWN_MODULE_${module}_TOOLS ""
            CACHE INTERNAL "Known Qt module ${module} tools" FORCE)
    endif()
endmacro()

macro(qt_internal_append_known_module_tool module tool)
    if(NOT ${tool} IN_LIST QT_KNOWN_MODULE_${module}_TOOLS)
        list(APPEND QT_KNOWN_MODULE_${module}_TOOLS "${tool}")
        set(QT_KNOWN_MODULE_${module}_TOOLS "${QT_KNOWN_MODULE_${module}_TOOLS}"
            CACHE INTERNAL "Known Qt module ${module} tools" FORCE)
    endif()
endmacro()
