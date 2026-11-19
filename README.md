# OBYS // MOOD v1.0.0 — Full Source + Cross-Platform Installers

## Что внутри архива

1. **`Installer/build_windows_installer.bat` + `Installer/OBYS_MOOD.iss`**
   - Автоматически собирает VST3 и Standalone-версию под Windows x64 и упаковывает их в полноценный графический установщик **`dist/OBYS_MOOD_Setup_v1.0.0.exe`** (через Inno Setup 6) с выбором компонентов (`VST3` в `C:\Program Files\Common Files\VST3` + `Standalone` в меню «Пуск» и деинсталлятором).
   - Если Inno Setup не установлен, скрипт автоматически установит готовый `OBYS // MOOD.vst3` напрямую в системную папку VST3.

2. **`Installer/build_macos_pkg.sh`**
   - Собирает Universal Binary (`Apple Silicon arm64 + Intel x86_64`) в форматах **VST3**, **AU (.component)** и **Standalone (.app)** и упаковывает их в стандартный установщик macOS **`dist/OBYS_MOOD_Installer_v1.0.0_macOS.pkg`**.

3. **`Installer/install_linux.sh`**
   - Собирает и устанавливает плагин в `~/.vst3/` на Linux.

4. **`.github/workflows/build_installers.yml`**
   - Готовый CI/CD пайплайн: при загрузке папки в любой репозиторий GitHub автоматически собирает готовые файлы `OBYS_MOOD_Setup_v1.0.0.exe` и `OBYS_MOOD_Installer_v1.0.0_macOS.pkg` в облаке (во вкладке *Actions -> Artifacts*) без необходимости ставить компиляторы локально.
