!include "MUI2.nsh"

Name "QGroundcontrol"
Var StartMenuFolder

InstallDir $PROGRAMFILES\qgroundcontrol

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "installheader.bmp";

!insertmacro MUI_PAGE_LICENSE "license.txt"
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section
  SetOutPath $INSTDIR
  File /r build_windows_install\release\*.*
  File deploy\px4driver.msi
  WriteUninstaller $INSTDIR\QGroundControl_uninstall.exe
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QGroundControl" "DisplayName" "QGroundControl"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QGroundControl" "UninstallString" "$\"$INSTDIR\QGroundControl_uninstall.exe$\""

  ; Only attempt to install the PX4 driver if the version isn't present
  !define ROOTKEY "SYSTEM\CurrentControlSet\Control\Class\{4D36E978-E325-11CE-BFC1-08002BE10318}"
  StrCpy $0 0
loop:
  EnumRegKey $1 HKLM ${ROOTKEY} $0
  StrCmp $1 "" notfound cont1

cont1:
  StrCpy     $2 "${ROOTKEY}\$1"
  ReadRegStr $3 HKLM $2 "ProviderName"
  StrCmp     $3 "3D Robotics" found_provider
mismatch:
  IntOp      $0 $0 + 1
  goto  loop

found_provider:
  ReadRegStr $3 HKLM $2 "DriverVersion"
  StrCmp     $3 "2.0.0.4" skip_driver
  goto  mismatch

notfound:
  DetailPrint "USB Driver not found... installing"
  ExecWait '"msiexec" /i "px4driver.msi"'
  goto done

skip_driver:
  DetailPrint "USB Driver found... skipping install"
done:
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
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\QGroundControl (GPU Safe Mode).lnk" "$INSTDIR\qgroundcontrol.exe" "-swrast" "$INSTDIR\qgroundcontrol.exe" 0
SectionEnd

Function .onInit
  ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QGroundControl" "UninstallString"
  StrCmp $R0 "" done
 
  MessageBox MB_OK|MB_ICONEXCLAMATION \
  	"QGroundControl is already installed. $\n$\nYou must uninstall the previous version before installing a new one."
  Abort
done: 
FunctionEnd
