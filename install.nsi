; tea.nsi
!include MUI2.nsh
; The name of the installer
Name "Drumlabooh installer"

Caption "Drumlabooh VSTi setup"
SubCaption 2 ": Select VSTi folder"
SubCaption 0 ": License agreement"
SubCaption 3 ": Files installation"
SubCaption 4 ": Done"
SubCaption 1 ": Components"

CRCCheck on
;SetCompressor zlib
;SetCompressor bzip2
SetCompressor lzma

;Icon "tea_app.ico"

InstallColors 000000 FCE6BD
LicenseBkColor FCE6BD

;InstProgressFlags "colored"
InstProgressFlags smooth colored

LicenseData "LICENSE"
UninstallText "Removing Drumlabooh. Are you sure?"
ShowInstDetails show
ShowUninstDetails show
CompletedText "Done!"
MiscButtonText "< Previous" "Next >" "Cancel" "Close"
InstallButtonText "Install"
UninstallButtonText "Uninstall"
UninstallCaption "Remove Drumlabooh"
UninstallSubCaption 0 ": Confirmation"
UninstallSubCaption 1 ": Removing files"
UninstallSubCaption 2 ": Done"

; The file to write
OutFile "drumlabooh-installer.exe"

; The default installation directory
InstallDir "$PROGRAMFILES64\Common Files\VST3\drumlabooh.vst3"

; The stuff to install
Section "Drumlabooh (Stereo)"
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put file there
  File /r "C:\a\drumlabooh\drumlabooh\build\drumlabooh_artefacts\Release\VST3\drumlabooh.vst3\Contents\Resources\moduleinfo.json"
  File /r "C:\a\drumlabooh\drumlabooh\build\drumlabooh_artefacts\Release\VST3\drumlabooh.vst3\Contents\x86_64-win\drumlabooh.vst3"

  WriteRegStr HKLM SOFTWARE\Drumlabooh "Install_Dir" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Drumlabooh" "DisplayName" "Drumlabooh (remove)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Drumlabooh" "UninstallString" '"$INSTDIR\nsUninst.exe"'
  WriteUninstaller "nsUninst.exe"

;  ExecShell "open" '"$INSTDIR\doc\note_for_users.html"'

SectionEnd


; The stuff to install
Section "Drumlabooh (Multi)"
  ; Set output path to the installation directory.
  SetOutPath "$PROGRAMFILES64\Common Files\VST3\drumlabooh-multi.vst3"
  ; Put file there
  File /r "C:\a\drumlabooh\drumlabooh\build\drumlabooh-multi_artefacts\Release\VST3\drumlabooh-multi.vst3\Contents\Resources\moduleinfo.json"
  File /r "C:\a\drumlabooh\drumlabooh\build\drumlabooh-multi_artefacts\Release\VST3\drumlabooh-multi.vst3\Contents\x86_64-win\drumlabooh-multi.vst3"

;  WriteRegStr HKLM SOFTWARE\Drumlabooh_multi "Install_Dir" "$PROGRAMFILES\Common Files\VST3\drumlabooh-multi.vst3"
  ;WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Drumlabooh-multi" "DisplayName" "Drumlabooh-multi (remove)"
  ;WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Drumlabooh-multi" "UninstallString" '"$PROGRAMFILES\Common Files\VST3\drumlabooh-multi.vst3\nsUninst.exe"'
  ;WriteUninstaller "nsUninst.exe"

;  ExecShell "open" '"$INSTDIR\doc\note_for_users.html"'

SectionEnd


Section "Basic drumkits"
  ; Set output path to the installation directory.
  SetOutPath "C:\drumlabooh-kits"
;D:/a/drumlabooh/drumlabooh/build/_deps/drum_sklad-src
  ; Put file there
  File /r "C:\a\drumlabooh\drumlabooh\build\_deps\drum_sklad-src\*.*"

;  ExecShell "open" '"$INSTDIR\doc\note_for_users.html"'

SectionEnd



Section "Uninstall"

          Delete $INSTDIR\nsUninst.exe
          Delete $INSTDIR\*.*
          Delete "C:\drumlabooh-kits\*.*"
          Delete "$PROGRAMFILES64\Common Files\VST3\drumlabooh-multi.vst3\*.*"

          RMDir /r $INSTDIR

SectionEnd


; eof
