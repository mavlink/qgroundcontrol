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
    list(APPEND deploy_tool_options_arg "-qmldir=${CMAKE_SOURCE_DIR}")
    if(MACOS)
        # if(DEFINED ENV{QGC_MACOS_SIGNING_IDENTITY})
        #     message(STATUS "QGC: Deploy Sign For Notarization")
        #     list(APPEND deploy_tool_options_arg "-sign-for-notarization=\"$ENV{QGC_MACOS_SIGNING_IDENTITY}\"")
        # else()
        #     message(STATUS "QGC: Deploy AD-HOC Codesign")
        #     list(APPEND deploy_tool_options_arg "-codesign=-")
        # endif()
        # list(APPEND deploy_tool_options_arg "-dmg" "-fs=APFS")
    endif()
endif()

qt_generate_deploy_qml_app_script(
    TARGET ${CMAKE_PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
    DEPLOY_TOOL_OPTIONS ${deploy_tool_options_arg}
)
install(SCRIPT ${deploy_script})

if(ANDROID)
    # get_target_property(QGC_ANDROID_DEPLOY_FILE ${CMAKE_PROJECT_NAME} QT_ANDROID_DEPLOYMENT_SETTINGS_FILE)
    # cmake_print_variables(QGC_ANDROID_DEPLOY_FILE)
elseif(LINUX)
    configure_file(
        "${QGC_APPIMAGE_DESKTOP_ENTRY_PATH}"
        "${CMAKE_BINARY_DIR}/${QGC_PACKAGE_NAME}.desktop"
        @ONLY
    )
    install(
        FILES "${CMAKE_BINARY_DIR}/${QGC_PACKAGE_NAME}.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
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
        "${CMAKE_BINARY_DIR}/${QGC_PACKAGE_NAME}.appdata.xml"
        @ONLY
    )
    install(
        PROGRAMS "${CMAKE_BINARY_DIR}/${QGC_PACKAGE_NAME}.appdata.xml"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo/"
    )
    install(
        FILES "${QGC_APPIMAGE_APPRUN_PATH}"
        DESTINATION "${CMAKE_BINARY_DIR}/"
    )
    install(CODE "
        set(CMAKE_PROJECT_NAME ${CMAKE_PROJECT_NAME})
        set(CMAKE_PROJECT_VERSION ${CMAKE_PROJECT_VERSION})
        set(QGC_PACKAGE_NAME ${QGC_PACKAGE_NAME})
        set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})
    ")
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/install/CreateAppImage.cmake")
elseif(WIN32)
    install(CODE "
        set(CMAKE_PROJECT_NAME ${CMAKE_PROJECT_NAME})
        set(QGC_ORG_NAME ${QGC_ORG_NAME})
        set(QGC_WINDOWS_ICON_PATH \"${QGC_WINDOWS_ICON_PATH}\")
        set(QGC_WINDOWS_INSTALL_HEADER_PATH \"${QGC_WINDOWS_INSTALL_HEADER_PATH}\")
        set(QGC_WINDOWS_DRIVER_MSI \"${CMAKE_SOURCE_DIR}/deploy/windows/driver.msi\")
        if(CMAKE_CROSSCOMPILING)
            set(QGC_WINDOWS_OUT \"${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-installer-${CMAKE_HOST_SYSTEM_PROCESSOR}-${CMAKE_SYSTEM_PROCESSOR}.exe\")
        else()
            set(QGC_WINDOWS_OUT \"${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-installer-${CMAKE_SYSTEM_PROCESSOR}.exe\")
        endif()
        set(QGC_WINDOWS_INSTALLER_SCRIPT \"${CMAKE_SOURCE_DIR}/deploy/windows/nullsoft_installer.nsi\")
    ")
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/install/CreateWinInstaller.cmake")
elseif(MACOS)
    install(CODE "
        set(QGC_STAGING_BUNDLE_PATH \"${CMAKE_BINARY_DIR}/staging/${CMAKE_PROJECT_NAME}.app\")

        message(STATUS \"QGC: Signing Bundle - \${QGC_STAGING_BUNDLE_PATH}\")
        execute_process(
            COMMAND codesign --force --deep -s - \"\${QGC_STAGING_BUNDLE_PATH}\"
            COMMAND_ERROR_IS_FATAL ANY
        )
    ")

    # include(BundleUtilities)
    # fixup_bundle(\"${QGC_STAGING_BUNDLE_PATH}\" \"\" \"${CMAKE_BINARY_DIR}\")
    # verify_app(\"${QGC_STAGING_BUNDLE_PATH}\")
    # verify_bundle_prerequisites(\"${QGC_STAGING_BUNDLE_PATH}\" _bundle_prereqs_result _bundle_prereqs_info)
    # verify_bundle_symlinks(\"${QGC_STAGING_BUNDLE_PATH}\" _bundle_symlinks_result _bundle_symlinks_info)

    # include(CreateCPackDMG)

    # - or

    install(CODE "
        include(${CMAKE_SOURCE_DIR}/cmake/modules/CPM.cmake)
    ")
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/install/CreateMacDMG.cmake")
endif()
