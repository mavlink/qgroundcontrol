cmake_minimum_required(VERSION 3.25)

project(QGCCMakeModuleContracts LANGUAGES CXX)

if(NOT QGC_MODULE_DIR)
    message(FATAL_ERROR "QGC_MODULE_DIR is required")
endif()
if(NOT TEST_CASE)
    message(FATAL_ERROR "TEST_CASE is required")
endif()

if(TEST_CASE MATCHES "^compiler-warnings-(on|off)$")
    add_executable(${PROJECT_NAME} main.cpp)
    if(CMAKE_MATCH_1 STREQUAL "on")
        set(QGC_ENABLE_WERROR ON)
    else()
        set(QGC_ENABLE_WERROR OFF)
    endif()
    include("${QGC_MODULE_DIR}/CompilerWarnings.cmake")
    qgc_apply_warning_flags()

    get_target_property(_warnings_as_errors ${PROJECT_NAME} COMPILE_WARNING_AS_ERROR)
    if(NOT _warnings_as_errors STREQUAL "${QGC_ENABLE_WERROR}")
        message(FATAL_ERROR "COMPILE_WARNING_AS_ERROR should be ${QGC_ENABLE_WERROR}")
    endif()
    get_target_property(_compile_options ${PROJECT_NAME} COMPILE_OPTIONS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT "-Wall" IN_LIST _compile_options)
        message(FATAL_ERROR "Warning flags should remain enabled when QGC_ENABLE_WERROR is off")
    endif()
    return()
endif()

if(TEST_CASE STREQUAL "compiler-warnings-project-targets")
    set(QGC_ENABLE_WERROR ON)
    include("${QGC_MODULE_DIR}/CompilerWarnings.cmake")

    add_executable(${PROJECT_NAME} main.cpp)
    add_library(ProjectLibrary STATIC main.cpp)
    add_library(DependencyLibrary STATIC main.cpp)
    set_target_properties(
        ProjectLibrary
        PROPERTIES
            AUTOMOC ON
    )
    qgc_disable_dependency_warnings(DependencyLibrary)

    qgc_configure_target(${PROJECT_NAME})
    qgc_configure_directory_targets("${CMAKE_CURRENT_SOURCE_DIR}")

    get_target_property(_project_configured ProjectLibrary QGC_PROJECT_TARGET_CONFIGURED)
    get_target_property(_dependency_configured DependencyLibrary QGC_PROJECT_TARGET_CONFIGURED)
    get_target_property(_dependency_werror DependencyLibrary COMPILE_WARNING_AS_ERROR)
    if(NOT _project_configured)
        message(FATAL_ERROR "Project library was not configured")
    endif()
    if(_dependency_configured)
        message(FATAL_ERROR "Dependency library was configured as project code")
    endif()
    if(_dependency_werror)
        message(FATAL_ERROR "Dependency library retained warnings-as-errors")
    endif()

    get_target_property(_compile_options ${PROJECT_NAME} COMPILE_OPTIONS)
    get_target_property(_library_compile_options ProjectLibrary COMPILE_OPTIONS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        foreach(_generated_warning IN ITEMS shadow unused-parameter)
            if(NOT "-Wno-error=${_generated_warning}" IN_LIST _library_compile_options)
                message(FATAL_ERROR "AUTOMOC library lacks generated-code exception: ${_generated_warning}")
            endif()
            if("-Wno-error=${_generated_warning}" IN_LIST _compile_options)
                message(FATAL_ERROR "Main target unexpectedly received library exception: ${_generated_warning}")
            endif()
        endforeach()
    endif()
    set_target_properties(
        ProjectLibrary
        PROPERTIES
            AUTOMOC OFF
    )
    set(_wall_count 0)
    foreach(_option IN LISTS _compile_options)
        if(_option STREQUAL "-Wall")
            math(EXPR _wall_count "${_wall_count} + 1")
        endif()
    endforeach()
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT _wall_count EQUAL 1)
        message(FATAL_ERROR "Project target configuration was not idempotent: -Wall count=${_wall_count}")
    endif()
    return()
endif()

