# ============================================================================
# CreateCPackBundle.cmake
# macOS application bundle package generator
# ============================================================================

include(CreateCPackCommon)

# ----------------------------------------------------------------------------
# Bundle Generator Configuration
# ----------------------------------------------------------------------------
set(CPACK_GENERATOR "Bundle")
set(CPACK_BINARY_BUNDLE ON)

set(QGC_INSTALLER_SOURCE "${CMAKE_SOURCE_DIR}/deploy/macos")

# ----------------------------------------------------------------------------
# Bundle Configuration
# ----------------------------------------------------------------------------
set(CPACK_BUNDLE_NAME "${CMAKE_PROJECT_NAME}")
# CPack BUNDLE consumes the plist verbatim — render @VAR@ tokens first.
configure_file(
    "${QGC_INSTALLER_SOURCE}/MacOSXBundleInfo.plist.in"
    "${CMAKE_BINARY_DIR}/MacOSXBundleInfo.plist"
    @ONLY
)
set(CPACK_BUNDLE_PLIST "${CMAKE_BINARY_DIR}/MacOSXBundleInfo.plist")
set(CPACK_BUNDLE_ICON "${QGC_APP_ICON}")

include(CPack)
