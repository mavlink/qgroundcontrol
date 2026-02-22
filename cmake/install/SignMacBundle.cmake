# ============================================================================
# SignMacBundle.cmake
# Code signing and notarization for macOS application bundles
# ============================================================================

message(STATUS "QGC: Signing Bundle using signing identity")

# ----------------------------------------------------------------------------
# Environment Variable Validation
# ----------------------------------------------------------------------------
if(NOT DEFINED ENV{QGC_MACOS_SIGNING_IDENTITY} OR "$ENV{QGC_MACOS_SIGNING_IDENTITY}" STREQUAL "")
    message(FATAL_ERROR "QGC: QGC_MACOS_SIGNING_IDENTITY environment variable must be set to sign MacOS bundle")
endif()
if(NOT DEFINED ENV{QGC_MACOS_NOTARIZATION_USERNAME} OR "$ENV{QGC_MACOS_NOTARIZATION_USERNAME}" STREQUAL "")
    message(FATAL_ERROR "QGC: QGC_MACOS_NOTARIZATION_USERNAME environment variable must be set to notarize MacOS bundle")
endif()
if(NOT DEFINED ENV{QGC_MACOS_NOTARIZATION_TEAM_ID} OR "$ENV{QGC_MACOS_NOTARIZATION_TEAM_ID}" STREQUAL "")
    message(FATAL_ERROR "QGC: QGC_MACOS_NOTARIZATION_TEAM_ID environment variable must be set to notarize MacOS bundle")
endif()
if(NOT DEFINED ENV{QGC_MACOS_NOTARIZATION_PASSWORD} OR "$ENV{QGC_MACOS_NOTARIZATION_PASSWORD}" STREQUAL "")
    message(FATAL_ERROR "QGC: QGC_MACOS_NOTARIZATION_PASSWORD environment variable must be set to notarize MacOS bundle")
endif()

# ----------------------------------------------------------------------------
# Clean Up GStreamer Symlinks
# ----------------------------------------------------------------------------
file(REMOVE "${QGC_STAGING_BUNDLE_PATH}/Contents/Frameworks/GStreamer.framework/Commands")
file(REMOVE "${QGC_STAGING_BUNDLE_PATH}/Contents/Frameworks/GStreamer.framework/Versions/1.0/Commands")

# ----------------------------------------------------------------------------
# Sign All Libraries and Executables
# ----------------------------------------------------------------------------
# Sign all dynamic libraries
execute_process(
    COMMAND find "${QGC_STAGING_BUNDLE_PATH}/Contents" -type f -name "*.dylib" -exec codesign --timestamp --options=runtime --force -s "$ENV{QGC_MACOS_SIGNING_IDENTITY}" "{}" \\;
    COMMAND_ERROR_IS_FATAL ANY
)

# Sign all shared objects
execute_process(
    COMMAND find "${QGC_STAGING_BUNDLE_PATH}/Contents" -type f -name "*.so" -exec codesign --timestamp --options=runtime --force -s "$ENV{QGC_MACOS_SIGNING_IDENTITY}" "{}" \\;
    COMMAND_ERROR_IS_FATAL ANY
)

