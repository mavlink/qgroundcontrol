include(InstallRequiredSystemLibraries)

install(
    TARGETS ${CMAKE_PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    BUNDLE  DESTINATION .
)

set(_deploy_opts)
if(MACOS OR WIN32)
    set(_deploy_opts DEPLOY_TOOL_OPTIONS "-qmldir=${CMAKE_SOURCE_DIR}")
    if(MACOS AND DEFINED ENV{QGC_MACOS_SIGNING_IDENTITY})
        message(STATUS "Signing MacOS Bundle")
        set(_deploy_opts "${_deploy_opts} -sign-for-notarization=$ENV{QGC_MACOS_SIGNING_IDENTITY}")
    endif()
endif()

qt_generate_deploy_qml_app_script(
    TARGET ${CMAKE_PROJECT_NAME}
    OUTPUT_SCRIPT _deploy_script
    ${_deploy_opts}
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${_deploy_script})

if(ANDROID)
    # get_target_property(QGC_ANDROID_DEPLOY_FILE ${CMAKE_PROJECT_NAME} QT_ANDROID_DEPLOYMENT_SETTINGS_FILE)
    # cmake_print_variables(QGC_ANDROID_DEPLOY_FILE)
elseif(LINUX)
    configure_file(
        "${QGC_APPIMAGE_DESKTOP_ENTRY_PATH}"
        "${CMAKE_BINARY_DIR}/staging/$<CONFIG>/${QGC_PACKAGE_NAME}.desktop"
        @ONLY
    )
    install(
        FILES "${CMAKE_BINARY_DIR}/staging/$<CONFIG>/${QGC_PACKAGE_NAME}.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications/"
    )
    install(
        FILES "${QGC_APPIMAGE_ICON_256_PATH}"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps/"
        RENAME ${CMAKE_PROJECT_NAME}.png
    )
    install(
        FILES "${QGC_APPIMAGE_ICON_SCALABLE_PATH}"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps/"
        RENAME ${CMAKE_PROJECT_NAME}.svg
    )
    configure_file(
        "${QGC_APPIMAGE_METADATA_PATH}"
        "${CMAKE_BINARY_DIR}/staging/$<CONFIG>/${QGC_PACKAGE_NAME}.appdata.xml"
        @ONLY
    )
    install(
        FILES "${CMAKE_BINARY_DIR}/staging/$<CONFIG>/${QGC_PACKAGE_NAME}.appdata.xml"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo/"
    )
    install(
        PROGRAMS "${QGC_APPIMAGE_APPRUN_PATH}"
        DESTINATION "${CMAKE_BINARY_DIR}/staging/$<CONFIG>/"
    )
    install(CODE "
        set(CMAKE_PROJECT_NAME ${CMAKE_PROJECT_NAME})
        set(CMAKE_PROJECT_VERSION ${CMAKE_PROJECT_VERSION})
        set(QGC_PACKAGE_NAME ${QGC_PACKAGE_NAME})
        set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})
        "
    )
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/CreateAppImage.cmake")
elseif(WIN32)
    install(CODE "
        set(CMAKE_PROJECT_NAME ${CMAKE_PROJECT_NAME})
        set(QGC_ORG_NAME ${QGC_ORG_NAME})
        set(QGC_WINDOWS_ICON_PATH ${QGC_WINDOWS_ICON_PATH})
        set(QGC_WINDOWS_INSTALL_HEADER_PATH ${QGC_WINDOWS_INSTALL_HEADER_PATH})
        set(QGC_WINDOWS_DRIVER_MSI ${CMAKE_SOURCE_DIR}/deploy/windows/driver.msi)
        set(QGC_WINDOWS_OUT ${CMAKE_BINARY_DIR}/staging/$<CONFIG>/${CMAKE_PROJECT_NAME}-installer.exe)
        set(QGC_WINDOWS_INSTALLER_SCRIPT ${CMAKE_SOURCE_DIR}/deploy/windows/nullsoft_installer.nsi)
        "
    )
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/CreateWinInstaller.cmake")
elseif(MACOS)
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"$<TARGET_BUNDLE_DIR:${CMAKE_PROJECT_NAME}>\" \"\" \"${CMAKE_BINARY_DIR}\")
        "
    )
    install(CODE "
        set(QGC_BUNDLE_PATH $<TARGET_BUNDLE_DIR:${CMAKE_PROJECT_NAME}>)
        set(TARGET_APP_NAME ${CMAKE_PROJECT_NAME})
        "
    )
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/CreateMacDMG.cmake")
endif()
