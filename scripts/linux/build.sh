#!/bin/bash

echo "=========================================="
echo "Building DrogonApp for Linux"
echo "=========================================="

cd /var/www/drogonApp

# Clean and rebuild
rm -rf build
mkdir -p build
cd build

# Configure for Linux
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

# Run if successful
if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful!"
    echo "Running DrogonApp..."
    ./DrogonApp
else
    echo ""
    echo "Build failed!"
    exit 1
fi