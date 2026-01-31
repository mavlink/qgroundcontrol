# ============================================================================
# Parsing Utilities
# Format parsing utilities (EXIF metadata, DataFlash logs, ULog logs)
# ============================================================================

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(DataFlash)
add_subdirectory(Exif)
add_subdirectory(ULog)
