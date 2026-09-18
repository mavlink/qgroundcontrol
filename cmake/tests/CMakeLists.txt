set(_qgc_cmake_fixture_source "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ModuleContracts")
set(_qgc_cmake_fixture_root "${CMAKE_CURRENT_BINARY_DIR}/ModuleContracts")

# Keep fixture configuration independent of cached application options.
function(qgc_add_cmake_contract_test test_name test_case)
    set(_fixture_source "${_qgc_cmake_fixture_source}")
    if(ARGC GREATER 2)
        set(_fixture_source "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/${ARGV2}")
    endif()
    add_test(NAME "CMake.${test_name}"
             COMMAND "${CMAKE_COMMAND}" --fresh -S "${_fixture_source}" -B "${_qgc_cmake_fixture_root}/${test_name}"
                     "-DQGC_MODULE_DIR=${PROJECT_SOURCE_DIR}/cmake" "-DTEST_CASE=${test_case}"
    )
    set_tests_properties("CMake.${test_name}" PROPERTIES LABELS "CMake;Unit")
endfunction()

# Require the intended failure, not an unrelated configure error.
function(qgc_add_cmake_failure_contract_test test_name test_case expected_regex)
    set(_fixture_source "${_qgc_cmake_fixture_source}")
    if(ARGC GREATER 3)
        set(_fixture_source "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/${ARGV3}")
    endif()
    add_test(
        NAME "CMake.${test_name}"
        COMMAND
            "${CMAKE_COMMAND}" "-DCMAKE_EXECUTABLE=${CMAKE_COMMAND}" "-DSOURCE_DIR=${_fixture_source}"
            "-DBINARY_DIR=${_qgc_cmake_fixture_root}/${test_name}" "-DQGC_MODULE_DIR=${PROJECT_SOURCE_DIR}/cmake"
            "-DTEST_CASE=${test_case}" "-DEXPECTED_REGEX=${expected_regex}" -P
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
qgc_add_cmake_contract_test(LibraryConsumerDefaults library-consumer-defaults LibraryConsumer)
qgc_add_cmake_contract_test(LibraryConsumerMetadata library-consumer-metadata LibraryConsumer)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_test(
        NAME CMake.GPSReceiverTransportsMinimal
        COMMAND
            "${CMAKE_COMMAND}" --fresh -G "${CMAKE_GENERATOR}" -S "${PROJECT_SOURCE_DIR}/test/GPS/Standalone" -B
            "${CMAKE_CURRENT_BINARY_DIR}/GPSReceiverTransportsMinimal" "-DQt6_DIR=${Qt6_DIR}"
            -DQGC_GPS_COMPONENTS=ReceiverTransports -DQGC_NO_SERIAL_LINK=ON
            -DCMAKE_DISABLE_FIND_PACKAGE_Qt6CorePrivate=ON -DCMAKE_DISABLE_FIND_PACKAGE_Qt6Positioning=ON
            -DCMAKE_DISABLE_FIND_PACKAGE_Qt6SerialPort=ON
    )
    set_tests_properties(CMake.GPSReceiverTransportsMinimal PROPERTIES LABELS "CMake;Unit")
endif()

add_test(
    NAME CMake.CustomDirEscape
    COMMAND
        "${CMAKE_COMMAND}" "-DCMAKE_EXECUTABLE=${CMAKE_COMMAND}" "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
        "-DBINARY_DIR=${_qgc_cmake_fixture_root}/custom-dir-escape" "-DCONFIGURE_ARGUMENT=-DQGC_CUSTOM_DIR=../outside"
        "-DEXPECTED_REGEX=QGC_CUSTOM_DIR must be a non-empty path" -P
        "${CMAKE_CURRENT_SOURCE_DIR}/run_expected_failure_test.cmake"
)
set_tests_properties(CMake.CustomDirEscape PROPERTIES LABELS "CMake;Unit")

set(_qgc_expected_failure_cases
    "LibraryConsumerMissingSource|library-consumer-missing-source|SOURCE is required|LibraryConsumer"
    "LibraryConsumerMissingValue|library-consumer-missing-value|missing values for|LibraryConsumer"
    "LibraryConsumerUnknownArgument|library-consumer-unknown-argument|unknown arguments|LibraryConsumer"
    "LibraryConsumerMissingTarget|library-consumer-missing-target|target does not exist|LibraryConsumer"
    "LibraryConsumerInvalidTimeout|library-consumer-invalid-timeout|TIMEOUT must be a positive integer|LibraryConsumer"
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
    qgc_add_cmake_failure_contract_test(${_qgc_failure_parts})
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
