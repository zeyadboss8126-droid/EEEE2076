; McLaren Senna VR Viewer — Inno Setup 6 installer script
; Run build_installer.bat first to populate ..\build\deploy\

#define AppName      "McLaren Senna VR Viewer"
#define AppVersion   "1.0.0"
#define AppPublisher "EEEE2076 Group — University of Nottingham"
#define AppExeName   "Worksheet6.exe"
#define DeployDir    "..\build\deploy"

[Setup]
AppId={{B7A4E2F1-3C8D-4A9B-8E5F-1234ABCD5678}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
OutputDir=Output
OutputBaseFilename=McLarenSennaViewer_Setup_{#AppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"

[Files]
; Everything in the deploy folder (exe, Qt DLLs, VTK DLLs, OpenVR DLL,
; STL model, VR bindings JSON files, Qt platform/style plugins)
Source: "{#DeployDir}\*"; DestDir: "{app}"; \
    Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\{#AppName}";             Filename: "{app}\{#AppExeName}"
Name: "{group}\Uninstall {#AppName}";   Filename: "{uninstallexe}"
Name: "{commondesktop}\{#AppName}";     Filename: "{app}\{#AppExeName}"; \
    Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; \
    Description: "Launch {#AppName}"; \
    Flags: nowait postinstall skipifsilent
