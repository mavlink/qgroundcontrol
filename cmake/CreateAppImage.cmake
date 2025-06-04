message(STATUS "Creating AppImage")
# TODO: https://github.com/AppImageCommunity/AppImageUpdate

set(APPDIR_PATH "${CMAKE_BINARY_DIR}/AppDir")
set(APPIMAGETOOL_PATH "${CMAKE_BINARY_DIR}/appimagetool-x86_64.AppImage")
set(LD_PATH "${CMAKE_BINARY_DIR}/linuxdeploy-${CMAKE_SYSTEM_PROCESSOR}.AppImage")
# set(LD_APPIMAGEPLUGIN_PATH "${CMAKE_BINARY_DIR}/linuxdeploy-plugin-appimage-${CMAKE_SYSTEM_PROCESSOR}.AppImage")
# set(LD_QTPLUGIN_PATH "${CMAKE_BINARY_DIR}/linuxdeploy-plugin-qt-${CMAKE_SYSTEM_PROCESSOR}.AppImage")
# set(LD_GSTPLUGIN_PATH "${CMAKE_BINARY_DIR}/linuxdeploy-plugin-gstreamer.sh")

if(NOT EXISTS "${APPIMAGETOOL_PATH}")
    file(DOWNLOAD https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-${CMAKE_SYSTEM_PROCESSOR}.AppImage "${APPIMAGETOOL_PATH}")
    # file(DOWNLOAD https://github.com/probonopd/go-appimage/releases/download/832/appimagetool-823-${CMAKE_SYSTEM_PROCESSOR}.AppImage "${APPIMAGETOOL_PATH}") # TODO: Use Continuous Release
    execute_process(COMMAND chmod a+x "${APPIMAGETOOL_PATH}")
endif()
if(NOT EXISTS "${LD_PATH}")
    file(DOWNLOAD https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${CMAKE_SYSTEM_PROCESSOR}.AppImage "${LD_PATH}")
    execute_process(COMMAND chmod a+x "${LD_PATH}")
endif()
# if(NOT EXISTS "${LD_APPIMAGEPLUGIN_PATH}")
#     file(DOWNLOAD https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-${CMAKE_SYSTEM_PROCESSOR}.AppImage "${LD_APPIMAGEPLUGIN_PATH}")
#     execute_process(COMMAND chmod a+x "${LD_APPIMAGEPLUGIN_PATH}")
# endif()
# if(NOT EXISTS "${LD_QTPLUGIN_PATH}")
#     file(DOWNLOAD https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${CMAKE_SYSTEM_PROCESSOR}.AppImage "${LD_QTPLUGIN_PATH}")
#     execute_process(COMMAND chmod a+x "${LD_QTPLUGIN_PATH}")
# endif()
# if(NOT EXISTS "${LD_GSTPLUGIN_PATH}")
#     file(DOWNLOAD https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gstreamer/master/linuxdeploy-plugin-gstreamer.sh "${LD_GSTPLUGIN_PATH}")
#     execute_process(COMMAND chmod a+x "${LD_GSTPLUGIN_PATH}")
# endif()

execute_process(COMMAND ${LD_PATH}
    --appdir ${APPDIR_PATH}
    --executable ${APPDIR_PATH}/usr/bin/QGroundControl
    --desktop-file ${APPDIR_PATH}/usr/share/applications/org.mavlink.qgroundcontrol.desktop
    --custom-apprun ${CMAKE_BINARY_DIR}/AppRun)
    # --plugin qt --plugin gstreamer
    # --exclude-library "libgst*"

set(ENV{ARCH} ${CMAKE_SYSTEM_PROCESSOR})
# set(ENV{VERSION} 5.0)
execute_process(COMMAND ${APPIMAGETOOL_PATH} ${APPDIR_PATH})
