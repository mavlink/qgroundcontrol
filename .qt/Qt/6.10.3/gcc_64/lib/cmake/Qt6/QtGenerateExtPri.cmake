# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generate a qt_ext_XXX.pri file.
#
# - Replaces occurrences of the build libdir with $$[QT_INSTALL_LIBDIR/get].
#
# This file is to be used in CMake script mode with the following variables set:
# IN_FILES: path to the qt_ext_XXX.cmake files
# OUT_FILE: path to the generated qt_ext_XXX.pri file
# QT_BUILD_LIBDIR: path to Qt's libdir when building (those paths get replaced)
set(content "")
string(TOUPPER "${LIB}" uclib)
set(first_iteration TRUE)
list(LENGTH CONFIGS number_of_configs)
foreach(in_file ${IN_FILES})
    include(${in_file})
    if(first_iteration)
        # Add configuration-independent variables
        set(first_iteration FALSE)
        list(JOIN incdir " " incdir)
        list(JOIN defines " " defines)
        string(APPEND content "QMAKE_INCDIR_${uclib} = ${incdir}
QMAKE_DEFINES_${uclib} = ${defines}
")
    endif()
    set(config_suffix "")
    if(number_of_configs GREATER "1")
        # We're in multi-config mode. Use a _DEBUG or _RELEASE suffix for libs.
        # qmake_use.prf does not support other configurations.
        string(TOUPPER "${cfg}" config_suffix)
        if(config_suffix STREQUAL "DEBUG")
            set(config_suffix _DEBUG)
        else()
            set(config_suffix _RELEASE)
        endif()
    endif()

    # Replace the build libdir
    set(fixed_libs "")
    foreach(lib ${libs})
        string(REPLACE "${QT_BUILD_LIBDIR}" "$$[QT_INSTALL_LIBS/get]" lib "${lib}")
        list(APPEND fixed_libs "${lib}")
    endforeach()

    list(JOIN fixed_libs " " libs)
    string(APPEND content "QMAKE_LIBS_${uclib}${config_suffix} = ${libs}
")
endforeach()
file(WRITE "${OUT_FILE}" "${content}")
