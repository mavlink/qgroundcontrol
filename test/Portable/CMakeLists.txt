find_package(Qt6 REQUIRED COMPONENTS Test)
include(TestSupport)

add_custom_target(portable-tests COMMENT "Build the portable utility test executables")

# Register an executable that shares its test body with the application harness.
function(qgc_add_portable_test name source)
    add_executable(${name} ${source}.cc ${source}.h PortableTest.h)
    set_target_properties(
        ${name}
        PROPERTIES AUTOMOC ON
                   MACOSX_BUNDLE OFF
                   WIN32_EXECUTABLE OFF
    )
    target_compile_definitions(${name} PRIVATE QGC_PORTABLE_TEST QGC_UNITTEST_BUILD)
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_BINARY_DIR}
                                               ${PROJECT_SOURCE_DIR}/test/UnitTestFramework
    )
    target_link_libraries(${name} PRIVATE Qt6::Test)
    add_dependencies(portable-tests ${name})
    add_test(NAME Portable.${name} COMMAND ${name})
    set_tests_properties(Portable.${name} PROPERTIES LABELS "Unit;Portable" TIMEOUT 60 ENVIRONMENT
                                                     "QT_FATAL_CRITICALS=1"
    )
endfunction()

qgc_add_portable_test(DataRateTrackerTest ../Utilities/DataRateTrackerTest)
target_link_libraries(DataRateTrackerTest PRIVATE QGCIO)

qgc_add_portable_test(QGCCommandLineParserTest ../Utilities/QGCCommandLineParserTest)
target_sources(QGCCommandLineParserTest PRIVATE ${PROJECT_SOURCE_DIR}/src/Utilities/QGCCommandLineParser.cc)
target_include_directories(QGCCommandLineParserTest PRIVATE ${PROJECT_SOURCE_DIR}/src/Utilities)
target_link_libraries(QGCCommandLineParserTest PRIVATE QGCLoggingCategory)

qgc_add_portable_test(QGCMathTest ../Utilities/QGCMathTest)
target_link_libraries(QGCMathTest PRIVATE QGCMath)

qgc_add_portable_test(SecureMemoryTest ../Utilities/SecureMemoryTest)
target_include_directories(SecureMemoryTest PRIVATE ${PROJECT_SOURCE_DIR}/src/Utilities/Memory)

qgc_add_portable_test(JsonHelperTest ../Utilities/Parsing/Json/JsonHelperTest)
target_link_libraries(JsonHelperTest PRIVATE QGCGeoFormats QGCParsing)

qgc_add_portable_test(GeoTest ../Utilities/Geo/GeoTest)
target_link_libraries(GeoTest PRIVATE QGCGeoMath)
qgc_add_benchmark_support(GeoTest)
qgc_add_property_support(GeoTest)
