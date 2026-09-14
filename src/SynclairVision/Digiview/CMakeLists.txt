# ============================================================================
# Digiview Module
# DigiView MAVLink-over-UDP connection with an opt-in, limited legacy TCP control fallback
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        DigiviewConnection.cc
        DigiviewConnection.h
        DigiviewLegacyTcpAdapter.cc
        DigiviewLegacyTcpAdapter.h
        DigiviewLegacyTcpTransport.cc
        DigiviewLegacyTcpTransport.h
        DigiviewManager.cc
        DigiviewManager.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/message-definitions
)
