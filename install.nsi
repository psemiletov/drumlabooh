; tea.nsi
!include MUI2.nsh
; The name of the installer
Name "Drumlabooh installer"

;Caption "Установка TEA (Win32) 18.0.0"
;SubCaption 2 ": Выбор папки"
;SubCaption 0 ": Условия установки"
;SubCaption 3 ": Установка файлов"
;SubCaption 4 ": Завершено"
;SubCaption 1 ": Компоненты"

CRCCheck on
;SetCompressor zlib
;SetCompressor bzip2
SetCompressor lzma

;Icon "tea_app.ico"

InstallColors 000000 FCE6BD
LicenseBkColor FCE6BD

;InstProgressFlags "colored"
InstProgressFlags smooth colored

;LicenseText "Условия установки и новости" "Хорошо!"
;LicenseData "tealic.txt"
LicenseData "LICENSE"
;UninstallText "Удаление TEA. Еще не поздно отказаться ;)"
ShowInstDetails show
ShowUninstDetails show
;CompletedText "Выполнено!"
;MiscButtonText "< Назад" "Дальше >" "Отмена" "Закрыть"
;InstallButtonText "Установить"
;UninstallButtonText "Стереть"
;UninstallCaption "Удалить TEA"
;UninstallSubCaption 0 ": Подтверждение"
;UninstallSubCaption 1 ": Стираю файлы"
;UninstallSubCaption 2 ": Завершено"

;C:\Program Files\Common Files\VST3\drumlabooh.vst3

; The file to write
OutFile "drumlabooh-installer.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\Common Files\VST3\drumlabooh.vst3"


; The text to prompt the user to enter a directory
;DirText "Сейчас, наверное, установка начнется. Выберите папку."
;ComponentText "Выберите компоненты для установки"


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

;Section "Ярлыки в Главном меню (выборочно)"

;  CreateDirectory "$SMPROGRAMS\TEA"
;  CreateShortCut "$SMPROGRAMS\TEA\Tea.lnk" "$INSTDIR\tea.exe"
;  CreateShortCut "$SMPROGRAMS\TEA\Удалить Tea.lnk" "$INSTDIR\nsUninst.exe"

;SectionEnd

;Section "Создать ярлык на Рабочем столе"

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