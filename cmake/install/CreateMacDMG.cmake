# -- Variables ----------------------------------------
# QGC_STAGING_BUNDLE_PATH => full path to MyApp.app

# ---------------------------------------------------------------------------
# 1. Grab or locate create-dmg
# ---------------------------------------------------------------------------
find_program(CREATE_DMG_PROGRAM create-dmg)
if(NOT CREATE_DMG_PROGRAM)
    message(STATUS "QGC: Downloading create-dmg")
    CPMAddPackage(
        NAME create-dmg
        GITHUB_REPOSITORY create-dmg/create-dmg
        GIT_TAG master
        DOWNLOAD_ONLY
    )
    set(CREATE_DMG_PROGRAM "${create-dmg_SOURCE_DIR}/create-dmg")
endif()

# ---------------------------------------------------------------------------
# 2. Build the DMG with a nice drag-and-drop layout
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
