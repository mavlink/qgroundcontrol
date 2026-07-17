# ============================================================================
# CreateCPackNSIS.cmake
# Windows NSIS installer package generator
# ============================================================================

include(CreateCPackCommon)

# ----------------------------------------------------------------------------
# NSIS Generator Configuration
# ----------------------------------------------------------------------------
set(CPACK_GENERATOR "NSIS")
set(CPACK_BINARY_NSIS ON)

if(CMAKE_CROSSCOMPILING)
    set(_qgc_nsis_arch "${CMAKE_HOST_SYSTEM_PROCESSOR}-${CMAKE_SYSTEM_PROCESSOR}")
else()
    set(_qgc_nsis_arch "${CMAKE_SYSTEM_PROCESSOR}")
endif()
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-installer-${_qgc_nsis_arch}")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CMAKE_PROJECT_NAME}")
# Custom shortcut commands below create both launch modes; suppress CPack's
# default executable shortcut so the normal shortcut is not duplicated.
set(CPACK_PACKAGE_EXECUTABLES "")

# ----------------------------------------------------------------------------
# Installer Appearance
# ----------------------------------------------------------------------------
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
set(CPACK_NSIS_MUI_ICON "${QGC_WINDOWS_ICON_PATH}")
set(CPACK_NSIS_MUI_UNIICON "${QGC_WINDOWS_ICON_PATH}")

# ----------------------------------------------------------------------------
# Install/Uninstall Commands
# ----------------------------------------------------------------------------
set(_qgc_nsis_extra_preinstall
    [=[
    DetailPrint "Checking for previous @CMAKE_PROJECT_NAME@ installation"
    SetRegView 64
    ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CMAKE_PROJECT_NAME@" "UninstallString"
    ReadRegStr $R3 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CMAKE_PROJECT_NAME@" "InstallLocation"
    StrCmp $R0 "" qgc_install_continue qgc_check_uninstaller

qgc_check_uninstaller:
    StrCpy $R1 $R0 1
    StrCmp $R1 "$\"" 0 qgc_uninstaller_path_unquoted
    StrCpy $R1 $R0 "" 1
    StrLen $R2 $R1
    IntOp $R2 $R2 - 1
    StrCpy $R1 $R1 $R2
    Goto qgc_check_uninstaller_path

qgc_uninstaller_path_unquoted:
    StrCpy $R1 $R0

qgc_check_uninstaller_path:
    IfFileExists "$R1" qgc_uninstall_previous qgc_cleanup_orphaned_registry

qgc_cleanup_orphaned_registry:
    DetailPrint "Previous uninstaller not found; cleaning orphaned registry entries"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CMAKE_PROJECT_NAME@"
    DeleteRegKey HKLM "Software\Microsoft\Windows\Windows Error Reporting\LocalDumps\@CMAKE_PROJECT_NAME@.exe"
    DeleteRegKey /ifempty HKLM "Software\@QGC_ORG_NAME@\@CMAKE_PROJECT_NAME@"
    Goto qgc_install_continue

qgc_uninstall_previous:
    DetailPrint "Uninstalling previous @CMAKE_PROJECT_NAME@ version"
    StrCmp $R3 "" qgc_use_uninstaller_parent
    IfFileExists "$R3\*.*" qgc_have_previous_install_dir qgc_use_uninstaller_parent

qgc_use_uninstaller_parent:
    GetFullPathName $R3 "$R1\.."

qgc_have_previous_install_dir:
    ClearErrors
    ExecWait '$R0 /S -LEAVE_DATA=1 _?=$R3' $R2
    IfErrors qgc_uninstall_failed
    IntCmp $R2 0 qgc_uninstall_succeeded qgc_uninstall_failed qgc_uninstall_failed

qgc_uninstall_succeeded:
    Delete "$R1"
    RMDir "$R3"
    Goto qgc_install_continue

qgc_uninstall_failed:
    MessageBox MB_OK|MB_ICONSTOP \
        "Could not remove the previously installed @CMAKE_PROJECT_NAME@ version." \
        /SD IDOK
    Abort

qgc_install_continue:
]=]
)
string(CONFIGURE "${_qgc_nsis_extra_preinstall}" CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS @ONLY)

