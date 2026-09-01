@echo off

cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

if exist avis_tray.exe del /f /q avis_tray.exe
if exist *.obj del /f /q *.obj
if exist *.res del /f /q *.res

rem Compile the resource file (only once)
rc.exe /fo avis_tray.res avis_tray.rc

rem Compile and link with the resource
cl.exe /nologo /O2 /W4 ^
    avis_tray.c ^
    ..\ggml_clean\cJSON\cJSON.c ^
    avis_tray.res ^
    /Fe:avis_tray.exe ^
    /I ..\ggml_clean\cJSON ^
    user32.lib ^
    shell32.lib

pause
