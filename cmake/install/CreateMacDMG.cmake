# ============================================================================
# CreateMacDMG.cmake
# Creates macOS DMG disk image for distribution
# ============================================================================
#
# Required Variables (passed from Install.cmake):
#   QGC_STAGING_BUNDLE_PATH => Full path to MyApp.app bundle
#   CREATE_DMG_PROGRAM      => Full path to create-dmg program
#

message(STATUS "QGC: Creating macOS DMG disk image...")

# ============================================================================
# Prepare Package Directory
# ============================================================================

set(QGC_DMG_PATH "${CMAKE_BINARY_DIR}/package")

# Clean and create package directory
file(REMOVE_RECURSE "${QGC_DMG_PATH}")
file(MAKE_DIRECTORY "${QGC_DMG_PATH}")

# Copy the application bundle to package directory
file(COPY "${QGC_STAGING_BUNDLE_PATH}" DESTINATION "${QGC_DMG_PATH}")

# ============================================================================
# Create DMG
# ============================================================================

cmake_path(GET QGC_STAGING_BUNDLE_PATH STEM QGC_TARGET_APP_NAME)
set(QGC_DMG_NAME "${QGC_TARGET_APP_NAME}.dmg")

message(STATUS "QGC: Building ${QGC_DMG_NAME}...")

execute_process(
    COMMAND "${CREATE_DMG_PROGRAM}"
            --volname "${QGC_TARGET_APP_NAME}"
            --filesystem APFS
            "${QGC_DMG_NAME}"
            "${QGC_DMG_PATH}/"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
)

message(STATUS "QGC: DMG created successfully: ${CMAKE_BINARY_DIR}/${QGC_DMG_NAME}")
