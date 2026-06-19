; Inno Setup Script for Daily Routine Dashboard
[Setup]
AppName=Daily Routine Dashboard
AppVersion=1.0
DefaultDirName={localappdata}\Programs\DailyRoutineDashboard
DefaultGroupName=Daily Routine Dashboard
OutputDir=d:\Maroof\Me\Task_Manager\routine-widget-cpp\build
OutputBaseFilename=DailyRoutineDashboardSetup
Compression=lzma2/max
SolidCompression=yes
PrivilegesRequired=lowest
SetupIconFile=d:\Maroof\Me\Task_Manager\routine-widget-cpp\icon.ico
UninstallDisplayIcon={app}\DailyRoutineDashboard.exe
WizardStyle=modern
DisableDirPage=no
WizardImageFile=d:\Maroof\Me\Task_Manager\routine-widget-cpp\installer_banner.bmp
WizardSmallImageFile=d:\Maroof\Me\Task_Manager\routine-widget-cpp\icon.bmp
InfoBeforeFile=d:\Maroof\Me\Task_Manager\routine-widget-cpp\info.rtf

[Files]
Source: "d:\Maroof\Me\Task_Manager\routine-widget-cpp\build\Release\DailyRoutineDashboard.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "d:\Maroof\Me\Task_Manager\routine-widget-cpp\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "d:\Maroof\Me\Task_Manager\routine-widget-cpp\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "d:\Maroof\Me\Task_Manager\routine-widget-cpp\build\Release\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "d:\Maroof\Me\Task_Manager\routine-widget-cpp\build\Release\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "d:\Maroof\Me\Task_Manager\routine-widget-cpp\build\Release\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "d:\Maroof\Me\Task_Manager\routine-widget-cpp\build\Release\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "d:\Maroof\Me\Task_Manager\routine-widget-cpp\build\Release\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "d:\Maroof\Me\Task_Manager\routine-widget-cpp\build\Release\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "d:\Maroof\Me\Task_Manager\routine-widget-cpp\build\Release\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "d:\Maroof\Me\Task_Manager\routine-widget-cpp\build\Release\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "d:\Maroof\Me\Task_Manager\routine-widget-cpp\build\Release\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Daily Routine Dashboard"; Filename: "{app}\DailyRoutineDashboard.exe"
Name: "{userdesktop}\Daily Routine Dashboard"; Filename: "{app}\DailyRoutineDashboard.exe"
Name: "{userstartup}\Daily Routine Dashboard"; Filename: "{app}\DailyRoutineDashboard.exe"

[Run]
Filename: "{app}\DailyRoutineDashboard.exe"; Description: "Launch Daily Routine Dashboard"; Flags: nowait postinstall skipifsilent
