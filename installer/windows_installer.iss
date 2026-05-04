; McLaren Senna VR Viewer — Inno Setup 6 installer script
; Run build_installer.bat first to populate ..\build\deploy\

#define AppName      "McLaren Senna VR Viewer"
#define AppVersion   "1.0.0"
#define AppPublisher "EEEE2076 — University of Nottingham"
#define AppExeName   "Worksheet6.exe"
#define DeployDir    "..\build\deploy"

[Setup]
AppId={{B7A4E2F1-3C8D-4A9B-8E5F-1234ABCD5678}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppComments=Qt 6 + VTK 9 + OpenVR interactive 3D viewer
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\{#AppExeName}
OutputDir=Output
OutputBaseFilename=McLarenSennaViewer_Setup_{#AppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"

[Files]
; The deploy folder contains the exe, all Qt DLLs, VTK DLLs, openvr_api.dll,
; the STL model, VR binding JSONs, and Qt platform/style plugins.
Source: "{#DeployDir}\*"; DestDir: "{app}"; \
    Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\{#AppName}";            Filename: "{app}\{#AppExeName}"
Name: "{group}\Uninstall {#AppName}";  Filename: "{uninstallexe}"
Name: "{commondesktop}\{#AppName}";    Filename: "{app}\{#AppExeName}"; \
    Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; \
    Description: "Launch {#AppName} now"; \
    Flags: nowait postinstall skipifsilent
