# -- Variables ----------------------------------------
# QGC_STAGING_BUNDLE_PATH => full path to MyApp.app
# CREATE_DMG_PROGRAM => full path to create-dmg program
# ---------------------------------------------------------------------------

set(QGC_DMG_PATH "${CMAKE_BINARY_DIR}/package")
file(REMOVE_RECURSE "${QGC_DMG_PATH}")
file(MAKE_DIRECTORY "${QGC_DMG_PATH}")
file(COPY "${QGC_STAGING_BUNDLE_PATH}" DESTINATION "${QGC_DMG_PATH}")

cmake_path(GET QGC_STAGING_BUNDLE_PATH STEM QGC_TARGET_APP_NAME)

set(QGC_DMG_NAME "${QGC_TARGET_APP_NAME}.dmg")

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

message(STATUS "QGC: Created ${QGC_DMG_NAME}")
