# ============================================================================
# NTRIP Module
# NTRIP caster client: connection lifecycle, correction stream, GGA reports and source tables. HTTP/ is the library
# QGCNTRIPHttp, which builds without the application.
# ============================================================================

add_subdirectory(HTTP)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCNTRIPHttp)

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE NTRIPConnectionStats.cc
            NTRIPConnectionStats.h
            NTRIPGgaReporter.cc
            NTRIPGgaReporter.h
            NTRIPHttpTransport.cc
            NTRIPHttpTransport.h
            NTRIPManager.cc
            NTRIPManager.h
            NTRIPSourceTable.cc
            NTRIPSourceTable.h
            NTRIPSourceTableController.cc
            NTRIPSourceTableController.h
            NTRIPTransport.h
            NTRIPVehicleGgaSource.cc
            NTRIPVehicleGgaSource.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
