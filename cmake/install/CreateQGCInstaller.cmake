# ============================================================================
# CreateQGCInstaller.cmake
# Generates installer packages using Qt Installer Framework
# ============================================================================

message(STATUS "QGC: Creating installer package")

include(CMakePrintHelpers)

# ----------------------------------------------------------------------------
# Qt Installer Framework Detection
# ----------------------------------------------------------------------------
set(QT_INSTALLER_FRAMEWORK_DIR ${QT_ROOT_DIR}/../../Tools/QtInstallerFramework)
find_program(QT_INSTALLER_FRAMEWORK binarycreator
    PATHS "${QT_INSTALLER_FRAMEWORK_ROOT}/*/bin"
)

# ----------------------------------------------------------------------------
# Installer Source Directories
# ----------------------------------------------------------------------------
set(INSTALLER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/deploy/installer)
set(INSTALLER_SOURCE_CONFIG_DIR ${INSTALLER_SOURCE_DIR}/config)
set(INSTALLER_SOURCE_PACKAGES_DIR ${INSTALLER_SOURCE_DIR}/packages)

set(INSTALLER_SOURCE_PACKAGES_QGC_DIR ${INSTALLER_SOURCE_PACKAGES_DIR}/org.mavlink.qgroundcontrol)
set(INSTALLER_SOURCE_PACKAGES_QGC_DATA_DIR ${INSTALLER_SOURCE_PACKAGES_QGC_DIR}/data)
set(INSTALLER_SOURCE_PACKAGES_QGC_META_DIR ${INSTALLER_SOURCE_PACKAGES_QGC_DIR}/meta)

# ----------------------------------------------------------------------------
# Installer Output Directories
# ----------------------------------------------------------------------------
set(INSTALLER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/installer)
set(INSTALLER_OUTPUT_CONFIG_DIR ${INSTALLER_OUTPUT_DIR}/config)
set(INSTALLER_OUTPUT_PACKAGES_DIR ${INSTALLER_OUTPUT_DIR}/packages)

set(INSTALLER_OUTPUT_PACKAGES_QGC_DIR ${INSTALLER_OUTPUT_PACKAGES_DIR}/org.mavlink.qgroundcontrol)
set(INSTALLER_OUTPUT_PACKAGES_QGC_DATA_DIR ${INSTALLER_OUTPUT_PACKAGES_QGC_DIR}/data)
set(INSTALLER_OUTPUT_PACKAGES_QGC_META_DIR ${INSTALLER_OUTPUT_PACKAGES_QGC_DIR}/meta)

# ----------------------------------------------------------------------------
# Create Output Directory Structure
# ----------------------------------------------------------------------------

file(MAKE_DIRECTORY ${INSTALLER_OUTPUT_DIR})
file(MAKE_DIRECTORY ${INSTALLER_OUTPUT_CONFIG_DIR})
file(MAKE_DIRECTORY ${INSTALLER_OUTPUT_PACKAGES_DIR})

file(MAKE_DIRECTORY ${INSTALLER_OUTPUT_PACKAGES_QGC_DIR})
file(MAKE_DIRECTORY ${INSTALLER_OUTPUT_PACKAGES_QGC_DATA_DIR})
file(MAKE_DIRECTORY ${INSTALLER_OUTPUT_PACKAGES_QGC_META_DIR})

# ----------------------------------------------------------------------------
# Configure Installer Templates
# ----------------------------------------------------------------------------
configure_file(
    ${INSTALLER_SOURCE_CONFIG_DIR}/config.xml.in
    ${INSTALLER_OUTPUT_CONFIG_DIR}/config.xml
    @ONLY
)

configure_file(
    ${INSTALLER_SOURCE_PACKAGES_QGC_META_DIR}/package.xml.in
    ${INSTALLER_OUTPUT_PACKAGES_QGC_META_DIR}/package.xml
    @ONLY
)

# TODO: Copy additional resources when needed
# file(COPY ${QGC_APP_ICON} DESTINATION ${QGC_INSTALLER_ROOT}/config/)
# file(COPY ${CMAKE_SOURCE_DIR}/README.md DESTINATION ${QGC_INSTALLER_ROOT}/README.md)
# file(COPY ${CMAKE_SOURCE_DIR}/.github/COPYING.md DESTINATION ${QGC_PACKAGE_ROOT}/meta/license.txt)
# file(GLOB_RECURSE FILES_TO_INSTALL RELATIVE ${CMAKE_INSTALL_PREFIX} ${CMAKE_INSTALL_PREFIX}/**)
# file(COPY ${FILES_TO_INSTALL} DESTINATION ${QGC_PACKAGE_ROOT}/data/)

# ----------------------------------------------------------------------------
# Platform-Specific Installer Names
# ----------------------------------------------------------------------------
if(WIN32)
    set(QGC_INSTALLER_NAME ${CMAKE_PROJECT_NAME}-Installer-${CMAKE_SYSTEM_PROCESSOR}.exe)
endif()

# ----------------------------------------------------------------------------
# Generate Installer
# ----------------------------------------------------------------------------
execute_process(
    COMMAND ${QT_INSTALLER_FRAMEWORK} --offline-only -c ${INSTALLER_OUTPUT_CONFIG_DIR}/config.xml -p ${INSTALLER_OUTPUT_PACKAGES_DIR} ${CMAKE_BINARY_DIR}/${QGC_INSTALLER_NAME}
)
