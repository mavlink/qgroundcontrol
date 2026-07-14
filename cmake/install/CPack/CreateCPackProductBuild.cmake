# ============================================================================
# CreateCPackProductBuild.cmake
# macOS .pkg installer package generator using productbuild
# ============================================================================

include(CreateCPackCommon)

# ----------------------------------------------------------------------------
# ProductBuild Generator Configuration
# ----------------------------------------------------------------------------
set(CPACK_GENERATOR "productbuild")
set(CPACK_BINARY_PRODUCTBUILD ON)
set(CPACK_PRODUCTBUILD_IDENTIFIER "${QGC_PACKAGE_NAME}")
set(CPACK_PACKAGING_INSTALL_PREFIX "/Applications")

include(CPack)
