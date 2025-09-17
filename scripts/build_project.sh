#!/bin/bash

# Exit on any error
set -e

# Set variables
PROJ_DIR="$(dirname "$(realpath "$0")")/.."
EXT_NAME="flock"
EXT_CONFIG="${PROJ_DIR}/extension_config.cmake"
GENERATOR="Ninja"  # Set this to the desired generator if different
BUILD_FLAGS=""     # Add any additional CMake build flags here
EXT_RELEASE_FLAGS=""  # Add any extension release flags here
DUCKDB_SRCDIR="${PROJ_DIR}/duckdb"  # Adjust the path to the source directory

# Check if the source directory exists
if [ ! -d "${DUCKDB_SRCDIR}" ]; then
    echo "Error: Source directory '${DUCKDB_SRCDIR}' does not exist."
    exit 1
fi

# Create build directory
mkdir -p build/release

# Configure the project with CMake
cmake ${GENERATOR} ${BUILD_FLAGS} ${EXT_RELEASE_FLAGS} \
  -DCMAKE_BUILD_TYPE=Release \
  -S "${DUCKDB_SRCDIR}" \
  -B build/release

# Build the project
cmake --build build/release --config Release

echo "Build completed successfully."
