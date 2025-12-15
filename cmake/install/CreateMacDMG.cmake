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
set(QGC_TARGET_APP_BUNDLE "${QGC_TARGET_APP_NAME}.app")

set(QGC_DMG_WINDOW_POS 200 120)
set(QGC_DMG_WINDOW_SIZE 640 480)
set(QGC_DMG_ICON_SIZE 128)
set(QGC_DMG_APP_ICON_POS 160 220)
set(QGC_DMG_DROP_ICON_POS 480 220)

message(STATUS "QGC: Building ${QGC_DMG_NAME}...")

set(QGC_CREATE_DMG_ARGS
    --volname "${QGC_TARGET_APP_NAME}"
    --filesystem APFS
)
list(APPEND QGC_CREATE_DMG_ARGS --window-pos ${QGC_DMG_WINDOW_POS})
list(APPEND QGC_CREATE_DMG_ARGS --window-size ${QGC_DMG_WINDOW_SIZE})
list(APPEND QGC_CREATE_DMG_ARGS --icon-size ${QGC_DMG_ICON_SIZE})
list(APPEND QGC_CREATE_DMG_ARGS --icon "${QGC_TARGET_APP_BUNDLE}" ${QGC_DMG_APP_ICON_POS})
# Drop target drives drag-and-drop install experience for Applications folder
list(APPEND QGC_CREATE_DMG_ARGS --app-drop-link ${QGC_DMG_DROP_ICON_POS})
list(APPEND QGC_CREATE_DMG_ARGS "${QGC_DMG_NAME}" "${QGC_DMG_PATH}/")

execute_process(
    COMMAND "${CREATE_DMG_PROGRAM}" ${QGC_CREATE_DMG_ARGS}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
)

message(STATUS "QGC: DMG created successfully: ${CMAKE_BINARY_DIR}/${QGC_DMG_NAME}")
