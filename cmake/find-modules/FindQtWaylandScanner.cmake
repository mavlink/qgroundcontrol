# SPDX-FileCopyrightText: 2012-2014 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
#
# SPDX-License-Identifier: BSD-3-Clause

#[=======================================================================[.rst:
FindQtWaylandScanner
--------------------

Try to find qtwaylandscanner.

If the qtwaylandscanner executable is not in your PATH, you can provide
an alternative name or full path location with the ``QtWaylandScanner_EXECUTABLE``
variable.

This will define the following variables:

``QtWaylandScanner_FOUND``
    True if qtwaylandscanner is available

``QtWaylandScanner_EXECUTABLE``
    The qtwaylandscanner executable.

If ``QtWaylandScanner_FOUND`` is TRUE, it will also define the following imported
target:

``Wayland::QtScanner``
    The qtwaylandscanner executable.

This module provides the following functions to generate C++ protocol
implementations:

  - ``ecm_add_qtwayland_client_protocol``
  - ``ecm_add_qtwayland_server_protocol``

::

  ecm_add_qtwayland_client_protocol(<target>
                                    PROTOCOL <xmlfile>
                                    BASENAME <basename>
                                    [PREFIX <prefix>]
                                    [PRIVATE_CODE])

  ecm_add_qtwayland_client_protocol(<source_files_var>
                                    PROTOCOL <xmlfile>
                                    BASENAME <basename>
                                    [PREFIX <prefix>]
                                    [PRIVATE_CODE])

Generate C++ wrapper to Wayland client protocol files from ``<xmlfile>``
XML definition for the ``<basename>`` interface and append those files
to ``<source_files_var>`` or ``<target>``.  Pass the ``<prefix>`` argument if the interface
names don't start with ``qt_`` or ``wl_``.
``PRIVATE_CODE`` instructs wayland-scanner to hide marshalling code
from the compiled DSO for use in other DSOs. The default is to
export this code.

WaylandScanner is required and will be searched for.

::

  ecm_add_qtwayland_server_protocol(<target>
                                    PROTOCOL <xmlfile>
                                    BASENAME <basename>
                                    [PREFIX <prefix>]
                                    [PRIVATE_CODE])

  ecm_add_qtwayland_server_protocol(<source_files_var>
                                    PROTOCOL <xmlfile>
                                    BASENAME <basename>
                                    [PREFIX <prefix>]
                                    [PRIVATE_CODE])

Generate C++ wrapper to Wayland server protocol files from ``<xmlfile>``
XML definition for the ``<basename>`` interface and append those files
to ``<source_files_var>`` or ``<target>``.  Pass the ``<prefix>`` argument if the interface
names don't start with ``qt_`` or ``wl_``.
``PRIVATE_CODE`` instructs wayland-scanner to hide marshalling code
from the compiled DSO for use in other DSOs. The default is to
export this code.

WaylandScanner is required and will be searched for.

Since 1.4.0.
#]=======================================================================]

include(${CMAKE_CURRENT_LIST_DIR}/ECMFindModuleHelpersStub.cmake)
include("${ECM_MODULE_DIR}/ECMQueryQt.cmake")

ecm_find_package_version_check(QtWaylandScanner)

if (QT_MAJOR_VERSION STREQUAL "5")
    ecm_query_qt(qtwaylandscanner_dir QT_HOST_BINS)
else()
    ecm_query_qt(qtwaylandscanner_dir QT_HOST_LIBEXECS)
endif()

# Find qtwaylandscanner
find_program(QtWaylandScanner_EXECUTABLE NAMES qtwaylandscanner HINTS ${qtwaylandscanner_dir})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QtWaylandScanner
    FOUND_VAR
        QtWaylandScanner_FOUND
    REQUIRED_VARS
        QtWaylandScanner_EXECUTABLE
)

mark_as_advanced(QtWaylandScanner_EXECUTABLE)

if(NOT TARGET Wayland::QtScanner AND QtWaylandScanner_FOUND)
    add_executable(Wayland::QtScanner IMPORTED)
    set_target_properties(Wayland::QtScanner PROPERTIES
        IMPORTED_LOCATION "${QtWaylandScanner_EXECUTABLE}"
    )
endif()

include(FeatureSummary)
set_package_properties(QtWaylandScanner PROPERTIES
    URL "https://qt.io/"
    DESCRIPTION "Executable that converts XML protocol files to C++ code"
)

