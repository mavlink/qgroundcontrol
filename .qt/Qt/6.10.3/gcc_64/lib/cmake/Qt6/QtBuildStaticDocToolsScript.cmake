# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

# Query the var name from the CMake cache or the environment or use a default value.
function(qt_internal_get_cmake_or_env_or_default out_var var_name_to_check default_value)
    if(${var_name_to_check})
        set(value "${var_name_to_check}")
    elseif(DEFINED ENV{${var_name_to_check}})
        set(value "$ENV{${var_name_to_check}}")
    else()
        set(value "${default_value}")
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

# Prepares paths for the cloning of the sources, the build and the installation.
function(qt_internal_prepare_build_paths)
    set(current_dir "${CMAKE_CURRENT_BINARY_DIR}")

    qt_internal_get_cmake_or_env_or_default(working_dir
        QT_CI_DOC_TOOLS_WORKING_DIR "${current_dir}/qdoc_build")

    qt_internal_get_cmake_or_env_or_default(qt_src_dir
        QT_CI_DOC_TOOLS_SRC_DIR "${working_dir}/qt5")

    qt_internal_get_cmake_or_env_or_default(qt_build_dir
        QT_CI_DOC_TOOLS_BUILD__DIR "${working_dir}/build")

    qt_internal_get_cmake_or_env_or_default(qt_installed_dir
        QT_CI_DOC_TOOLS_INSTALL_DIR "${working_dir}/install")

    set(QT_DOC_TOOLS_WORKING_DIR "${working_dir}" PARENT_SCOPE)
    set(QT_DOC_TOOLS_SRC_DIR "${qt_src_dir}" PARENT_SCOPE)
    set(QT_DOC_TOOLS_BUILD_DIR "${qt_build_dir}" PARENT_SCOPE)
    set(QT_DOC_TOOLS_INSTALL_DIR "${qt_installed_dir}" PARENT_SCOPE)
endfunction()

# Gets the remote git base URL for qt repositories.
function(qt_internal_get_repo_base_url out_var)
    # This Coin CI git daemon IP is set in all CI jobs.
    # Prefer using it when available, to avoid general network issues.
    # Sample value: QT_COIN_GIT_DAEMON=10.215.0.212:9418
    qt_internal_get_cmake_or_env_or_default(coin_git_ip_port QT_COIN_GIT_DAEMON "")

    # Allow opting out of using the coin git daemon.
    qt_internal_get_cmake_or_env_or_default(skip_coin_git QT_DOC_TOOLS_SKIP_COIN_GIT_DAEMON FALSE)

    # Allow override of the default remote.
    qt_internal_get_cmake_or_env_or_default(git_remote QT_CI_DOC_TOOLS_GIT_REMOTE "")

    if(coin_git_ip_port AND NOT skip_coin_git)
        set(value "git://${coin_git_ip_port}/qt-project")
    elseif(git_remote)
        set(value "${git_remote}")
    else()
        set(value "https://code.qt.io")
    endif()

    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

