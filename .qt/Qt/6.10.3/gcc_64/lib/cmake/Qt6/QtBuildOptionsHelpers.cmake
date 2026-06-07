# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Try to detect if CMAKE_BUILD_TYPE is default initialized by CMake, or it was set by the user.
#
# CMake initializes CMAKE_BUILD_TYPE to the value of CMAKE_BUILD_TYPE_INIT during the first
# project() call if CMAKE_BUILD_TYPE is empty.
#
# Unfortunately on most Windows platforms, it defaults to 'Debug', so we can't differentiate
# between a 'Debug' value set on the command line by the user, a value set by the project, or if it
# was default initialized.
# We need to rely on heuristics to determine that.
#
# We try to check the value of CMAKE_BUILD_TYPE before the first project() call by inspecting
# various variables:
# 1) When using a qt.toolchain.cmake file, we rely on the toolchain file to tell us
#    if a value was set by the user at initial configure time via the
#    __qt_toolchain_cmake_build_type_before_project_call variable. On a 2nd run there will
#    always be a value in the cache, but at that point we've already set it to whatever it needs
#    to be.
# 2) Whe configuring qtbase, a top-level qt, or a standalone project we rely on one of the following
#    variables being set:
#    - __qt_auto_detect_cmake_build_type_before_project_call (e.g for qtbase)
#    - __qt_internal_standalone_project_cmake_build_type_before_project_call (e.g for sqldrivers)
# 3) When using a multi-config generator, we assume that the CMAKE_BUILD_TYPE is not default
#    initialized.
# 4) The user can also force the build type to be considered non-default-initialized by setting
#    QT_NO_FORCE_SET_CMAKE_BUILD_TYPE to TRUE. It has weird naming that doesn't quite correspond
#    to the meaning, but it's been called like that for a while now and I'm hesitant to change
#    the name in case it's used by various projects.
#
# The code doesn't handle an empty "" config set by the user, but we claim that's an
# unsupported config when building Qt.
function(qt_internal_is_cmake_build_type_default_initialized_heuristic out_var)
    get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    get_cmake_property(aready_force_set _qt_build_internals_cmake_build_type_set)

    if(
        # Set by CMake's Platform/Windows-MSVC.cmake when CMAKE_BUILD_TYPE is empty
        # The STREQUAL check needs to have expanded variables because an undefined var is not equal
        # to an empty defined var.
        "${CMAKE_BUILD_TYPE}" STREQUAL "${CMAKE_BUILD_TYPE_INIT}"

        # Set by qt_internal_force_set_cmake_build_type()
        AND aready_force_set MATCHES "NOTFOUND"

        # Set by qt_auto_detect_cmake_build_type()
        AND NOT __qt_auto_detect_cmake_build_type_before_project_call

        # Set by sqldrivers project
        AND NOT __qt_internal_standalone_project_cmake_build_type_before_project_call

        # Set by qt.toolchain.cmake
        AND NOT __qt_toolchain_cmake_build_type_before_project_call

        # Set by user explicitily
        AND NOT QT_NO_FORCE_SET_CMAKE_BUILD_TYPE

        # Set in multi-config builds
        AND NOT is_multi_config)

        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(qt_internal_force_set_cmake_build_type value)
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "SHOW_MESSAGE"
        ""
        ""
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    set(CMAKE_BUILD_TYPE "${value}" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE
        PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo") # Set the possible values for cmake-gui.
    if(arg_SHOW_MESSAGE)
        message(STATUS "Force setting build type to '${value}'.")
    endif()
    set_property(GLOBAL PROPERTY _qt_build_internals_cmake_build_type_set "${value}")
endfunction()

# Only override the build type if it was default initialized by CMake.
function(qt_internal_force_set_cmake_build_type_if_cmake_default_initialized value)
    qt_internal_is_cmake_build_type_default_initialized_heuristic(is_default_cmake_build_type)
    if(is_default_cmake_build_type)
        qt_internal_force_set_cmake_build_type("${value}" SHOW_MESSAGE)
    endif()
endfunction()

function(qt_internal_set_cmake_build_type)
    # When building standalone tests against a multi-config Qt, we want to configure the
    # tests / examples with
    # the first multi-config configuration, rather than use CMake's default configuration.
    # In the case of Windows, we definitely don't want it to default to Debug, because that causes
    # issues in the CI.
    if(QT_INTERNAL_BUILD_STANDALONE_PARTS AND QT_MULTI_CONFIG_FIRST_CONFIG)
        qt_internal_force_set_cmake_build_type_if_cmake_default_initialized(
            "${QT_MULTI_CONFIG_FIRST_CONFIG}")

    # We want the same build type to be used when configuring all Qt repos or standalone
    # tests or single tests, so we reuse the initial build type set by qtbase.
    # __qt_internal_initial_qt_cmake_build_type is saved in QtBuildInternalsExtra.cmake.in.
    elseif(__qt_internal_initial_qt_cmake_build_type)
        qt_internal_force_set_cmake_build_type_if_cmake_default_initialized(
            "${__qt_internal_initial_qt_cmake_build_type}")

    # Default to something sensible when configuring qtbase / top-level.
    else()
        qt_internal_set_qt_appropriate_default_cmake_build_type()
    endif()
endfunction()

# Sets a default cmake build type for qtbase / top-level.
macro(qt_internal_set_qt_appropriate_default_cmake_build_type)
    set(_default_build_type "Release")
    if(FEATURE_developer_build)
        set(_default_build_type "Debug")
    endif()

    qt_internal_is_cmake_build_type_default_initialized_heuristic(is_default_cmake_build_type)
    if(is_default_cmake_build_type)
        qt_internal_force_set_cmake_build_type("${_default_build_type}")
        message(STATUS "Setting build type to '${_default_build_type}' as none was specified.")
    elseif(CMAKE_CONFIGURATION_TYPES)
        message(STATUS "Building for multiple configurations: ${CMAKE_CONFIGURATION_TYPES}.")
        message(STATUS "Main configuration is: ${QT_MULTI_CONFIG_FIRST_CONFIG}.")
        if(CMAKE_NINJA_MULTI_DEFAULT_BUILD_TYPE)
            message(STATUS
                "Default build configuration set to '${CMAKE_NINJA_MULTI_DEFAULT_BUILD_TYPE}'.")
        endif()
        if(CMAKE_GENERATOR STREQUAL "Ninja")
            message(FATAL_ERROR
                "It's not possible to build multiple configurations with the single config Ninja "
                "generator. Consider configuring with -G\"Ninja Multi-Config\" instead of -GNinja."
            )
        endif()
    else()
        message(STATUS "CMAKE_BUILD_TYPE was already explicitly set to: '${CMAKE_BUILD_TYPE}'")
    endif()
endmacro()

macro(qt_internal_set_configure_from_ide)
    # QT_INTERNAL_CONFIGURE_FROM_IDE is set to TRUE for the following known IDE applications:
    # - Qt Creator, detected by QTC_RUN environment variable
    # - CLion, detected by CLION_IDE environment variable
    # - Visual Studio Code, detected by VSCODE_CLI environment variable
    if("$ENV{QTC_RUN}" OR "$ENV{CLION_IDE}" OR "$ENV{VSCODE_CLI}")
        set(QT_INTERNAL_CONFIGURE_FROM_IDE TRUE CACHE INTERNAL "Configuring Qt Project from IDE")
    else()
        set(QT_INTERNAL_CONFIGURE_FROM_IDE FALSE CACHE INTERNAL "Configuring Qt Project from IDE")
    endif()
endmacro()

macro(qt_internal_set_sync_headers_at_configure_time)
    set(_qt_sync_headers_at_configure_time_default ${QT_INTERNAL_CONFIGURE_FROM_IDE})

    if(FEATURE_developer_build)
        # Sync headers during the initial configuration of a -developer-build to facilitate code
        # navigation for code editors that use an LSP-based code model.
        set(_qt_sync_headers_at_configure_time_default TRUE)
    endif()

    # Sync Qt header files at configure time
    option(QT_SYNC_HEADERS_AT_CONFIGURE_TIME "Run syncqt at configure time already"
        ${_qt_sync_headers_at_configure_time_default})
    unset(_qt_sync_headers_at_configure_time_default)

    # In static Ninja Multi-Config builds the sync_headers dependencies(and other autogen
    # dependencies are not added to '_autogen/timestamp' targets. See QTBUG-113974.
    if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config" AND NOT QT_BUILD_SHARED_LIBS)
        set(QT_SYNC_HEADERS_AT_CONFIGURE_TIME TRUE CACHE BOOL "" FORCE)
    endif()
endmacro()

macro(qt_internal_set_export_compile_commands)
    if(FEATURE_developer_build)
        if(DEFINED QT_CMAKE_EXPORT_COMPILE_COMMANDS)
            set(CMAKE_EXPORT_COMPILE_COMMANDS ${QT_CMAKE_EXPORT_COMPILE_COMMANDS})
        else()
            set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
        endif()
    endif()
endmacro()

macro(qt_internal_setup_build_benchmarks)
    if(FEATURE_developer_build)
        set(__build_benchmarks ON)

        # Disable benchmarks for single configuration generators which do not build
        # with release configuration.
        if(CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE STREQUAL Debug)
            set(__build_benchmarks OFF)
        endif()
    else()
        set(__build_benchmarks OFF)
    endif()

    # Build Benchmarks
    option(QT_BUILD_BENCHMARKS "Build Qt Benchmarks" ${__build_benchmarks})
endmacro()

macro(qt_internal_setup_build_tests)
    if(FEATURE_developer_build)
        set(_qt_build_tests_default ON)

        # Tests are not built by default with qmake for iOS and friends, and thus the overall build
        # tends to fail. Disable them by default when targeting uikit.
        if(UIKIT OR ANDROID)
            set(_qt_build_tests_default OFF)
        endif()
    else()
        set(_qt_build_tests_default OFF)
    endif()

    # If benchmarks are explicitly enabled, force tests to also be built, even if they might
    # not work on the platform.
    if(QT_BUILD_BENCHMARKS)
        set(_qt_build_tests_default ON)
    endif()

    ## Set up testing
    option(QT_BUILD_TESTS "Build the testing tree." ${_qt_build_tests_default})
    unset(_qt_build_tests_default)
    option(QT_BUILD_TESTS_BY_DEFAULT
        "Should tests be built as part of the default 'all' target." ON)
    if(QT_BUILD_STANDALONE_TESTS)
        # BuildInternals might have set it to OFF on initial configuration. So force it to ON when
        # building standalone tests.
        set(QT_BUILD_TESTS ON CACHE BOOL "Build the testing tree." FORCE)

        # Also force the tests to be built as part of the default build target.
        set(QT_BUILD_TESTS_BY_DEFAULT ON CACHE BOOL
            "Should tests be built as part of the default 'all' target." FORCE)
    endif()
    set(BUILD_TESTING ${QT_BUILD_TESTS} CACHE INTERNAL "")

    if(WASM)
        set(_qt_batch_tests ON)
    else()
        set(_qt_batch_tests OFF)
    endif()

    if(DEFINED INPUT_batch_tests)
        if (${INPUT_batch_tests})
            set(_qt_batch_tests ON)
        else()
            set(_qt_batch_tests OFF)
        endif()
    endif()

    option(QT_BUILD_TESTS_BATCHED "Link all tests into a single binary." ${_qt_batch_tests})

    if(QT_BUILD_TESTS AND QT_BUILD_TESTS_BATCHED AND CMAKE_VERSION VERSION_LESS "3.19")
        message(FATAL_ERROR
            "Test batching requires at least CMake 3.19, due to requiring per-source "
            "TARGET_DIRECTORY assignments and DEFER calls.")
    endif()

    option(QT_BUILD_MANUAL_TESTS "Build Qt manual tests" OFF)

    if(WASM AND _qt_batch_tests)
        set(_qt_wasm_and_batch_tests ON)
    else()
        set(_qt_wasm_and_batch_tests OFF)
    endif()

    option(QT_BUILD_MINIMAL_STATIC_TESTS "Build minimal subset of tests for static Qt builds" ${_qt_wasm_and_batch_tests})

    option(QT_BUILD_WASM_BATCHED_TESTS "Build subset of tests for wasm batched tests" ${_qt_wasm_and_batch_tests})

    option(QT_BUILD_MINIMAL_ANDROID_MULTI_ABI_TESTS
        "Build minimal subset of tests for Android multi-ABI Qt builds" OFF)

    include(CTest)
    enable_testing()
endmacro()

macro(qt_internal_setup_build_tools)
    # QT_BUILD_TOOLS_WHEN_CROSSCOMPILING -> QT_FORCE_BUILD_TOOLS
    # pre-6.4 compatibility flag (remove sometime in the future)
    if(CMAKE_CROSSCOMPILING AND QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
        message(WARNING "QT_BUILD_TOOLS_WHEN_CROSSCOMPILING is deprecated. "
            "Please use QT_FORCE_BUILD_TOOLS instead.")
        set(QT_FORCE_BUILD_TOOLS TRUE CACHE INTERNAL "" FORCE)
    endif()

    # When cross-building, we don't build tools by default. Sometimes this also covers Qt apps as
    # well. Like in qttools/assistant/assistant.pro, load(qt_app), which is guarded by a
    # qtNomakeTools() call.
    set(_qt_build_tools_by_default_default ON)
    if(CMAKE_CROSSCOMPILING AND NOT QT_FORCE_BUILD_TOOLS)
        set(_qt_build_tools_by_default_default OFF)
    endif()
    option(QT_BUILD_TOOLS_BY_DEFAULT "Should tools be built as part of the default 'all' target."
           "${_qt_build_tools_by_default_default}")
    unset(_qt_build_tools_by_default_default)
endmacro()

# A heuristic to determine that the currently processed project is a cmake build test project.
function(qt_internal_current_project_is_cmake_build_test_project out_var)
    set(current_dir "${CMAKE_CURRENT_SOURCE_DIR}")

    set(result FALSE)

    if(current_dir MATCHES "tests/auto/cmake")
        set(result TRUE)
    endif()

    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(qt_internal_compute_sbom_default out_var)
    # Default to generating the SBOM, except for:
    # - developer-builds
    # - no-prefix builds.
    # - standalone tests or examples
    # - cmake build tests
    # Regular developers of Qt (which would pass -developer-build) likely don't need SBOMs.
    # -no-prefix builds don't make much sense for SBOMs because the installed file checksums
    # will be missing, and thus the SBOMs will not be complete / valid.
    # Honor any explicitly or previously set value.
    qt_internal_current_project_is_cmake_build_test_project(is_cmake_build_test)

    if(FEATURE_developer_build
            OR QT_FEATURE_developer_build
            OR FEATURE_no_prefix
            OR QT_FEATURE_no_prefix
            OR QT_BUILD_STANDALONE_EXAMPLES
            OR QT_BUILD_STANDALONE_TESTS
            OR QT_INTERNAL_BUILD_STANDALONE_PARTS
            OR is_cmake_build_test
        )
        set(enable_sbom OFF)
    else()
        set(enable_sbom ON)
    endif()
    set(${out_var} "${enable_sbom}" PARENT_SCOPE)
endfunction()

macro(qt_internal_setup_sbom)
    qt_internal_compute_sbom_default(_qt_generate_sbom_default)

    _qt_internal_setup_sbom(
        GENERATE_SBOM_DEFAULT "${_qt_generate_sbom_default}"
    )
endmacro()

macro(qt_internal_setup_build_examples)
    option(QT_BUILD_EXAMPLES "Build Qt examples" OFF)
    option(QT_BUILD_EXAMPLES_BY_DEFAULT
        "Should examples be built as part of the default 'all' target." ON)
    option(QT_INSTALL_EXAMPLES_SOURCES "Install example sources" OFF)
    option(QT_INSTALL_EXAMPLES_SOURCES_BY_DEFAULT
        "Install example sources as part of the default 'install' target" ON)

    # We need a way to force disable building in-tree examples in the CI, so that we instead build
    # standalone examples. Because the Coin yaml instructions don't allow us to remove
    # -make examples from from the configure args, we instead read a variable that only Coin sets.
    if(QT_INTERNAL_CI_NO_BUILD_IN_TREE_EXAMPLES)
        set(QT_BUILD_EXAMPLES OFF CACHE BOOL "Build Qt examples" FORCE)
    endif()

    if(QT_BUILD_STANDALONE_EXAMPLES)
        # BuildInternals might have set it to OFF on initial configuration. So force it to ON when
        # building standalone examples.
        set(QT_BUILD_EXAMPLES ON CACHE BOOL "Build Qt examples" FORCE)

        # Also force the examples to be built as part of the default build target.
        set(QT_BUILD_EXAMPLES_BY_DEFAULT ON CACHE BOOL
            "Should examples be built as part of the default 'all' target." FORCE)
    endif()

    option(QT_DEPLOY_MINIMAL_EXAMPLES
        "Deploy minimal subset of examples to save time and space" OFF)

    # FIXME: Support prefix builds as well QTBUG-96232
    # We don't want to enable EP examples with -debug-and-release because starting with CMake 3.24
    # ExternalProject_Add ends up creating build rules twice, once for each configuration, in the
    # same build dir, which ends up causing various issues due to concurrent builds as well as
    # clobbered CMakeCache.txt and ninja files.
    if(QT_WILL_INSTALL OR QT_FEATURE_debug_and_release)
        set(_qt_build_examples_as_external OFF)
    else()
        set(_qt_build_examples_as_external ON)
    endif()
    option(QT_BUILD_EXAMPLES_AS_EXTERNAL "Should examples be built as ExternalProjects."
           ${_qt_build_examples_as_external})
    unset(_qt_build_examples_as_external)
endmacro()

macro(qt_internal_set_qt_host_path)
    ## Path used to find host tools, either when cross-compiling or just when using the tools from
    ## a different host build.
    set(QT_HOST_PATH "$ENV{QT_HOST_PATH}" CACHE PATH
        "Installed Qt host directory path, used for cross compiling.")
endmacro()

macro(qt_internal_setup_build_docs)
    option(QT_BUILD_DOCS "Generate Qt documentation targets" ON)
endmacro()

macro(qt_internal_setup_build_java_docs_on_host)
    option(QT_BUILD_HOST_JAVA_DOCS "Generate Java documentation targets on host" OFF)
    if(QT_BUILD_HOST_JAVA_DOCS)
        find_package(Java)
        if(Java_FOUND)
            include(UseJava)
        endif()
    endif()
endmacro()

macro(qt_internal_set_use_ccache)
    option(QT_USE_CCACHE "Enable the use of ccache")
    if(QT_USE_CCACHE)
        find_program(CCACHE_PROGRAM ccache)
        if(CCACHE_PROGRAM)
            set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
            set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
            set(CMAKE_OBJC_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
            set(CMAKE_OBJCXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        else()
            message(FATAL_ERROR "Ccache use was requested, but the program was not found.")
        endif()
    endif()
endmacro()

macro(qt_internal_set_unity_build)
    option(QT_UNITY_BUILD "Enable unity (jumbo) build")
    set(QT_UNITY_BUILD_BATCH_SIZE "32" CACHE STRING "Unity build batch size")

    include(CMakeDependentOption)
    string(TOLOWER "${PROJECT_NAME}" project_name_lower)
    cmake_dependent_option(
        "QT_UNITY_BUILD_PROJECT_${project_name_lower}"
        "Enable unity builds for project ${PROJECT_NAME}"
        TRUE
        QT_UNITY_BUILD
        FALSE
    )

    set(CMAKE_UNITY_BUILD "${QT_UNITY_BUILD_PROJECT_${project_name_lower}}")
    set(CMAKE_UNITY_BUILD_BATCH_SIZE "${QT_UNITY_BUILD_BATCH_SIZE}")
endmacro()

macro(qt_internal_set_allow_symlink_in_paths)
    option(QT_ALLOW_SYMLINK_IN_PATHS "Allows symlinks in paths." OFF)
endmacro()

macro(qt_internal_set_qt_allow_download)
    option(QT_ALLOW_DOWNLOAD "Allows files to be downloaded when building Qt." OFF)
endmacro()
