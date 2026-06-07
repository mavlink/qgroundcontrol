# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was QtModuleToolsConfig.cmake.in                            ########

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
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/Qt6QmlToolsDependencies.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6QmlToolsDependencies.cmake")
endif()

# If *Dependencies.cmake exists, the variable value will be defined there.
# Don't override it in that case.
if(NOT DEFINED "Qt6QmlTools_FOUND")
    set("Qt6QmlTools_FOUND" TRUE)
endif()

if (NOT QT_NO_CREATE_TARGETS AND Qt6QmlTools_FOUND)
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6QmlToolsTargets.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6QmlToolsAdditionalTargetInfo.cmake")
    if(NOT QT_NO_CREATE_VERSIONLESS_TARGETS)
        include("${CMAKE_CURRENT_LIST_DIR}/Qt6QmlToolsVersionlessTargets.cmake")
    endif()
endif()

foreach(extra_cmake_include )
    include("${CMAKE_CURRENT_LIST_DIR}/${extra_cmake_include}")
endforeach()


if(NOT QT_NO_CREATE_TARGETS AND Qt6QmlTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qmlaotstats)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6QmlTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qmlcachegen)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6QmlTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qmldom)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6QmlTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qmllint)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6QmlTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qmltc)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6QmlTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qmltyperegistrar)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6QmlTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qmljsrootgen)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6QmlTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qmlformat)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6QmlTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qmlimportscanner)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6QmlTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qmlprofiler)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6QmlTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qmltime)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6QmlTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qmlplugindump)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6QmlTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qmltestrunner)
endif()
set(Qt6QmlTools_TARGETS "Qt6::qmlaotstats;Qt6::qmlcachegen;Qt6::qmldom;Qt6::qmllint;Qt6::qmltc;Qt6::qmltyperegistrar;Qt6::qmljsrootgen;Qt6::qmlformat;Qt6::qmlimportscanner;Qt6::qmlprofiler;Qt6::qmltime;Qt6::qmlplugindump;Qt6::qmltestrunner")
