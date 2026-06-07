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
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/Qt6QuickControls2FluentWinUI3StyleImplPrivateDependencies.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6QuickControls2FluentWinUI3StyleImplPrivateDependencies.cmake")
    _qt_internal_suggest_dependency_debugging(QuickControls2FluentWinUI3StyleImplPrivate
        __qt_QuickControls2FluentWinUI3StyleImplPrivate_pkg ${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE)
endif()

# If *ConfigDependencies.cmake exists, the variable value will be defined there.
# Don't override it in that case.
if(NOT DEFINED "Qt6QuickControls2FluentWinUI3StyleImplPrivate_FOUND")
    set("Qt6QuickControls2FluentWinUI3StyleImplPrivate_FOUND" TRUE)
endif()

if(NOT __qt_QuickControls2FluentWinUI3StyleImpl_always_load_private_module)
    _qt_internal_show_private_module_warning(QuickControls2FluentWinUI3StyleImplPrivate)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6QuickControls2FluentWinUI3StyleImplPrivate_FOUND)
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6QuickControls2FluentWinUI3StyleImplPrivateTargets.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6QuickControls2FluentWinUI3StyleImplPrivateAdditionalTargetInfo.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6QuickControls2FluentWinUI3StyleImplPrivateExtraProperties.cmake"
        OPTIONAL)
endif()

if(TARGET Qt6::QuickControls2FluentWinUI3StyleImplPrivate)
    if(NOT QT_NO_CREATE_VERSIONLESS_TARGETS)
        if(CMAKE_VERSION VERSION_LESS 3.18 OR QT_USE_OLD_VERSION_LESS_TARGETS)
            include("${CMAKE_CURRENT_LIST_DIR}/Qt6QuickControls2FluentWinUI3StyleImplPrivateVersionlessTargets.cmake")
        else()
            include("${CMAKE_CURRENT_LIST_DIR}/Qt6QuickControls2FluentWinUI3StyleImplPrivateVersionlessAliasTargets.cmake")
        endif()
    endif()
else()
    set(Qt6QuickControls2FluentWinUI3StyleImplPrivate_FOUND FALSE)
    if(NOT DEFINED Qt6QuickControls2FluentWinUI3StyleImplPrivate_NOT_FOUND_MESSAGE)
        set(Qt6QuickControls2FluentWinUI3StyleImplPrivate_NOT_FOUND_MESSAGE
            "Target \"Qt6::QuickControls2FluentWinUI3StyleImplPrivate\" was not found.")

        if(QT_NO_CREATE_TARGETS)
            string(APPEND Qt6QuickControls2FluentWinUI3StyleImplPrivate_NOT_FOUND_MESSAGE
                "Possibly due to QT_NO_CREATE_TARGETS being set to TRUE and thus "
                "${CMAKE_CURRENT_LIST_DIR}/Qt6QuickControls2FluentWinUI3StyleImplPrivateTargets.cmake was not "
                "included to define the target.")
        endif()
    endif()
endif()
