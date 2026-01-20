@REM @echo off
@REM cd /d "H:\drogonApp\build"
@REM cmake --build . --config Release
@REM if %ERRORLEVEL% EQU 0 .\Release\DrogonApp.exe


@echo off
REM Build
cd /d "H:\drogonApp\build"
cmake --build . --config Release

REM Run from project root
cd /d "H:\drogonApp"
if %ERRORLEVEL% EQU 0 .\build\Release\DrogonApp.exe