# NTRIP tests.
target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE MockNTRIPStream.h
            NTRIPManagerTest.cc
            NTRIPManagerTest.h
            NTRIPSourceTableTest.cc
            NTRIPSourceTableTest.h
            NTRIPHttpTransportTest.cc
            NTRIPHttpTransportTest.h
            NTRIPConnectionStatsTest.cc
            NTRIPConnectionStatsTest.h
            NTRIPGgaProviderTest.cc
            NTRIPGgaProviderTest.h
            NTRIPSourceTableControllerTest.cc
            NTRIPSourceTableControllerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(NTRIPManagerTest LABELS Unit)
add_qgc_test(NTRIPSourceTableTest LABELS Unit)
add_qgc_test(NTRIPHttpTransportTest LABELS Unit)
add_qgc_test(NTRIPConnectionStatsTest LABELS Unit)
add_qgc_test(NTRIPGgaProviderTest LABELS Unit RESOURCE_LOCK MockLink)
add_qgc_test(NTRIPSourceTableControllerTest LABELS Unit)

qt_add_executable(QGCGPSNTRIPSessionTest NTRIPSessionTest.cc MockNTRIPStream.h)
target_link_libraries(QGCGPSNTRIPSessionTest PRIVATE QGCGPSNTRIPSession Qt6::Test)
add_test(NAME NTRIPSessionTest COMMAND QGCGPSNTRIPSessionTest)
set_tests_properties(NTRIPSessionTest PROPERTIES LABELS "Unit;GPS" TIMEOUT 60)
