# -- Variables ----------------------------------------
# QGC_BUNDLE_PATH               => full path to MyApp.app
# QGC_TARGET_APP_NAME           => MyApp
# QGC_MACOS_SIGNING_IDENTITY    => "Developer ID Application: ... (TEAMID)"
# QGC_MACOS_ENTITLEMENTS_PATH   => path/to/QGroundControl.entitlements

# ---------------------------------------------------------------------------
# 1. Sign the app bundle
# ---------------------------------------------------------------------------
find_program(CODESIGN_PROGRAM codesign REQUIRED)

execute_process(
    COMMAND "${CODESIGN_PROGRAM}"
            --force --deep --options runtime --timestamp
            --entitlements "${QGC_MACOS_ENTITLEMENTS_PATH}"
            -s "${QGC_MACOS_SIGNING_IDENTITY}"
            "${QGC_BUNDLE_PATH}"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
)

# ---------------------------------------------------------------------------
# 2. Grab or locate create-dmg
# ---------------------------------------------------------------------------
find_program(CREATE_DMG_PROGRAM create-dmg)
if(NOT CREATE_DMG_PROGRAM)
    CPMAddPackage(
        NAME create-dmg
        GITHUB_REPOSITORY create-dmg/create-dmg
        GIT_TAG master
        DOWNLOAD_ONLY
    )
    set(CREATE_DMG_PROGRAM "${create-dmg_SOURCE_DIR}/create-dmg")
endif()

# ---------------------------------------------------------------------------
# 3. Build the DMG with a nice drag-and-drop layout
# ---------------------------------------------------------------------------
set(QGC_DMG_NAME "${QGC_TARGET_APP_NAME}.dmg")

execute_process(
    COMMAND "${CREATE_DMG_PROGRAM}"
            --volname "${QGC_TARGET_APP_NAME}"
            --filesystem APFS
            "${QGC_DMG_NAME}"
            "${QGC_BUNDLE_PATH}"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
)

message(STATUS "QGC: Signed, notarised and packed ${QGC_DMG_NAME}")
