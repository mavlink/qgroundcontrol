# ============================================================================
# Vehicle FTP Subsystem
# MAVLink FTP operations and QML-facing control
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FTPController.cc
        FTPController.h
        FTPManager.cc
        FTPManager.h
        FTPManagerJob.cc
        FTPManagerJob.h
        FTPManagerOperations.cc
        FTPManagerOperations.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