# ----------------------------------------------------------------------------
# Sign GStreamer Framework Components
# ----------------------------------------------------------------------------
if(EXISTS "${QGC_STAGING_BUNDLE_PATH}/Contents/Frameworks/GStreamer.framework")
    execute_process(
        COMMAND find "${QGC_STAGING_BUNDLE_PATH}/Contents/Frameworks/GStreamer.framework/Versions/1.0/libexec/gstreamer-1.0" -type f -exec codesign --timestamp --options=runtime --force -s "$ENV{QGC_MACOS_SIGNING_IDENTITY}" "{}" \\;
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND codesign --timestamp --options=runtime --force -s "$ENV{QGC_MACOS_SIGNING_IDENTITY}" "${QGC_STAGING_BUNDLE_PATH}/Contents/Frameworks/GStreamer.framework/Versions/1.0/lib/GStreamer"
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND codesign --timestamp --options=runtime --force -s "$ENV{QGC_MACOS_SIGNING_IDENTITY}" "${QGC_STAGING_BUNDLE_PATH}/Contents/Frameworks/GStreamer.framework/Versions/1.0/GStreamer"
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

# Sign GStreamer helper binaries (auto-downloaded SDK installs these outside the framework)
if(EXISTS "${QGC_STAGING_BUNDLE_PATH}/Contents/libexec/gstreamer-1.0")
    file(GLOB_RECURSE GSTREAMER_HELPER_CANDIDATES
        LIST_DIRECTORIES false
        "${QGC_STAGING_BUNDLE_PATH}/Contents/libexec/gstreamer-1.0/*"
    )
    foreach(GSTREAMER_HELPER_FILE ${GSTREAMER_HELPER_CANDIDATES})
        if(IS_EXECUTABLE "${GSTREAMER_HELPER_FILE}")
            execute_process(
                COMMAND codesign --timestamp --options=runtime --force -s "$ENV{QGC_MACOS_SIGNING_IDENTITY}" "${GSTREAMER_HELPER_FILE}"
                COMMAND_ERROR_IS_FATAL ANY
            )
        endif()
    endforeach()
endif()

# ----------------------------------------------------------------------------
# Sign All Frameworks
# ----------------------------------------------------------------------------
file(GLOB FRAMEWORK_DIRS "${QGC_STAGING_BUNDLE_PATH}/Contents/Frameworks/*.framework")
foreach(FRAMEWORK_DIR ${FRAMEWORK_DIRS})
    if(EXISTS "${FRAMEWORK_DIR}/Versions/1.0")
        execute_process(
            COMMAND find "${FRAMEWORK_DIR}/Versions/1.0" -type f -exec codesign --timestamp --options=runtime --force -s "$ENV{QGC_MACOS_SIGNING_IDENTITY}" "{}" \\;
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()
    if(EXISTS "${FRAMEWORK_DIR}/Versions/A")
        execute_process(
            COMMAND find "${FRAMEWORK_DIR}/Versions/A" -type f -exec codesign --timestamp --options=runtime --force -s "$ENV{QGC_MACOS_SIGNING_IDENTITY}" "{}" \\;
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()
    # Some frameworks (notably GStreamer) use versioned directories and symlinked
    # entry points. Sign concrete version roots before signing the bundle path.
    if(EXISTS "${FRAMEWORK_DIR}/Versions/Current")
        execute_process(
            COMMAND codesign --deep --timestamp --options=runtime --force -s "$ENV{QGC_MACOS_SIGNING_IDENTITY}" "${FRAMEWORK_DIR}/Versions/Current"
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()
    if(EXISTS "${FRAMEWORK_DIR}/Versions/1.0")
        execute_process(
            COMMAND codesign --deep --timestamp --options=runtime --force -s "$ENV{QGC_MACOS_SIGNING_IDENTITY}" "${FRAMEWORK_DIR}/Versions/1.0"
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()
    if(EXISTS "${FRAMEWORK_DIR}/Versions/A")
        execute_process(
            COMMAND codesign --deep --timestamp --options=runtime --force -s "$ENV{QGC_MACOS_SIGNING_IDENTITY}" "${FRAMEWORK_DIR}/Versions/A"
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()
    # Framework bundles themselves must be signed after their internal binaries.
    execute_process(
        COMMAND codesign --deep --timestamp --options=runtime --force -s "$ENV{QGC_MACOS_SIGNING_IDENTITY}" "${FRAMEWORK_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
endforeach()

# Re-sign and verify GStreamer.framework as the final framework step. This
# avoids versioned/symlinked framework edge cases that can survive earlier passes.
if(EXISTS "${QGC_STAGING_BUNDLE_PATH}/Contents/Frameworks/GStreamer.framework")
    execute_process(
        COMMAND codesign --deep --timestamp --options=runtime --force -s "$ENV{QGC_MACOS_SIGNING_IDENTITY}" "${QGC_STAGING_BUNDLE_PATH}/Contents/Frameworks/GStreamer.framework/Versions/1.0"
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND codesign --deep --timestamp --options=runtime --force -s "$ENV{QGC_MACOS_SIGNING_IDENTITY}" "${QGC_STAGING_BUNDLE_PATH}/Contents/Frameworks/GStreamer.framework"
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND codesign --verify --deep --strict --verbose=2 "${QGC_STAGING_BUNDLE_PATH}/Contents/Frameworks/GStreamer.framework"
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

# ----------------------------------------------------------------------------
# Sign Main Application Bundle
# ----------------------------------------------------------------------------
execute_process(
    COMMAND codesign --deep --timestamp --options=runtime --force -s "$ENV{QGC_MACOS_SIGNING_IDENTITY}" "${QGC_STAGING_BUNDLE_PATH}"
    COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
    COMMAND codesign --verify --deep --strict --verbose=2 "${QGC_STAGING_BUNDLE_PATH}"
    COMMAND_ERROR_IS_FATAL ANY
)

# ============================================================================
# Notarization Process
# ============================================================================

set(_notarize_zip "${CMAKE_BINARY_DIR}/qgc_notarization_upload.zip")
message(STATUS "QGC: Archiving Bundle for Notarization upload")
file(REMOVE "${_notarize_zip}")
execute_process(
    COMMAND ditto -c -k --keepParent "${QGC_STAGING_BUNDLE_PATH}" "${_notarize_zip}"
    COMMAND_ERROR_IS_FATAL ANY
)
message(STATUS "QGC: Notarizing app bundle. This may take a while...")
execute_process(
    COMMAND xcrun notarytool submit "${_notarize_zip}" --apple-id "$ENV{QGC_MACOS_NOTARIZATION_USERNAME}" --team-id "$ENV{QGC_MACOS_NOTARIZATION_TEAM_ID}" --password "$ENV{QGC_MACOS_NOTARIZATION_PASSWORD}" --wait
    COMMAND_ERROR_IS_FATAL ANY
)
message(STATUS "QGC: Stapling notarization ticket to app bundle")
execute_process(
    COMMAND xcrun stapler staple "${QGC_STAGING_BUNDLE_PATH}"
    COMMAND_ERROR_IS_FATAL ANY
)
