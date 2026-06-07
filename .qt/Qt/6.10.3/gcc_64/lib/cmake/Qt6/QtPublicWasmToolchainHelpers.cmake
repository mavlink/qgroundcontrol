# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Assuming EMSDK == /path/emsdk
#
# Then we expect /path/emsdk/.emscripten file to contain the following line
#   EMSCRIPTEN_ROOT = emsdk_path + '/upstream/emscripten'
#
# then we set out_var to '/upstream/emscripten', so it's not a full path
function(__qt_internal_get_emroot_path_suffix_from_emsdk_env out_var)
    # Query EMSCRIPTEN_ROOT path.
    file(READ "$ENV{EMSDK}/.emscripten" ver)
    string(REGEX MATCH "EMSCRIPTEN_ROOT.*$" EMROOT "${ver}")
    string(REGEX MATCH "'([^' ]*)'" EMROOT2 "${EMROOT}")
    string(REPLACE "'" "" EMROOT_PATH "${EMROOT2}")

    set(${out_var} "${EMROOT_PATH}" PARENT_SCOPE)
endfunction()

function(__qt_internal_get_emscripten_cmake_toolchain_file_path_from_emsdk_env emroot_path out_var)
    set(wasm_toolchain_file "$ENV{EMSDK}/${emroot_path}/cmake/Modules/Platform/Emscripten.cmake")
    set(${out_var} "${wasm_toolchain_file}" PARENT_SCOPE)
endfunction()

function(__qt_internal_query_emsdk_version emroot_path is_fatal out_var)
    # get emscripten version
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        set(EXECUTE_COMMANDPATH "$ENV{EMSDK}/${emroot_path}/emcc.bat")
    else()
        set(EXECUTE_COMMANDPATH "$ENV{EMSDK}/${emroot_path}/emcc")
    endif()

    file(TO_CMAKE_PATH "${EXECUTE_COMMANDPATH}" EXECUTE_COMMAND)
    execute_process(COMMAND ${EXECUTE_COMMAND} --version
        OUTPUT_VARIABLE emOutput
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_VARIABLE emrun_error
        RESULT_VARIABLE result)
    message(DEBUG "emcc --version output: ${emOutput}")

    if(NOT emOutput)
        if(is_fatal)
            message(FATAL_ERROR
                "Couldn't determine Emscripten version from running ${EXECUTE_COMMAND} --version. "
                "Error: ${emrun_error}")
        endif()
        set(${out_var} "" PARENT_SCOPE)
    else()
        string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" CMAKE_EMSDK_REGEX_VERSION "${emOutput}")
        set(${out_var} "${CMAKE_EMSDK_REGEX_VERSION}" PARENT_SCOPE)
    endif()
endfunction()

function(__qt_internal_get_emcc_recommended_version out_var)
    # This version of Qt needs this version of emscripten.
    set(QT_EMCC_RECOMMENDED_VERSION "4.0.7")
    set(${out_var} "${QT_EMCC_RECOMMENDED_VERSION}" PARENT_SCOPE)
endfunction()

function(__qt_internal_show_error_no_emscripten_toolchain_file_found_when_building_qt)
    message(FATAL_ERROR
        "Cannot find the toolchain file Emscripten.cmake. "
        "Please specify the toolchain file with -DCMAKE_TOOLCHAIN_FILE=<file> "
        "or provide a path to a valid emscripten installation via the EMSDK "
        "environment variable.")
endfunction()

function(__qt_internal_show_error_no_emscripten_toolchain_file_found_when_using_qt)
    message(FATAL_ERROR
        "Cannot find the toolchain file Emscripten.cmake. "
        "Please specify the toolchain file with -DQT_CHAINLOAD_TOOLCHAIN_FILE=<file> "
        "or provide a path to a valid emscripten installation via the EMSDK "
        "environment variable.")
endfunction()

function(__qt_internal_get_qt_build_emsdk_version out_var)
    if(QT6_INSTALL_PREFIX)
        set(WASM_BUILD_DIR "${QT6_INSTALL_PREFIX}")
    elseif(QT_BUILD_DIR)
        set(WASM_BUILD_DIR "${QT_BUILD_DIR}")
    endif()
    if(EXISTS "${WASM_BUILD_DIR}/src/corelib/global/qconfig.h")
        file(READ "${WASM_BUILD_DIR}/src/corelib/global/qconfig.h" ver)
    elseif(EXISTS "${WASM_BUILD_DIR}/include/QtCore/qconfig.h")
        file(READ "${WASM_BUILD_DIR}/include/QtCore/qconfig.h" ver)
    endif()
    if (ver)
        string(REGEX MATCH "#define QT_EMCC_VERSION.\"[0-9]+\\.[0-9]+\\.[0-9]+\"" emOutput ${ver})
        string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" build_emcc_version "${emOutput}")
        set(${out_var} "${build_emcc_version}" PARENT_SCOPE)
    endif()
endfunction()

function(_qt_test_emscripten_version)
    __qt_internal_get_emcc_recommended_version(_recommended_emver)
    __qt_internal_get_emroot_path_suffix_from_emsdk_env(emroot_path)
    __qt_internal_query_emsdk_version("${emroot_path}" TRUE current_emsdk_ver)
    __qt_internal_get_qt_build_emsdk_version(qt_build_emcc_version)

    if(NOT "${qt_build_emcc_version}" STREQUAL "" AND NOT "${qt_build_emcc_version}" STREQUAL "${current_emsdk_ver}")
        message(FATAL_ERROR
            "Qt Wasm was built with Emscripten version: ${qt_build_emcc_version}\n"
            "You are using Emscripten version: ${current_emsdk_ver}\n"
            "The recommended version of Emscripten for this Qt is: ${_recommended_emver}\n"
            "Stopping configuration due to mismatch of Emscripten versions."
        )
    endif()
endfunction()
