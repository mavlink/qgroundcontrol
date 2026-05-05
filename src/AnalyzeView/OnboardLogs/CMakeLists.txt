# ============================================================================
# Onboard Logs Module
# Onboard log list and download (LOG protocol + MAVLink FTP)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FtpTransport.cc
        FtpTransport.h
        LogProtocolTransport.cc
        LogProtocolTransport.h
        OnboardLogController.cc
        OnboardLogController.h
        OnboardLogDownloadData.cc
        OnboardLogDownloadData.h
        OnboardLogEntry.cc
        OnboardLogEntry.h
        OnboardLogTransport.cc
        OnboardLogTransport.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
