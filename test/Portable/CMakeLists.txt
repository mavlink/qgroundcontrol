find_package(Qt6 REQUIRED COMPONENTS Test)
include(TestSupport)

add_custom_target(portable-tests COMMENT "Build the portable utility test executables")

# The harness of every portable test: the part of the application's harness that needs no application, its own
# capture of the log, and the fixtures that need no application
set(_qgc_test_framework_dir ${PROJECT_SOURCE_DIR}/test/UnitTestFramework)
add_library(
    QGCPortableTestHarness STATIC
    PortableTest.cc
    PortableTest.h
    ${_qgc_test_framework_dir}/TestTimeout.h
    ${_qgc_test_framework_dir}/UnitTestBase.cc
    ${_qgc_test_framework_dir}/UnitTestBase.h
    ${_qgc_test_framework_dir}/Fixtures/FileFixtures.cc
    ${_qgc_test_framework_dir}/Fixtures/FileFixtures.h
    ${_qgc_test_framework_dir}/Fixtures/ProcessFixtures.cc
    ${_qgc_test_framework_dir}/Fixtures/ProcessFixtures.h
)
set_target_properties(
    QGCPortableTestHarness
    PROPERTIES
        AUTOMOC ON
)
target_compile_definitions(
    QGCPortableTestHarness
    PUBLIC
        QGC_PORTABLE_TEST
        QGC_UNITTEST_BUILD
)
target_include_directories(
    QGCPortableTestHarness
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${_qgc_test_framework_dir}
        ${_qgc_test_framework_dir}/Fixtures
)
target_link_libraries(
    QGCPortableTestHarness
    PUBLIC Qt6::Test
    PRIVATE
        QGCChecksums
        QGCLoggingCategory
)

# Register an executable that shares its test body with the application harness. Like the tests of the application,
# each test gets a temporary directory of its own.
function(qgc_add_portable_test name source)
    add_executable(${name} ${source}.cc ${source}.h)
    set_target_properties(
        ${name}
        PROPERTIES
            AUTOMOC ON
            MACOSX_BUNDLE OFF
            WIN32_EXECUTABLE OFF
    )
    target_include_directories(${name} PRIVATE ${PROJECT_BINARY_DIR})
    target_link_libraries(${name} PRIVATE QGCPortableTestHarness)
    add_dependencies(portable-tests ${name})
    add_test(NAME Portable.${name} COMMAND ${name})

    set(_tmpdir "${CMAKE_BINARY_DIR}/test-tmp/Portable.${name}")
    file(MAKE_DIRECTORY "${_tmpdir}")
    set(_environment "QT_FATAL_CRITICALS=1" "TMPDIR=${_tmpdir}")
    if(WIN32)
        list(APPEND _environment "TMP=${_tmpdir}" "TEMP=${_tmpdir}")
    endif()
    set_tests_properties(
        Portable.${name}
        PROPERTIES
            LABELS "Unit;Portable"
            TIMEOUT 60
            ENVIRONMENT "${_environment}"
    )
endfunction()

qgc_add_portable_test(DataRateTrackerTest ../Utilities/DataRateTrackerTest)
target_link_libraries(DataRateTrackerTest PRIVATE QGCIO)

qgc_add_portable_test(QGCCommandLineParserTest ../Utilities/QGCCommandLineParserTest)
target_sources(
    QGCCommandLineParserTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/Utilities/QGCCommandLineParser.cc
)
target_include_directories(QGCCommandLineParserTest PRIVATE ${PROJECT_SOURCE_DIR}/src/Utilities)
target_link_libraries(QGCCommandLineParserTest PRIVATE QGCLoggingCategory)

qgc_add_portable_test(QGCFormatTest ../Utilities/Format/QGCFormatTest)
target_link_libraries(QGCFormatTest PRIVATE QGCFormat)

qgc_add_portable_test(QGCMathTest ../Utilities/QGCMathTest)
target_link_libraries(QGCMathTest PRIVATE QGCMath)