function(ecm_add_qtwayland_client_protocol target_or_sources_var)
    # Parse arguments
    set(oneValueArgs PROTOCOL BASENAME PREFIX)
    set(options PRIVATE_CODE)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "" ${ARGN})

    if(ARGS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to ecm_add_qtwayland_client_protocol(): \"${ARGS_UNPARSED_ARGUMENTS}\"")
    endif()

    set(_prefix "${ARGS_PREFIX}")
    if(ARGS_PRIVATE_CODE)
        set(_private_code_option PRIVATE_CODE)
    endif()

    find_package(WaylandScanner REQUIRED QUIET)
    ecm_add_wayland_client_protocol(${target_or_sources_var}
                                    PROTOCOL ${ARGS_PROTOCOL}
                                    BASENAME ${ARGS_BASENAME}
                                    ${_private_code_option})

    get_filename_component(_infile ${ARGS_PROTOCOL} ABSOLUTE)
    set(_header "${CMAKE_CURRENT_BINARY_DIR}/qwayland-${ARGS_BASENAME}.h")
    set(_code "${CMAKE_CURRENT_BINARY_DIR}/qwayland-${ARGS_BASENAME}.cpp")

    set_source_files_properties(${_header} ${_code} GENERATED)

    add_custom_command(OUTPUT "${_header}"
        COMMAND ${QtWaylandScanner_EXECUTABLE} client-header ${_infile} "" ${_prefix} > ${_header}
        DEPENDS ${_infile} ${_cheader} VERBATIM)

    add_custom_command(OUTPUT "${_code}"
        COMMAND ${QtWaylandScanner_EXECUTABLE} client-code ${_infile} "" ${_prefix} > ${_code}
        DEPENDS ${_infile} ${_header} VERBATIM)

    set_property(SOURCE ${_header} ${_code} PROPERTY SKIP_AUTOMOC ON)

    if (TARGET ${target_or_sources_var})
        target_sources(${target_or_sources_var} PRIVATE "${_code}")
    else()
        list(APPEND ${target_or_sources_var} "${_code}")
        set(${target_or_sources_var} ${${target_or_sources_var}} PARENT_SCOPE)
    endif()
endfunction()


function(ecm_add_qtwayland_server_protocol target_or_sources_var)
    # Parse arguments
    set(oneValueArgs PROTOCOL BASENAME PREFIX)
    set(options PRIVATE_CODE)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "" ${ARGN})

    if(ARGS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to ecm_add_qtwayland_server_protocol(): \"${ARGS_UNPARSED_ARGUMENTS}\"")
    endif()

    set(_prefix "${ARGS_PREFIX}")
    if(ARGS_PRIVATE_CODE)
        set(_private_code_option PRIVATE_CODE)
    endif()

    find_package(WaylandScanner REQUIRED QUIET)
    ecm_add_wayland_server_protocol(${target_or_sources_var}
                                    PROTOCOL ${ARGS_PROTOCOL}
                                    BASENAME ${ARGS_BASENAME}
                                    ${_private_code_option})

    get_filename_component(_infile ${ARGS_PROTOCOL} ABSOLUTE)
    set(_header "${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-${ARGS_BASENAME}.h")
    set(_code "${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-${ARGS_BASENAME}.cpp")

    set_source_files_properties(${_header} ${_code} GENERATED)

    add_custom_command(OUTPUT "${_header}"
        COMMAND ${QtWaylandScanner_EXECUTABLE} server-header ${_infile} "" ${_prefix} > ${_header}
        DEPENDS ${_infile} VERBATIM)

    add_custom_command(OUTPUT "${_code}"
        COMMAND ${QtWaylandScanner_EXECUTABLE} server-code ${_infile} "" ${_prefix} > ${_code}
        DEPENDS ${_infile} ${_header} VERBATIM)

    set_property(SOURCE ${_header} ${_code} PROPERTY SKIP_AUTOMOC ON)

    if (TARGET ${target_or_sources_var})
        target_sources(${target_or_sources_var} PRIVATE "${_code}")
    else()
        list(APPEND ${target_or_sources_var} "${_code}")
        set(${target_or_sources_var} ${${target_or_sources_var}} PARENT_SCOPE)
    endif()
endfunction()
