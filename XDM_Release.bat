rem Build script to be used without Visual Studio
rem UNDONE: this does not work yet
rem /p:VCTargetsPath=..\\MSVCTargets\\;Configuration=Release
"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" XHL.sln /maxcpucount /target:Rebuild /validate: /p:Configuration=Release
rem "C:\dev\WDK\Program Files\MSBuild\14.0\Bin\MSBuild.exe" XHL.sln /maxcpucount /target:Rebuild /validate: /p:Configuration=Release
pause
cls
