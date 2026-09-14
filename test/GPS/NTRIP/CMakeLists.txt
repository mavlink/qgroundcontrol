# NTRIP tests.
target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE MockNTRIPTransport.h
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
add_qgc_test(NTRIPGgaProviderTest LABELS Unit)
add_qgc_test(NTRIPSourceTableControllerTest LABELS Unit)
