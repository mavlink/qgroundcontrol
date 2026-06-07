# Copyright (C) 2022 The Qt Company Ltd.
# Copyright (C) 2022 Mimer Information Technology
# SPDX-License-Identifier: BSD-3-Clause

# FindMimer
# ---------
# Try to locate the Mimer SQL client library
if(NOT DEFINED MimerSQL_ROOT)
    if(DEFINED ENV{MIMERSQL_DEV_ROOT})
        set(MimerSQL_ROOT "$ENV{MIMERSQL_DEV_ROOT}")
    endif()
endif()

if(NOT DEFINED MimerSQL_ROOT)
    find_package(PkgConfig QUIET)
endif()
if(PkgConfig_FOUND AND NOT DEFINED MimerSQL_ROOT)
    pkg_check_modules(PC_Mimer QUIET mimcontrol)
    set(MimerSQL_include_dir_hints "${PC_MimerSQL_INCLUDEDIR}")
    set(MimerSQL_library_hints "${PC_MimerSQL_LIBDIR}")
else()
    if(DEFINED MimerSQL_ROOT)
        if(WIN32)
            set(MimerSQL_include_dir_hints "${MimerSQL_ROOT}\\include")
            if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|X86)$")
                set(MimerSQL_library_hints "${MimerSQL_ROOT}\\lib\\x86")
             elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(amd64|AMD64)$")
                set(MimerSQL_library_hints "${MimerSQL_ROOT}\\lib\\amd64")
            else()
                set(MimerSQL_library_hints "")
            endif()
        else()
            set(MimerSQL_include_dir_hints "${MimerSQL_ROOT}/include")
            set(MimerSQL_library_hints "${MimerSQL_ROOT}/lib")
        endif()
    else()
        if(WIN32)
            set(MimerSQL_include_dir_hints "C:\\MimerSQLDev\\include")
            if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|X86)$")
                set(MimerSQL_library_hints "C:\\MimerSQLDev\\lib\\x86")
            elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(amd64|AMD64)$")
                set(MimerSQL_library_hints "C:\\MimerSQLDev\\lib\\amd64")
            else()
                set(MimerSQL_library_hints "")
            endif()
        elseif(APPLE AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            set(MimerSQL_library_hints "/usr/local/lib")
            set(MimerSQL_include_dir_hints "/usr/local/include")
        else()
            set(MimerSQL_include_dir_hints "")
            set(MimerSQL_library_hints "")
        endif()
    endif()
endif()

find_path(Mimer_INCLUDE_DIR
    NAMES mimerapi.h
    HINTS ${MimerSQL_include_dir_hints})

if(WIN32)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|X86)$")
    set(MIMER_LIBS_NAMES mimapi32)
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(amd64|AMD64)$")
    set(MIMER_LIBS_NAMES mimapi64)
  endif()
else()
  set(MIMER_LIBS_NAMES mimerapi)
endif()

find_library(Mimer_LIBRARIES
    NAMES ${MIMER_LIBS_NAMES}
    HINTS ${MimerSQL_library_hints})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Mimer
    REQUIRED_VARS Mimer_LIBRARIES Mimer_INCLUDE_DIR)



# Now try to get the include and library path.
if(Mimer_FOUND)
  set(Mimer_INCLUDE_DIRS ${Mimer_INCLUDE_DIR})
  set(Mimer_LIBRARY_DIRS ${Mimer_LIBRARIES})
  if (NOT TARGET MimerSQL::MimerSQL)
    add_library(MimerSQL::MimerSQL UNKNOWN IMPORTED)
    set_target_properties(MimerSQL::MimerSQL PROPERTIES
      IMPORTED_LOCATION "${Mimer_LIBRARY_DIRS}"
      INTERFACE_INCLUDE_DIRECTORIES "${Mimer_INCLUDE_DIRS}")
  endif ()
endif()

mark_as_advanced(Mimer_INCLUDE_DIR Mimer_LIBRARIES)

include(FeatureSummary)
set_package_properties(MimerSQL PROPERTIES
  URL "https://www.mimer.com"
  DESCRIPTION "Mimer client library")
