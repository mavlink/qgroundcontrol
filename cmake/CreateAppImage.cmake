message(STATUS "Creating AppImage")
# TODO: https://github.com/AppImageCommunity/AppImageUpdate

set(APPDIR_PATH "${CMAKE_BINARY_DIR}/AppDir")
# set(APPIMAGETOOL_PATH "${CMAKE_BINARY_DIR}/appimagetool-x86_64.AppImage")
set(LD_PATH "${CMAKE_BINARY_DIR}/linuxdeploy-x86_64.AppImage")
set(LD_APPIMAGEPLUGIN_PATH "${CMAKE_BINARY_DIR}/linuxdeploy-plugin-appimage-x86_64.AppImage")
# set(LD_QTPLUGIN_PATH "${CMAKE_BINARY_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage")
# set(LD_GSTPLUGIN_PATH "${CMAKE_BINARY_DIR}/linuxdeploy-plugin-gstreamer.sh")
# set(LD_GTKPLUGIN_PATH "${CMAKE_BINARY_DIR}/linuxdeploy-plugin-gtk.sh")

# if(NOT EXISTS "${APPIMAGETOOL_PATH}")
#     file(DOWNLOAD https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage "${APPIMAGETOOL_PATH}")
#     execute_process(COMMAND chmod a+x "${APPIMAGETOOL_PATH}")
# endif()
if(NOT EXISTS "${LD_PATH}")
    file(DOWNLOAD https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage "${LD_PATH}")
    execute_process(COMMAND chmod a+x "${LD_PATH}")
endif()
if(NOT EXISTS "${LD_APPIMAGEPLUGIN_PATH}")
    file(DOWNLOAD https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage "${LD_APPIMAGEPLUGIN_PATH}")
    execute_process(COMMAND chmod a+x "${LD_APPIMAGEPLUGIN_PATH}")
endif()
# if(NOT EXISTS "${LD_QTPLUGIN_PATH}")
#     file(DOWNLOAD https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage "${LD_QTPLUGIN_PATH}")
#     execute_process(COMMAND chmod a+x "${LD_QTPLUGIN_PATH}")
# endif()
# if(NOT EXISTS "${LD_GTKPLUGIN_PATH}")
#     file(DOWNLOAD https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh "${LD_GTKPLUGIN_PATH}")
#     execute_process(COMMAND chmod a+x "${LD_GTKPLUGIN_PATH}")
# endif()
# if(NOT EXISTS "${LD_GSTPLUGIN_PATH}")
#     file(DOWNLOAD https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gstreamer/master/linuxdeploy-plugin-gstreamer.sh "${LD_GSTPLUGIN_PATH}")
#     execute_process(COMMAND chmod a+x "${LD_GSTPLUGIN_PATH}")
# endif()

execute_process(COMMAND ${LD_PATH} --appdir ${APPDIR_PATH} --output appimage --custom-apprun ${CMAKE_BINARY_DIR}/AppRun)
# --exclude-library "libX*"
# --exclude-library "libglib*"
# --exclude-library "libgobject*"
# --exclude-library "libgdk_pixbuf*"
# --exclude-library "libwayland*"
# --exclude-library "libgmodule*"
# --exclude-library "libgio*"
# --exclude-library "libxcb*"
# --exclude-library "libxkbcommon*"
# --exclude-library "libdb*"
# --exclude-library "libgst*"
# --exclude-library "libgthread*"
# --plugin qt --plugin gtk --plugin gstreamer
