# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include_guard(DIRECTORY)

if(DEFINED QT_REPO_DEPENDENCIES
    AND NOT QT_INTERNAL_BUILD_STANDALONE_PARTS
    AND NOT QT_BUILD_STANDALONE_TESTS)
    # We're building a Qt repository.
    # Skip this plugin if it has not been provided by one of this repo's dependencies.
    string(TOLOWER "QtBase" lower_case_project_name)
    if(NOT lower_case_project_name IN_LIST QT_REPO_DEPENDENCIES)
        return()
    endif()
endif()




####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was QtPluginConfig.cmake.in                            ########

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

if (NOT QT_NO_CREATE_TARGETS)
    # Find required dependencies, if any.
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/Qt6QSQLiteDriverPluginDependencies.cmake")
        include("${CMAKE_CURRENT_LIST_DIR}/Qt6QSQLiteDriverPluginDependencies.cmake")
    else()
        set(QSQLiteDriverPlugin_FOUND TRUE)
    endif()

    if(QSQLiteDriverPlugin_FOUND)
        include("${CMAKE_CURRENT_LIST_DIR}/Qt6QSQLiteDriverPluginTargets.cmake")
        include("${CMAKE_CURRENT_LIST_DIR}/Qt6QSQLiteDriverPluginAdditionalTargetInfo.cmake")
    endif()
endif()
