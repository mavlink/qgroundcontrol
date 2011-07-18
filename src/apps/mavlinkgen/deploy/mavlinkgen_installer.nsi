Name "MAVLink Generator"

OutFile "mavlinkgen-installer-win32.exe"

InstallDir $PROGRAMFILES\mavlinkgen

Page license 
Page directory
Page components
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

LicenseData ..\license.txt

Section ""

  SetOutPath $INSTDIR
  File ..\release\*.*
  WriteUninstaller $INSTDIR\mavlinkgen_uninstall.exe
SectionEnd 

Section "Uninstall"
  Delete $INSTDIR\mavlinkgen_uninstall.exe
  Delete $INSTDIR\*.*
  RMDir $INSTDIR
  Delete "$SMPROGRAMS\mavlinkgen\*.*"
  RMDir "$SMPROGRAMS\mavlinkgen\"
SectionEnd

Section "create Start Menu Shortcuts"
  CreateDirectory "$SMPROGRAMS\MAVLink"
  CreateShortCut "$SMPROGRAMS\MAVLink\Uninstall.lnk" "$INSTDIR\mavlinkgen_uninstall.exe" "" "$INSTDIR\mavlinkgen_uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\MAVLink\MAVLinkGen.lnk" "$INSTDIR\mavlinkgen.exe" "" "$INSTDIR\mavlinkgen.exe" 0
SectionEnd