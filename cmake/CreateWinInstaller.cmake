message(STATUS "Creating Windows NSIS Installer")

# function(download_tool VAR URL)
#     cmake_path(GET URL FILENAME _name)
#     set(_dest "${CMAKE_BINARY_DIR}/staging/tools/${_name}")
#     if(NOT EXISTS "${_dest}")
#         file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/staging/tools")
#         message(STATUS "QGC: Downloading ${_name} to ${_dest}")
#         file(DOWNLOAD "${URL}" "${_dest}" STATUS _status)
#         list(GET _status 0 _result)
#         if(NOT _result EQUAL 0)
#             message(FATAL_ERROR "Failed to download ${URL} to ${_dest}: ${_status}")
#         endif()
#         file(CHMOD "${_dest}" FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
#     endif()
#     set(${VAR}_PATH "${_dest}" PARENT_SCOPE)
# endfunction()

# download_tool(DRIVERS https://firmware.ardupilot.org/Tools/MissionPlanner/driver.msi)



file(TO_NATIVE_PATH "${QGC_WINDOWS_ICON_PATH}" QGC_INSTALLER_ICON)
file(TO_NATIVE_PATH "${QGC_WINDOWS_INSTALL_HEADER_PATH}" QGC_INSTALLER_HEADER_BITMAP)
file(TO_NATIVE_PATH "${QGC_WINDOWS_DRIVER_MSI}" QGC_INSTALLER_DRIVER_MSI)
file(TO_NATIVE_PATH "${QGC_WINDOWS_INSTALLER_SCRIPT}" QGC_NSIS_INSTALLER_SCRIPT)
file(TO_NATIVE_PATH "${QGC_WINDOWS_OUT}" QGC_INSTALLER_OUT)

set(QGC_NSIS_INSTALLER_PARAMETERS
    /DDRIVER_MSI="${QGC_INSTALLER_DRIVER_MSI}"
    /DINSTALLER_ICON="${QGC_INSTALLER_ICON}"
    /DHEADER_BITMAP="${QGC_INSTALLER_HEADER_BITMAP}"
    /DAPPNAME=${CMAKE_PROJECT_NAME}
    /DEXENAME=${CMAKE_PROJECT_NAME}
    /DORGNAME=${QGC_ORG_NAME}
    /DDESTDIR="${CMAKE_INSTALL_PREFIX}"
    /NOCD
    "/XOutFile ${QGC_INSTALLER_OUT}"
    "${QGC_NSIS_INSTALLER_SCRIPT}"
)

find_program(QGC_NSIS_INSTALLER_CMD makensis
    PATHS "$ENV{Programfiles}/NSIS" "$ENV{ProgramFiles(x86)}/NSIS" "$ENV{ProgramW6432}/NSIS"
    DOC "Path to the makensis utility."
    REQUIRED
)

execute_process(
    COMMAND "${QGC_NSIS_INSTALLER_CMD}" "${QGC_NSIS_INSTALLER_PARAMETERS}"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
)
