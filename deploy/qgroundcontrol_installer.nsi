!include "MUI2.nsh"
!include LogicLib.nsh
!include Win\COM.nsh
!include Win\Propkey.nsh
!include "FileFunc.nsh"

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

Name "${APPNAME}"
BrandingText "${APPNAME} ${VERSION}"
Var StartMenuFolder

InstallDir "$PROGRAMFILES64\${APPNAME}"

SetCompressor /SOLID /FINAL lzma

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${HEADER_BITMAP}"
!define MUI_ICON "${INSTALLER_ICON}"
!define MUI_UNICON "${INSTALLER_ICON}"

!define ARP "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section
  DetailPrint "Checking for uninstall of previous version"

  # Check uninstall of previous 32 bit version
  SetRegView 32
  ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString"

  # Remainder of installer takes place in 64 bit space
  SetRegView 64

  # Check uninstall of previous 64 bit version
  ${If} $R0 == ""
    ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString"
  ${EndIf}

  # Uninstall previous version
  ${If} $R0 != ""
    DetailPrint "Uninstalling previous version $R0 from $R1"
    ExecWait "$R0 /S -LEAVE_DATA=1" $0
    ${If} $0 <> 0
      MessageBox MB_OK|MB_ICONEXCLAMATION "Could not remove a previously installed ${APPNAME} version.$\n$\nPlease remove it before continuing."
      Abort
    ${EndIf}
  ${Else}
    DetailPrint "No previous version found"
  ${EndIf}

  # Install new QGC version
  SetOutPath $INSTDIR
  File /r /x ${EXENAME}.pdb /x ${EXENAME}.lib /x ${EXENAME}.exp ${DESTDIR}\*.*
  File ${INSTALLER_ICON}
  File deploy\px4driver.msi

  # Create the uninstaller
  WriteUninstaller $INSTDIR\${EXENAME}-Uninstall.exe

  # Write the uninstaller reg keys
  WriteRegStr HKLM "${ARP}" "DisplayName" "${APPNAME}"
  WriteRegStr HKLM "${ARP}" "UninstallString" "$\"$INSTDIR\${EXENAME}-Uninstall.exe$\""
  WriteRegStr HKLM "${ARP}" "InstallLocation" "$\"$INSTDIR$\""
  WriteRegStr HKLM "${ARP}" "DisplayIcon" "$\"$INSTDIR\${INSTALLER_ICON}$\""
  WriteRegStr HKLM "${ARP}" "Publisher" "${INSTALLER_PUBLISHER}"
  WriteRegStr HKLM "${ARP}" "Version" "${VERSION}"
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "${ARP}" "EstimatedSize" "$0"

  # Write reg keys for crash dump reporting
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\${EXENAME}.exe" "DumpCount" 5 
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\${EXENAME}.exe" "DumpType" 2 
  WriteRegExpandStr HKLM "SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\${EXENAME}.exe" "DumpFolder" "%LOCALAPPDATA%\QGCCrashDumps"

  # Install PX4 USB Driver if not already installed
  !define ROOTKEY "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\434608CF2B6E31F0DDBA5C511053F957B55F098E"
  ReadRegStr $0 HKLM "${ROOTKEY}" "Publisher"
  DetailPrint "Checking for USB driver already installed"
  StrCpy $1 ""
  ${If} $0 == "3D Robotics"
    ReadRegStr $1 HKLM "${ROOTKEY}" "DisplayVersion"
  ${EndIf}
  ${If} $1 != "04/11/2013 2.0.0.4"
    DetailPrint "USB Driver not found, installing"
    ExecWait '"msiexec" /i "px4driver.msi"'
  ${Else}
    DetailPrint "USB Driver already installed"
  ${EndIf}

  # Create Start menu shortcuts
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${APPNAME}.lnk" "$INSTDIR\${EXENAME}.exe" "" "$INSTDIR\${EXENAME}.exe" 0
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${APPNAME} (GPU Compatibility Mode).lnk" "$INSTDIR\${EXENAME}.exe" "-angle" "$INSTDIR\${EXENAME}.exe" 0
  !insertmacro DemoteShortCut "$SMPROGRAMS\$StartMenuFolder\${APPNAME} (GPU Compatibility Mode).lnk"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${APPNAME} (GPU Safe Mode).lnk" "$INSTDIR\${EXENAME}.exe" "-swrast" "$INSTDIR\${EXENAME}.exe" 0
  !insertmacro DemoteShortCut "$SMPROGRAMS\$StartMenuFolder\${APPNAME} (GPU Safe Mode).lnk"

  # Create Desktop shortcuts
  SetShellVarContext all
  CreateShortCut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\${EXENAME}.exe"
SectionEnd 

Section "Uninstall"
  SetRegView 64
  ${GetParameters} $R0
  ${GetOptions} $R0 "-LEAVE_DATA=" $R1
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  SetShellVarContext all
  RMDir /r /REBOOTOK $INSTDIR
  RMDir /r /REBOOTOK "$SMPROGRAMS\$StartMenuFolder\"
  Delete "$DESKTOP\${APPNAME}.lnk"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\${EXENAME}.exe"
  SetShellVarContext current
  ${If} $R1 != 1
    RMDir /r /REBOOTOK "$APPDATA\${ORGNAME}\"
  ${Endif}
SectionEnd