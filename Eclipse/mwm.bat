@echo off
cd /d %~dp0

echo [AVIS] Initializing MSVC x64 build environment...
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

echo [AVIS] Terminating any running instances of eclipse.exe...
taskkill /f /im eclipse.exe >nul 2>&1

:: Clean old artifacts if exist
if exist eclipse.exe del /f /q eclipse.exe
if exist avis_tray.res del /f /q avis_tray.res
if exist avis_tray.obj del /f /q avis_tray.obj
if exist system_loader.obj del /f /q system_loader.obj
if exist cJSON.obj del /f /q cJSON.obj
if exist *.obj del /f /q *.obj

echo [AVIS] Compiling Avis Tray Resource...
rc.exe /fo avis_tray.res avis_tray.rc

echo [AVIS] Compiling Application with System Loader cJSON...
cl.exe /nologo /O2 /W4 ^
 avis_tray.c ^
 system_loader.c ^
 cJSON\cJSON.c ^
 avis_tray.res ^
 /Fe:eclipse.exe ^
 /I cJSON ^
 /I include ^
 user32.lib ^
 shell32.lib ^
 advapi32.lib ^
 ole32.lib

if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed.
    pause
    exit /b %errorlevel%
)

echo [AVIS] Build Complete. Launching eclipse.exe...
start eclipse.exe
pause