if(TEST_CASE STREQUAL "unity-project-targets")
    set(QGC_ENABLE_WERROR OFF)
    set(QGC_UNITY_BUILD ON)
    include("${QGC_MODULE_DIR}/CompilerWarnings.cmake")

    add_executable(${PROJECT_NAME} main.cpp)
    add_library(ProjectLibrary STATIC main.cpp)
    add_library(OptedOutLibrary STATIC main.cpp)
    add_library(DependencyLibrary STATIC main.cpp)
    set_target_properties(
        OptedOutLibrary
        PROPERTIES
            UNITY_BUILD OFF
    )
    qgc_disable_dependency_warnings(DependencyLibrary)

    qgc_configure_target(${PROJECT_NAME})
    qgc_configure_directory_targets("${CMAKE_CURRENT_SOURCE_DIR}")

    foreach(_target IN ITEMS ${PROJECT_NAME} ProjectLibrary)
        get_target_property(_unity ${_target} UNITY_BUILD)
        get_target_property(_batch ${_target} UNITY_BUILD_BATCH_SIZE)
        if(NOT _unity OR NOT _batch EQUAL 16)
            message(FATAL_ERROR "${_target} is not a unity target: UNITY_BUILD=${_unity} batch=${_batch}")
        endif()
    endforeach()
    get_target_property(_opted_out OptedOutLibrary UNITY_BUILD)
    if(_opted_out)
        message(FATAL_ERROR "Unity build overrode a target's own UNITY_BUILD OFF")
    endif()
    get_target_property(_dependency_unity DependencyLibrary UNITY_BUILD)
    if(_dependency_unity)
        message(FATAL_ERROR "Dependency library was built as a unity target")
    endif()

    string(TOLOWER "${PROJECT_NAME}_qmltyperegistrations.cpp" _registrations)
    foreach(
        _aggregate IN ITEMS "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_autogen/mocs_compilation.cpp"
                            "${CMAKE_CURRENT_BINARY_DIR}/${_registrations}"
    )
        get_source_file_property(_skip "${_aggregate}" TARGET_DIRECTORY ${PROJECT_NAME} SKIP_UNITY_BUILD_INCLUSION)
        if(NOT _skip)
            message(FATAL_ERROR "Aggregate unit is batched with sources: ${_aggregate}")
        endif()
    endforeach()
    return()
endif()

if(TEST_CASE STREQUAL "coverage-project-targets")
    set(CMAKE_BUILD_TYPE
        Debug
        CACHE STRING "" FORCE
    )
    set(QGC_ENABLE_COVERAGE ON)
    set(QGC_COVERAGE_LINE_THRESHOLD 30)
    set(QGC_COVERAGE_BRANCH_THRESHOLD 20)
    set(QGC_BUILD_TESTING OFF)
    add_executable(${PROJECT_NAME} main.cpp)
    add_library(ProjectLibrary STATIC main.cpp)
    include("${QGC_MODULE_DIR}/modules/Coverage.cmake")
    qgc_apply_coverage_to_target(${PROJECT_NAME})
    qgc_apply_coverage_to_target(ProjectLibrary)

    get_target_property(_library_compile_options ProjectLibrary COMPILE_OPTIONS)
    get_target_property(_library_link_options ProjectLibrary LINK_OPTIONS)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(_expected_coverage_option "$<$<CONFIG:Debug>:--coverage>")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(_expected_coverage_option "$<$<CONFIG:Debug>:--coverage>")
    else()
        return()
    endif()
    if(NOT _expected_coverage_option IN_LIST _library_compile_options)
        message(FATAL_ERROR "Static library lacks coverage instrumentation: ${_library_compile_options}")
    endif()
    if("$<$<CONFIG:Debug>:-g>" IN_LIST _library_compile_options)
        message(FATAL_ERROR "Coverage overrides the CMAKE_<LANG>_FLAGS_DEBUG debug info level")
    endif()
    if(_library_link_options)
        message(FATAL_ERROR "Static library should not receive link options: ${_library_link_options}")
    endif()
    if(GCOVR_EXECUTABLE
       AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
       AND NOT "--gcov-ignore-parse-errors=negative_hits.warn_once_per_file" IN_LIST GCOVR_COMMON_ARGS
    )
        message(FATAL_ERROR "GCC coverage must tolerate known negative branch counters")
    endif()
    return()
endif()

if(TEST_CASE STREQUAL "coverage-invalid-threshold")
    set(CMAKE_BUILD_TYPE
        Debug
        CACHE STRING "" FORCE
    )
    set(QGC_ENABLE_COVERAGE ON)
    set(QGC_COVERAGE_LINE_THRESHOLD 101)
    set(QGC_COVERAGE_BRANCH_THRESHOLD 20)
    include("${QGC_MODULE_DIR}/modules/Coverage.cmake")
    return()
