# ============================================================================
# CreateCPackDeb.cmake
# Debian/Ubuntu .deb package generator
# ============================================================================

include(CreateCPackCommon)

# ----------------------------------------------------------------------------
# DEB Generator Configuration
# ----------------------------------------------------------------------------
set(CPACK_GENERATOR "DEB")
set(CPACK_BINARY_DEB ON)

# ----------------------------------------------------------------------------
# Package Metadata
# ----------------------------------------------------------------------------
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/QGroundControl")
set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_SOURCE_DIR}/cmake/install/FinalizeNativePackage.cmake")
set(CPACK_DEBIAN_PACKAGE_NAME "qgroundcontrol")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
# Monotonic release so apt orders rebuilds of the same version: commits-since-tag
# (increments per commit) with the short hash appended for traceability.
set(CPACK_DEBIAN_PACKAGE_RELEASE "${QGC_APP_VERSION_DEV}.g${QGC_GIT_HASH}")
# Debian policy requires "Name <email>"; bare "Dronecode" is malformed.
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Dronecode <dev@dronecode.org>")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
    "QGroundControl provides full flight control and mission planning for MAVLink-compatible vehicles."
)
set(CPACK_DEBIAN_PACKAGE_SECTION "electronics")
# CPACK_DEBIAN_ARCHIVE_TYPE is the data.tar archive *format* (gnutar/paxr), not
# the compressor; xz is a compression type and belongs in COMPRESSION_TYPE.
set(CPACK_DEBIAN_ARCHIVE_TYPE "gnutar")
set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${PROJECT_HOMEPAGE_URL}")

# ----------------------------------------------------------------------------
# Advanced DEB Options
# ----------------------------------------------------------------------------
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS "/opt/QGroundControl/lib")

include(CPack)
