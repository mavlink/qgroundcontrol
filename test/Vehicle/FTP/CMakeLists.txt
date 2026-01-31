# ============================================================================
# FTP Unit Tests
# Tests for FTP manager and operation state machines
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FTPControllerTest.cc
        FTPControllerTest.h
        FTPDeleteStateMachineTest.cc
        FTPDeleteStateMachineTest.h
        FTPDownloadStateMachineTest.cc
        FTPDownloadStateMachineTest.h
        FTPListDirectoryStateMachineTest.cc
        FTPListDirectoryStateMachineTest.h
        FTPManagerTest.cc
        FTPManagerTest.h
        FTPUploadStateMachineTest.cc
        FTPUploadStateMachineTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
