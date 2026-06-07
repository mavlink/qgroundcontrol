# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Ensures that the provided uri matches the URI requirements. Write the result to the result
# variable.
function(_qt_internal_is_qml_uri_valid result uri)
    if("${uri}" MATCHES "^[a-zA-Z].*")
        set(${result} TRUE PARENT_SCOPE)
    else()
        set(${result} FALSE PARENT_SCOPE)
    endif()
endfunction()

# Ensures that the provided uri matches the URI requirements. Errors out if the URI is malformed.
function(_qt_internal_require_qml_uri_valid uri)
    _qt_internal_is_qml_uri_valid(ok "${uri}")
    if(NOT ok)
        message(FATAL_ERROR "URI must start with letter. Please specify a valid URI for ${target}.")
    endif()
endfunction()

# Copies the file from src to dest.
# Creates missing directories at dest if they do not exist.
function(_qt_internal_qml_copy_file src dest)
    # Create the dest directory, it might not exist.
    get_filename_component(dest_out_dir "${dest}" DIRECTORY)
    file(MAKE_DIRECTORY "${dest_out_dir}")

    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
        # Significantly increases copying speed according to profiling, presumably
        # because we bypass process creation.
        file(COPY_FILE "${src}" "${dest}" ONLY_IF_DIFFERENT)
    else()
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${src}" "${dest}")
    endif()
endfunction()
