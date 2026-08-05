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
set(deploy_include_plugins "")

if(MACOS OR WIN32)
    list(APPEND deploy_tool_options_arg "-qmldir=${CMAKE_SOURCE_DIR}")
    if(MACOS)
        list(APPEND deploy_tool_options_arg "-appstore-compliant")
    endif()
    if(WIN32)
        # windeployqt and qt6_deploy_qml_imports both deploy QML plugins into qml/
        # and relock qtquick2plugin.dll; -no-quick-import lets CMake own QML deploy.
        list(APPEND deploy_tool_options_arg "-no-quick-import")
    endif()
endif()

if(NOT ANDROID AND NOT IOS)
    set(deploy_include_plugins INCLUDE_PLUGINS qoffscreen)
    if(LINUX)
        # Wayland platform plugin (libqwayland.so)
        list(APPEND deploy_include_plugins qwayland)
    endif()
endif()

# First-pass filter for unused Qt plugins whose missing backing libs would trip
# CPackDeb's shlibdeps; the post-deploy strip below is the real guarantee.
set(deploy_exclude_plugins "")
if(LINUX)
    set(deploy_exclude_plugins EXCLUDE_PLUGINS
        qsqlmysql qsqlpsql qsqlodbc qsqloci qsqlibase qsqlmimer qtiff)
endif()

