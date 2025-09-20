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
    if(MACOS)
        list(APPEND deploy_tool_options_arg "-appstore-compliant")
    endif()
endif()

# Set extra deploy QML app script options for Qt 6.7.0 and above
set(EXTRA_DEPLOY_QML_APP_SCRIPT_OPTIONS)
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.7.0)
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
        RENAME ${CMAKE_PROJECT_NAME}.png
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
    install(CODE "set(CMAKE_PROJECT_NAME ${CMAKE_PROJECT_NAME})")
    install(CODE "set(QGC_ORG_NAME ${QGC_ORG_NAME})")
    install(CODE "set(QGC_WINDOWS_ICON_PATH ${QGC_WINDOWS_ICON_PATH})")
    install(CODE "set(QGC_WINDOWS_INSTALL_HEADER_PATH ${QGC_WINDOWS_INSTALL_HEADER_PATH})")
    install(CODE "set(QGC_WINDOWS_DRIVER_MSI ${CMAKE_SOURCE_DIR}/deploy/windows/driver.msi)")
    install(CODE "set(QGC_WINDOWS_OUT ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-installer.exe)")
    install(CODE "set(QGC_WINDOWS_INSTALLER_SCRIPT ${CMAKE_SOURCE_DIR}/deploy/windows/nullsoft_installer.nsi)")
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/CreateWinInstaller.cmake")
elseif(MACOS)
    install(CODE "set(QGC_STAGING_BUNDLE_PATH \"${CMAKE_BINARY_DIR}/staging/${CMAKE_PROJECT_NAME}.app\")")
    if(QGC_MACOS_SIGN_WITH_IDENTITY)
        message(STATUS "QGC: Signing Bundle using signing identity")
        install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/SignMacBundle.cmake")
    else()
        message(STATUS "QGC: Signing Bundle using Ad-Hoc signing")
        install(CODE "
            message(STATUS \"QGC: Signing Bundle using Ad-Hoc signing\")
            execute_process(
                COMMAND codesign --force --deep -s - \"\${QGC_STAGING_BUNDLE_PATH}\"
                COMMAND_ERROR_IS_FATAL ANY
            )
        ")
    endif()

    install(CODE "set(TARGET_APP_NAME ${QGC_APP_NAME})")
    find_program(CREATE_DMG_PROGRAM create-dmg)
    if(NOT CREATE_DMG_PROGRAM)
        message(FATAL_ERROR "create-dmg not found. Please install it using `sh qgroundcontrol/tools/setup/install-dependencies-osx.sh`")
    endif()
    install(CODE "set(CREATE_DMG_PROGRAM \"${CREATE_DMG_PROGRAM}\")")
    install(CODE "set(MACDEPLOYQT ${Qt6_DIR}/../../../bin/macdeployqt)")
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/CreateMacDMG.cmake")
endif()
