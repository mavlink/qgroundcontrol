Name "QGroundcontrol"

OutFile "qgroundcontrol-installer-win32.exe"

InstallDir $PROGRAMFILES\qgroundcontrol

Page license 
Page directory
Page components
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

LicenseData ..\license.txt

Section ""

  SetOutPath $INSTDIR
  File qgroundcontrol\*.*
  WriteUninstaller $INSTDIR\QGroundcontrol_uninstall.exe
SectionEnd 

Section "Uninstall"
  Delete $INSTDIR\QGroundcontrol_uninstall.exe
  Delete $INSTDIR\*.*
  RMDir $INSTDIR
  Delete "$SMPROGRAMS\QGroundcontrol\*.*"
  RMDir "$SMPROGRAMS\QGroundcontrol\"
SectionEnd

Section "create Start Menu Shortcuts"
  CreateDirectory "$SMPROGRAMS\QGroundcontrol"
  CreateShortCut "$SMPROGRAMS\QGroundcontrol\uninstall.lnk" "$INSTDIR\QGroundcontrol_uninstall.exe" "" "$INSTDIR\QGroundcontrol_uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\QGroundcontrol\QGroundcontrol.lnk" "$INSTDIR\qgroundcontrol.exe" "" "$INSTDIR\qgroundcontrol.exe" 0
SectionEnd