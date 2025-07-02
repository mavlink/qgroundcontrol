if(NOT WIN32)
    message(FATAL_ERROR "Invalid Platform")
    return()
endif()

# CPMAddPackage(
#     NAME windows_drivers
#     URL https://firmware.ardupilot.org/Tools/MissionPlanner/driver.msi
# )
# ${windows_drivers_SOURCE_DIR}/driver.msi

# windows installer files shared with core and custom
set(DEPLOY_WIN_FILES
    "${CMAKE_SOURCE_DIR}/deploy/windows/driver.msi"
    "${CMAKE_SOURCE_DIR}/deploy/windows/nullsoft_installer.nsi"
    "${QGC_WINDOWS_RESOURCE_FILE_PATH}"
    "${QGC_WINDOWS_INSTALL_HEADER_PATH}"
    "${QGC_WINDOWS_ICON_PATH}"
)

# Destination directory where files will be copied
set(QGC_INSTALLER_SOURCE_WIN "${CMAKE_BINARY_DIR}/deploy/windows")
file(MAKE_DIRECTORY ${QGC_INSTALLER_SOURCE_WIN})
foreach(FILE ${DEPLOY_WIN_FILES})
    # filename without the path
    get_filename_component(FILE_NAME ${FILE} NAME)
    # re-copy the file if it changes
    add_custom_command(
        OUTPUT "${QGC_INSTALLER_SOURCE_WIN}/${FILE_NAME}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FILE}" "${QGC_INSTALLER_SOURCE_WIN}/${FILE_NAME}"
        DEPENDS "${FILE}"  # Depend on the source file so that it re-copies when it changes
    )
    list(APPEND QGC_INSTALLER_SOURCE_WIN_FILES "${QGC_INSTALLER_SOURCE_WIN}/${FILE_NAME}")
endforeach()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${QGC_INSTALLER_SOURCE_WIN_FILES})
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
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE _USE_MATH_DEFINES)
