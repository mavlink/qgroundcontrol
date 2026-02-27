# ============================================================================
# CreateCPackBundle.cmake
# macOS application bundle package generator
# ============================================================================

include(CreateCPackCommon)

# ----------------------------------------------------------------------------
# Bundle Generator Configuration
# ----------------------------------------------------------------------------
list(APPEND CPACK_GENERATOR "BUNDLE")
set(CPACK_BINARY_BUNDLE ON)

set(QGC_INSTALLER_SOURCE "${CMAKE_SOURCE_DIR}/deploy/macos")

# ----------------------------------------------------------------------------
# Bundle Configuration
# ----------------------------------------------------------------------------
set(CPACK_BUNDLE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_BUNDLE_PLIST "${QGC_INSTALLER_SOURCE}/MacOSXBundleInfo.plist.in")
set(CPACK_BUNDLE_ICON "${QGC_APP_ICON}")
# set(CPACK_BUNDLE_STARTUP_COMMAND "")

# ----------------------------------------------------------------------------
# Code Signing Options
# ----------------------------------------------------------------------------
# set(CPACK_BUNDLE_APPLE_CERT_APP "")
# set(CPACK_BUNDLE_APPLE_ENTITLEMENTS "")
# set(CPACK_BUNDLE_APPLE_CODESIGN_FILES "")
# set(CPACK_BUNDLE_APPLE_CODESIGN_PARAMETER "")
# set(CPACK_COMMAND_CODESIGN "")

include(CPack)
