; TB-3030 Windows installer (Inno Setup 6)
; Installs per user by default (no admin rights, nothing to close) - can also be installed for all users.
#define AppName "TB-3030"
#define AppVersion "1.0.0"
#define Publisher "Landgren Audio"
#define Build "..\build\TB3030_artefacts\Release"

[Setup]
AppId={{6A3F3E30-3030-4B30-9303-3030AC1D3030}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#Publisher}
DefaultDirName={autopf}\{#Publisher}\{#AppName}
DefaultGroupName={#Publisher}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
CloseApplications=no
RestartApplications=no
OutputDir=Output
OutputBaseFilename=TB-3030-{#AppVersion}-Windows-Setup
SetupIconFile=..\resources\icon.ico
UninstallDisplayIcon={app}\TB-3030.exe
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern

[Types]
Name: "full"; Description: "Stand-alone app and VST3 plug-in"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "app";  Description: "TB-3030 stand-alone app"; Types: full custom
Name: "vst3"; Description: "TB-3030 VST3 instrument (for L-DAW, Reason, Cubase, Ableton...)"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "Create a desktop icon"; Components: app

[Files]
Source: "{#Build}\Standalone\TB-3030.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion
Source: "{#Build}\VST3\TB-3030.vst3\*"; DestDir: "{autocf}\VST3\TB-3030.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\README.md"; DestDir: "{app}"; DestName: "README.txt"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\TB-3030.exe"; Components: app
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\TB-3030.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\TB-3030.exe"; Description: "Start TB-3030"; Flags: nowait postinstall skipifsilent; Components: app
