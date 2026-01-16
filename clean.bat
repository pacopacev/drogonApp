@echo off
cd /d "H:\drogonApp"
if exist "build" rmdir /s /q build
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="H:/vcpkg/scripts/buildsystems/vcpkg.cmake" -G "Visual Studio 17 2022" -A x64