qgc_add_portable_test(ChecksumsTest ../Utilities/Math/ChecksumsTest)
target_link_libraries(ChecksumsTest PRIVATE QGCChecksums)

qgc_add_portable_test(WireFieldsTest ../Utilities/Parsing/Wire/WireFieldsTest)
target_link_libraries(WireFieldsTest PRIVATE QGCWire)

qgc_add_portable_test(ExponentialBackoffTest ../Utilities/Timing/ExponentialBackoffTest)
target_link_libraries(ExponentialBackoffTest PRIVATE QGCTiming)

qgc_add_portable_test(MonotonicClockTest ../Utilities/Timing/MonotonicClockTest)
target_link_libraries(MonotonicClockTest PRIVATE QGCClock)

qgc_add_portable_test(WorkerJobTest ../Utilities/Concurrency/WorkerJobTest)
target_include_directories(WorkerJobTest PRIVATE ${PROJECT_SOURCE_DIR}/src/Utilities/Concurrency)
target_link_libraries(WorkerJobTest PRIVATE Qt6::Concurrent)

qgc_add_portable_test(SecureMemoryTest ../Utilities/SecureMemoryTest)
target_include_directories(SecureMemoryTest PRIVATE ${PROJECT_SOURCE_DIR}/src/Utilities/Memory)

qgc_add_portable_test(JsonHelperTest ../Utilities/Parsing/Json/JsonHelperTest)
target_link_libraries(
    JsonHelperTest
    PRIVATE
        QGCGeoFormats
        QGCParsing
)

# The Compression suites, reading the same resources as in the application
include(${PROJECT_SOURCE_DIR}/test/Utilities/Compression/CompressionTestResources.cmake)
foreach(_suite IN ITEMS QGCArchiveExtractionTest QGCCompressionJobTest QGCCompressionTest)
    qgc_add_portable_test(${_suite} ../Utilities/Compression/${_suite})
    target_sources(
        ${_suite}
        PRIVATE
            ../Utilities/Compression/ArchiveFixtures.cc
            ../Utilities/Compression/ArchiveFixtures.h
    )
    target_link_libraries(${_suite} PRIVATE QGCCompression)
    qgc_add_compression_test_resources(${_suite})
endforeach()
target_link_libraries(QGCArchiveExtractionTest PRIVATE Qt6::Concurrent)
include(${PROJECT_SOURCE_DIR}/test/Utilities/Compression/Fuzz/BidAgreement.cmake)
target_sources(
    QGCCompressionTest
    PRIVATE
        ../Utilities/Compression/Fuzz/BidAgreement.cc
        ../Utilities/Compression/Fuzz/BidAgreement.h
)
qgc_add_bid_agreement(QGCCompressionTest)

# QGCCompression on the fixed inputs of its fuzz harness. The test build has the target already.
if(NOT TARGET qgc_compression_fuzz_smoke)
    add_executable(
        qgc_compression_fuzz_smoke
        ../Utilities/Compression/Fuzz/BidAgreement.cc ../Utilities/Compression/Fuzz/QGCCompressionFuzzTargets.cc
        ../Utilities/Compression/Fuzz/QGCCompressionFuzzSmoke.cc
    )
    qgc_add_bid_agreement(qgc_compression_fuzz_smoke)
endif()
add_dependencies(portable-tests qgc_compression_fuzz_smoke)
add_test(
    NAME Portable.QGCCompressionFuzzSmoke COMMAND qgc_compression_fuzz_smoke
                                                  "${PROJECT_SOURCE_DIR}/test/Utilities/Compression"
)
set_tests_properties(
    Portable.QGCCompressionFuzzSmoke
    PROPERTIES
        LABELS "Unit;Portable;Fuzz"
        TIMEOUT 60
)

qgc_add_portable_test(GeoTest ../Utilities/Geo/GeoTest)
target_link_libraries(GeoTest PRIVATE QGCGeoMath)
qgc_add_benchmark_support(GeoTest)
qgc_add_property_support(GeoTest)
