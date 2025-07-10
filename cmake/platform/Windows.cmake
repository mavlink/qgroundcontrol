if(NOT WIN32)
    message(FATAL_ERROR "Invalid Platform")
    return()
endif()

# CPMAddPackage(
#     NAME windows_drivers
#     URL https://firmware.ardupilot.org/Tools/MissionPlanner/driver.msi
# )
# ${windows_drivers_SOURCE_DIR}/driver.msi

set_target_properties(${CMAKE_PROJECT_NAME}
    PROPERTIES
        WIN32_EXECUTABLE TRUE
        # QT_TARGET_WINDOWS_RC_FILE "${QGC_WINDOWS_RESOURCE_FILE_PATH}"
        QT_TARGET_COMPANY_NAME "${QGC_ORG_NAME}"
        QT_TARGET_DESCRIPTION "${CMAKE_PROJECT_DESCRIPTION}"
        QT_TARGET_VERSION "${CMAKE_PROJECT_VERSION}"
        QT_TARGET_COPYRIGHT "${QGC_APP_COPYRIGHT}"
        QT_TARGET_PRODUCT_NAME "${CMAKE_PROJECT_NAME}"
        # QT_TARGET_COMMENTS: RC Comments
        # QT_TARGET_ORIGINAL_FILENAME: RC Original FileName
        # QT_TARGET_TRADEMARKS: RC LegalTrademarks
        # QT_TARGET_INTERNALNAME: RC InternalName
        QT_TARGET_RC_ICONS "${QGC_WINDOWS_ICON_PATH}"
)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE _USE_MATH_DEFINES NOMINMAX WIN32_LEAN_AND_MEAN)
