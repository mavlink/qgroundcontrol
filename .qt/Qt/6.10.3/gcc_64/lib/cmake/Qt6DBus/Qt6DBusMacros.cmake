# Copyright 2005-2011 Kitware, Inc.
# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(MacroAddFileDependencies)

function(qt6_add_dbus_interface _sources _interface _relativename)
    get_filename_component(_infile ${_interface} ABSOLUTE)
    get_filename_component(_basepath ${_relativename} DIRECTORY)
    get_filename_component(_basename ${_relativename} NAME)
    set(_header "${CMAKE_CURRENT_BINARY_DIR}/${_relativename}.h")
    set(_impl   "${CMAKE_CURRENT_BINARY_DIR}/${_relativename}.cpp")
    if(_basepath)
        set(_moc "${CMAKE_CURRENT_BINARY_DIR}/${_basepath}/moc_${_basename}.cpp")
    else()
        set(_moc "${CMAKE_CURRENT_BINARY_DIR}/moc_${_basename}.cpp")
    endif()

    get_source_file_property(_nonamespace ${_interface} NO_NAMESPACE)
    if(_nonamespace)
        set(_params -N -m)
    else()
        set(_params -m)
    endif()

    get_source_file_property(_classname ${_interface} CLASSNAME)
    if(_classname)
        set(_params ${_params} -c ${_classname})
    endif()

    get_source_file_property(_include ${_interface} INCLUDE)
    if(_include)
        set(_params ${_params} -i ${_include})
    endif()

    add_custom_command(OUTPUT "${_impl}" "${_header}"
        COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::qdbusxml2cpp ${_params} -p ${_relativename} ${_infile}
        DEPENDS ${_infile} ${QT_CMAKE_EXPORT_NAMESPACE}::qdbuscpp2xml
        VERBATIM
    )

    _qt_internal_set_source_file_generated(
        SOURCES "${_impl}" "${_header}"
        SKIP_AUTOGEN
    )

    qt6_generate_moc("${_header}" "${_moc}")

    list(APPEND ${_sources} "${_impl}" "${_header}")
    macro_add_file_dependencies("${_impl}" "${_moc}")
    set(${_sources} ${${_sources}} PARENT_SCOPE)
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    # All three positional arguments are mandatory and there are no optional
    # arguments, so we can preserve them exactly. As an added bonus, if the
    # caller doesn't provide enough arguments, they will get an error message
    # for their call site instead of here in the wrapper.
    function(qt_add_dbus_interface sources interface relativename)
        if(ARGC GREATER 3)
            message(FATAL_ERROR "Unexpected arguments: ${ARGN}")
        endif()
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_add_dbus_interface("${sources}" "${interface}" "${relativename}")
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_dbus_interface("${sources}" "${interface}" "${relativename}")
        endif()
        set("${sources}" "${${sources}}" PARENT_SCOPE)
    endfunction()
endif()


function(qt6_add_dbus_interfaces _sources)
    foreach(_current_FILE ${ARGN})
        get_filename_component(_infile ${_current_FILE} ABSOLUTE)
        get_filename_component(_basename ${_current_FILE} NAME)
        # get the part before the ".xml" suffix
        string(TOLOWER ${_basename} _basename)
        string(REGEX REPLACE "(.*\\.)?([^\\.]+)\\.xml" "\\2" _basename ${_basename})
        qt6_add_dbus_interface(${_sources} ${_infile} ${_basename}interface)
    endforeach()
    set(${_sources} ${${_sources}} PARENT_SCOPE)
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_dbus_interfaces sources)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_add_dbus_interfaces("${sources}" ${ARGN})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_dbus_interfaces("${sources}" ${ARGN})
        endif()
        set("${sources}" "${${sources}}" PARENT_SCOPE)
    endfunction()
endif()


function(qt6_generate_dbus_interface _header) # _customName OPTIONS -some -options )
    set(options)
    set(oneValueArgs)
    set(multiValueArgs OPTIONS)

    cmake_parse_arguments(_DBUS_INTERFACE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(_customName ${_DBUS_INTERFACE_UNPARSED_ARGUMENTS})

    get_filename_component(_in_file ${_header} ABSOLUTE)
    get_filename_component(_basename ${_header} NAME_WE)

    if(_customName)
        if(IS_ABSOLUTE ${_customName})
          get_filename_component(_containingDir ${_customName} PATH)
          if(NOT EXISTS ${_containingDir})
              file(MAKE_DIRECTORY "${_containingDir}")
          endif()
          set(_target ${_customName})
        else()
            set(_target ${CMAKE_CURRENT_BINARY_DIR}/${_customName})
        endif()
    else()
        set(_target ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.xml)
    endif()

    add_custom_command(OUTPUT ${_target}
        COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::qdbuscpp2xml ${_DBUS_INTERFACE_OPTIONS} ${_in_file} -o ${_target}
        DEPENDS ${_in_file} ${QT_CMAKE_EXPORT_NAMESPACE}::qdbuscpp2xml
        VERBATIM
    )
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_generate_dbus_interface)
        # The versioned function's implementation doesn't preserve empty options,
        # so we don't need to here either. Using ARGV is fine under those assumptions.
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_generate_dbus_interface(${ARGV})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_generate_dbus_interface(${ARGV})
        endif()
    endfunction()
