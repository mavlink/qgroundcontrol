; NSIS installer script for GarudX GCS
; Variables:
;   OUTDIR will be passed from the workflow to point at the assembled app folder

!define APP_NAME "garudxgcs"
!define APP_VERSION "1.0.0"

Name "${APP_NAME} ${APP_VERSION}"
OutFile "${APP_NAME}-installer.exe"
InstallDir "$PROGRAMFILES64\\${APP_NAME}"
SetCompress auto
SetCompressor lzma

Page directory
Page instfiles

Section "Install"
  SetOutPath "$INSTDIR"
  ; If OUTDIR is provided, copy from there; otherwise expect files next to the NSI
  StrCpy $0 "$OUTDIR"
  ${If} $0 == ""
    ; copy from current
    File /r "installer_out\\*"
  ${Else}
    CopyFiles /r "$0\\*" "$INSTDIR"
  ${EndIf}

  ; Create shortcuts
  CreateShortcut "$SMPROGRAMS\\${APP_NAME}.lnk" "$INSTDIR\\${APP_NAME}.exe"
  CreateShortcut "$DESKTOP\\${APP_NAME}.lnk" "$INSTDIR\\${APP_NAME}.exe"

SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\\${APP_NAME}.exe"
  RMDir /r "$INSTDIR"
  Delete "$SMPROGRAMS\\${APP_NAME}.lnk"
  Delete "$DESKTOP\\${APP_NAME}.lnk"
SectionEnd
