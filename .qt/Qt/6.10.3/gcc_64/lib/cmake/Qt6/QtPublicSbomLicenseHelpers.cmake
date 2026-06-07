# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Adds a license id and its text to the sbom.
function(_qt_internal_sbom_add_license)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args
        NO_LICENSE_REF_PREFIX
    )
    set(single_args
        LICENSE_ID
        LICENSE_PATH
        EXTRACTED_TEXT
    )
    set(multi_args
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_LICENSE_ID)
        message(FATAL_ERROR "LICENSE_ID must be set")
    endif()

    if(NOT arg_TEXT AND NOT arg_LICENSE_PATH)
        message(FATAL_ERROR "Either TEXT or LICENSE_PATH must be set")
    endif()

    # Sanitize the content a bit.
    if(arg_TEXT)
        set(text "${arg_TEXT}")
        string(REPLACE ";" "$<SEMICOLON>" text "${text}")
        string(REPLACE "\"" "\\\"" text "${text}")
    else()
        file(READ "${arg_LICENSE_PATH}" text)
        string(REPLACE ";" "$<SEMICOLON>" text "${text}")
        string(REPLACE "\"" "\\\"" text "${text}")
    endif()

    set(license_id "${arg_LICENSE_ID}")
    if(NOT arg_NO_LICENSE_REF_PREFIX)
        set(license_id "LicenseRef-${license_id}")
    endif()


    if(QT_SBOM_GENERATE_SPDX_V2)
        _qt_internal_sbom_generate_add_license(
            LICENSE_ID "${license_id}"
            EXTRACTED_TEXT "<text>${text}</text>"
        )
    endif()

    if(QT_SBOM_GENERATE_CYDX_V1_6)
        _qt_internal_sbom_record_license_cydx(
            LICENSE_ID "${license_id}"
            EXTRACTED_TEXT "${text}"
        )
    endif()
endfunction()

# Get a qt spdx license expression given the id.
function(_qt_internal_sbom_get_spdx_license_expression id out_var)
    set(license "")

    # The default for modules / plugins
    if(id STREQUAL "QT_DEFAULT" OR id STREQUAL "QT_COMMERCIAL_OR_LGPL3")
        set(license "LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only")

    # For commercial only entities
    elseif(id STREQUAL "QT_COMMERCIAL")
        set(license "LicenseRef-Qt-Commercial")

    # For GPL3 only modules
    elseif(id STREQUAL "QT_COMMERCIAL_OR_GPL3")
        set(license "LicenseRef-Qt-Commercial OR GPL-3.0-only")

    # For tools and apps
    elseif(id STREQUAL "QT_COMMERCIAL_OR_GPL3_WITH_EXCEPTION")
        set(license "LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0")

    # For things like the qtmain library
    elseif(id STREQUAL "QT_COMMERCIAL_OR_BSD3")
        set(license "LicenseRef-Qt-Commercial OR BSD-3-Clause")

    # For documentation
    elseif(id STREQUAL "QT_COMMERCIAL_OR_GFDL1_3")
        set(license "LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only")

    # For examples and the like
    elseif(id STREQUAL "BSD3")
        set(license "BSD-3-Clause")

    endif()

    if(NOT license)
        message(FATAL_ERROR "No SPDX license expression found for id: ${id}")
    endif()

    set(${out_var} "${license}" PARENT_SCOPE)
endfunction()

# Joins two license IDs with the given ${op}, avoiding parenthesis when possible.
function(_qt_internal_sbom_join_two_license_ids_with_op left_id op right_id out_var)
    if(NOT left_id)
        set(${out_var} "${right_id}" PARENT_SCOPE)
        return()
    endif()

    if(NOT right_id)
        set(${out_var} "${left_id}" PARENT_SCOPE)
        return()
    endif()

    set(value "(${left_id}) ${op} (${right_id})")
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()
