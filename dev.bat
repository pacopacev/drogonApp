@echo off
cd /d "H:\drogonApp\build"
cmake --build . --config Release
if %ERRORLEVEL% EQU 0 .\Release\DrogonApp.exe