endif()


function(qt6_add_dbus_adaptor _sources _xml_file _include) # _optionalParentClass _optionalRelativename _optionalClassName)
    get_filename_component(_infile ${_xml_file} ABSOLUTE)

    set(_optionalParentClass "${ARGV3}")
    if(_optionalParentClass)
        set(_parentClassOption "-l")
        set(_parentClass "${_optionalParentClass}")
    endif()

    set(_optionalRelativename "${ARGV4}")
    if(_optionalRelativename)
        set(_relativename ${_optionalRelativename})
    else()
        string(REGEX REPLACE "(.*[/\\.])?([^\\.]+)\\.xml" "\\2adaptor" _relativename ${_infile})
        string(TOLOWER ${_relativename} _relativename)
    endif()
    get_filename_component(_basepath ${_relativename} DIRECTORY)
    get_filename_component(_basename ${_relativename} NAME)

    set(_optionalClassName "${ARGV5}")
    set(_header "${CMAKE_CURRENT_BINARY_DIR}/${_relativename}.h")
    set(_impl   "${CMAKE_CURRENT_BINARY_DIR}/${_relativename}.cpp")
    if(_basepath)
        set(_moc "${CMAKE_CURRENT_BINARY_DIR}/${_basepath}/moc_${_basename}.cpp")
    else()
        set(_moc "${CMAKE_CURRENT_BINARY_DIR}/moc_${_basename}.cpp")
    endif()

    if(_optionalClassName)
        add_custom_command(OUTPUT "${_impl}" "${_header}"
          COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::qdbusxml2cpp -m -a ${_relativename} -c ${_optionalClassName} -i ${_include} ${_parentClassOption} ${_parentClass} ${_infile}
          DEPENDS ${_infile} ${QT_CMAKE_EXPORT_NAMESPACE}::qdbuscpp2xml
          VERBATIM
        )
    else()
        add_custom_command(OUTPUT "${_impl}" "${_header}"
          COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::qdbusxml2cpp -m -a ${_relativename} -i ${_include} ${_parentClassOption} ${_parentClass} ${_infile}
          DEPENDS ${_infile} ${QT_CMAKE_EXPORT_NAMESPACE}::qdbuscpp2xml
          VERBATIM
        )
    endif()

    _qt_internal_set_source_file_generated(
        SOURCES "${_impl}" "${_header}"
        SKIP_AUTOGEN
    )
    qt6_generate_moc("${_header}" "${_moc}")
    macro_add_file_dependencies("${_impl}" "${_moc}")

    list(APPEND ${_sources} "${_impl}" "${_header}")
    set(${_sources} ${${_sources}} PARENT_SCOPE)
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_dbus_adaptor sources dbus_spec header)
        # We need to preserve empty values in both positional and optional arguments.
        # The following explicit use of ARGVx variables ensures we don't silently
        # drop any empty values, which is especially important if there are any
        # non-empty values after empty ones. Note that we must not try to read
        # ARGVx variables where x >= ARGC, as that is undefined behavior.
        # Also note that the parent_class argument is required for qt5, but is
        # optional for qt6.
        if(ARGC LESS 4)
            set(parent_class "")
        else()
            set(parent_class "${ARGV3}")
        endif()

        if(ARGC LESS 5)
            set(basename "")
        else()
            set(basename "${ARGV4}")
        endif()

        if(ARGC LESS 6)
            set(classname "")
        else()
            set(classname "${ARGV5}")
        endif()

        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_add_dbus_adaptor(
                "${sources}" "${dbus_spec}" "${header}"
                "${parent_class}" "${basename}" "${classname}"
            )
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_dbus_adaptor(
                "${sources}" "${dbus_spec}" "${header}"
                "${parent_class}" "${basename}" "${classname}"
            )
        endif()
        set("${sources}" "${${sources}}" PARENT_SCOPE)
    endfunction()
endif()
