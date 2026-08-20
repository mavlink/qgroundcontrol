set(_qgc_cmake_fixture_source "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ModuleContracts")
set(_qgc_cmake_fixture_root "${CMAKE_CURRENT_BINARY_DIR}/ModuleContracts")

function(qgc_add_cmake_contract_test test_name test_case)
    add_test(
        NAME "CMake.${test_name}"
        COMMAND
            "${CMAKE_COMMAND}" --fresh -S "${_qgc_cmake_fixture_source}" -B "${_qgc_cmake_fixture_root}/${test_name}"
            "-DQGC_MODULE_DIR=${PROJECT_SOURCE_DIR}/cmake" "-DTEST_CASE=${test_case}"
    )
    set_tests_properties("CMake.${test_name}" PROPERTIES LABELS "CMake;Unit")
endfunction()

function(qgc_add_cmake_failure_contract_test test_name test_case expected_regex)
    add_test(
        NAME "CMake.${test_name}"
        COMMAND
            "${CMAKE_COMMAND}" "-DCMAKE_EXECUTABLE=${CMAKE_COMMAND}"
            "-DSOURCE_DIR=${_qgc_cmake_fixture_source}"
            "-DBINARY_DIR=${_qgc_cmake_fixture_root}/${test_name}"
            "-DQGC_MODULE_DIR=${PROJECT_SOURCE_DIR}/cmake" "-DTEST_CASE=${test_case}"
            "-DEXPECTED_REGEX=${expected_regex}" -P
            "${CMAKE_CURRENT_SOURCE_DIR}/run_expected_failure_test.cmake"
    )
    set_tests_properties("CMake.${test_name}" PROPERTIES LABELS "CMake;Unit")
endfunction()

qgc_add_cmake_contract_test(CompilerWarningsWerrorOn compiler-warnings-on)
qgc_add_cmake_contract_test(CompilerWarningsWerrorOff compiler-warnings-off)
qgc_add_cmake_contract_test(CompilerWarningsProjectTargets compiler-warnings-project-targets)
qgc_add_cmake_contract_test(CoverageProjectTargets coverage-project-targets)
qgc_add_cmake_contract_test(CoverageCleanWithoutGcovr coverage-clean-without-gcovr)
qgc_add_cmake_contract_test(SanitizerProjectTargets sanitizer-project-targets)
qgc_add_cmake_contract_test(CoverageClean coverage-clean)
qgc_add_cmake_contract_test(HelpersCpmAdded helpers-cpm-added)
qgc_add_cmake_contract_test(HelpersMoccacheTargets helpers-moccache-targets)
qgc_add_cmake_contract_test(PythonVenvStaleCache python-venv-stale-cache)
qgc_add_cmake_contract_test(QmlCodegenValid qml-codegen-valid)
qgc_add_cmake_contract_test(QmlCodegenConfigureValid qml-codegen-configure-valid)
qgc_add_cmake_contract_test(QGCTestTimeoutZero qgc-test-timeout-zero)

add_test(
    NAME CMake.CustomDirEscape
    COMMAND
        "${CMAKE_COMMAND}" "-DCMAKE_EXECUTABLE=${CMAKE_COMMAND}"
        "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
        "-DBINARY_DIR=${_qgc_cmake_fixture_root}/custom-dir-escape"
        "-DCONFIGURE_ARGUMENT=-DQGC_CUSTOM_DIR=../outside"
        "-DEXPECTED_REGEX=QGC_CUSTOM_DIR must be a non-empty path" -P
        "${CMAKE_CURRENT_SOURCE_DIR}/run_expected_failure_test.cmake"
)
set_tests_properties(CMake.CustomDirEscape PROPERTIES LABELS "CMake;Unit")

set(_qgc_expected_failure_cases
    "QmlCodegenMissingNames|qml-codegen-missing-names|QML_NAMES is required"
    "QmlCodegenMissingPython|qml-codegen-missing-python|Python3::Interpreter target is required"
    "QmlCodegenUnknownArgument|qml-codegen-unknown-argument|unknown arguments"
    "QmlCodegenDuplicateOutput|qml-codegen-duplicate-output|contains duplicate outputs"
    "QmlCodegenEscapedOutput|qml-codegen-escaped-output|must stay under OUTPUT_DIR"
    "QmlCodegenOutputOutsideBuild|qml-codegen-output-outside-build|OUTPUT_DIR must stay under CMAKE_BINARY_DIR"
    "QmlCodegenConfigureMissingOutput|qml-codegen-configure-missing-output|did not produce declared output"
    "QGCTestUnknownArgument|qgc-test-unknown-argument|unknown arguments"
    "QGCTestConflictingSerialization|qgc-test-conflicting-serialization|SERIAL and RESOURCE_LOCK are mutually exclusive"
    "QGCTestMissingTimeout|qgc-test-missing-timeout|missing values for"
    "QGCTestUnsafeName|qgc-test-unsafe-name|test name contains unsafe characters"
    "QGCTestInvalidParallelLevel|qgc-test-invalid-parallel-level|QGC_TEST_PARALLEL_LEVEL must be a positive integer"
    "CoverageInvalidThreshold|coverage-invalid-threshold|must be an integer from 0 through 100"
    "CoverageReleaseRequest|coverage-release-request|Code coverage requires a Debug build"
    "SanitizerReleaseRequest|sanitizer-release-request|Sanitizers require Debug or RelWithDebInfo builds"
    "DownloadTimeoutZero|download-timeout-zero|TIMEOUT must be a positive integer"
    "HelpersCpmMissing|helpers-cpm-missing|required dependency"
    "HelpersCpmInvalidName|helpers-cpm-invalid-name|exactly one valid CPM package name is required"
    "HelpersJsonUnknownArgument|helpers-json-unknown-argument|unknown arguments"
    "HelpersJsonEscapedPattern|helpers-json-escaped-pattern|PATTERN must remain under the current source directory"
)
foreach(qgc_failure_case IN LISTS _qgc_expected_failure_cases)
    string(REPLACE "|" ";" _qgc_failure_parts "${qgc_failure_case}")
    list(GET _qgc_failure_parts 0 _qgc_test_name)
    list(GET _qgc_failure_parts 1 _qgc_test_case)
    list(GET _qgc_failure_parts 2 _qgc_expected_regex)
    qgc_add_cmake_failure_contract_test(
        "${_qgc_test_name}" "${_qgc_test_case}" "${_qgc_expected_regex}"
    )
endforeach()

add_test(
    NAME CMake.QGCTestMultiConfig
    COMMAND
        "${CMAKE_COMMAND}" "-DQGC_MODULE_DIR=${PROJECT_SOURCE_DIR}/cmake"
        "-DFIXTURE_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/MultiConfigCTest"
        "-DFIXTURE_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/MultiConfigCTest" -P
        "${CMAKE_CURRENT_SOURCE_DIR}/run_multi_config_test.cmake"
)
set_tests_properties(CMake.QGCTestMultiConfig PROPERTIES LABELS "CMake;Unit")
