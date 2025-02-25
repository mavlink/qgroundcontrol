include(InstallRequiredSystemLibraries)

install(
    TARGETS ${CMAKE_PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    BUNDLE  DESTINATION .
)

set(deploy_tool_options_arg "")
if(MACOS OR WIN32)
    set(deploy_tool_options_arg "-qmldir=${CMAKE_SOURCE_DIR}")
    if(MACOS_SIGNING_IDENTITY)
        message(STATUS "Signing MacOS Bundle")
        set(deploy_tool_options_arg "${deplay_tool_options_arg} -sign-for-notarization=${MACOS_SIGNING_IDENTITY}")
    endif()
endif()

# Set extra deploy QML app script options for Qt 6.7.0 and above
set(EXTRA_DEPLOY_QML_APP_SCRIPT_OPTIONS)
if(QT_VERSION VERSION_GREATER_EQUAL 6.7.0)
    list(APPEND EXTRA_DEPLOY_QML_APP_SCRIPT_OPTIONS DEPLOY_TOOL_OPTIONS ${deploy_tool_options_arg})
endif()

qt_generate_deploy_qml_app_script(
    TARGET ${CMAKE_PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    ${EXTRA_DEPLOY_QML_APP_SCRIPT_OPTIONS}
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})

if(ANDROID)
    # get_target_property(QGC_ANDROID_DEPLOY_FILE ${CMAKE_PROJECT_NAME} QT_ANDROID_DEPLOYMENT_SETTINGS_FILE)
    # cmake_print_variables(QGC_ANDROID_DEPLOY_FILE)
elseif(LINUX)
    configure_file(
        ${CMAKE_SOURCE_DIR}/deploy/linux/org.mavlink.qgroundcontrol.desktop.in
        ${CMAKE_BINARY_DIR}/org.mavlink.qgroundcontrol.desktop
        @ONLY
    )
    install(
        FILES ${CMAKE_BINARY_DIR}/org.mavlink.qgroundcontrol.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    install(
        FILES ${QGC_APPIMAGE_ICON_PATH}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps/
        RENAME org.mavlink.qgroundcontrol.png
    )
    configure_file(
        ${CMAKE_SOURCE_DIR}/deploy/linux/org.mavlink.qgroundcontrol.metainfo.xml.in
        ${CMAKE_BINARY_DIR}/metainfo/org.mavlink.qgroundcontrol.metainfo.xml
        @ONLY
    )
    install(
        FILES ${CMAKE_BINARY_DIR}/metainfo/org.mavlink.qgroundcontrol.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo/
    )
    install(
        FILES ${CMAKE_SOURCE_DIR}/deploy/linux/AppRun
        DESTINATION ${CMAKE_BINARY_DIR}
    )
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/CreateAppImage.cmake")
elseif(WIN32)
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/CreateWinInstaller.cmake")
elseif(MACOS)
    install(CODE "set(TARGET_APP_NAME ${QGC_APP_NAME})")
    install(CODE "set(MACDEPLOYQT ${Qt6_DIR}/../../../bin/macdeployqt)")
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/CreateMacDMG.cmake")
endif()
