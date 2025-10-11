# ============================================================================
# CreateWinInstaller.cmake
# Windows NSIS installer creation using makensis
# ============================================================================

message(STATUS "QGC: Creating Windows NSIS Installer")

# ----------------------------------------------------------------------------
# Validate Required Variables
# ----------------------------------------------------------------------------
foreach(p IN ITEMS
    QGC_WINDOWS_ICON_PATH
    QGC_WINDOWS_INSTALL_HEADER_PATH
    QGC_WINDOWS_INSTALLER_SCRIPT
    QGC_WINDOWS_OUT
    CMAKE_INSTALL_PREFIX)
    if(NOT DEFINED ${p})
        message(FATAL_ERROR "QGC: Missing required var: ${p}")
    endif()
endforeach()

# ----------------------------------------------------------------------------
# Convert Paths to Native Windows Format
# ----------------------------------------------------------------------------
file(TO_NATIVE_PATH "${QGC_WINDOWS_ICON_PATH}" QGC_INSTALLER_ICON)
file(TO_NATIVE_PATH "${QGC_WINDOWS_INSTALL_HEADER_PATH}" QGC_INSTALLER_HEADER_BITMAP)
file(TO_NATIVE_PATH "${QGC_WINDOWS_INSTALLER_SCRIPT}" QGC_NSIS_INSTALLER_SCRIPT)
file(TO_NATIVE_PATH "${QGC_WINDOWS_OUT}" QGC_INSTALLER_OUT)
file(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}" QGC_PAYLOAD_DIR)

# ----------------------------------------------------------------------------
# Locate NSIS makensis Utility
# ----------------------------------------------------------------------------
set(_pf86 "ProgramFiles(x86)")
set(_PF86 "PROGRAMFILES(x86)")
find_program(QGC_NSIS_INSTALLER_CMD makensis
    PATHS "$ENV{Programfiles}" "$ENV{PROGRAMFILES}" "$ENV{${_pf86}}" "$ENV{${_PF86}}" "$ENV{ProgramW6432}" "$ENV{PROGRAMW6432}"
    PATH_SUFFIXES "NSIS"
    DOC "Path to the makensis utility."
    REQUIRED
)

# ----------------------------------------------------------------------------
# Build NSIS Command Arguments
# ----------------------------------------------------------------------------

set(_nsis_args
    /NOCD
    /INPUTCHARSET UTF8
    /V4
    "/DAPPNAME=${CMAKE_PROJECT_NAME}"
    "/DEXENAME=${CMAKE_PROJECT_NAME}"
    "/DORGNAME=${QGC_ORG_NAME}"
    "/DDESTDIR=${QGC_PAYLOAD_DIR}"
)

if(EXISTS "${QGC_INSTALLER_ICON}")
    list(APPEND _nsis_args "/DINSTALLER_ICON=${QGC_INSTALLER_ICON}")
endif()

if(EXISTS "${QGC_INSTALLER_HEADER_BITMAP}")
    list(APPEND _nsis_args "/DHEADER_BITMAP=${QGC_INSTALLER_HEADER_BITMAP}")
endif()

set(_APPVER "${CMAKE_PROJECT_VERSION}")
if(_APPVER)
    list(APPEND _nsis_args "/DAPPVERSION=${_APPVER}")
endif()

list(APPEND _nsis_args "/XOutFile ${QGC_INSTALLER_OUT}")

# ----------------------------------------------------------------------------
# Execute NSIS Installer Creation
# ----------------------------------------------------------------------------
execute_process(
    COMMAND "${QGC_NSIS_INSTALLER_CMD}" ${_nsis_args} "${QGC_NSIS_INSTALLER_SCRIPT}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
)
