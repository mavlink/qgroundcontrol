set(_qgc_cmake_fixture_source "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ModuleContracts")
set(_qgc_cmake_fixture_root "${CMAKE_CURRENT_BINARY_DIR}/ModuleContracts")

# Register a configure-only contract test for one fixture case.
function(qgc_add_cmake_contract_test test_name test_case)
    add_test(
        NAME "CMake.${test_name}"
        COMMAND
            "${CMAKE_COMMAND}" --fresh -S "${_qgc_cmake_fixture_source}" -B "${_qgc_cmake_fixture_root}/${test_name}"
            "-DQGC_MODULE_DIR=${PROJECT_SOURCE_DIR}/cmake" "-DTEST_CASE=${test_case}"
    )
    set_tests_properties("CMake.${test_name}" PROPERTIES LABELS "CMake;Unit")
endfunction()

qgc_add_cmake_contract_test(CompilerWarningsWerrorOn compiler-warnings-on)
qgc_add_cmake_contract_test(CompilerWarningsWerrorOff compiler-warnings-off)
qgc_add_cmake_contract_test(CompilerWarningsProjectTargets compiler-warnings-project-targets)
qgc_add_cmake_contract_test(CoverageProjectTargets coverage-project-targets)
qgc_add_cmake_contract_test(SanitizerProjectTargets sanitizer-project-targets)
qgc_add_cmake_contract_test(CoverageClean coverage-clean)
qgc_add_cmake_contract_test(XCFrameworkSlices xcframework-slices)
qgc_add_cmake_contract_test(HelpersCpmAdded helpers-cpm-added)
qgc_add_cmake_contract_test(PythonVenvStaleCache python-venv-stale-cache)
qgc_add_cmake_contract_test(QmlCodegenValid qml-codegen-valid)
qgc_add_cmake_contract_test(QGCTestTimeoutZero qgc-test-timeout-zero)

add_test(
    NAME CMake.CustomDirEscape
    COMMAND "${CMAKE_COMMAND}" --fresh -S "${PROJECT_SOURCE_DIR}"
            -B "${_qgc_cmake_fixture_root}/custom-dir-escape"
            -DQGC_CUSTOM_DIR=../outside
)
set_tests_properties(CMake.CustomDirEscape PROPERTIES LABELS "CMake;Unit" WILL_FAIL TRUE)

set(_qgc_expected_failure_cases
    QmlCodegenMissingNames=qml-codegen-missing-names
    QmlCodegenMissingPython=qml-codegen-missing-python
    QmlCodegenUnknownArgument=qml-codegen-unknown-argument
    QmlCodegenDuplicateOutput=qml-codegen-duplicate-output
    QmlCodegenEscapedOutput=qml-codegen-escaped-output
    QmlCodegenOutputOutsideBuild=qml-codegen-output-outside-build
    QGCTestUnknownArgument=qgc-test-unknown-argument
    QGCTestConflictingSerialization=qgc-test-conflicting-serialization
    QGCTestMissingTimeout=qgc-test-missing-timeout
    QGCTestUnsafeName=qgc-test-unsafe-name
    QGCTestInvalidParallelLevel=qgc-test-invalid-parallel-level
    CoverageInvalidThreshold=coverage-invalid-threshold
    CoverageReleaseRequest=coverage-release-request
    SanitizerReleaseRequest=sanitizer-release-request
    HelpersCpmMissing=helpers-cpm-missing
    HelpersCpmInvalidName=helpers-cpm-invalid-name
    HelpersJsonUnknownArgument=helpers-json-unknown-argument
)
foreach(qgc_failure_case IN LISTS _qgc_expected_failure_cases)
    string(REPLACE "=" ";" _qgc_failure_parts "${qgc_failure_case}")
    list(GET _qgc_failure_parts 0 _qgc_test_name)
    list(GET _qgc_failure_parts 1 _qgc_test_case)
    qgc_add_cmake_contract_test("${_qgc_test_name}" "${_qgc_test_case}")
    set_tests_properties("CMake.${_qgc_test_name}" PROPERTIES WILL_FAIL TRUE)
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
