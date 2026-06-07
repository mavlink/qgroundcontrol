# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapSystemDoubleConversion::WrapSystemDoubleConversion)
    set(WrapSystemDoubleConversion_FOUND ON)
    return()
endif()

set(WrapSystemDoubleConversion_REQUIRED_VARS "__double_conversion_found")

# Find either Config package or Find module.
# Upstream can be built either with CMake and then provides a Config file, or with Scons in which
# case there's no Config file.
# A Find module might be provided by a 3rd party, for example Conan might generate a Find module.
find_package(double-conversion ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} QUIET)
set(__double_conversion_target_name "double-conversion::double-conversion")
if(double-conversion_FOUND AND TARGET "${__double_conversion_target_name}")
    set(__double_conversion_found TRUE)
    # This ensures the Config file is shown in the fphsa message.
    if(double-conversion_CONFIG)
        list(PREPEND WrapSystemDoubleConversion_REQUIRED_VARS
            double-conversion_CONFIG)
    endif()
endif()

if(NOT __double_conversion_found)
    list(PREPEND WrapSystemDoubleConversion_REQUIRED_VARS
        DOUBLE_CONVERSION_LIBRARY DOUBLE_CONVERSION_INCLUDE_DIR)

    find_path(DOUBLE_CONVERSION_INCLUDE_DIR
        NAMES
            double-conversion/double-conversion.h
    )

    find_library(DOUBLE_CONVERSION_LIBRARY_RELEASE NAMES double-conversion)

    # We assume a possible debug build of this library to be named with a d suffix.
    # Adjust accordingly if a different naming scheme is established.
    find_library(DOUBLE_CONVERSION_LIBRARY_DEBUG NAMES double-conversiond)

    include(SelectLibraryConfigurations)
    select_library_configurations(DOUBLE_CONVERSION)
    mark_as_advanced(DOUBLE_CONVERSION_INCLUDE_DIR DOUBLE_CONVERSION_LIBRARY)
    set(DOUBLE_CONVERSION_INCLUDE_DIRS "${DOUBLE_CONVERSION_INCLUDE_DIR}")

    if(DOUBLE_CONVERSION_LIBRARIES AND DOUBLE_CONVERSION_INCLUDE_DIRS)
        set(__double_conversion_found TRUE)
    endif()
endif()

include(FindPackageHandleStandardArgs)

set(__double_conversion_fphsa_args "")
if(double-conversion_VERSION)
    set(WrapSystemDoubleConversion_VERSION "${double-conversion_VERSION}")
    list(APPEND __double_conversion_fphsa_args VERSION_VAR WrapSystemDoubleConversion_VERSION)
endif()

find_package_handle_standard_args(WrapSystemDoubleConversion
    REQUIRED_VARS ${WrapSystemDoubleConversion_REQUIRED_VARS}
    ${__double_conversion_fphsa_args})

if(WrapSystemDoubleConversion_FOUND)
    add_library(WrapSystemDoubleConversion::WrapSystemDoubleConversion INTERFACE IMPORTED)
    if(TARGET "${__double_conversion_target_name}")
        target_link_libraries(WrapSystemDoubleConversion::WrapSystemDoubleConversion
                              INTERFACE "${__double_conversion_target_name}")
    else()
        target_link_libraries(WrapSystemDoubleConversion::WrapSystemDoubleConversion
            INTERFACE ${DOUBLE_CONVERSION_LIBRARIES})
        target_include_directories(WrapSystemDoubleConversion::WrapSystemDoubleConversion
            INTERFACE ${DOUBLE_CONVERSION_INCLUDE_DIRS})
    endif()
endif()
unset(__double_conversion_target_name)
unset(__double_conversion_found)
unset(__double_conversion_fphsa_args)

include(FeatureSummary)
set_package_properties(WrapSystemDoubleConversion PROPERTIES
  URL "https://github.com/google/double-conversion"
  DESCRIPTION "double-conversion library")