endif()

if(TEST_CASE STREQUAL "coverage-clean-without-gcovr")
    set(CMAKE_BUILD_TYPE
        Debug
        CACHE STRING "" FORCE
    )
    set(QGC_ENABLE_COVERAGE ON)
    set(QGC_COVERAGE_LINE_THRESHOLD 30)
    set(QGC_COVERAGE_BRANCH_THRESHOLD 20)
    set(QGC_BUILD_TESTING OFF)
    set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH OFF)
    set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH OFF)
    add_executable(${PROJECT_NAME} main.cpp)
    include("${QGC_MODULE_DIR}/modules/Coverage.cmake")
    if(NOT TARGET coverage-clean)
        message(FATAL_ERROR "coverage-clean must be available without gcovr")
    endif()
    return()
endif()

if(TEST_CASE STREQUAL "coverage-release-request")
    set(CMAKE_BUILD_TYPE
        Release
        CACHE STRING "" FORCE
    )
    set(QGC_ENABLE_COVERAGE ON)
    include("${QGC_MODULE_DIR}/modules/Coverage.cmake")
    return()
endif()

if(TEST_CASE STREQUAL "sanitizer-project-targets")
    set(CMAKE_BUILD_TYPE
        Debug
        CACHE STRING "" FORCE
    )
    set(QGC_ENABLE_ASAN
        ON
        CACHE BOOL "" FORCE
    )
    set(QGC_BUILD_TESTING OFF)
    add_executable(${PROJECT_NAME} main.cpp)
    add_library(ProjectLibrary STATIC main.cpp)
    include("${QGC_MODULE_DIR}/modules/Sanitizers.cmake")
    qgc_apply_sanitizers_to_target(${PROJECT_NAME})
    qgc_apply_sanitizers_to_target(ProjectLibrary)

    get_target_property(_library_compile_options ProjectLibrary COMPILE_OPTIONS)
    get_target_property(_library_link_options ProjectLibrary LINK_OPTIONS)
    set(_expected_sanitizer_option "$<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=address>")
    if(NOT _expected_sanitizer_option IN_LIST _library_compile_options)
        message(FATAL_ERROR "Static library lacks sanitizer instrumentation: ${_library_compile_options}")
    endif()
    if(_library_link_options)
        message(FATAL_ERROR "Static library should not receive link options: ${_library_link_options}")
    endif()
    return()
endif()

if(TEST_CASE STREQUAL "sanitizer-release-request")
    set(CMAKE_BUILD_TYPE
        Release
        CACHE STRING "" FORCE
    )
    set(QGC_ENABLE_ASAN
        ON
        CACHE BOOL "" FORCE
    )
    set(QGC_BUILD_TESTING OFF)
    include("${QGC_MODULE_DIR}/modules/Sanitizers.cmake")
    return()
endif()

if(TEST_CASE STREQUAL "coverage-clean")
    set(_coverage_dir "${CMAKE_CURRENT_BINARY_DIR}/coverage-data")
    file(MAKE_DIRECTORY "${_coverage_dir}/nested")
    file(WRITE "${_coverage_dir}/one.gcda" "data")
    file(WRITE "${_coverage_dir}/nested/two.profraw" "data")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" "-DQGC_COVERAGE_BUILD_DIR=${_coverage_dir}" -P
                "${QGC_MODULE_DIR}/modules/CleanCoverage.cmake" COMMAND_ERROR_IS_FATAL ANY
    )
    if(EXISTS "${_coverage_dir}/one.gcda" OR EXISTS "${_coverage_dir}/nested/two.profraw")
        message(FATAL_ERROR "Coverage data was not removed")
    endif()
    return()
endif()

if(TEST_CASE STREQUAL "download-timeout-zero")
    include("${QGC_MODULE_DIR}/modules/Download.cmake")
    qgc_resilient_download(
        FILENAME
        invalid.bin
        DESTINATION_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/downloads"
        RESULT_VAR
        _download_result
        URLS
        "https://invalid.example.invalid/invalid.bin"
        TIMEOUT
        0
    )
    return()
endif()

