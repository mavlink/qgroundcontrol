# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#.rst:
# FindZstd
# ---------
#
# Try to locate the Zstd library.
# If found, this will define the following variables:
#
# ``WrapZSTD_FOUND``
#     True if the zstd library is available
# ``ZSTD_INCLUDE_DIRS``
#     The zstd include directories
# ``ZSTD_LIBRARIES``
#     The zstd libraries for linking
#
# If ``WrapZSTD_FOUND`` is TRUE, it will also define the following
# imported target:
#
# ``WrapZSTD::WrapZSTD``
#     The zstd library

find_package(zstd CONFIG QUIET)

include(FindPackageHandleStandardArgs)

if(TARGET zstd::libzstd_static OR TARGET zstd::libzstd_shared OR TARGET zstd::libzstd)
    find_package_handle_standard_args(WrapZSTD
                                      REQUIRED_VARS zstd_VERSION VERSION_VAR zstd_VERSION)
    if(TARGET zstd::libzstd_shared)
        set(zstdtargetsuffix "_shared")
    elseif(TARGET zstd::libzstd)
        set(zstdtargetsuffix "")
    else()
        set(zstdtargetsuffix "_static")
    endif()

    if(NOT TARGET WrapZSTD::WrapZSTD)
        add_library(WrapZSTD::WrapZSTD INTERFACE IMPORTED)
        set_target_properties(WrapZSTD::WrapZSTD PROPERTIES
                              INTERFACE_LINK_LIBRARIES "zstd::libzstd${zstdtargetsuffix}")
    endif()
else()
    get_cmake_property(__packages_not_found PACKAGES_NOT_FOUND)
    if(__packages_not_found)
        list(REMOVE_ITEM __packages_not_found zstd)
        set_property(GLOBAL PROPERTY PACKAGES_NOT_FOUND "${__packages_not_found}")
    endif()
    unset(__packages_not_found)

    find_package(PkgConfig QUIET)
    pkg_check_modules(PC_ZSTD QUIET "libzstd")

    find_path(ZSTD_INCLUDE_DIRS
              NAMES zstd.h
              HINTS ${PC_ZSTD_INCLUDEDIR}
              PATH_SUFFIXES zstd)

    find_library(ZSTD_LIBRARY_RELEASE
                 NAMES zstd zstd_static
                 HINTS ${PC_ZSTD_LIBDIR}
    )
    find_library(ZSTD_LIBRARY_DEBUG
                 NAMES zstdd zstd_staticd zstd zstd_static
                 HINTS ${PC_ZSTD_LIBDIR}
    )

    include(SelectLibraryConfigurations)
    select_library_configurations(ZSTD)

    if(PC_ZSTD_VERSION)
        set(WrapZSTD_VERSION "${PC_ZSTD_VERSION}")
    endif()
    find_package_handle_standard_args(WrapZSTD
                                      REQUIRED_VARS ZSTD_LIBRARIES ZSTD_INCLUDE_DIRS
                                      VERSION_VAR WrapZSTD_VERSION)

    if(WrapZSTD_FOUND AND NOT TARGET WrapZSTD::WrapZSTD)
      add_library(WrapZSTD::WrapZSTD UNKNOWN IMPORTED)
      set_target_properties(WrapZSTD::WrapZSTD PROPERTIES
                            INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_INCLUDE_DIRS}")
      set_target_properties(WrapZSTD::WrapZSTD PROPERTIES
                            IMPORTED_LOCATION "${ZSTD_LIBRARY}")
      if(ZSTD_LIBRARY_RELEASE)
          set_target_properties(WrapZSTD::WrapZSTD PROPERTIES
                                IMPORTED_LOCATION_RELEASE "${ZSTD_LIBRARY_RELEASE}")
      endif()
      if(ZSTD_LIBRARY_DEBUG)
          set_target_properties(WrapZSTD::WrapZSTD PROPERTIES
                                IMPORTED_LOCATION_DEBUG "${ZSTD_LIBRARY_DEBUG}")
      endif()
    endif()

    mark_as_advanced(ZSTD_INCLUDE_DIRS ZSTD_LIBRARIES ZSTD_LIBRARY_RELEASE ZSTD_LIBRARY_DEBUG)
endif()
include(FeatureSummary)
set_package_properties(WrapZSTD PROPERTIES
  URL "https://github.com/facebook/zstd"
  DESCRIPTION "ZSTD compression library")

