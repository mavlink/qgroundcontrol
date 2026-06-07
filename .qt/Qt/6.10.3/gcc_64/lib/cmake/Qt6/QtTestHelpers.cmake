# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Simple wrapper around qt_internal_add_executable for benchmarks which insure that
# the binary is built under ${CMAKE_CURRENT_BINARY_DIR} and never installed.
# See qt_internal_add_executable() for more details.
function(qt_internal_add_benchmark target)
    if(QT_BUILD_TESTS_BATCHED)
        message(WARNING "Benchmarks won't be batched - unsupported (yet)")
    endif()

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${__qt_internal_add_executable_optional_args}"
        "${__qt_internal_add_executable_single_args}"
        "${__qt_internal_add_executable_multi_args}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)
    _qt_internal_validate_no_unity_build(arg)

    qt_remove_args(exec_args
        ARGS_TO_REMOVE
            ${target}
            OUTPUT_DIRECTORY
            INSTALL_DIRECTORY
        ALL_ARGS
            "${__qt_internal_add_executable_optional_args}"
            "${__qt_internal_add_executable_single_args}"
            "${__qt_internal_add_executable_multi_args}"
        ARGS
            ${ARGV}
    )

    if(NOT arg_OUTPUT_DIRECTORY)
        if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
            set(arg_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        else()
            set(arg_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
        endif()
    endif()

    qt_internal_add_executable(${target}
        NO_INSTALL # we don't install benchmarks
        NO_UNITY_BUILD # excluded by default
        QT_BENCHMARK_TEST
        OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}" # avoid polluting bin directory
        ${exec_args}
    )
    qt_internal_extend_target(${target}
        DEFINES
            ${deprecation_define}
    )

    # Benchmarks on iOS must be app bundles.
    if(IOS)
        set_target_properties(${target} PROPERTIES MACOSX_BUNDLE TRUE)
    endif()

    qt_internal_add_repo_local_defines(${target})

    # Disable the QT_NO_NARROWING_CONVERSIONS_IN_CONNECT define for benchmarks
    qt_internal_undefine_global_definition(${target} QT_NO_NARROWING_CONVERSIONS_IN_CONNECT)

    qt_internal_collect_command_environment(benchmark_env_path benchmark_env_plugin_path)

    # Add a ${target}_benchmark generator target, to run single benchmark more easily.
    set(benchmark_wrapper_file "${arg_OUTPUT_DIRECTORY}/${target}Wrapper$<CONFIG>.cmake")
    _qt_internal_create_command_script(COMMAND "$<TARGET_FILE:${target}>"
                                      OUTPUT_FILE "${benchmark_wrapper_file}"
                                      ENVIRONMENT "PATH" "${benchmark_env_path}"
                                                  "QT_PLUGIN_PATH" "${benchmark_env_plugin_path}"
    )

    add_custom_target("${target}_benchmark"
        VERBATIM
        COMMENT "Running benchmark ${target}"
        COMMAND "${CMAKE_COMMAND}" "-P" "${benchmark_wrapper_file}"
    )

    add_dependencies("${target}_benchmark" "${target}")

    # Add benchmark to meta target if it exists.
    if (TARGET benchmark)
        add_dependencies("benchmark" "${target}_benchmark")
    endif()

    qt_internal_add_test_finalizers("${target}")
endfunction()

function(qt_internal_add_test_dependencies target)
    if(QT_BUILD_TESTS_BATCHED)
        _qt_internal_test_batch_target_name(target)
    endif()
    add_dependencies(${target} ${ARGN})
endfunction()

# Simple wrapper around qt_internal_add_executable for manual tests which insure that
# the binary is built under ${CMAKE_CURRENT_BINARY_DIR} and never installed.
# See qt_internal_add_executable() for more details.
function(qt_internal_add_manual_test target)
    qt_internal_add_test(${ARGV} MANUAL)
endfunction()

macro(qt_internal_skip_docker_compose)
    set(QT_SKIP_DOCKER_COMPOSE ON CACHE BOOL "Skip setting docker on Linux." FORCE)
endmacro()

# This function will configure the fixture for the network tests that require docker network services
# qmake counterpart: qtbase/mkspecs/features/unsupported/testserver.prf
function(qt_internal_setup_docker_test_fixture name)
    # Only Linux is provisioned with docker at this time
    if (NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
      return()
    endif()

    option(QT_SKIP_DOCKER_COMPOSE "Skip setting up docker on Linux." OFF)
    if(QT_SKIP_DOCKER_COMPOSE)
      return()
    endif()

    set(QT_TEST_SERVER_LIST ${ARGN})
    set(DNSDOMAIN test-net.qt.local)

    find_program(QT_DOCKER_COMPOSE docker-compose)
    if (NOT QT_DOCKER_COMPOSE)
        message(WARNING "docker-compose was not found. Docker network tests will not be run.")
        qt_internal_skip_docker_compose()
        return()
    endif()
    if (NOT DEFINED QT_DOCKER_COMPOSE_VERSION)
      execute_process(COMMAND "${QT_DOCKER_COMPOSE}" --version OUTPUT_VARIABLE QT_DOCKER_COMPOSE_VERSION)
      string(REPLACE "\n" "" QT_DOCKER_COMPOSE_VERSION "${QT_DOCKER_COMPOSE_VERSION}")
      set(QT_DOCKER_COMPOSE_VERSION "${QT_DOCKER_COMPOSE_VERSION}" CACHE STRING "docker compose version")
    endif()

    find_program(QT_DOCKER docker)
    if (NOT QT_DOCKER)
        message(WARNING "docker was not found. Docker network tests will not be run.")
        qt_internal_skip_docker_compose()
        return()
    endif()
    if (NOT DEFINED QT_DOCKER_TEST_SERVER)
        execute_process(COMMAND "${QT_DOCKER}" images -aq "qt-test-server-*" OUTPUT_VARIABLE QT_DOCKER_TEST_SERVER)
        if (NOT QT_DOCKER_TEST_SERVER)
            message(WARNING
                "Docker image qt-test-server-* not found.\n"
                "Run the provisioning script (coin/provisioning/.../testserver/docker_testserver.sh) in advance\n"
                "Docker network tests will not be run.")
            qt_internal_skip_docker_compose()
            return()
        endif()
        set(QT_DOCKER_TEST_SERVER "ON" CACHE BOOL "docker qt-test-server-* present")
    endif()

    target_compile_definitions("${name}"
        PRIVATE
            QT_TEST_SERVER QT_TEST_SERVER_NAME QT_TEST_SERVER_DOMAIN=\"${DNSDOMAIN}\"
    )

    if(DEFINED QT_TESTSERVER_COMPOSE_FILE)
        set(TESTSERVER_COMPOSE_FILE ${QT_TESTSERVER_COMPOSE_FILE})
    elseif(QNX OR VXWORKS)
        set(TESTSERVER_COMPOSE_FILE "${QT_SOURCE_TREE}/tests/testserver/docker-compose-qemu-bridge-network.yml")
    else()
        set(TESTSERVER_COMPOSE_FILE "${QT_SOURCE_TREE}/tests/testserver/docker-compose-bridge-network.yml")
    endif()

    # Bring up test servers and make sure the services are ready.
    add_test(NAME ${name}-setup COMMAND
        "${QT_DOCKER_COMPOSE}" -f ${TESTSERVER_COMPOSE_FILE} up --build -d --force-recreate --timeout 1 ${QT_TEST_SERVER_LIST}
    )
    # Stop and remove test servers after testing.
    add_test(NAME ${name}-cleanup COMMAND
        "${QT_DOCKER_COMPOSE}" -f ${TESTSERVER_COMPOSE_FILE} down --timeout 1
    )

    set_tests_properties(${name}-setup PROPERTIES FIXTURES_SETUP ${name}-docker)
    set_tests_properties(${name}-cleanup PROPERTIES FIXTURES_CLEANUP ${name}-docker)
    set_tests_properties(${name} PROPERTIES FIXTURES_REQUIRED ${name}-docker)

    foreach(test_name ${name} ${name}-setup ${name}-cleanup)
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT "testserver=${QT_DOCKER_COMPOSE_VERSION}")
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT TEST_DOMAIN=${DNSDOMAIN})
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT "SHARED_DATA=${QT_MKSPECS_DIR}/features/data/testserver")
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT SHARED_SERVICE=bridge-network)
    endforeach()