qt_generate_deploy_qml_app_script(
    TARGET ${CMAKE_PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
    DEPLOY_TOOL_OPTIONS ${deploy_tool_options_arg}
    ${deploy_include_plugins}
    ${deploy_exclude_plugins}
)

install(SCRIPT ${deploy_script})
message(STATUS "QGC: Qt deployment script: ${deploy_script}")

# Strip unused SQL-driver/TIFF plugins whose missing backing libs would abort
# CPackDeb's dpkg-shlibdeps; Unspecified component so it runs for install + CPack.
if(LINUX)
    install(CODE [[
        set(_qgc_prefix "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}")
        file(GLOB _qgc_strip_plugins
            "${_qgc_prefix}/plugins/sqldrivers/libqsqlmysql.*"
            "${_qgc_prefix}/plugins/sqldrivers/libqsqlpsql.*"
            "${_qgc_prefix}/plugins/sqldrivers/libqsqlodbc.*"
            "${_qgc_prefix}/plugins/sqldrivers/libqsqloci.*"
            "${_qgc_prefix}/plugins/sqldrivers/libqsqlibase.*"
            "${_qgc_prefix}/plugins/sqldrivers/libqsqlmimer.*"
            "${_qgc_prefix}/plugins/imageformats/libqtiff.*")
        if(_qgc_strip_plugins)
            message(STATUS "QGC: stripping unused Qt plugins from package: ${_qgc_strip_plugins}")
            file(REMOVE ${_qgc_strip_plugins})
        endif()
    ]])
endif()

# GStreamer framework rpath fixup: component dylibs (libgstrtsp, libgstvideo, etc.)
# are inside the framework's lib/ directory, but the binary's @rpath resolves to
# Contents/Frameworks/ (flat). Add the framework lib path so dyld finds them.
# Runs after Qt deploy to survive any rpath rewriting by macdeployqt.
set(_qgc_gst_framework_bundle "")
if(MACOS AND TARGET GStreamer::Layout)
    get_target_property(_qgc_gst_framework_bundle GStreamer::Layout GSTREAMER_FRAMEWORK_BUNDLE)
    if(_qgc_gst_framework_bundle STREQUAL "_qgc_gst_framework_bundle-NOTFOUND")
        set(_qgc_gst_framework_bundle "")
    endif()
endif()
if(MACOS AND _qgc_gst_framework_bundle)
    install(CODE "
        set(_binary \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_PROJECT_NAME}.app/Contents/MacOS/${CMAKE_PROJECT_NAME}\")
        if(EXISTS \"\${_binary}\")
            execute_process(
                COMMAND install_name_tool -add_rpath
                    @executable_path/../Frameworks/GStreamer.framework/Versions/1.0/lib
                    \"\${_binary}\"
                ERROR_QUIET
            )
        endif()
    ")
endif()

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
    configure_file(
        "${QGC_APPIMAGE_APPRUN_PATH}"
        "${CMAKE_BINARY_DIR}/AppRun"
        COPYONLY
    )

    # Non-fatal lint of the generated .desktop/appstream metadata: runs only when
    # the linters are present, so hosts without them aren't gated.
    find_program(QGC_DESKTOP_FILE_VALIDATE desktop-file-validate)
    if(QGC_DESKTOP_FILE_VALIDATE)
        execute_process(
            COMMAND "${QGC_DESKTOP_FILE_VALIDATE}" "${CMAKE_BINARY_DIR}/${QGC_PACKAGE_NAME}.desktop"
            RESULT_VARIABLE _qgc_desktop_lint
        )
        if(NOT _qgc_desktop_lint EQUAL 0)
            message(WARNING "QGC: desktop-file-validate reported issues for ${QGC_PACKAGE_NAME}.desktop")
        endif()
    endif()
    find_program(QGC_APPSTREAMCLI appstreamcli)
    if(QGC_APPSTREAMCLI)
        execute_process(
            COMMAND "${QGC_APPSTREAMCLI}" validate --no-net "${CMAKE_BINARY_DIR}/${QGC_PACKAGE_NAME}.appdata.xml"
            RESULT_VARIABLE _qgc_appstream_lint
        )
        if(NOT _qgc_appstream_lint EQUAL 0)
            message(WARNING "QGC: appstreamcli validate reported issues for ${QGC_PACKAGE_NAME}.appdata.xml")
        endif()
    endif()

    # Dedicated "appimage" component so `cmake --install` builds the AppImage but
    # CPack can exclude it (CPACK_COMPONENTS_ALL) and skip it during .deb/.rpm staging.
    install(CODE "
        set(CMAKE_PROJECT_NAME \"${CMAKE_PROJECT_NAME}\")
        set(CMAKE_PROJECT_VERSION \"${CMAKE_PROJECT_VERSION}\")
        set(QGC_PACKAGE_NAME \"${QGC_PACKAGE_NAME}\")
        set(QGC_BUILD_DIR \"${CMAKE_BINARY_DIR}\")
        set(CMAKE_SYSTEM_PROCESSOR \"${CMAKE_SYSTEM_PROCESSOR}\")
        set(QGC_RUN_APPIMAGELINT \"${QGC_RUN_APPIMAGELINT}\")
        set(QGC_LINUX_DISTRO_FAMILY \"${QGC_LINUX_DISTRO_FAMILY}\")
    " COMPONENT appimage)
    if(QGC_CREATE_APPIMAGE AND NOT CMAKE_CROSSCOMPILING)
        install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/install/CreateAppImage.cmake" COMPONENT appimage)
    endif()

    # Native package (.deb/.rpm via CPack, Arch via makepkg) is built by the
    # shared qgc-package dispatch at the end of this file.

# ----------------------------------------------------------------------------
# Windows Installation & Installer Creation
# ----------------------------------------------------------------------------
elseif(WIN32)
    # Pass variables to Windows installer creation script
    if(CMAKE_CROSSCOMPILING)
        set(_win_installer_out "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-installer-${CMAKE_HOST_SYSTEM_PROCESSOR}-${CMAKE_SYSTEM_PROCESSOR}.exe")
    else()
        set(_win_installer_out "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-installer-${CMAKE_SYSTEM_PROCESSOR}.exe")
    endif()
    install(CODE "
        set(CMAKE_PROJECT_NAME \"${CMAKE_PROJECT_NAME}\")
        set(CMAKE_PROJECT_VERSION \"${CMAKE_PROJECT_VERSION}\")
        set(QGC_ORG_NAME \"${QGC_ORG_NAME}\")
        set(QGC_WINDOWS_ICON_PATH \"${QGC_WINDOWS_ICON_PATH}\")
        set(QGC_WINDOWS_INSTALL_HEADER_PATH \"${QGC_WINDOWS_INSTALL_HEADER_PATH}\")
        set(QGC_WINDOWS_OUT \"${_win_installer_out}\")
        set(QGC_WINDOWS_INSTALLER_SCRIPT \"${CMAKE_SOURCE_DIR}/deploy/windows/nullsoft_installer.nsi\")
    ")
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/install/CreateWinInstaller.cmake")

# ----------------------------------------------------------------------------
# macOS Installation, Code Signing & DMG Creation
# ----------------------------------------------------------------------------
elseif(MACOS)
    # macdeployqt ignores INCLUDE_PLUGINS — manually deploy offscreen plugin
    if(TARGET Qt6::QOffscreenIntegrationPlugin)
        install(FILES "$<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>"
            DESTINATION "${CMAKE_PROJECT_NAME}.app/Contents/PlugIns/platforms"
        )
    endif()

    install(CODE "set(QGC_STAGING_BUNDLE_PATH \"${CMAKE_BINARY_DIR}/staging/${CMAKE_PROJECT_NAME}.app\")")

    # Code signing
    option(QGC_MACOS_SIGN_WITH_IDENTITY "Sign macOS bundle with developer identity (requires signing env vars)" OFF)
    if(QGC_MACOS_SIGN_WITH_IDENTITY)
        message(STATUS "QGC: macOS bundle will be signed with developer identity")
        install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/install/SignMacBundle.cmake")
    else()
        message(STATUS "QGC: macOS bundle will be signed with ad-hoc signature")
        install(CODE "
            message(STATUS \"QGC: Signing macOS bundle (ad-hoc)\")
            execute_process(
                COMMAND codesign --deep --force -s - \"\${QGC_STAGING_BUNDLE_PATH}\"
                COMMAND_ERROR_IS_FATAL ANY
            )
            execute_process(
                COMMAND codesign --verify --deep --verbose=2 \"\${QGC_STAGING_BUNDLE_PATH}\"
                COMMAND_ERROR_IS_FATAL ANY
            )
        ")
    endif()

    find_program(CREATE_DMG_PROGRAM create-dmg)
    if(NOT CREATE_DMG_PROGRAM)
        message(STATUS "QGC: Fetching create-dmg tool via CPM")
        CPMAddPackage(
            NAME create-dmg
            GITHUB_REPOSITORY create-dmg/create-dmg
            GIT_TAG v1.2.3
            DOWNLOAD_ONLY YES
        )
        set(CREATE_DMG_PROGRAM "${create-dmg_SOURCE_DIR}/create-dmg")
    endif()

    install(CODE "set(CREATE_DMG_PROGRAM \"${CREATE_DMG_PROGRAM}\")")
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/install/CreateMacDMG.cmake")
endif()

# ============================================================================
# Optional CPack native package — `qgc-package` target, opt-in via
# QGC_CPACK_GENERATOR. Runs alongside the platform's default installer
# (AppImage / NSIS .exe / DMG), which `cmake --install` still produces.
# Must follow all install() rules above: each CreateCPack*.cmake ends in
# include(CPack), which snapshots the install layout.
# ============================================================================
set(_qgc_cpack_module "")
if(QGC_CPACK_GENERATOR STREQUAL "DEB")
    set(_qgc_cpack_module "CPack/CreateCPackDeb.cmake")
elseif(QGC_CPACK_GENERATOR STREQUAL "RPM")
    set(_qgc_cpack_module "CPack/CreateCPackRPM.cmake")
elseif(QGC_CPACK_GENERATOR STREQUAL "NSIS")
    set(_qgc_cpack_module "CPack/CreateCPackNSIS.cmake")
elseif(QGC_CPACK_GENERATOR STREQUAL "IFW")
    set(_qgc_cpack_module "CPack/CreateCPackIFW.cmake")
elseif(QGC_CPACK_GENERATOR STREQUAL "DragNDrop")
    set(_qgc_cpack_module "CPack/CreateCPackDMG.cmake")
elseif(QGC_CPACK_GENERATOR STREQUAL "Bundle")
    set(_qgc_cpack_module "CPack/CreateCPackBundle.cmake")
elseif(QGC_CPACK_GENERATOR STREQUAL "productbuild")
    set(_qgc_cpack_module "CPack/CreateCPackProductBuild.cmake")
elseif(QGC_CPACK_GENERATOR STREQUAL "TXZ")
    set(_qgc_cpack_module "CPack/CreateCPackArchive.cmake")
endif()

# Warn when the host can't actually build the selected Linux native package.
if(QGC_CPACK_GENERATOR STREQUAL "DEB" AND DEFINED QGC_LINUX_DISTRO_FAMILY
        AND NOT QGC_LINUX_DISTRO_FAMILY STREQUAL "" AND NOT QGC_LINUX_DISTRO_FAMILY STREQUAL "debian")
    message(WARNING "QGC: QGC_CPACK_GENERATOR=DEB on a non-Debian host (${QGC_LINUX_DISTRO}); "
        "cpack needs dpkg-dev/dpkg-shlibdeps to build the .deb.")
elseif(QGC_CPACK_GENERATOR STREQUAL "RPM" AND DEFINED QGC_LINUX_DISTRO_FAMILY
        AND NOT QGC_LINUX_DISTRO_FAMILY STREQUAL "" AND NOT QGC_LINUX_DISTRO_FAMILY STREQUAL "rhel")
    message(WARNING "QGC: QGC_CPACK_GENERATOR=RPM on a non-RPM host (${QGC_LINUX_DISTRO}); "
        "cpack needs rpmbuild to build the .rpm.")
endif()

if(_qgc_cpack_module)
    include("${CMAKE_SOURCE_DIR}/cmake/install/${_qgc_cpack_module}")
    add_custom_target(qgc-package
        COMMAND "${CMAKE_CPACK_COMMAND}" -G "${QGC_CPACK_GENERATOR}"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        VERBATIM
        COMMENT "QGC: building ${QGC_CPACK_GENERATOR} package via CPack"
    )
elseif(LINUX AND QGC_LINUX_DISTRO_FAMILY STREQUAL "arch")
    include("${CMAKE_SOURCE_DIR}/cmake/install/CreateArchPackage.cmake")
elseif(LINUX OR WIN32 OR MACOS)
    add_custom_target(qgc-package
        COMMAND "${CMAKE_COMMAND}" -E echo
            "QGC: QGC_CPACK_GENERATOR unset; the default installer is produced by 'cmake --install'."
        VERBATIM
        COMMENT "QGC: no CPack generator selected"
    )
endif()