# Clones qt5.git into the specified src directory.
function(qt_internal_clone_qt5)
    file(MAKE_DIRECTORY "${QT_DOC_TOOLS_SRC_DIR}")

    qt_internal_get_repo_base_url(remote_url)
    execute_process(
        COMMAND git clone "${remote_url}/qt/qt5.git" "${QT_DOC_TOOLS_SRC_DIR}"
        COMMAND_ECHO STDOUT
        WORKING_DIRECTORY "${QT_DOC_TOOLS_WORKING_DIR}"
        RESULT_VARIABLE result
        ERROR_VARIABLE proc_err
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(result)
        message(WARNING "Cloning qt5.git sources failed. Output: ${result}")
    endif()

    # Allow pinning the qt5.git sha1 or any other git ref, based on a cmake var or env var.
    qt_internal_get_cmake_or_env_or_default(super_repo_ref
        QT_CI_DOC_TOOLS_TOP_LEVEL_PIN_GIT_REF "dev")

    # Allow using the branch that the Coin CI sets, as an opt-in.
    qt_internal_get_cmake_or_env_or_default(use_ci_branch
        QT_CI_DOC_TOOLS_USE_CI_TOP_LEVEL_BRANCH "OFF")
    qt_internal_get_cmake_or_env_or_default(ci_branch
        TESTED_MODULE_BRANCH_COIN "")

    if(use_ci_branch AND ci_branch)
        set(super_repo_ref "${ci_branch}")
    endif()

    execute_process(
        COMMAND git checkout "${super_repo_ref}"
        COMMAND_ECHO STDOUT
        WORKING_DIRECTORY "${QT_DOC_TOOLS_SRC_DIR}"
        RESULT_VARIABLE result
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(result)
        message(FATAL_ERROR
            "Checking out qt5.git '${super_repo_ref}' ref failed. Output: ${result}")
    endif()
endfunction()

# Checks out qttools to the dev branch (or other specified ref) and syncs its submodule
# dependencies according to qttools dependencies.yaml file.
function(qt_internal_sync_to_qttools)
    execute_process(
        COMMAND
            git submodule update
            --init
            --recursive
            --progress
            --depth 1
            qttools

            # TODO: Remove these once the sync-to script is taught to automatically clone these.
            qtbase
            qtshadertools
            qtdeclarative
            qtsvg
            qtimageformats
            qtactiveqt
            qtlanguageserver

        COMMAND_ECHO STDOUT
        WORKING_DIRECTORY "${QT_DOC_TOOLS_SRC_DIR}"
        RESULT_VARIABLE result
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(result)
        message(FATAL_ERROR "Cloning qttools submodule failed. Output: ${result}")
    endif()

    # Allow pinning the qttools sha1 or any other git ref, based on a cmake var or env var.
    qt_internal_get_cmake_or_env_or_default(qttools_sync_to_ref QT_CI_DOC_TOOLS_PIN_GIT_REF "dev")

    execute_process(
        COMMAND
            ${CMAKE_COMMAND}
                -DSYNC_TO_MODULE=qttools
                "-DSYNC_TO_BRANCH=${qttools_sync_to_ref}"
                -DSHOW_PROGRESS=1
                -DVERBOSE=1
                -P cmake/QtSynchronizeRepo.cmake
                --log-level=DEBUG
        COMMAND_ECHO STDOUT
        WORKING_DIRECTORY "${QT_DOC_TOOLS_SRC_DIR}"
        RESULT_VARIABLE result
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(result)
        message(FATAL_ERROR "Syncing qttools submodule dependencies failed: Output: ${result}")
    endif()
endfunction()

# Configures a top-level static qt build for the checked out qt5.git sources.
function(qt_internal_configure_qt)
    file(MAKE_DIRECTORY "${QT_DOC_TOOLS_BUILD_DIR}")
    file(MAKE_DIRECTORY "${QT_DOC_TOOLS_INSTALL_DIR}")

    # Get the common CI args, to set the sccache args, etc.
    qt_internal_get_cmake_or_env_or_default(ci_common_cmake_args COMMON_CMAKE_ARGS "")
    if(ci_common_cmake_args)
        separate_arguments(ci_common_cmake_args NATIVE_COMMAND ${ci_common_cmake_args})
    endif()

    execute_process(
        COMMAND
            "${QT_DOC_TOOLS_SRC_DIR}/configure"
            -release
            -static
            -prefix "${QT_DOC_TOOLS_INSTALL_DIR}"
            # Skip optional dependencies we don't need to build.
            -skip qtactiveqt,qtimageformats,qtlanguageserver,qtsvg
            --
            -DWARNINGS_ARE_ERRORS=OFF
            # When 6.x.y version bumps are not merged in DAG-dependency order, this avoids
            # blocking integrations due to mismatch of qtools package version and any of its
            # dependencies.
            -DQT_NO_PACKAGE_VERSION_INCOMPATIBLE_WARNING=ON
            --log-level STATUS
            --fresh
            -GNinja
            ${ci_common_cmake_args}

        COMMAND_ECHO STDOUT
        WORKING_DIRECTORY "${QT_DOC_TOOLS_BUILD_DIR}"
        RESULT_VARIABLE result
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(result)
        message(FATAL_ERROR "Configuring top-level qt5.git build failed. Output: ${result}")
    endif()
endfunction()

# Builds the main doc tools in a configured qt5.git build.
function(qt_internal_build_tools)
    execute_process(
        COMMAND
            cmake
            --build .
            --verbose
            --parallel
            --target
                qdoc
                qtattributionsscanner
                qdbusxml2cpp
                qdbuscpp2xml
                qvkgen
        COMMAND_ECHO STDOUT
        WORKING_DIRECTORY "${QT_DOC_TOOLS_BUILD_DIR}"
        RESULT_VARIABLE result
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(result)
        message(FATAL_ERROR "Failed to build doc tools. Output: ${result}")
    endif()
endfunction()

# Installs the built doc tools.
function(qt_internal_install_tools)
    execute_process(
        COMMAND
            cmake
            --build .
            --target
                # TODO: Replace with ninja install_doc_tools_stripped once it lands.
                qttools/src/qdoc/qdoc/install/strip
                qttools/src/qtattributionsscanner/install/strip
                qtbase/src/tools/qdbusxml2cpp/install/strip
                qtbase/src/tools/qdbuscpp2xml/install/strip
                qtbase/src/tools/qvkgen/install/strip
        COMMAND_ECHO STDOUT
        WORKING_DIRECTORY "${QT_DOC_TOOLS_BUILD_DIR}"
        RESULT_VARIABLE result
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(result)
        message(FATAL_ERROR "Failed to install doc tools. Output: ${result}")
    endif()
endfunction()

function(qt_internal_run_script)
    qt_internal_prepare_build_paths()
    qt_internal_clone_qt5()
    qt_internal_sync_to_qttools()
    qt_internal_configure_qt()
    qt_internal_build_tools()
    qt_internal_install_tools()
endfunction()

qt_internal_run_script()