set(_qgc_nsis_extra_install
    [=[
    SetRegView 64
    WriteRegStr HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CMAKE_PROJECT_NAME@" \
        "InstallLocation" "$INSTDIR"
    WriteRegDWORD HKLM \
        "Software\Microsoft\Windows\Windows Error Reporting\LocalDumps\@CMAKE_PROJECT_NAME@.exe" \
        "DumpCount" 5
    WriteRegDWORD HKLM \
        "Software\Microsoft\Windows\Windows Error Reporting\LocalDumps\@CMAKE_PROJECT_NAME@.exe" \
        "DumpType" 1
    WriteRegExpandStr HKLM \
        "Software\Microsoft\Windows\Windows Error Reporting\LocalDumps\@CMAKE_PROJECT_NAME@.exe" \
        "DumpFolder" "%LOCALAPPDATA%\QGCCrashDumps"
    Delete "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk"
]=]
)
string(CONFIGURE "${_qgc_nsis_extra_install}" CPACK_NSIS_EXTRA_INSTALL_COMMANDS @ONLY)

set(_qgc_nsis_extra_uninstall
    [=[
    !include "FileFunc.nsh"
    SetRegView 64
    ; The CPack template reads these before this block runs, in the 32-bit
    ; view; re-read them under the 64-bit view the installer wrote them in.
    ReadRegStr $START_MENU SHCTX \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CMAKE_PROJECT_NAME@" "StartMenu"
    ReadRegStr $DO_NOT_ADD_TO_PATH SHCTX \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CMAKE_PROJECT_NAME@" "DoNotAddToPath"
    ReadRegStr $ADD_TO_PATH_ALL_USERS SHCTX \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CMAKE_PROJECT_NAME@" "AddToPathAllUsers"
    ReadRegStr $ADD_TO_PATH_CURRENT_USER SHCTX \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CMAKE_PROJECT_NAME@" "AddToPathCurrentUser"
    ReadRegStr $INSTALL_DESKTOP SHCTX \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CMAKE_PROJECT_NAME@" "InstallToDesktop"
    ${GetParameters} $R0
    ${GetOptions} $R0 "-LEAVE_DATA=" $R1
    StrCmp $R1 "1" qgc_keep_app_data
    SetShellVarContext current
    RMDir /r /REBOOTOK "$APPDATA\@QGC_ORG_NAME@"
    SetShellVarContext all

qgc_keep_app_data:
    DeleteRegKey HKLM \
        "Software\Microsoft\Windows\Windows Error Reporting\LocalDumps\@CMAKE_PROJECT_NAME@.exe"
]=]
)
string(CONFIGURE "${_qgc_nsis_extra_uninstall}" CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS @ONLY)

