Name "QGroundcontrol"

InstallDir $PROGRAMFILES\qgroundcontrol

Page license 
Page directory
Page components
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

LicenseData license.txt

Section ""

  SetOutPath $INSTDIR
  File /r release\*.*
  WriteUninstaller $INSTDIR\QGroundControl_uninstall.exe
SectionEnd 

Section "Uninstall"
  Delete $INSTDIR\QGroundControl_uninstall.exe
  RMDir /r /REBOOTOK $INSTDIR
  RMDir /r /REBOOTOK "$SMPROGRAMS\QGroundControl\"
SectionEnd

Section "create Start Menu Shortcuts"
  CreateDirectory "$SMPROGRAMS\QGroundControl"
  CreateShortCut "$SMPROGRAMS\QGroundControl\uninstall.lnk" "$INSTDIR\QGroundControl_uninstall.exe" "" "$INSTDIR\QGroundControl_uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\QGroundControl\QGroundControl.lnk" "$INSTDIR\qgroundcontrol.exe" "" "$INSTDIR\qgroundcontrol.exe" 0
SectionEnd