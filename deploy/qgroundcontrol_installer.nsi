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
