# ============================================================================
# CreateCPackDMG.cmake
# macOS DMG disk image package generator
# ============================================================================

include(CreateCPackCommon)

# ----------------------------------------------------------------------------
# DMG Generator Configuration
# ----------------------------------------------------------------------------
list(APPEND CPACK_GENERATOR "DragNDrop")
set(CPACK_BINARY_DRAGNDROP ON)

set(QGC_INSTALLER_SOURCE "${CMAKE_SOURCE_DIR}/deploy/macos")

# ----------------------------------------------------------------------------
# DMG Options
# ----------------------------------------------------------------------------
# set(CPACK_DMG_VOLUME_NAME ${CPACK_PACKAGE_FILE_NAME})
set(CPACK_DMG_FORMAT UDBZ)
# set(CPACK_DMG_DS_STORE)
# set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "")
# set(CPACK_DMG_BACKGROUND_IMAGE "")
# set(CPACK_DMG_DISABLE_APPLICATIONS_SYMLINK OFF)
# set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE OFF)
# set(CPACK_DMG_SLA_DIR "")
# set(CPACK_DMG_SLA_LANGUAGES "")
# set(CPACK_DMG_<component>_FILE_NAME "")
set(CPACK_DMG_FILESYSTEM APFS)

# ----------------------------------------------------------------------------
# macOS Disk Utility Commands
# ----------------------------------------------------------------------------
# set(CPACK_COMMAND_HDIUTIL "/usr/bin/sudo /usr/bin/hdiutil")
# set(CPACK_COMMAND_SETFILE "")
# set(CPACK_COMMAND_REZ "")

include(CPack)
