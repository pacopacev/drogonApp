@echo off
cd /d "H:\drogonApp\build"
echo Quick rebuild...
cmake --build . --config Release
if %ERRORLEVEL% EQU 0 (
    echo Starting server...
    .\Release\DrogonApp.exe
)