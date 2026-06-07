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
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/Qt6CoreToolsDependencies.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6CoreToolsDependencies.cmake")
endif()

# If *Dependencies.cmake exists, the variable value will be defined there.
# Don't override it in that case.
if(NOT DEFINED "Qt6CoreTools_FOUND")
    set("Qt6CoreTools_FOUND" TRUE)
endif()

if (NOT QT_NO_CREATE_TARGETS AND Qt6CoreTools_FOUND)
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6CoreToolsTargets.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6CoreToolsAdditionalTargetInfo.cmake")
    if(NOT QT_NO_CREATE_VERSIONLESS_TARGETS)
        include("${CMAKE_CURRENT_LIST_DIR}/Qt6CoreToolsVersionlessTargets.cmake")
    endif()
endif()

foreach(extra_cmake_include )
    include("${CMAKE_CURRENT_LIST_DIR}/${extra_cmake_include}")
endforeach()


if(NOT QT_NO_CREATE_TARGETS AND Qt6CoreTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::syncqt)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6CoreTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::moc)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6CoreTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::rcc)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6CoreTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::tracepointgen)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6CoreTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::tracegen)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6CoreTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::cmake_automoc_parser)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6CoreTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qlalr)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6CoreTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qtpaths)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6CoreTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::androiddeployqt)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6CoreTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::androidtestrunner)
endif()

if(NOT QT_NO_CREATE_TARGETS AND Qt6CoreTools_FOUND)
    __qt_internal_promote_target_to_global(Qt6::qmake)
endif()
set(Qt6CoreTools_TARGETS "Qt6::syncqt;Qt6::moc;Qt6::rcc;Qt6::tracepointgen;Qt6::tracegen;Qt6::cmake_automoc_parser;Qt6::qlalr;Qt6::qtpaths;Qt6::androiddeployqt;Qt6::androidtestrunner;Qt6::qmake")