endfunction()

function(qt_internal_get_test_batch out)
    get_property(batched_list GLOBAL PROPERTY _qt_batched_test_list_property)
    set(${out} ${batched_list} PARENT_SCOPE)
endfunction()

function(qt_internal_prepare_test_target_flags version_arg exceptions_text gui_text)
    cmake_parse_arguments(arg "EXCEPTIONS;NO_EXCEPTIONS;GUI" "VERSION" "" ${ARGN})

    if (arg_VERSION)
        set(${version_arg} VERSION "${arg_VERSION}" PARENT_SCOPE)
    endif()

    # Qt modules get compiled without exceptions enabled by default.
    # However, testcases should be still built with exceptions.
    set(${exceptions_text} "EXCEPTIONS" PARENT_SCOPE)
    if (${arg_NO_EXCEPTIONS})
        set(${exceptions_text} "" PARENT_SCOPE)
    endif()

    if (${arg_GUI})
        set(${gui_text} "GUI" PARENT_SCOPE)
    endif()
endfunction()

function(qt_internal_get_test_arg_definitions optional_args single_value_args multi_value_args)
    set(${optional_args}
        RUN_SERIAL
        EXCEPTIONS
        NO_EXCEPTIONS
        GUI
        QMLTEST
        CATCH
        LOWDPI
        NO_WRAPPER
        BUILTIN_TESTDATA
        MANUAL
        NO_BATCH
        NO_INSTALL
        BUNDLE_ANDROID_OPENSSL_LIBS
        NO_WASM_DEFAULT_FILES
        PARENT_SCOPE
    )
    set(${single_value_args}
        OUTPUT_DIRECTORY
        WORKING_DIRECTORY
        TIMEOUT
        VERSION
        PARENT_SCOPE
    )
    set(${multi_value_args}
        QML_IMPORTPATH
        TESTDATA
        QT_TEST_SERVER_LIST
        ANDROID_TESTRUNNER_PRE_TEST_ADB_COMMANDS
        ${__default_private_args}
        ${__default_public_args}
        PARENT_SCOPE
    )
endfunction()

