!include "MUI2.nsh"
!include LogicLib.nsh
!include Win\COM.nsh
!include Win\Propkey.nsh

!macro DemoteShortCut target
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

                    ; Reuse the PROPERTYKEY & PROPVARIANT buffers to set another property
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

Name "QGroundcontrol"
Var StartMenuFolder

InstallDir $PROGRAMFILES\qgroundcontrol

SetCompressor /SOLID /FINAL lzma

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "installheader.bmp";
!define MUI_ICON "WindowsQGC.ico";
!define MUI_UNICON "WindowsQGC.ico";

!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section
  ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QGroundControl" "UninstallString"
  StrCmp $R0 "" doinstall

  ExecWait "$R0 /S _?=$INSTDIR"
  IntCmp $0 0 doinstall

  MessageBox MB_OK|MB_ICONEXCLAMATION \
        "Could not remove a previously installed QGroundControl version.$\n$\nPlease remove it before continuing."
  Abort

doinstall:
  SetOutPath $INSTDIR
  File /r /x qgroundcontrol.pdb /x qgroundcontrol.lib /x qgroundcontrol.exp build_windows_install\release\*.* 
  File deploy\px4driver.msi
  WriteUninstaller $INSTDIR\QGroundControl_uninstall.exe
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QGroundControl" "DisplayName" "QGroundControl"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QGroundControl" "UninstallString" "$\"$INSTDIR\QGroundControl_uninstall.exe$\""

  ; Only attempt to install the PX4 driver if the version isn't present
  !define ROOTKEY "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\434608CF2B6E31F0DDBA5C511053F957B55F098E"

  SetRegView 64
  ReadRegStr $0 HKLM "${ROOTKEY}" "Publisher"
  StrCmp     $0 "3D Robotics" found_provider notfound

found_provider:
  ReadRegStr $0 HKLM "${ROOTKEY}" "DisplayVersion"
  DetailPrint "Checking USB driver version... $0"
  StrCmp     $0 "04/11/2013 2.0.0.4" skip_driver notfound

notfound:
  DetailPrint "USB Driver not found... installing"
  ExecWait '"msiexec" /i "px4driver.msi"'
  goto done

skip_driver:
  DetailPrint "USB Driver found... skipping install"
done:
  SetRegView lastused
SectionEnd 

Section "Uninstall"
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  SetShellVarContext all
  RMDir /r /REBOOTOK $INSTDIR
  RMDir /r /REBOOTOK "$SMPROGRAMS\$StartMenuFolder\"
  SetShellVarContext current
  RMDir /r /REBOOTOK "$APPDATA\QGROUNDCONTROL.ORG\"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QGroundControl"
SectionEnd

Section "create Start Menu Shortcuts"
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\QGroundControl.lnk" "$INSTDIR\qgroundcontrol.exe" "" "$INSTDIR\qgroundcontrol.exe" 0
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\QGroundControl (GPU Compatibility Mode).lnk" "$INSTDIR\qgroundcontrol.exe" "-angle" "$INSTDIR\qgroundcontrol.exe" 0
  !insertmacro DemoteShortCut "$SMPROGRAMS\$StartMenuFolder\QGroundControl (GPU Compatibility Mode).lnk"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\QGroundControl (GPU Safe Mode).lnk" "$INSTDIR\qgroundcontrol.exe" "-swrast" "$INSTDIR\qgroundcontrol.exe" 0
  !insertmacro DemoteShortCut "$SMPROGRAMS\$StartMenuFolder\QGroundControl (GPU Safe Mode).lnk"
SectionEnd

