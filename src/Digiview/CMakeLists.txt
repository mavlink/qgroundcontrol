# ============================================================================
# Digiview Module
# Dedicated MAVLink-over-UDP connection for Digiview endpoints
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        DigiviewConnection.cc
        DigiviewConnection.h
        DigiviewManager.cc
        DigiviewManager.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
