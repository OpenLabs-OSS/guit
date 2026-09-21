; Guit Inno Setup Script
; Build with: iscc guit.iss
; Requires Inno Setup 6+

#define AppName "Guit"
#define AppVersion "0.1.0"
#define AppPublisher "OpenLabs"
#define AppURL "https://github.com/OpenLabs-OSS/guit"
#define AppSupportURL "https://github.com/OpenLabs-OSS/guit/issues"
#define AppUpdatesURL "https://github.com/OpenLabs-OSS/guit/releases"
#define AppContact "openlabs-oss@protonmail.com"
#define AppExeName "guit.exe"
#define DeployDir "..\build-release\deploy"

[Setup]
AppId={{8F3A2B1C-7D4E-4F9A-B6C3-1E2D8F7A4B5C}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppSupportURL}
AppUpdatesURL={#AppUpdatesURL}
AppContact={#AppContact}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
AllowNoIcons=yes
OutputDir=..\build-release\installer
OutputBaseFilename=Guit-{#AppVersion}-windows-x64-Setup
Compression=lzma/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
UninstallDisplayIcon={app}\{#AppExeName}
ArchitecturesInstallIn64BitMode=x64os
ArchitecturesAllowed=x64os

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#DeployDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"
Name: "{commondesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,Guit}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}"

[UninstallRun]
Filename: "{app}\unins000.exe"; RunOnceId: "guit_uninstall"

[Registry]
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\{#AppExeName}"; ValueType: string; ValueData: "{app}\{#AppExeName}"; Flags: uninsdeletevalue

[Messages]
; Custom messages for the installer
; (none)

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    // Nothing special needed
  end;
end;