if(TEST_CASE MATCHES "^helpers-")
    include("${QGC_MODULE_DIR}/Helpers.cmake")
    if(TEST_CASE STREQUAL "helpers-cpm-added")
        # CPM names this variable <PackageName>_ADDED.
        # cmake-lint: disable=C0103
        set(Example_ADDED TRUE)
        qgc_require_cpm_added(Example)
    elseif(TEST_CASE STREQUAL "helpers-moccache-targets")
        set(_moccache_launcher "${CMAKE_CURRENT_BINARY_DIR}/moccache-launcher")
        set(_custom_moc "${CMAKE_CURRENT_BINARY_DIR}/custom-moc")
        set_property(GLOBAL PROPERTY QGC_MOCCACHE_EXECUTABLE "${_moccache_launcher}")
        add_library(AutomocTarget STATIC main.cpp)
        add_library(CustomMocTarget STATIC main.cpp)
        add_library(PlainTarget STATIC main.cpp)
        set_target_properties(
            AutomocTarget
            CustomMocTarget
            PROPERTIES
                AUTOMOC ON
        )
        set_property(TARGET CustomMocTarget PROPERTY AUTOMOC_EXECUTABLE "${_custom_moc}")

        qgc_apply_moccache()

        get_target_property(_automoc_launcher AutomocTarget AUTOMOC_EXECUTABLE)
        get_target_property(_custom_launcher CustomMocTarget AUTOMOC_EXECUTABLE)
        get_property(
            _plain_launcher_set
            TARGET PlainTarget
            PROPERTY AUTOMOC_EXECUTABLE
            SET
        )
        if(NOT "${_automoc_launcher}" STREQUAL "${_moccache_launcher}")
            message(FATAL_ERROR "AUTOMOC target did not receive moccache: ${_automoc_launcher}")
        endif()
        if(CMAKE_HOST_WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.29)
            get_target_property(_automoc_command_line_limit AutomocTarget AUTOGEN_COMMAND_LINE_LENGTH_MAX)
            if(NOT "${_automoc_command_line_limit}" STREQUAL "7000")
                message(
                    FATAL_ERROR
                        "AUTOMOC target did not receive Windows response-file limit: ${_automoc_command_line_limit}"
                )
            endif()
        endif()
        if(NOT "${_custom_launcher}" STREQUAL "${_custom_moc}")
            message(FATAL_ERROR "Existing AUTOMOC executable was replaced: ${_custom_launcher}")
        endif()
        if(_plain_launcher_set)
            message(FATAL_ERROR "Non-AUTOMOC target received moccache")
        endif()
        set_target_properties(
            AutomocTarget
            CustomMocTarget
            PROPERTIES
                AUTOMOC OFF
        )
    elseif(TEST_CASE STREQUAL "helpers-write-if-changed")
        set(_file "${CMAKE_CURRENT_BINARY_DIR}/write-if-changed.txt")
        file(REMOVE "${_file}")
        _qgc_write_if_changed("${_file}" "first\n")
        # Microseconds so a same-second rewrite is still detected.
        file(TIMESTAMP "${_file}" _first_mtime "%Y-%m-%dT%H:%M:%S.%f" UTC)
        _qgc_write_if_changed("${_file}" "first\n")
        file(TIMESTAMP "${_file}" _second_mtime "%Y-%m-%dT%H:%M:%S.%f" UTC)
        if(NOT _first_mtime STREQUAL _second_mtime)
            message(FATAL_ERROR "Unchanged content rewrote the file: ${_first_mtime} -> ${_second_mtime}")
        endif()
        _qgc_write_if_changed("${_file}" "second\n")
        file(READ "${_file}" _content)
        if(NOT _content STREQUAL "second\n")
            message(FATAL_ERROR "Changed content was not written: ${_content}")
        endif()
    elseif(TEST_CASE STREQUAL "helpers-rcc-launcher")
        # CMake stands in for rcc so the launcher can be executed.
        add_executable(Qt6::rcc IMPORTED)
        set_target_properties(
            Qt6::rcc
            PROPERTIES
                IMPORTED_CONFIGURATIONS RELEASE
                IMPORTED_LOCATION_RELEASE "${CMAKE_COMMAND}"
        )
        set(QT_FEATURE_zstd OFF)
        qgc_config_rcc_launcher()

        if(CMAKE_HOST_WIN32)
            set(_launcher "${CMAKE_CURRENT_BINARY_DIR}/rcc-launcher.cmd")
        else()
            set(_launcher "${CMAKE_CURRENT_BINARY_DIR}/rcc-launcher")
        endif()
        foreach(_property IN ITEMS IMPORTED_LOCATION IMPORTED_LOCATION_RELEASE)
            get_target_property(_location Qt6::rcc ${_property})
            if(NOT "${_location}" STREQUAL "${_launcher}")
                message(FATAL_ERROR "Qt6::rcc ${_property} does not use the launcher: ${_location}")
            endif()
        endforeach()

        if(NOT CMAKE_HOST_WIN32)
            execute_process(
                COMMAND "${CMAKE_COMMAND}" -E env --unset=QT_RCC_SOURCE_DATE_OVERRIDE "${_launcher}" -E environment
                RESULT_VARIABLE _result
                OUTPUT_VARIABLE _environment
            )
            if(NOT _result EQUAL 0 OR NOT _environment MATCHES "(^|\n)QT_RCC_SOURCE_DATE_OVERRIDE=1\n")
                message(FATAL_ERROR "rcc launcher did not pin the timestamp:\n${_environment}")
            endif()
            execute_process(
                COMMAND "${CMAKE_COMMAND}" -E env QT_RCC_SOURCE_DATE_OVERRIDE=42 "${_launcher}" -E environment
                RESULT_VARIABLE _result
                OUTPUT_VARIABLE _environment
            )
            if(NOT _result EQUAL 0 OR NOT _environment MATCHES "(^|\n)QT_RCC_SOURCE_DATE_OVERRIDE=42\n")
                message(FATAL_ERROR "rcc launcher replaced an exported timestamp:\n${_environment}")
            endif()

            # A stand-in rcc that echoes its arguments: the zstd level must come before, so options a
            # resource passes itself still override it.
            set(_echo_rcc "${CMAKE_CURRENT_BINARY_DIR}/echo-rcc")
            file(WRITE "${_echo_rcc}" "#!/bin/sh\necho \"$*\"\n")
            # cmake-lint: disable=E1126
            file(CHMOD "${_echo_rcc}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
            set_target_properties(
                Qt6::rcc
                PROPERTIES
                    IMPORTED_LOCATION ""
                    IMPORTED_LOCATION_RELEASE "${_echo_rcc}"
            )
            set(QT_FEATURE_zstd ON)
            qgc_config_rcc_launcher()
            execute_process(
                COMMAND "${_launcher}" --no-compress in.qrc
                RESULT_VARIABLE _result
                OUTPUT_VARIABLE _arguments
            )
            set(_expected "--compress-algo zstd --compress 9 --no-compress in.qrc\n")
            if(NOT _result EQUAL 0 OR NOT _arguments STREQUAL _expected)
                message(FATAL_ERROR "rcc launcher arguments with zstd: ${_arguments}")
            endif()
        endif()
    elseif(TEST_CASE STREQUAL "helpers-big-resources")
        # Stand-in for Qt: two-pass resources create a pass-1 object library unless Qt falls back (Xcode for iOS).
        function(qt_add_resources target resource_name)
            if(NOT resource_name STREQUAL "fallback")
                add_library(${target}_${resource_name}_obj OBJECT main.cpp)
            endif()
        endfunction()
        # Per-configuration IPO properties are only initialized for the configured build type.
        set(CMAKE_BUILD_TYPE Release)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        add_executable(${PROJECT_NAME} main.cpp)
        qgc_add_big_resources(
            ${PROJECT_NAME} payload
            PREFIX "/"
            FILES
                main.cpp
        )
        qgc_add_big_resources(
            ${PROJECT_NAME} fallback
            PREFIX "/"
            FILES
                main.cpp
        )

        set(_object_library ${PROJECT_NAME}_payload_obj)
        foreach(_property INTERPROCEDURAL_OPTIMIZATION INTERPROCEDURAL_OPTIMIZATION_RELEASE UNITY_BUILD)
            get_target_property(_value ${_object_library} ${_property})
            if(_value)
                message(FATAL_ERROR "rcc pass-1 object library keeps ${_property}=${_value}")
            endif()
        endforeach()
    elseif(TEST_CASE STREQUAL "helpers-ccache-pch")
        if(CMAKE_HOST_WIN32)
            return()
        endif()
        # Stand-in ccache that fails (3) when the compile's output already exists.
        set(_fake_ccache "${CMAKE_CURRENT_BINARY_DIR}/fake-ccache/ccache")
        file(WRITE "${_fake_ccache}"
             "#!/bin/sh\n[ \"$1\" = --version ] && exit 0\n_prev=\n"
             "for _arg; do [ \"$_prev\" = -o ] && [ -e \"$_arg\" ] && exit 3; _prev=$_arg; done\nexit 0\n"
        )
        # cmake-lint: disable=E1126
        file(CHMOD "${_fake_ccache}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
        set(QGC_CACHE_PROGRAM
            "${_fake_ccache}"
            CACHE FILEPATH "" FORCE
        )
        qgc_config_caching()
        get_directory_property(_options COMPILE_OPTIONS)
        if(NOT _options MATCHES "CXX_COMPILER_ID:AppleClang,Clang>>:SHELL:-Xclang -fno-pch-timestamp>")
            message(FATAL_ERROR "Clang PCH timestamps are not disabled for cached builds: ${_options}")
        endif()

        set(_pch "${CMAKE_CURRENT_BINARY_DIR}/stale.hxx.gch")
        set(_object "${CMAKE_CURRENT_BINARY_DIR}/stale.o")
        file(WRITE "${_pch}" "stale")
        file(WRITE "${_object}" "stale")
        execute_process(
            COMMAND "${CMAKE_CURRENT_BINARY_DIR}/ccache-launcher" c++ -x c++-header -c in.hxx -o "${_pch}"
            RESULT_VARIABLE _result
        )
        if(NOT _result EQUAL 0 OR EXISTS "${_pch}")
            message(FATAL_ERROR "ccache launcher kept the stale PCH (exit ${_result})")
        endif()
        execute_process(
            COMMAND "${CMAKE_CURRENT_BINARY_DIR}/ccache-launcher" c++ -c in.cpp -o "${_object}" RESULT_VARIABLE _result
        )
        if(NOT _result EQUAL 3 OR NOT EXISTS "${_object}")
            message(FATAL_ERROR "ccache launcher deleted a non-header compile's output (exit ${_result})")
        endif()
    elseif(TEST_CASE STREQUAL "helpers-cpm-missing")
        qgc_require_cpm_added(Example)
    elseif(TEST_CASE STREQUAL "helpers-cpm-invalid-name")
        qgc_require_cpm_added("invalid-name")
    elseif(TEST_CASE STREQUAL "helpers-json-unknown-argument")
        add_executable(${PROJECT_NAME} main.cpp)
        qgc_add_json_resources(resources TYPO value)
    elseif(TEST_CASE STREQUAL "helpers-json-escaped-pattern")
        add_executable(${PROJECT_NAME} main.cpp)
        qgc_add_json_resources(resources PATTERN [[..\outside\*.json]])
    else()
        message(FATAL_ERROR "Unknown helper TEST_CASE: ${TEST_CASE}")
    endif()
    return()
endif()

if(TEST_CASE STREQUAL "python-venv-stale-cache")
    set(QGC_AUTO_PYTHON_VENV
        OFF
        CACHE BOOL "" FORCE
    )
    # These are canonical FindPython cache variable names.
    # cmake-lint: disable=C0103
    set(Python3_EXECUTABLE
        "${CMAKE_CURRENT_BINARY_DIR}/missing/python"
        CACHE FILEPATH "" FORCE
    )
    set(Python_EXECUTABLE
        "${CMAKE_CURRENT_BINARY_DIR}/missing/python"
        CACHE FILEPATH "" FORCE
    )
    include("${QGC_MODULE_DIR}/modules/PythonVenv.cmake")
    if(DEFINED CACHE{Python3_EXECUTABLE} OR DEFINED CACHE{Python_EXECUTABLE})
        message(FATAL_ERROR "Stale Python interpreter cache entries were not cleared")
    endif()
    return()
endif()

if(TEST_CASE MATCHES "^qml-codegen-")
    include("${QGC_MODULE_DIR}/modules/QGCQmlCodegen.cmake")

    if(TEST_CASE MATCHES "^qml-codegen-configure-")
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
    elseif(NOT TEST_CASE STREQUAL "qml-codegen-missing-python")
        add_executable(Python3::Interpreter IMPORTED)
        set_target_properties(
            Python3::Interpreter
            PROPERTIES
                IMPORTED_LOCATION "${CMAKE_COMMAND}"
        )
    endif()

    if(TEST_CASE STREQUAL "qml-codegen-valid")
        qgc_add_qml_codegen(
            generate_qml
            GENERATOR_MODULE generator.module
            OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
            QML_NAMES Example.qml Nested/Other.qml
        )
        if(NOT TARGET generate_qml)
            message(FATAL_ERROR "Code generation target was not created")
        endif()
        if(NOT generate_qml_OUTPUTS STREQUAL
           "${CMAKE_CURRENT_BINARY_DIR}/generated/Example.qml;${CMAKE_CURRENT_BINARY_DIR}/generated/Nested/Other.qml"
        )
            message(FATAL_ERROR "Unexpected generated QML output list: ${generate_qml_OUTPUTS}")
        endif()
    elseif(TEST_CASE STREQUAL "qml-codegen-configure-valid")
        qgc_add_qml_codegen(
            generate_qml GENERATE_AT_CONFIGURE
            GENERATOR_MODULE qml_generator
            OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
            QML_NAMES Example.qml Nested/Other.qml
            EXTRA_ARGS --write-output Example.qml --write-output Nested/Other.qml
        )
    elseif(TEST_CASE STREQUAL "qml-codegen-configure-missing-output")
        qgc_add_qml_codegen(
            generate_qml GENERATE_AT_CONFIGURE
            GENERATOR_MODULE qml_generator
            OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
            QML_NAMES Example.qml
        )
    elseif(TEST_CASE STREQUAL "qml-codegen-missing-names")
        qgc_add_qml_codegen(
            generate_qml
            GENERATOR_MODULE generator.module
            OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
        )
    elseif(TEST_CASE STREQUAL "qml-codegen-missing-python")
        qgc_add_qml_codegen(
            generate_qml
            GENERATOR_MODULE generator.module
            OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
            QML_NAMES Example.qml
        )
    elseif(TEST_CASE STREQUAL "qml-codegen-unknown-argument")
        qgc_add_qml_codegen(
            generate_qml TYPO value
            GENERATOR_MODULE generator.module
            OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
            QML_NAMES Example.qml
        )
    elseif(TEST_CASE STREQUAL "qml-codegen-duplicate-output")
        qgc_add_qml_codegen(
            generate_qml
            GENERATOR_MODULE generator.module
            OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
            QML_NAMES Example.qml Example.qml
        )
    elseif(TEST_CASE STREQUAL "qml-codegen-escaped-output")
        qgc_add_qml_codegen(
            generate_qml
            GENERATOR_MODULE generator.module
            OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
            QML_NAMES ../Example.qml
        )
    elseif(TEST_CASE STREQUAL "qml-codegen-output-outside-build")
        qgc_add_qml_codegen(
            generate_qml
            GENERATOR_MODULE generator.module
            OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated"
            QML_NAMES Example.qml
        )
    else()
        message(FATAL_ERROR "Unknown QML codegen TEST_CASE: ${TEST_CASE}")
    endif()
    return()
endif()

if(TEST_CASE MATCHES "^qgc-test-")
    add_executable(${PROJECT_NAME} main.cpp)
    set(QGC_BUILD_TESTING ON)
    if(TEST_CASE STREQUAL "qgc-test-invalid-parallel-level")
        set(QGC_TEST_PARALLEL_LEVEL
            0
            CACHE STRING "" FORCE
        )
    endif()
    include(CTest)
    include("${QGC_MODULE_DIR}/QGCTest.cmake")

    if(TEST_CASE STREQUAL "qgc-test-timeout-zero")
        add_qgc_test(TimeoutZero TIMEOUT 0 LABELS Unit)
        get_test_property(TimeoutZero TIMEOUT _timeout)
        if(NOT _timeout STREQUAL "0")
            message(FATAL_ERROR "TIMEOUT 0 was not preserved: ${_timeout}")
        endif()
    elseif(TEST_CASE STREQUAL "qgc-test-unknown-argument")
        add_qgc_test(UnknownArgument TYPO value)
    elseif(TEST_CASE STREQUAL "qgc-test-conflicting-serialization")
        add_qgc_test(ConflictingSerialization SERIAL RESOURCE_LOCK MockLink)
    elseif(TEST_CASE STREQUAL "qgc-test-missing-timeout")
        add_qgc_test(MissingTimeout TIMEOUT)
    elseif(TEST_CASE STREQUAL "qgc-test-unsafe-name")
        add_qgc_test("../UnsafeName")
    else()
        message(FATAL_ERROR "Unknown QGC test TEST_CASE: ${TEST_CASE}")
    endif()
    return()
endif()

message(FATAL_ERROR "Unknown TEST_CASE: ${TEST_CASE}")
