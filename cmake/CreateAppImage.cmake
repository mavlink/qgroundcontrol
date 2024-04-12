message(STATUS "Creating AppImage")

set(APPDIR_PATH "${CMAKE_BINARY_DIR}/AppDir")
set(LD_PATH "${CMAKE_BINARY_DIR}/linuxdeploy-x86_64.AppImage")
set(LD_APPIMAGEPLUGIN_PATH "${CMAKE_BINARY_DIR}/linuxdeploy-plugin-appimage-x86_64.AppImage")
set(LD_QTPLUGIN_PATH "${CMAKE_BINARY_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage")

if(NOT EXISTS "${LD_PATH}")
    file(DOWNLOAD https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage "${LD_PATH}")
    execute_process(COMMAND chmod a+x "${LD_PATH}")
endif()
if(NOT EXISTS "${LD_APPIMAGEPLUGIN_PATH}")
    file(DOWNLOAD https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage "${LD_APPIMAGEPLUGIN_PATH}")
    execute_process(COMMAND chmod a+x "${LD_APPIMAGEPLUGIN_PATH}")
endif()
if(NOT EXISTS "${LD_QTPLUGIN_PATH}")
    file(DOWNLOAD https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage "${LD_QTPLUGIN_PATH}")
    execute_process(COMMAND chmod a+x "${LD_QTPLUGIN_PATH}")
endif()

execute_process(COMMAND ${LD_PATH} --appdir ${APPDIR_PATH} --output appimage) # --plugin qt
