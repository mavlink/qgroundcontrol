# ============================================================================
# QGroundControl Installation Configuration
# Handles platform-specific installation and packaging
# ============================================================================

include(InstallRequiredSystemLibraries)

# Note: Installer generation could be conditioned on Release builds
# if(QGC_BUILD_INSTALLER AND CMAKE_INSTALL_CONFIG_NAME MATCHES "^[Rr]elease$")

# ----------------------------------------------------------------------------
# Main Target Installation
# ----------------------------------------------------------------------------
install(
    TARGETS ${CMAKE_PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    BUNDLE  DESTINATION .
)

# ----------------------------------------------------------------------------
# Qt Deployment Script Generation
# ----------------------------------------------------------------------------
set(deploy_tool_options_arg "")

if(MACOS OR WIN32)
    list(APPEND deploy_tool_options_arg "-qmldir=${CMAKE_SOURCE_DIR}")
    if(MACOS)
        list(APPEND deploy_tool_options_arg "-appstore-compliant")
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
message(STATUS "QGC: Qt deployment script: ${deploy_script}")

# ============================================================================
# Platform-Specific Installation
# ============================================================================

# ----------------------------------------------------------------------------
# Android Installation
# ----------------------------------------------------------------------------
if(ANDROID)
    # Android deployment handled by Qt
    # get_target_property(QGC_ANDROID_DEPLOY_FILE ${CMAKE_PROJECT_NAME} QT_ANDROID_DEPLOYMENT_SETTINGS_FILE)
    # cmake_print_variables(QGC_ANDROID_DEPLOY_FILE)

# ----------------------------------------------------------------------------
# Linux Installation & AppImage Creation
# ----------------------------------------------------------------------------
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
        FILES "${CMAKE_BINARY_DIR}/${QGC_PACKAGE_NAME}.appdata.xml"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo/"
    )
    install(
        FILES "${QGC_APPIMAGE_APPRUN_PATH}"
        DESTINATION "${CMAKE_BINARY_DIR}/"
    )
    # Pass variables to AppImage creation script
    install(CODE "
        set(CMAKE_PROJECT_NAME ${CMAKE_PROJECT_NAME})
        set(CMAKE_PROJECT_VERSION ${CMAKE_PROJECT_VERSION})
        set(QGC_PACKAGE_NAME ${QGC_PACKAGE_NAME})
        set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})
    ")
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/install/CreateAppImage.cmake")

# ----------------------------------------------------------------------------
# Windows Installation & Installer Creation
# ----------------------------------------------------------------------------
elseif(WIN32)
    # Pass variables to Windows installer creation script
    install(CODE "
        set(CMAKE_PROJECT_NAME ${CMAKE_PROJECT_NAME})
        set(CMAKE_PROJECT_VERSION ${CMAKE_PROJECT_VERSION})
        set(QGC_ORG_NAME ${QGC_ORG_NAME})
        set(QGC_WINDOWS_ICON_PATH \"${QGC_WINDOWS_ICON_PATH}\")
        set(QGC_WINDOWS_INSTALL_HEADER_PATH \"${QGC_WINDOWS_INSTALL_HEADER_PATH}\")
        if(CMAKE_CROSSCOMPILING)
            set(QGC_WINDOWS_OUT \"${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-installer-${CMAKE_HOST_SYSTEM_PROCESSOR}-${CMAKE_SYSTEM_PROCESSOR}.exe\")
        else()
            set(QGC_WINDOWS_OUT \"${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-installer-${CMAKE_SYSTEM_PROCESSOR}.exe\")
        endif()
        set(QGC_WINDOWS_INSTALLER_SCRIPT \"${CMAKE_SOURCE_DIR}/deploy/windows/nullsoft_installer.nsi\")
    ")
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/install/CreateWinInstaller.cmake")

# ----------------------------------------------------------------------------
# macOS Installation, Code Signing & DMG Creation
# ----------------------------------------------------------------------------
elseif(MACOS)
    # Set bundle path for subsequent operations
    install(CODE "set(QGC_STAGING_BUNDLE_PATH \"${CMAKE_BINARY_DIR}/staging/${CMAKE_PROJECT_NAME}.app\")")

    # Code signing
    if(QGC_MACOS_SIGN_WITH_IDENTITY)
        message(STATUS "QGC: macOS bundle will be signed with developer identity")
        install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/install/SignMacBundle.cmake")
    else()
        message(STATUS "QGC: macOS bundle will be signed with ad-hoc signature")
        install(CODE "
            message(STATUS \"QGC: Signing macOS bundle (ad-hoc)\")
            execute_process(
                COMMAND codesign --force --deep -s - \"\${QGC_STAGING_BUNDLE_PATH}\"
                COMMAND_ERROR_IS_FATAL ANY
            )
        ")
    endif()

    # Find or fetch create-dmg tool
    find_program(CREATE_DMG_PROGRAM create-dmg)
    if(NOT CREATE_DMG_PROGRAM)
        message(STATUS "QGC: Fetching create-dmg tool via CPM")
        CPMAddPackage(
            NAME create-dmg
            GITHUB_REPOSITORY create-dmg/create-dmg
            GIT_TAG master
            DOWNLOAD_ONLY
        )
        set(CREATE_DMG_PROGRAM "${create-dmg_SOURCE_DIR}/create-dmg")
    endif()

    install(CODE "set(CREATE_DMG_PROGRAM \"${CREATE_DMG_PROGRAM}\")")
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/install/CreateMacDMG.cmake")
endif()
