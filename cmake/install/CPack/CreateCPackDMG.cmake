# ============================================================================
# CreateCPackDMG.cmake
# macOS DMG disk image package generator
# ============================================================================

include(CreateCPackCommon)

# ----------------------------------------------------------------------------
# DMG Generator Configuration
# ----------------------------------------------------------------------------
set(CPACK_GENERATOR "DragNDrop")
set(CPACK_BINARY_DRAGNDROP ON)

# ----------------------------------------------------------------------------
# DMG Options
# ----------------------------------------------------------------------------
set(CPACK_DMG_FORMAT "UDBZ")
set(CPACK_DMG_FILESYSTEM "APFS")

include(CPack)
