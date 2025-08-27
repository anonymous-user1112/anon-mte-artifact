#!/bin/bash
set -e
set -x

# Configuration
DR_VERSION="11.3.0"
DR_RELEASE_URL="https://github.com/DynamoRIO/dynamorio/releases/download/release_11.3.0-1/DynamoRIO-AArch64-Linux-11.3.0.tar.gz"
DR_INSTALL_DIR="$PWD/release"
DR_SRC_DIR="DynamoRIO-"$DR_VERSION

# Check if DynamoRIO is already installed
if [ -d "$DR_INSTALL_DIR" ]; then
    echo "DynamoRIO is already installed at $DR_INSTALL_DIR."
else
    echo "DynamoRIO not found. Downloading and installing..."

    # Create a temporary directory for the download
    mkdir -p /tmp/dr_install
    pushd /tmp/dr_install

    # Download the release tarball
    echo "Downloading DynamoRIO from $DR_RELEASE_URL..."
    wget -q --show-progress "$DR_RELEASE_URL"

    # Extract the tarball
    echo "Extracting DynamoRIO..."
    tar -xzf "DynamoRIO-AArch64-Linux-11.3.0.tar.gz"
    mkdir -p "$DR_INSTALL_DIR"
    mv DynamoRIO-AArch64-Linux-11.3.0-1/* "$DR_INSTALL_DIR"

    # Clean up temporary files
    echo "Cleaning up temporary files..."
    popd
    rm -rf /tmp/dr_install

    echo "DynamoRIO installation complete."
fi

# Compile the DynamoRIO client
echo "Compiling the DynamoRIO client..."

# Run CMake
# The -DCMAKE_MODULE_PATH points to the DynamoRIO installation's CMake configuration files
# https://github.com/DynamoRIO/dynamorio/issues/7493
cmake -S . -B build -DDynamoRIO_DIR="$DR_INSTALL_DIR/cmake"
make -C build

echo "---"
echo "Client compilation complete. The client binary is in the 'build' directory."

#-DDynamoRIO_DIR=dynamorio/build/cmake
