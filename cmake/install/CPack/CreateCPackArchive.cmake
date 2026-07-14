# ============================================================================
# CreateCPackArchive.cmake
# XZ-compressed tar archive generator for cross-platform distribution
# ============================================================================

include(CreateCPackCommon)

# ----------------------------------------------------------------------------
# Archive Generator Configuration
# ----------------------------------------------------------------------------
set(CPACK_GENERATOR "TXZ")
set(CPACK_BINARY_TXZ ON)

set(CPACK_ARCHIVE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}")
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_ARCHIVE_THREADS 2)

include(CPack)
