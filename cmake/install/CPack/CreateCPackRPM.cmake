# ============================================================================
# CreateCPackRPM.cmake
# Red Hat/Fedora/CentOS .rpm package generator
# ============================================================================

include(CreateCPackCommon)

# ----------------------------------------------------------------------------
# RPM Generator Configuration
# ----------------------------------------------------------------------------
set(CPACK_GENERATOR "RPM")
set(CPACK_BINARY_RPM ON)

# ----------------------------------------------------------------------------
# Package Metadata
# ----------------------------------------------------------------------------
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/QGroundControl")
set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_SOURCE_DIR}/cmake/install/FinalizeNativePackage.cmake")
set(CPACK_RPM_PACKAGE_NAME "qgroundcontrol")
# Let CPackRPM include the package release and normalized architecture instead
# of the generator-agnostic CPACK_PACKAGE_FILE_NAME.
set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
set(CPACK_RPM_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
# Mirror the DEB monotonic release (commits-since-tag + short hash); RELEASE_DIST
# appends %{?dist} (e.g. .fc41) so rebuilds across Fedora releases stay ordered.
set(CPACK_RPM_PACKAGE_RELEASE "${QGC_APP_VERSION_DEV}.g${QGC_GIT_HASH}")
set(CPACK_RPM_PACKAGE_RELEASE_DIST ON)
set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0 OR GPL-3.0-only")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
set(CPACK_RPM_PACKAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
set(CPACK_RPM_COMPRESSION_TYPE "xz")

# ----------------------------------------------------------------------------
# RPM Dependencies and Requirements
# ----------------------------------------------------------------------------
# Verbatim into the spec, so use rpmbuild's "yes", not CMake's "ON".
set(CPACK_RPM_PACKAGE_AUTOREQ "yes")

include(CPack)