function(qt_internal_add_test_to_batch batch_name name)
    qt_internal_get_test_arg_definitions(optional_args single_value_args multi_value_args)

    cmake_parse_arguments(
        arg "${optional_args}" "${single_value_args}" "${multi_value_args}" ${ARGN})
    qt_internal_prepare_test_target_flags(version_arg exceptions_text gui_text ${ARGN})

    _qt_internal_validate_no_unity_build(arg)

    _qt_internal_test_batch_target_name(target)

    # Lazy-init the test batch
    if(NOT TARGET ${target})
        if(${arg_MANUAL})
            set(is_manual "QT_MANUAL_TEST")
        else()
            set(is_manual "")
        endif()

        qt_internal_add_executable(${target}
            ${exceptions_text}
            ${gui_text}
            ${version_arg}
            NO_INSTALL
            QT_TEST
            ${is_manual}
            OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build_dir"
            SOURCES "${QT_CMAKE_DIR}/qbatchedtestrunner.in.cpp"
            DEFINES QTEST_BATCH_TESTS ${deprecation_define}
            INCLUDE_DIRECTORIES ${private_includes}
            LIBRARIES ${QT_CMAKE_EXPORT_NAMESPACE}::Core
                    ${QT_CMAKE_EXPORT_NAMESPACE}::Test
                    ${QT_CMAKE_EXPORT_NAMESPACE}::TestPrivate
                    # Add GUI by default so that the plugins link properly with non-standalone
                    # build of tests. Plugin handling is currently only done in
                    # qt_internal_add_executable if Gui is present. This should be reevaluated with
                    # multiple batches.
                    ${QT_CMAKE_EXPORT_NAMESPACE}::Gui
        )
        # TODO: QTBUG-131745
        # Emscripten runs out of memory in CI after upgrade to 3.1.70 when linking test_batch.
        # In future we will disable test batching and use JSPI instead.
        # For now disable optimizations for test_batch target so it can run in CI.
        if(WASM)
            target_compile_options(${target} PRIVATE "-O0")
        endif()

        set_property(TARGET ${target} PROPERTY _qt_has_exceptions ${arg_EXCEPTIONS})
        set_property(TARGET ${target} PROPERTY _qt_has_gui ${arg_GUI})
        set_property(TARGET ${target} PROPERTY _qt_has_lowdpi ${arg_LOWDPI})
        set_property(TARGET ${target} PROPERTY _qt_version ${version_arg})
    else()
        # Check whether the args match with the batch. Some differences between
        # flags cannot be reconciled - one should not combine these tests into
        # a single binary.
        qt_internal_get_target_property(
            batch_has_exceptions ${target} _qt_has_exceptions)
        if(NOT ${batch_has_exceptions} STREQUAL ${arg_EXCEPTIONS})
            qt_internal_get_test_batch(test_batch_contents)
            message(FATAL_ERROR "Conflicting exceptions declaration between test \
    batch (${test_batch_contents}) and ${name}")
        endif()
        qt_internal_get_target_property(batch_has_gui ${target} _qt_has_gui)
        if(NOT ${batch_has_gui} STREQUAL ${arg_GUI})
            qt_internal_get_test_batch(test_batch_contents)
            message(FATAL_ERROR "Conflicting gui declaration between test batch \
    (${test_batch_contents}) and ${name}")
        endif()
        qt_internal_get_target_property(
            batch_has_lowdpi ${target} _qt_has_lowdpi)
        if(NOT ${batch_has_lowdpi} STREQUAL ${arg_LOWDPI})
            qt_internal_get_test_batch(test_batch_contents)
            message(FATAL_ERROR "Conflicting lowdpi declaration between test batch \
    (${test_batch_contents}) and ${name}")
        endif()
        qt_internal_get_target_property(batch_version ${target} _qt_version)
        if(NOT "${batch_version} " STREQUAL " " AND
            NOT "${version_arg} " STREQUAL " " AND
            NOT "${batch_version} " STREQUAL "${version_arg} ")
            qt_internal_get_test_batch(test_batch_contents)
            message(FATAL_ERROR "Conflicting version declaration between test \
    batch ${test_batch_contents} (${batch_version}) and ${name} (${version_arg})")
        endif()
    endif()

    get_property(batched_test_list GLOBAL PROPERTY _qt_batched_test_list_property)
    if(NOT batched_test_list)
        set_property(GLOBAL PROPERTY _qt_batched_test_list_property "")
        set(batched_test_list "")
    endif()
    list(PREPEND batched_test_list ${name})
    set_property(GLOBAL PROPERTY _qt_batched_test_list_property ${batched_test_list})

    # Test batching produces single executable which can result in one source file being added
    # multiple times (with different definitions) to one translation unit. This is not supported by
    # CMake so instead we try to detect such situation and rename file every time it's added
    # to the build more than once. This avoids filenames collisions in one translation unit.
    get_property(batched_test_sources_list GLOBAL PROPERTY _qt_batched_test_sources_list_property)
    if(NOT batched_test_sources_list)
        set_property(GLOBAL PROPERTY _qt_batched_test_sources_list_property "")
        set(batched_test_sources_list "")
    endif()
    foreach(source ${arg_SOURCES})
        set(source_path ${source})
        if(${source} IN_LIST batched_test_sources_list)
            set(new_filename ${name}.cpp)
            configure_file(${source} ${new_filename})
            set(source_path ${CMAKE_CURRENT_BINARY_DIR}/${new_filename})
            set(skip_automoc ON)
            list(APPEND arg_SOURCES ${source_path})
        else()
            set(skip_automoc OFF)
            list(APPEND batched_test_sources_list ${source})
        endif()
        set_source_files_properties(${source_path}
            TARGET_DIRECTORY ${target} PROPERTIES
                SKIP_AUTOMOC ${skip_automoc}
                COMPILE_DEFINITIONS "BATCHED_TEST_NAME=\"${name}\";${arg_DEFINES}")
    endforeach()
    set_property(GLOBAL PROPERTY _qt_batched_test_sources_list_property ${batched_test_sources_list})

    # Merge the current test with the rest of the batch
    qt_internal_extend_target(${target}
        INCLUDE_DIRECTORIES ${arg_INCLUDE_DIRECTORIES}
        PUBLIC_LIBRARIES ${arg_PUBLIC_LIBRARIES}
        LIBRARIES ${arg_LIBRARIES}
        SOURCES ${arg_SOURCES}
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        COMPILE_FLAGS ${arg_COMPILE_FLAGS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
        DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
        NO_UNITY_BUILD # Tests should not be built using UNITY_BUILD
        )

    set(${batch_name} ${target} PARENT_SCOPE)

    # Add a dummy target so that new tests don't have problems with a nonexistent
    # target when calling cmake functions.
    # The batch tests that include this target will compile, but may fail to work.
    # Manual action is required then.
    add_custom_target(${name})

    # Add the dependency to the dummy target so that it is indirectly added to the test batch
    # dependencies.
    add_dependencies(${target} ${name})
endfunction()

# Checks whether the test 'name' is present in the test batch. See QT_BUILD_TESTS_BATCHED.
# The result of the check is placed in the 'out' variable.
function(qt_internal_is_in_test_batch out name)
    set(${out} FALSE PARENT_SCOPE)
    if(QT_BUILD_TESTS_BATCHED)
        get_property(batched_test_list GLOBAL PROPERTY _qt_batched_test_list_property)
        if("${name}" IN_LIST batched_test_list)
            set(${out} TRUE PARENT_SCOPE)
        endif()
    endif()
endfunction()

function(qt_internal_is_skipped_test out name)
    get_target_property(is_skipped_test ${name} _qt_is_skipped_test)
    set(${out} ${is_skipped_test} PARENT_SCOPE)
endfunction()

function(qt_internal_set_skipped_test name)
    set_target_properties(${name} PROPERTIES _qt_is_skipped_test TRUE)
endfunction()

function(qt_internal_is_qtbase_test out)
    get_filename_component(dir "${CMAKE_CURRENT_BINARY_DIR}" ABSOLUTE)
    set(${out} FALSE PARENT_SCOPE)

    while(TRUE)
        get_filename_component(filename "${dir}" NAME)
        if("${filename}" STREQUAL "qtbase")
            set(${out} TRUE PARENT_SCOPE)
            break()
        endif()

        set(prev_dir "${dir}")
        get_filename_component(dir "${dir}" DIRECTORY)
        if("${dir}" STREQUAL "${prev_dir}")
            break()
        endif()
    endwhile()
endfunction()

function(qt_internal_get_batched_test_arguments out testname)
    if(WASM)
        # Add a query string to the runner document, so that the script therein
        # knows which test to run in response to launching the testcase by ctest.
        list(APPEND args "qbatchedtest")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            list(APPEND args "qvisualoutput")
        endif()
    else()
        # Simply add the test name in case of standard executables.
        list(APPEND args "${testname}")
    endif()
    set(${out} ${args} PARENT_SCOPE)
endfunction()

# This function creates a CMake test target with the specified name for use with CTest.
#
# All tests are wrapped with cmake script that supports TESTARGS and TESTRUNNER environment
# variables handling. Endpoint wrapper may be used standalone as cmake script to run tests e.g.:
# TESTARGS="-o result.xml,junitxml" TESTRUNNER="testrunner --arg" ./tst_simpleTestWrapper.cmake
# On non-UNIX machine you may need to use 'cmake -P' explicitly to execute wrapper.
# You may avoid test wrapping by either passing NO_WRAPPER option or switching QT_NO_TEST_WRAPPERS
# to ON. This is helpful if you want to use internal CMake tools within tests, like memory or
# sanitizer checks. See https://cmake.org/cmake/help/v3.19/manual/ctest.1.html#ctest-memcheck-step
# Arguments:
#    BUILTIN_TESTDATA
#       The option forces adding the provided TESTDATA to resources.
#    MANUAL
#       The option indicates that the test is a manual test.
#    ANDROID_TESTRUNNER_PRE_TEST_ADB_COMMANDS
#       Passes --pre-test-adb-command <command> to androidTestRunner. Android specific argument.
function(qt_internal_add_test name)
    qt_internal_get_test_arg_definitions(optional_args single_value_args multi_value_args)

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${optional_args}"
        "${single_value_args}"
        "${multi_value_args}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)
    _qt_internal_validate_no_unity_build(arg)

    set(batch_current_test FALSE)
    if(QT_BUILD_TESTS_BATCHED AND NOT arg_NO_BATCH AND NOT arg_QMLTEST AND NOT arg_MANUAL
            AND ("${QT_STANDALONE_TEST_PATH}" STREQUAL ""
                 OR DEFINED ENV{QT_BATCH_STANDALONE_TESTS}))
        set(batch_current_test TRUE)
    endif()

    if(batch_current_test OR (QT_BUILD_TESTS_BATCHED AND arg_QMLTEST))
        if (QT_SUPERBUILD OR DEFINED ENV{TESTED_MODULE_COIN})
            set(is_qtbase_test FALSE)
            if(QT_SUPERBUILD)
                qt_internal_is_qtbase_test(is_qtbase_test)
            elseif($ENV{TESTED_MODULE_COIN} STREQUAL "qtbase")
                set(is_qtbase_test TRUE)
            endif()
            if(NOT is_qtbase_test)
                file(GENERATE OUTPUT "dummy${name}.cpp" CONTENT "int main() { return 0; }")
                # Add a dummy target to tackle some potential problems
                qt_internal_add_executable(${name} SOURCES "dummy${name}.cpp")
                # Batched tests outside of qtbase are unsupported and skipped
                qt_internal_set_skipped_test(${name})
                return()
            endif()
        endif()
    endif()

    if(NOT arg_OUTPUT_DIRECTORY)
        if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
            set(arg_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        else()
            set(arg_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
        endif()
    endif()

    set(private_includes
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "$<BUILD_INTERFACE:${QT_BUILD_DIR}/include>"
    )

    set(testname "${name}")

    if(arg_PUBLIC_LIBRARIES)
        message(WARNING
            "qt_internal_add_test's PUBLIC_LIBRARIES option is deprecated, and will be "
            "removed in a future Qt version. Use the LIBRARIES option instead.")
    endif()

    if(batch_current_test)
        qt_internal_add_test_to_batch(name ${name} ${ARGN})
    elseif(arg_SOURCES)
        if(QT_BUILD_TESTS_BATCHED AND arg_QMLTEST)
            message(WARNING "QML tests won't be batched - unsupported (yet)")
        endif()
        # Handle cases where we have a qml test without source files
        list(APPEND private_includes ${arg_INCLUDE_DIRECTORIES})

        qt_internal_prepare_test_target_flags(version_arg exceptions_text gui_text ${ARGN})

        if(${arg_MANUAL})
            set(is_manual "QT_MANUAL_TEST")
        else()
            set(is_manual "")
        endif()

        qt_internal_add_executable("${name}"
            ${exceptions_text}
            ${gui_text}
            ${version_arg}
            NO_INSTALL
            QT_TEST
            ${is_manual}
            OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
            SOURCES "${arg_SOURCES}"
            INCLUDE_DIRECTORIES
                ${private_includes}
            DEFINES
                ${arg_DEFINES}
                ${deprecation_define}
            LIBRARIES
                ${arg_LIBRARIES}
                ${arg_PUBLIC_LIBRARIES}
                ${QT_CMAKE_EXPORT_NAMESPACE}::Core
                ${QT_CMAKE_EXPORT_NAMESPACE}::Test
            COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
            LINK_OPTIONS ${arg_LINK_OPTIONS}
            MOC_OPTIONS ${arg_MOC_OPTIONS}
            ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
            DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
            NO_UNITY_BUILD  # Tests should not be built using UNITY_BUILD
        )

        qt_internal_add_repo_local_defines(${name})

        # Disable the QT_NO_NARROWING_CONVERSIONS_IN_CONNECT define for tests
        qt_internal_undefine_global_definition(${name} QT_NO_NARROWING_CONVERSIONS_IN_CONNECT)

        # Manual tests can be bundle apps
        if(NOT arg_MANUAL)
            if(NOT DEFINED CMAKE_MACOSX_BUNDLE)
                # Tests should not be bundles on macOS even if arg_GUI is true, because some tests make
                # assumptions about the location of helper processes, and those paths would be different
                # if a test is built as a bundle.
                set_property(TARGET "${name}" PROPERTY MACOSX_BUNDLE FALSE)
            endif()
            if(NOT DEFINED CMAKE_WIN32_EXECUTABLE)
                # The same goes for WIN32_EXECUTABLE, but because it will detach from the console window
                # and not print anything.
                set_property(TARGET "${name}" PROPERTY WIN32_EXECUTABLE FALSE)
            endif()
        endif()

        # Tests on iOS must be app bundles.
        if(IOS)
            set_target_properties(${name} PROPERTIES MACOSX_BUNDLE TRUE)
        endif()

        # QMLTest specifics
        qt_internal_extend_target("${name}" CONDITION arg_QMLTEST
            LIBRARIES ${QT_CMAKE_EXPORT_NAMESPACE}::QuickTest
        )

        qt_internal_extend_target("${name}"
            CONDITION arg_QMLTEST AND NOT ANDROID AND NOT QT_FORCE_BUILTIN_TESTDATA
            DEFINES
                QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        )

        qt_internal_extend_target("${name}"
            CONDITION arg_QMLTEST AND (ANDROID OR QT_FORCE_BUILTIN_TESTDATA)
            DEFINES
                QUICK_TEST_SOURCE_DIR=":/"
        )

        # Android requires Qt::Gui so add it by default for tests
        qt_internal_extend_target("${name}" CONDITION ANDROID
            LIBRARIES ${QT_CMAKE_EXPORT_NAMESPACE}::Gui
        )

        set(blacklist_file "${CMAKE_CURRENT_SOURCE_DIR}/BLACKLIST")
        if(EXISTS ${blacklist_file})
            _qt_internal_expose_source_file_to_ide("${name}" ${blacklist_file})
        endif()
    endif()

    if (arg_NO_WASM_DEFAULT_FILES)
        set_target_properties(
                ${name}
            PROPERTIES
                NO_WASM_DEFAULT_FILES  True)
    endif()

    foreach(path IN LISTS arg_QML_IMPORTPATH)
        list(APPEND extra_test_args "-import" "${path}")
    endforeach()

    # Generate a label in the form tests/auto/foo/bar/tst_baz
    # and use it also for XML output
    set(label_base_directory "${PROJECT_SOURCE_DIR}")
    if (QT_SUPERBUILD)
        # Prepend repository name for qt5 builds, so that tests can be run for
        # individual repositories.
        set(label_base_directory "${label_base_directory}/..")
    endif()
    file(RELATIVE_PATH label "${label_base_directory}" "${CMAKE_CURRENT_SOURCE_DIR}/${name}")

    if (arg_LOWDPI)
        target_compile_definitions("${name}" PUBLIC TESTCASE_LOWDPI)
        if (MACOS)
            set_property(TARGET "${name}" PROPERTY MACOSX_BUNDLE_INFO_PLIST "${QT_MKSPECS_DIR}/macx-clang/Info.plist.disable_highdpi")
            set_property(TARGET "${name}" PROPERTY PROPERTY MACOSX_BUNDLE TRUE)
        endif()
    endif()

    if (ANDROID)
        # Pass 95% of the timeout to allow the test runner time to do any cleanup
        # before being killed.
        set(percentage "95")
        qt_internal_get_android_test_timeout("${arg_TIMEOUT}" "${percentage}" android_timeout)

        if(arg_BUNDLE_ANDROID_OPENSSL_LIBS)
            if(EXISTS "${OPENSSL_ROOT_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libcrypto_3.so")
                message(STATUS "Looking for OpenSSL in ${OPENSSL_ROOT_DIR}")
                set_property(TARGET ${name} APPEND PROPERTY QT_ANDROID_EXTRA_LIBS
                    "${OPENSSL_ROOT_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libcrypto_3.so"
                    "${OPENSSL_ROOT_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libssl_3.so")
            elseif(QT_USE_VCPKG AND DEFINED ENV{VCPKG_ROOT})
                message(STATUS "Looking for OpenSSL in $ENV{VCPKG_ROOT}")
                if (CMAKE_ANDROID_ARCH_ABI MATCHES "arm64-v8a")
                    set(coin_vcpkg_target_triplet "arm64-android-dynamic")
                elseif(CMAKE_ANDROID_ARCH_ABI MATCHES "armeabi-v7a")
                    set(coin_vcpkg_target_triplet "arm-neon-android-dynamic")
                elseif(CMAKE_ANDROID_ARCH_ABI MATCHES "x86_64")
                    set(coin_vcpkg_target_triplet "x64-android-dynamic")
                elseif(CMAKE_ANDROID_ARCH_ABI MATCHES "x86")
                    set(coin_vcpkg_target_triplet "x86-android-dynamic")
                endif()
                if(EXISTS "$ENV{VCPKG_ROOT}/installed/${coin_vcpkg_target_triplet}/lib/libcrypto.so")
                    message(STATUS "Found OpenSSL in $ENV{VCPKG_ROOT}/installed/${coin_vcpkg_target_triplet}/lib")
                    set_property(TARGET ${name} APPEND PROPERTY QT_ANDROID_EXTRA_LIBS
                        "$ENV{VCPKG_ROOT}/installed/${coin_vcpkg_target_triplet}/lib/libcrypto.so"
                        "$ENV{VCPKG_ROOT}/installed/${coin_vcpkg_target_triplet}/lib/libssl.so")
                endif()
            else()
                message(STATUS "The argument BUNDLE_ANDROID_OPENSSL_LIBS is set "
                               "but OPENSSL_ROOT_DIR parameter is not set. "
                               "Test should bundle OpenSSL libraries but they are not found. "
                               "This is fine if OpenSSL was built statically.")
            endif()
        endif()
        qt_internal_android_test_runner_arguments("${name}" test_executable extra_test_args)
        list(APPEND extra_test_args "--timeout" "${android_timeout}")

        set(build_environment "")
        if(DEFINED ENV{QT_BUILD_ENVIRONMENT})
            set(build_environment "$ENV{QT_BUILD_ENVIRONMENT}")
        endif()

        if(QT_ENABLE_VERBOSE_DEPLOYMENT OR build_environment STREQUAL "ci")
            list(APPEND extra_test_args "--verbose")
        endif()

        if(build_environment STREQUAL "ci")
            list(APPEND extra_test_args "--show-logcat")
        endif()

        if(arg_ANDROID_TESTRUNNER_PRE_TEST_ADB_COMMANDS)
            foreach(command IN LISTS arg_ANDROID_TESTRUNNER_PRE_TEST_ADB_COMMANDS)
                list(APPEND extra_test_args "--pre-test-adb-command" "${command}")
            endforeach()
        endif()

        set(test_working_dir "${CMAKE_CURRENT_BINARY_DIR}")
    elseif(QNX)
        set(test_working_dir "")
        set(test_executable "${name}")
    elseif(WASM)
        # The test script expects an html file. In case of batched tests, the
        # version specialized for running batches has to be supplied.
        if(batch_current_test)
            get_target_property(batch_output_dir ${name} RUNTIME_OUTPUT_DIRECTORY)
            set(test_executable "${batch_output_dir}/${name}.html")
        else()
            set(test_executable "${name}.html")
        endif()

        list(APPEND extra_test_args "quseemrun")
        list(APPEND extra_test_args "qtestname=${testname}")
        list(APPEND extra_test_args "--silence_timeout=60")
        # TODO: Add functionality to specify browser
        if(DEFINED ENV{BROWSER_FOR_WASM})
            set(browser $ENV{BROWSER_FOR_WASM})
        else()
            set(browser "chrome")
        endif()
        list(APPEND extra_test_args "--browser=${browser}")
        if(DEFINED ENV{HEADLESS_CHROME_FOR_TESTING})
            list(APPEND extra_test_args "--browser_args=\"--password-store=basic --headless\"")
        else()
            list(APPEND extra_test_args "--browser_args=\"--password-store=basic\"")
        endif()
        list(APPEND extra_test_args "--kill_exit")

        # Tests may require asyncify if they use exec(). Enable asyncify for
        # batched tests since this is the configuration used on the CI system.
        # Optimize for size (-Os), since asyncify tends to make the resulting
        # binary very large
        if(batch_current_test)
            target_link_options("${name}" PRIVATE "SHELL:-s ASYNCIFY" "-Os")
        endif()

        # This tells cmake to run the tests with this script, since wasm files can't be
        # executed directly
        if (CMAKE_HOST_WIN32)
            set_property(TARGET "${name}" PROPERTY CROSSCOMPILING_EMULATOR "emrun.bat")
        else()
            set_property(TARGET "${name}" PROPERTY CROSSCOMPILING_EMULATOR "emrun")
        endif()
    else()
        if(arg_QMLTEST AND NOT arg_SOURCES)
            set(qt_additional_libexec_paths "")

            if(DEFINED QT_ADDITIONAL_PACKAGES_PREFIX_PATH)
                foreach(additional_prefix IN LISTS QT_ADDITIONAL_PACKAGES_PREFIX_PATH)
                    set(additional_libexec "${additional_prefix}/${QT6_INSTALL_LIBEXECS}")
                    list(PREPEND qt_additional_libexec_paths "${additional_libexec}")
                endforeach()
            endif()

            # First look for the scanner in the target qt libexec dir. We prefer this one
            # over the tool target which might be for the host platform.
            find_program(qmltestrunner_executable
                NAMES qmltestrunner qmltestrunner.exe
                PATHS "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBEXECS}"
                    ${qt_additional_libexec_paths}
                NO_DEFAULT_PATH
            )

            # If we don't find it in the paths, fallback to using target names.
            if(NOT qmltestrunner_executable
                    AND TARGET "${QT_CMAKE_EXPORT_NAMESPACE}::qmltestrunner")
                set(qmltestrunner_executable ${QT_CMAKE_EXPORT_NAMESPACE}::qmltestrunner)
            endif()

            if(NOT qmltestrunner_executable AND TARGET qmltestrunner)
                set(qmltestrunner_executable qmltestrunner)
            endif()

            if(NOT qmltestrunner_executable)
                message(FATAL_ERROR "qmltestrunner not found.")
            endif()

            set(test_working_dir "${CMAKE_CURRENT_SOURCE_DIR}")
            set(test_executable "${qmltestrunner_executable}")
        else()
            if (arg_WORKING_DIRECTORY)
                set(test_working_dir "${arg_WORKING_DIRECTORY}")
            elseif(arg_OUTPUT_DIRECTORY)
                set(test_working_dir "${arg_OUTPUT_DIRECTORY}")
            else()
                set(test_working_dir "${CMAKE_CURRENT_BINARY_DIR}")
            endif()
            set(test_executable "${name}")
        endif()
    endif()

    if(NOT arg_MANUAL)
        if(batch_current_test)
            qt_internal_get_batched_test_arguments(batched_test_args ${testname})
            list(PREPEND extra_test_args ${batched_test_args})
        elseif(WASM AND CMAKE_BUILD_TYPE STREQUAL "Debug")
            list(PREPEND extra_test_args "qvisualoutput")
        endif()

        qt_internal_collect_command_environment(test_env_path test_env_plugin_path)

        set(add_test_args "")
        if(test_working_dir)
            list(APPEND add_test_args WORKING_DIRECTORY "${test_working_dir}")
        endif()

        if(arg_NO_WRAPPER OR QT_NO_TEST_WRAPPERS)
            if(QT_BUILD_TESTS_BATCHED)
                message(FATAL_ERROR "Wrapperless tests are unspupported with test batching")
            endif()



            add_test(NAME "${testname}"
                COMMAND ${test_executable} ${extra_test_args}
                ${add_test_args}
            )
            set_property(TEST "${testname}" APPEND PROPERTY
                         ENVIRONMENT "PATH=${test_env_path}"
                                     "QT_TEST_RUNNING_IN_CTEST=1"
                                     "QT_PLUGIN_PATH=${test_env_plugin_path}"
            )
        else()
            set(test_wrapper_file "${CMAKE_CURRENT_BINARY_DIR}/${testname}Wrapper$<CONFIG>.cmake")
            qt_internal_create_test_script(NAME "${testname}"
                                   COMMAND "${test_executable}"
                                   ARGS "${extra_test_args}"
                                   ${add_test_args}
                                   OUTPUT_FILE "${test_wrapper_file}"
                                   ENVIRONMENT "QT_TEST_RUNNING_IN_CTEST" 1
                                               "PATH" "${test_env_path}"
                                               "QT_PLUGIN_PATH" "${test_env_plugin_path}"
            )
        endif()

        if(arg_QT_TEST_SERVER_LIST AND NOT ANDROID)
            qt_internal_setup_docker_test_fixture(${testname} ${arg_QT_TEST_SERVER_LIST})
        endif()

        set_tests_properties("${testname}" PROPERTIES RUN_SERIAL "${arg_RUN_SERIAL}" LABELS "${label}")
        if(arg_TIMEOUT)
            set_tests_properties(${testname} PROPERTIES TIMEOUT ${arg_TIMEOUT})
        endif()

        if(ANDROID AND NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
            # Set timeout signal and some time for androidtestrunner to do cleanup
            set_tests_properties(${testname} PROPERTIES
                TIMEOUT_SIGNAL_NAME "SIGINT"
                TIMEOUT_SIGNAL_GRACE_PERIOD 10.0
            )
        endif()

        # Add a ${target}_check makefile target, to more easily test one test.
        # TODO: Note in batch mode testname tests would execute all batched tests defined in name
        _qt_internal_make_check_target(${testname} CTEST_TEST_NAME ${name})
        # Add appropriate dependencies to the targets as needed
        if(TARGET "${name}")
            add_dependencies("${testname}_check" "${name}")
            if(ANDROID)
                add_dependencies("${testname}_check" "${name}_make_apk")
            endif()
        endif()
    endif()

    if(ANDROID OR IOS OR WASM OR INTEGRITY OR arg_BUILTIN_TESTDATA OR QT_FORCE_BUILTIN_TESTDATA)
        set(builtin_testdata TRUE)
    endif()

    if(builtin_testdata)
        # Safe guard against qml only tests, no source files == no target
        if (TARGET "${name}")
            target_compile_definitions("${name}" PRIVATE BUILTIN_TESTDATA)

            foreach(testdata IN LISTS arg_TESTDATA)
                list(APPEND builtin_files ${testdata})
            endforeach()
            foreach(file IN LISTS builtin_files)
                set_source_files_properties(${file}
                    PROPERTIES QT_SKIP_QUICKCOMPILER TRUE
                )
            endforeach()

            if(batch_current_test)
                set(blacklist_path "BLACKLIST")
                if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${blacklist_path}")
                    get_target_property(blacklist_files ${name} _qt_blacklist_files)
                    if(NOT blacklist_files)
                        set_target_properties(${name} PROPERTIES _qt_blacklist_files "")
                        set(blacklist_files "")
                    endif()
                    list(PREPEND blacklist_files "${CMAKE_CURRENT_SOURCE_DIR}/${blacklist_path}")
                    set_target_properties(${name} PROPERTIES
                        _qt_blacklist_files "${blacklist_files}")
                endif()
            else()
                set(blacklist_path "BLACKLIST")
                if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${blacklist_path}")
                    list(APPEND builtin_files ${blacklist_path})
                endif()
            endif()

            list(REMOVE_DUPLICATES builtin_files)

            # Skip Qt quick compiler when embedding test resources
            foreach(file IN LISTS builtin_files)
                set_source_files_properties(${file}
                    PROPERTIES QT_SKIP_QUICKCOMPILER TRUE
                )
            endforeach()

            if(builtin_files)
                qt_internal_add_resource(${name} "${testname}_testdata_builtin"
                    PREFIX "/"
                    FILES ${builtin_files}
                    BASE ${CMAKE_CURRENT_SOURCE_DIR})
            endif()
        endif()
    else()
        # Install test data, when tests are built in-tree or as standalone tests, but not as a
        # single standalone test, which is checked by the existence of the QT_TOP_LEVEL_SOURCE_DIR
        # variable.
        # TODO: Shouldn't we also handle the single standalone test case?
        # TODO: Does installing even makes sense, given where QFINDTESTDATA looks for installed
        # test data, and where we end up installing it? See QTBUG-117098.
        if(QT_TOP_LEVEL_SOURCE_DIR)
            foreach(testdata IN LISTS arg_TESTDATA)
                set(testdata "${CMAKE_CURRENT_SOURCE_DIR}/${testdata}")

                # Get the relative source dir for each test data entry, because it might contain a
                # subdirectory.
                file(RELATIVE_PATH relative_path_to_test_project
                    "${QT_TOP_LEVEL_SOURCE_DIR}"
                    "${testdata}")
                get_filename_component(relative_path_to_test_project
                    "${relative_path_to_test_project}" DIRECTORY)

                qt_path_join(testdata_install_dir ${QT_INSTALL_DIR}
                             "${relative_path_to_test_project}")

                if (IS_DIRECTORY "${testdata}")
                    qt_install(
                        DIRECTORY "${testdata}"
                        DESTINATION "${testdata_install_dir}")
                else()
                    qt_install(
                        FILES "${testdata}"
                        DESTINATION "${testdata_install_dir}")
                endif()
            endforeach()
        endif()
    endif()

    if(MACOS AND NOT CMAKE_GENERATOR STREQUAL "Xcode")
        # Add com.apple.security.get-task-allow entitlement to each
        # test binary, so we can hook into the Swift crash handling.
        if(NOT arg_QMLTEST AND arg_SOURCES)
            set(entitlements_file
                "${__qt_internal_cmake_apple_support_files_path}/test.entitlements.plist")
            add_custom_command(TARGET "${name}"
                POST_BUILD COMMAND codesign --sign -
                    --entitlements "${entitlements_file}"
                    "$<TARGET_FILE:${name}>"
                )
        endif()
    endif()

    qt_internal_add_test_finalizers("${name}")
endfunction()

# Generates a blacklist file for the global batched test target.
function(qt_internal_finalize_test_batch_blacklist)
    _qt_internal_test_batch_target_name(batch_target_name)
    if(NOT TARGET "${batch_target_name}")
        return()
    endif()

    set(generated_blacklist_file "${CMAKE_CURRENT_BINARY_DIR}/BLACKLIST")

    set(final_contents "")

    get_target_property(blacklist_files "${batch_target_name}" _qt_blacklist_files)
    if(blacklist_files)
        foreach(blacklist_file ${blacklist_files})
            file(READ "${blacklist_file}" file_contents)
            if(file_contents)
                string(APPEND final_contents "${file_contents}\n")
            endif()
        endforeach()
    endif()

    qt_configure_file(OUTPUT "${generated_blacklist_file}" CONTENT "${final_contents}")

    qt_internal_add_resource(${batch_target_name} "batch_blacklist"
        PREFIX "/"
        FILES "${generated_blacklist_file}"
        BASE ${CMAKE_CURRENT_BINARY_DIR})
endfunction()

# Given an optional test timeout value (specified via qt_internal_add_test's TIMEOUT option)
# returns a percentage of the final timeout to be passed to the androidtestrunner executable.
#
# When the optional timeout is empty, default to cmake's defaults for getting the timeout.
function(qt_internal_get_android_test_timeout input_timeout percentage output_timeout_var)
    set(actual_timeout "${input_timeout}")
    if(NOT actual_timeout)
        if(DART_TESTING_TIMEOUT)
            # Related: https://gitlab.kitware.com/cmake/cmake/-/issues/20450
            set(actual_timeout "${DART_TESTING_TIMEOUT}")
        elseif(CTEST_TEST_TIMEOUT)
            set(actual_timeout "${CTEST_TEST_TIMEOUT}")
        else()
            # Default DART_TESTING_TIMEOUT is 25 minutes, specified in seconds
            # https://github.com/Kitware/CMake/blob/master/Modules/CTest.cmake#L167C16-L167C16
            set(actual_timeout "1500")
        endif()
    endif()

    math(EXPR calculated_timeout "${actual_timeout} * ${percentage} / 100")

    set(${output_timeout_var} "${calculated_timeout}" PARENT_SCOPE)
endfunction()

# This function adds test with specified NAME and wraps given test COMMAND with standalone cmake
# script.
#
# NAME must be compatible with add_test function, since it's propagated as is.
# COMMAND might be either target or path to executable. When test is called either by ctest or
# directly by 'cmake -P path/to/scriptWrapper.cmake', COMMAND will be executed in specified
# WORKING_DIRECTORY with arguments specified in ARGS.
#
# See also _qt_internal_create_command_script for details.
function(qt_internal_create_test_script)
    #This style of parsing keeps ';' in ENVIRONMENT variables
    cmake_parse_arguments(PARSE_ARGV 0 arg
                          ""
                          "NAME;COMMAND;OUTPUT_FILE;WORKING_DIRECTORY"
                          "ARGS;ENVIRONMENT;PRE_RUN;POST_RUN"
    )

    if(NOT arg_COMMAND)
        message(FATAL_ERROR "qt_internal_create_test_script: Test COMMAND is not specified")
    endif()

    if(NOT arg_NAME)
        message(FATAL_ERROR "qt_internal_create_test_script: Test NAME is not specified")
    endif()

    if(NOT arg_OUTPUT_FILE)
        message(FATAL_ERROR "qt_internal_create_test_script: Test Wrapper OUTPUT_FILE\
is not specified")
    endif()

    if(arg_PRE_RUN)
        message(WARNING "qt_internal_create_test_script: PRE_RUN is not acceptable argument\
for this function. Will be ignored")
    endif()

    if(arg_POST_RUN)
        message(WARNING "qt_internal_create_test_script: POST_RUN is not acceptable argument\
for this function. Will be ignored")
    endif()

    if(arg_ARGS)
        set(command_args ${arg_ARGS})# Avoid "${arg_ARGS}" usage and let cmake expand string to
                                    # semicolon-separated list
    endif()

    if(TARGET ${arg_COMMAND})
        set(executable_file "$<TARGET_FILE:${arg_COMMAND}>")
    else()
        set(executable_file "${arg_COMMAND}")
    endif()

    set(executable_name ${arg_NAME})
    qt_internal_is_in_test_batch(is_in_batch ${executable_name})
    if(is_in_batch)
        _qt_internal_test_batch_target_name(executable_name)
    endif()

    set(add_test_working_dir "")
    if(arg_WORKING_DIRECTORY)
        list(APPEND add_test_working_dir WORKING_DIRECTORY "${arg_WORKING_DIRECTORY}")
    endif()

    add_test(NAME "${arg_NAME}" COMMAND "${CMAKE_COMMAND}" "-P" "${arg_OUTPUT_FILE}"
                ${add_test_working_dir})

    # If crosscompiling is enabled, we should avoid run cmake in emulator environment.
    # Prepend emulator to test command in generated cmake script instead. Keep in mind that
    # CROSSCOMPILING_EMULATOR don't check if actual cross compilation is configured,
    # emulator is prepended independently.
    set(crosscompiling_emulator "")
    if(CMAKE_CROSSCOMPILING AND TARGET ${executable_name})
        get_target_property(crosscompiling_emulator ${executable_name} CROSSCOMPILING_EMULATOR)
        if(NOT crosscompiling_emulator)
            set(crosscompiling_emulator "")
        endif()
    endif()

    _qt_internal_create_command_script(COMMAND
                                           ${crosscompiling_emulator}
                                           "\${env_test_runner}"
                                           "${executable_file}"
                                           "\${env_test_args}"
                                           ${command_args}
                                      OUTPUT_FILE "${arg_OUTPUT_FILE}"
                                      ${add_test_working_dir}
                                      ENVIRONMENT ${arg_ENVIRONMENT}
                                      PRE_RUN "separate_arguments(env_test_args NATIVE_COMMAND \
\"\$ENV{TESTARGS}\")"
                                              "separate_arguments(env_test_runner NATIVE_COMMAND \
\"\$ENV{TESTRUNNER}\")"
    )
endfunction()



# This function creates an executable for use as a helper program with tests. Some
# tests launch separate programs to test certain input/output behavior.
# Specify OVERRIDE_OUTPUT_DIRECTORY if you don't want to place the helper in the parent directory,
# in which case you should specify OUTPUT_DIRECTORY "/foo/bar" manually.
function(qt_internal_add_test_helper name)

    set(qt_add_test_helper_optional_args
        "OVERRIDE_OUTPUT_DIRECTORY"
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${qt_add_test_helper_optional_args};${__qt_internal_add_executable_optional_args}"
        "${__qt_internal_add_executable_single_args}"
        "${__qt_internal_add_executable_multi_args}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    qt_remove_args(forward_args
        ARGS_TO_REMOVE
            "${name}"
            ${qt_add_test_helper_optional_args}
        ALL_ARGS
            ${qt_add_test_helper_optional_args}
            ${__qt_internal_add_executable_optional_args}
            ${__qt_internal_add_executable_single_args}
            ${__qt_internal_add_executable_multi_args}
        ARGS
            ${ARGV}
    )

    set(extra_args_to_pass)
    if(NOT arg_OVERRIDE_OUTPUT_DIRECTORY)
        _qt_internal_test_batch_target_name(test_batch_target_name)
        if(QT_BUILD_TESTS_BATCHED AND TARGET ${test_batch_target_name})
            get_target_property(
                test_batch_output_dir ${test_batch_target_name} RUNTIME_OUTPUT_DIRECTORY)
            set(extra_args_to_pass OUTPUT_DIRECTORY "${test_batch_output_dir}")
        else()
            set(extra_args_to_pass OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/..")
        endif()
    endif()

    qt_internal_add_executable("${name}" NO_INSTALL
                                         NO_UNITY_BUILD # excluded by default
                                         ${extra_args_to_pass} ${forward_args})

    # Disable the QT_NO_NARROWING_CONVERSIONS_IN_CONNECT define for test helpers
    qt_internal_undefine_global_definition(${name} QT_NO_NARROWING_CONVERSIONS_IN_CONNECT)

endfunction()

function(qt_internal_collect_command_environment out_path out_plugin_path)
    # Get path to <qt_relocatable_install_prefix>/bin, as well as CMAKE_INSTALL_PREFIX/bin, and
    # combine them with the PATH environment variable.
    # It's needed on Windows to find the shared libraries and plugins.
    # qt_relocatable_install_prefix is dynamically computed from the location of where the Qt CMake
    # package is found.
    # The regular CMAKE_INSTALL_PREFIX can be different for example when building standalone tests.
    # Any given CMAKE_INSTALL_PREFIX takes priority over qt_relocatable_install_prefix for the
    # PATH environment variable.
    set(install_prefixes "${CMAKE_INSTALL_PREFIX}")
    if(QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX)
        list(APPEND install_prefixes "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")
    endif()

    file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}" test_env_path)
    foreach(install_prefix ${install_prefixes})
        file(TO_NATIVE_PATH "${install_prefix}/${INSTALL_BINDIR}" install_prefix)
        set(test_env_path "${test_env_path}${QT_PATH_SEPARATOR}${install_prefix}")
    endforeach()
    set(test_env_path "${test_env_path}${QT_PATH_SEPARATOR}$ENV{PATH}")
    string(REPLACE ";" "\;" test_env_path "${test_env_path}")
    set(${out_path} "${test_env_path}" PARENT_SCOPE)

    # Add the install prefix to list of plugin paths when doing a prefix build
    if(NOT QT_INSTALL_DIR)
        foreach(install_prefix ${install_prefixes})
            file(TO_NATIVE_PATH "${install_prefix}/${INSTALL_BINDIR}" install_prefix)
            list(APPEND plugin_paths "${install_prefix}")
        endforeach()
    endif()

    #TODO: Collect all paths from known repositories when performing a super
    # build.
    file(TO_NATIVE_PATH "${PROJECT_BINARY_DIR}/${INSTALL_PLUGINSDIR}" install_pluginsdir)
    list(APPEND plugin_paths "${install_pluginsdir}")
    list(JOIN plugin_paths "${QT_PATH_SEPARATOR}" plugin_paths_joined)
    string(REPLACE ";" "\;" plugin_paths_joined "${plugin_paths_joined}")
    set(${out_plugin_path} "${plugin_paths_joined}" PARENT_SCOPE)
endfunction()

function(qt_internal_add_test_finalizers target)
    # Opt out to skip the new way of running test finalizers, and instead use the old way for
    # specific platforms.
    # TODO: Remove once we confirm that the new way of running test finalizers for all platforms
    # doesn't cause any issues.
    if(NOT QT_INTERNAL_SKIP_TEST_FINALIZERS_V2)
        return()
    endif()

    # It might not be safe to run all the finalizers of _qt_internal_finalize_executable
    # within the context of a Qt build (not a user project) when targeting a host build.
    # At least one issue is missing qmlimportscanner at configure time.
    # For now, we limit it to iOS, where it was tested to work, an we know that host tools
    # should already be built and available.
    if(IOS)
        qt_add_list_file_finalizer(_qt_internal_finalize_executable "${target}")
    endif()
endfunction()
