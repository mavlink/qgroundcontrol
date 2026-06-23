# ============================================================================
# Onboard Logs FTP Module
# Onboard log list and download via MAVLink FTP (@MAV_LOG)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        OnboardLogFtpController.cc
        OnboardLogFtpController.h
        OnboardLogFtpEntry.cc
        OnboardLogFtpEntry.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
