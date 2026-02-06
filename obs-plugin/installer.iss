; Inno Setup Script for OBS SR Tracker Plugin
; Requires Inno Setup 6.x — https://jrsoftware.org/isinfo.php

#define MyAppName "OBS SR Tracker"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Ranked Tracker"
#define MyAppURL "https://github.com/leoprim/ranked-tracker"

[Setup]
AppId={{E8F3A1B2-5C7D-4E9F-B6A0-1D2E3F4A5B6C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppSupportURL={#MyAppURL}
DefaultDirName={autopf}\obs-studio
DirExistsWarning=no
DisableProgramGroupPage=yes
OutputBaseFilename=OBS-SR-Tracker-Setup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; No need for a start menu entry — this is an OBS plugin
CreateAppDir=yes
Uninstallable=yes
UninstallDisplayName={#MyAppName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
SelectDirLabel3=Select the OBS Studio installation folder. This is usually "C:\Program Files\obs-studio".
SelectDirBrowseLabel=If OBS Studio is installed in a different location, click Browse to select it.

[Files]
; Plugin DLL
Source: "build_x64\Release\obs-sr-tracker.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion

; Tesseract trained data
Source: "tessdata\eng.traineddata"; DestDir: "{app}\data\obs-plugins\obs-sr-tracker\tessdata"; Flags: ignoreversion

; Locale data
Source: "data\locale\en-US.ini"; DestDir: "{app}\data\obs-plugins\obs-sr-tracker\locale"; Flags: ignoreversion

[Code]
// Try to auto-detect OBS install path from registry
function GetDefaultOBSDir(Param: String): String;
var
  InstallPath: String;
begin
  // Try 64-bit registry first
  if RegQueryStringValue(HKLM, 'SOFTWARE\OBS Studio', '', InstallPath) then
    Result := InstallPath
  // Try the default install location
  else if DirExists(ExpandConstant('{autopf}\obs-studio')) then
    Result := ExpandConstant('{autopf}\obs-studio')
  else
    Result := ExpandConstant('{autopf}\obs-studio');
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  // Validate that the selected directory looks like an OBS install
  if CurPageID = wpSelectDir then
  begin
    if not FileExists(ExpandConstant('{app}\bin\64bit\obs64.exe')) then
    begin
      if MsgBox('The selected folder does not appear to contain OBS Studio (obs64.exe not found).' + #13#10 + #13#10 +
                'Install anyway?', mbConfirmation, MB_YESNO) = IDNO then
        Result := False;
    end;
  end;
end;

function InitializeSetup(): Boolean;
var
  DefaultDir: String;
begin
  Result := True;
  DefaultDir := GetDefaultOBSDir('');
  // The default dir will be set by the [Setup] section's DefaultDirName,
  // but we override it in the registry check above
end;

[Run]
; Optionally launch OBS after install
Filename: "{app}\bin\64bit\obs64.exe"; Description: "Launch OBS Studio"; Flags: nowait postinstall skipifsilent unchecked
