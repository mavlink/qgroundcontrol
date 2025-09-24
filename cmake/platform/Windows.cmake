# ----------------------------------------------------------------------------
# QGroundControl Windows Platform Configuration
# ----------------------------------------------------------------------------

if(NOT WIN32)
    message(FATAL_ERROR "QGC: Invalid Platform: Windows.cmake included but platform is not Windows")
endif()

# ----------------------------------------------------------------------------
# Windows-Specific Definitions
# ----------------------------------------------------------------------------
target_compile_definitions(${CMAKE_PROJECT_NAME}
    PRIVATE
        _USE_MATH_DEFINES    # Enable M_PI and other math constants
        NOMINMAX             # Prevent min/max macro conflicts
        WIN32_LEAN_AND_MEAN  # Reduce Windows.h bloat
)

# ----------------------------------------------------------------------------
# Windows Executable Configuration
# ----------------------------------------------------------------------------
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

if(COMMAND _qt_internal_generate_win32_rc_file)
    set_target_properties(${CMAKE_PROJECT_NAME}
        PROPERTIES
            QT_TARGET_COMPANY_NAME "${QGC_ORG_NAME}"
            QT_TARGET_DESCRIPTION "${CMAKE_PROJECT_DESCRIPTION}"
            QT_TARGET_VERSION "${CMAKE_PROJECT_VERSION}"
            QT_TARGET_COPYRIGHT "${QGC_APP_COPYRIGHT}"
            QT_TARGET_PRODUCT_NAME "${CMAKE_PROJECT_NAME}"
            # QT_TARGET_COMMENTS: ${QGC_QT_TARGET_COMMENTS}
            # QT_TARGET_ORIGINAL_FILENAME: ${QGC_QT_TARGET_ORIGINAL_FILENAME}
            # QT_TARGET_TRADEMARKS: ${QGC_QT_TARGET_TRADEMARKS}
            # QT_TARGET_INTERNALNAME: ${QGC_QT_TARGET_INTERNALNAME}
            QT_TARGET_RC_ICONS "${QGC_WINDOWS_ICON_PATH}"
    )
    _qt_internal_generate_win32_rc_file(${CMAKE_PROJECT_NAME})
elseif(EXISTS "${QGC_WINDOWS_RESOURCE_FILE_PATH}")
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${QGC_WINDOWS_RESOURCE_FILE_PATH}")
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES QT_TARGET_WINDOWS_RC_FILE "${QGC_WINDOWS_RESOURCE_FILE_PATH}")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/deploy/windows/QGroundControl.rc.in")
    configure_file(
        "${CMAKE_SOURCE_DIR}/deploy/windows/QGroundControl.rc.in"
        "${CMAKE_BINARY_DIR}/QGroundControl.rc"
        @ONLY
    )
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_BINARY_DIR}/QGroundControl.rc")
else()
    message(WARNING "QGC: No Windows resource file found")
endif()

if(MSVC)
    # qt_add_win_app_sdk(${CMAKE_PROJECT_NAME} PRIVATE)
endif()

message(STATUS "QGC: Windows platform configuration applied")
