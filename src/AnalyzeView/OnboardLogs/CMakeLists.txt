# ============================================================================
# Onboard Logs Module
# Onboard log list and download (LOG protocol + MAVLink FTP)
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE FTP/FtpDownloadSession.cc
            FTP/FtpDownloadSession.h
            FTP/FtpEraseSession.cc
            FTP/FtpEraseSession.h
            FTP/FtpListingParser.cc
            FTP/FtpListingParser.h
            FTP/FtpListingSession.cc
            FTP/FtpListingSession.h
            FTP/FtpTransport.cc
            FTP/FtpTransport.h
            LogProtocolDownloadBatchSession.cc
            LogProtocolDownloadBatchSession.h
            LogProtocolDownloadSession.cc
            LogProtocolDownloadSession.h
            LogProtocolListingSession.cc
            LogProtocolListingSession.h
            LogProtocolTransport.cc
            LogProtocolTransport.h
            OnboardLogController.cc
            OnboardLogController.h
            OnboardLogEntry.cc
            OnboardLogEntry.h
            OnboardLogEntryQueue.cc
            OnboardLogEntryQueue.h
            OnboardLogFileName.cc
            OnboardLogFileName.h
            OnboardLogModel.cc
            OnboardLogModel.h
            OnboardLogSession.cc
            OnboardLogSession.h
            OnboardLogSortFilterModel.cc
            OnboardLogSortFilterModel.h
            OnboardLogTransport.cc
            OnboardLogTransport.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
