Name "QGroundcontrol"

InstallDir $PROGRAMFILES\qgroundcontrol

Page license 
Page directory
Page components
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

LicenseData license.txt

Section
  SetOutPath $INSTDIR
  File /r build_windows_install\release\*.*
  File deploy\px4driver.msi
  WriteUninstaller $INSTDIR\QGroundControl_uninstall.exe
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QGroundControl" "DisplayName" "QGroundControl"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QGroundControl" "UninstallString" "$\"$INSTDIR\QGroundControl_uninstall.exe$\""
  ExecWait '"msiexec" /i "px4driver.msi"'
SectionEnd 

Section "Uninstall"
  SetShellVarContext all
  Delete $INSTDIR\QGroundControl_uninstall.exe
  RMDir /r /REBOOTOK $INSTDIR
  RMDir /r /REBOOTOK "$SMPROGRAMS\QGroundControl\"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QGroundControl"
SectionEnd

Section "create Start Menu Shortcuts"
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\QGroundControl"
  CreateShortCut "$SMPROGRAMS\QGroundControl\QGroundControl.lnk" "$INSTDIR\qgroundcontrol.exe" "" "$INSTDIR\qgroundcontrol.exe" 0
SectionEnd

Function .onInit
  ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QGroundControl" "UninstallString"
  StrCmp $R0 "" done
 
  MessageBox MB_OK|MB_ICONEXCLAMATION \
  	"QGroundControl is already installed. $\n$\nYou must uninstall the previous version before installing a new one."
  Abort
done: 
FunctionEnd