; tea.nsi
!include MUI2.nsh
; The name of the installer
Name "Drumlabooh installer"

;Caption "��������� TEA (Win32) 18.0.0"
;SubCaption 2 ": ����� �����"
;SubCaption 0 ": ������� ���������"
;SubCaption 3 ": ��������� ������"
;SubCaption 4 ": ���������"
;SubCaption 1 ": ����������"

CRCCheck on
;SetCompressor zlib
;SetCompressor bzip2
SetCompressor lzma

;Icon "tea_app.ico"

InstallColors 000000 FCE6BD
LicenseBkColor FCE6BD

;InstProgressFlags "colored"
InstProgressFlags smooth colored

;LicenseText "������� ��������� � �������" "������!"
;LicenseData "tealic.txt"
LicenseData "LICENSE"
;UninstallText "�������� TEA. ��� �� ������ ���������� ;)"
ShowInstDetails show
ShowUninstDetails show
;CompletedText "���������!"
;MiscButtonText "< �����" "������ >" "������" "�������"
;InstallButtonText "����������"
;UninstallButtonText "�������"
;UninstallCaption "������� TEA"
;UninstallSubCaption 0 ": �������������"
;UninstallSubCaption 1 ": ������ �����"
;UninstallSubCaption 2 ": ���������"

;C:\Program Files\Common Files\VST3\drumlabooh.vst3

; The file to write
OutFile "drumlabooh-installer.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\Common Files\VST3\drumlabooh.vst3"


; The text to prompt the user to enter a directory
;DirText "������, ��������, ��������� ��������. �������� �����."
;ComponentText "�������� ���������� ��� ���������"


; The stuff to install
Section "Files (mandatory)"
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put file there
  File /r ".\build\drumlabooh_artefacts\Release\VST3\drumlabooh.vst3\Contents\Resources/moduleinfo.json"
  File /r ".\build\drumlabooh_artefacts\Release\VST3\drumlabooh.vst3\Contents\x86_64-win/drumlabooh.vst3"

  WriteRegStr HKLM SOFTWARE\Drumlabooh "Install_Dir" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Drumlabooh" "DisplayName" "Drumlabooh (remove)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Drumlabooh" "UninstallString" '"$INSTDIR\nsUninst.exe"'
  WriteUninstaller "nsUninst.exe"

;  ExecShell "open" '"$INSTDIR\doc\note_for_users.html"'


SectionEnd

;Section "������ � ������� ���� (���������)"

;  CreateDirectory "$SMPROGRAMS\TEA"
;  CreateShortCut "$SMPROGRAMS\TEA\Tea.lnk" "$INSTDIR\tea.exe"
;  CreateShortCut "$SMPROGRAMS\TEA\������� Tea.lnk" "$INSTDIR\nsUninst.exe"

;SectionEnd

;Section "������� ����� �� ������� �����"

  ;CreateShortCut "$DESKTOP\Tea(Win32).lnk" "$INSTDIR\tea.exe"

;SectionEnd


Section "Uninstall"

;          Delete $DESKTOP\Tea.lnk
;          Delete $SMPROGRAMS\TEA\*.*
;          RMDir $SMPROGRAMS\TEA
          Delete $INSTDIR\nsUninst.exe
          Delete $INSTDIR\*.*
          RMDir /r $INSTDIR

SectionEnd


; eof