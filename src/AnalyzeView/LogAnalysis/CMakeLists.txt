# ============================================================================
# Log Analysis Module
# PX4 ULog file loading and data access for Qt Graphs
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        PX4LogLoader.cc
        PX4LogLoader.h
        PX4LogTopic.cc
        PX4LogTopic.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
