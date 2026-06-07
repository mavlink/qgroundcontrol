# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was QtModuleConfigPrivate.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

cmake_minimum_required(VERSION 3.16...3.21)

include(CMakeFindDependencyMacro)

# Find required dependencies, if any.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/Qt6QmlWorkerScriptPrivateDependencies.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6QmlWorkerScriptPrivateDependencies.cmake")
    _qt_internal_suggest_dependency_debugging(QmlWorkerScriptPrivate
        __qt_QmlWorkerScriptPrivate_pkg ${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE)
endif()

# If *ConfigDependencies.cmake exists, the variable value will be defined there.
# Don't override it in that case.
if(NOT DEFINED "Qt6QmlWorkerScriptPrivate_FOUND")
    set("Qt6QmlWorkerScriptPrivate_FOUND" TRUE)
endif()

if(NOT __qt_QmlWorkerScript_always_load_private_module)
    _qt_internal_show_private_module_warning(QmlWorkerScriptPrivate)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6QmlWorkerScriptPrivate_FOUND)
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6QmlWorkerScriptPrivateTargets.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6QmlWorkerScriptPrivateAdditionalTargetInfo.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6QmlWorkerScriptPrivateExtraProperties.cmake"
        OPTIONAL)
endif()

if(TARGET Qt6::QmlWorkerScriptPrivate)
    if(NOT QT_NO_CREATE_VERSIONLESS_TARGETS)
        if(CMAKE_VERSION VERSION_LESS 3.18 OR QT_USE_OLD_VERSION_LESS_TARGETS)
            include("${CMAKE_CURRENT_LIST_DIR}/Qt6QmlWorkerScriptPrivateVersionlessTargets.cmake")
        else()
            include("${CMAKE_CURRENT_LIST_DIR}/Qt6QmlWorkerScriptPrivateVersionlessAliasTargets.cmake")
        endif()
    endif()
else()
    set(Qt6QmlWorkerScriptPrivate_FOUND FALSE)
    if(NOT DEFINED Qt6QmlWorkerScriptPrivate_NOT_FOUND_MESSAGE)
        set(Qt6QmlWorkerScriptPrivate_NOT_FOUND_MESSAGE
            "Target \"Qt6::QmlWorkerScriptPrivate\" was not found.")

        if(QT_NO_CREATE_TARGETS)
            string(APPEND Qt6QmlWorkerScriptPrivate_NOT_FOUND_MESSAGE
                "Possibly due to QT_NO_CREATE_TARGETS being set to TRUE and thus "
                "${CMAKE_CURRENT_LIST_DIR}/Qt6QmlWorkerScriptPrivateTargets.cmake was not "
                "included to define the target.")
        endif()
    endif()
endif()