# Keeps the secondary GPU Safe Mode shortcut out of Start "pin on install" and
# the "recently added" list (parity with the retired nullsoft_installer.nsi).
set(_qgc_nsis_create_icons
    [=[
    !include "LogicLib.nsh"
    !include "Win\COM.nsh"
    !include "Win\Propkey.nsh"

    !macro QGCDemoteShortCut target
        !insertmacro ComHlpr_CreateInProcInstance ${CLSID_ShellLink} ${IID_IShellLink} r0 ""
        ${If} $0 <> 0
            ${IUnknown::QueryInterface} $0 '("${IID_IPersistFile}",.r1)'
            ${If} $1 P<> 0
                ${IPersistFile::Load} $1 '("${target}",1)'
                ${IUnknown::Release} $1 ""
            ${EndIf}
            ${IUnknown::QueryInterface} $0 '("${IID_IPropertyStore}",.r1)'
            ${If} $1 P<> 0
                System::Call '*${SYSSTRUCT_PROPERTYKEY}(${PKEY_AppUserModel_StartPinOption})p.r2'
                System::Call '*${SYSSTRUCT_PROPVARIANT}(${VT_UI4},,&i4 ${APPUSERMODEL_STARTPINOPTION_NOPINONINSTALL})p.r3'
                ${IPropertyStore::SetValue} $1 '($2,$3)'

                System::Call '*$2${SYSSTRUCT_PROPERTYKEY}(${PKEY_AppUserModel_ExcludeFromShowInNewInstall})'
                System::Call '*$3${SYSSTRUCT_PROPVARIANT}(${VT_BOOL},,&i2 ${VARIANT_TRUE})'
                ${IPropertyStore::SetValue} $1 '($2,$3)'

                System::Free $2
                System::Free $3
                ${IPropertyStore::Commit} $1 ""
                ${IUnknown::Release} $1 ""
            ${EndIf}
            ${IUnknown::QueryInterface} $0 '("${IID_IPersistFile}",.r1)'
            ${If} $1 P<> 0
                ${IPersistFile::Save} $1 '("${target}",1)'
                ${IUnknown::Release} $1 ""
            ${EndIf}
            ${IUnknown::Release} $0 ""
        ${EndIf}
    !macroend

    CreateShortCut \
        "$SMPROGRAMS\$STARTMENU_FOLDER\@CMAKE_PROJECT_NAME@.lnk" \
        "$INSTDIR\bin\@CMAKE_PROJECT_NAME@.exe" "" \
        "$INSTDIR\bin\@CMAKE_PROJECT_NAME@.exe" 0
    CreateShortCut \
        "$SMPROGRAMS\$STARTMENU_FOLDER\@CMAKE_PROJECT_NAME@ (GPU Safe Mode).lnk" \
        "$INSTDIR\bin\@CMAKE_PROJECT_NAME@.exe" "-swrast" \
        "$INSTDIR\bin\@CMAKE_PROJECT_NAME@.exe" 0
    !insertmacro QGCDemoteShortCut \
        "$SMPROGRAMS\$STARTMENU_FOLDER\@CMAKE_PROJECT_NAME@ (GPU Safe Mode).lnk"
]=]
)
string(CONFIGURE "${_qgc_nsis_create_icons}" CPACK_NSIS_CREATE_ICONS_EXTRA @ONLY)

set(_qgc_nsis_delete_icons
    [=[
    Delete "$SMPROGRAMS\$MUI_TEMP\@CMAKE_PROJECT_NAME@.lnk"
    Delete "$SMPROGRAMS\$MUI_TEMP\@CMAKE_PROJECT_NAME@ (GPU Safe Mode).lnk"
]=]
)
string(CONFIGURE "${_qgc_nsis_delete_icons}" CPACK_NSIS_DELETE_ICONS_EXTRA @ONLY)

# ----------------------------------------------------------------------------
# Installer Options
# ----------------------------------------------------------------------------
set(CPACK_NSIS_COMPRESSOR "/SOLID /FINAL lzma")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL OFF)
set(CPACK_NSIS_MODIFY_PATH OFF)
set(CPACK_NSIS_DISPLAY_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_NSIS_PACKAGE_NAME "${CMAKE_PROJECT_NAME} ${CMAKE_SYSTEM_PROCESSOR} ${CMAKE_PROJECT_VERSION}")
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\${CMAKE_PROJECT_NAME}.exe")
set(CPACK_NSIS_HELP_LINK "https://qgroundcontrol.com/#resources")
set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
set(CPACK_NSIS_UNINSTALL_NAME "${CMAKE_PROJECT_NAME}-Uninstall")

# ----------------------------------------------------------------------------
# Installer UI Customization
# ----------------------------------------------------------------------------
set(CPACK_NSIS_MUI_HEADERIMAGE "${QGC_WINDOWS_INSTALL_HEADER_PATH}")
set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
set(CPACK_NSIS_IGNORE_LICENSE_PAGE ON)

include(CPack)
