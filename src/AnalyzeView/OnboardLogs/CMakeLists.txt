# ============================================================================
# Onboard Logs Module
# Onboard log list and download (LOG protocol + MAVLink FTP)
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE FtpDownloadSession.cc
            FtpDownloadSession.h
            FtpEraseSession.cc
            FtpEraseSession.h
            FtpListingParser.cc
            FtpListingParser.h
            FtpListingSession.cc
            FtpListingSession.h
            FtpTransport.cc
            FtpTransport.h
            LogProtocolDownloadBatchSession.cc
            LogProtocolDownloadBatchSession.h
            LogProtocolTransport.cc
            LogProtocolTransport.h
            LogProtocolDownloadSession.cc
            LogProtocolDownloadSession.h
            LogProtocolListingSession.cc
            LogProtocolListingSession.h
            OnboardLogController.cc
            OnboardLogController.h
            OnboardLogEntry.cc
            OnboardLogEntry.h
        OnboardLogFileName.cc
        OnboardLogFileName.h
        OnboardLogModel.cc
        OnboardLogModel.h
        OnboardLogSortFilterModel.cc
            OnboardLogSortFilterModel.h
            OnboardLogTransport.cc
            OnboardLogTransport.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
