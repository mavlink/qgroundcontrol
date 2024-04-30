message(STATUS "Creating Windows Installer")

include(CMakePrintHelpers)
set(INSTALLER_ROOT ${CMAKE_SOURCE_DIR}/deploy/installer)

file(GLOB_RECURSE FILES_TO_INSTALL RELATIVE ${CMAKE_INSTALL_PREFIX} ${CMAKE_INSTALL_PREFIX}/**)
file(COPY ${FILES_TO_INSTALL} DESTINATION ${INSTALLER_ROOT}/packages/org.mavlink.qgroundcontrol/data/)
cmake_print_variables(INSTALLER_ROOT FILES_TO_INSTALL)
execute_process(COMMAND ${QT_ROOT_DIR}/../../Tools/QtInstallerFramework/*/bin/binarycreator --offline-only -c ${INSTALLER_ROOT}/config/config.xml -p ${INSTALLER_ROOT}/packages ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-Installer.exe)
