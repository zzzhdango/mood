; Inno Setup Script for OBYS // MOOD VST3 & Standalone
; Compiles into OBYS_MOOD_Setup_v1.0.0.exe

#define MyAppName "OBYS // MOOD"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "OBYS_DSP"
#define MyAppExeName "OBYS // MOOD.exe"

[Setup]
AppId={{8F4E9A12-3B7D-4C2E-91F0-OBYSMOOD1000}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\OBYS_DSP\OBYS_MOOD
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir=..\dist
OutputBaseFilename=OBYS_MOOD_Setup_v{#MyAppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Types]
Name: "full"; Description: "Full Installation (VST3 Plugin + Standalone App)"
Name: "vst3only"; Description: "VST3 Plugin Only"
Name: "custom"; Description: "Custom Installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 Audio Plugin (C:\Program Files\Common Files\VST3)"; Types: full vst3only custom; Flags: fixed
Name: "standalone"; Description: "Standalone Desktop Application"; Types: full custom

[Files]
; VST3 Bundle
Source: "..\build\OBYS_MOOD_artefacts\Release\VST3\OBYS // MOOD.vst3\*"; DestDir: "{commoncf64}\VST3\OBYS // MOOD.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
; Standalone Executable
Source: "..\build\OBYS_MOOD_artefacts\Release\Standalone\OBYS // MOOD.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName} Standalone"; Filename: "{app}\{#MyAppExeName}"; Components: standalone
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Components: standalone; Flags: nowait postinstall skipifsilent
