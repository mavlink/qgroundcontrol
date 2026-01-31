# ============================================================================
# FTP Subsystem
# MAVLink FTP protocol and file operations
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FTPController.cc
        FTPController.h
        FTPDeleteStateMachine.cc
        FTPDeleteStateMachine.h
        FTPDownloadStateMachine.cc
        FTPDownloadStateMachine.h
        FTPListDirectoryStateMachine.cc
        FTPListDirectoryStateMachine.h
        FTPManager.cc
        FTPManager.h
        FTPOperationStateMachine.cc
        FTPOperationStateMachine.h
        FTPUploadStateMachine.cc
        FTPUploadStateMachine.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
