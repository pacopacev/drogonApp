@echo off
echo ==========================================
echo DrogonApp Development Build (Windows)
echo ==========================================
echo.

REM Check if we're in the right directory
if not exist "H:\drogonApp\CMakeLists.txt" (
    echo Error: CMakeLists.txt not found!
    echo Make sure you're running from H:\drogonApp
    pause
    exit /b 1
)

cd /d "H:\drogonApp"

REM Check if build directory exists
if not exist "build" (
    echo Creating build directory...
    mkdir build
    cd build
    
    echo Configuring CMake for Windows...
    cmake .. ^
        -DCMAKE_TOOLCHAIN_FILE="H:/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
        -G "Visual Studio 17 2022" ^
        -A x64
    
    if %ERRORLEVEL% NEQ 0 (
        echo CMake configuration failed!
        pause
        exit /b 1
    )
    
    cd ..
)

REM Build the project
echo Building project...
cd build
cmake --build . --config Release

REM Run if successful
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ==========================================
    echo Build successful!
    echo ==========================================
    
    if exist ".\Release\DrogonApp.exe" (
        echo Running DrogonApp...
        echo.
        .\Release\DrogonApp.exe
    ) else (
        echo Error: DrogonApp.exe not found!
        pause
    )
) else (
    echo.
    echo ==========================================
    echo Build failed!
    echo ==========================================
    pause
)