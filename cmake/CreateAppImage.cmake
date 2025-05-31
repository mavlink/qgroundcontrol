message(STATUS "QGC: Creating AppImage")

set(APPIMAGETOOL_PATH "${CMAKE_BINARY_DIR}/appimagetool.AppImage")
if(NOT EXISTS "${APPIMAGETOOL_PATH}")
    file(DOWNLOAD https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-${CMAKE_SYSTEM_PROCESSOR}.AppImage "${APPIMAGETOOL_PATH}")
    # file(DOWNLOAD https://github.com/probonopd/go-appimage/releases/download/continuous/appimagetool-886-${CMAKE_SYSTEM_PROCESSOR}.AppImage "${APPIMAGETOOL_PATH}")
    execute_process(COMMAND chmod a+x "${APPIMAGETOOL_PATH}")
endif()

set(LD_PATH "${CMAKE_BINARY_DIR}/linuxdeploy.AppImage")
if(NOT EXISTS "${LD_PATH}")
    file(DOWNLOAD https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${CMAKE_SYSTEM_PROCESSOR}.AppImage "${LD_PATH}")
    execute_process(COMMAND chmod a+x "${LD_PATH}")
endif()

set(APPDIR_PATH "${CMAKE_BINARY_DIR}/AppDir")
# execute_process(COMMAND "${APPIMAGETOOL_PATH}" deploy "${APPDIR_PATH}/usr/share/applications/${QGC_PACKAGE_NAME}.desktop")
execute_process(COMMAND ${LD_PATH}
    --appdir ${APPDIR_PATH}
    --executable ${APPDIR_PATH}/usr/bin/${CMAKE_PROJECT_NAME}
    --desktop-file ${APPDIR_PATH}/usr/share/applications/${QGC_PACKAGE_NAME}.desktop
    --custom-apprun ${CMAKE_BINARY_DIR}/AppRun
    --icon-file ${APPDIR_PATH}/usr/share/icons/hicolor/256x256/apps/${CMAKE_PROJECT_NAME}.png
)

set(ENV{ARCH} ${CMAKE_SYSTEM_PROCESSOR})
# set(ENV{VERSION} ${CMAKE_PROJECT_VERSION})
execute_process(COMMAND "${APPIMAGETOOL_PATH}" "${APPDIR_PATH}")

set(LD_APPIMAGELINT_PATH "${CMAKE_BINARY_DIR}/appimagelint.AppImage")
if(NOT EXISTS "${LD_APPIMAGELINT_PATH}")
    file(DOWNLOAD https://github.com/TheAssassin/appimagelint/releases/download/continuous/appimagelint-${CMAKE_SYSTEM_PROCESSOR}.AppImage "${LD_APPIMAGELINT_PATH}")
    execute_process(COMMAND chmod a+x "${LD_APPIMAGELINT_PATH}")
endif()
execute_process(COMMAND "${LD_APPIMAGELINT_PATH}" "${CMAKE_PROJECT_NAME}-$ENV{ARCH}.AppImage")

# file(RENAME "${CMAKE_PROJECT_NAME}-$ENV{VERSION}-$ENV{ARCH}.AppImage" "${CMAKE_PROJECT_NAME}-$ENV{ARCH}.AppImage")

# TODO: https://github.com/AppImageCommunity/AppImageUpdate
