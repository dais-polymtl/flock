#!/bin/bash

# Flock Build and Run Guide Script
# This script guides you through the complete build and run process for the Flock extension

set -e

# Colors for output
RESET='\033[0m'
BOLD='\033[1m'
GREEN='\033[32m'
RED='\033[31m'
YELLOW='\033[33m'
CYAN='\033[36m'
BLUE='\033[34m'

# Get project directory
PROJ_DIR="$(dirname "$(realpath "$0")")/.."
cd "$PROJ_DIR"

# Function to print section headers
print_section() {
    echo ""
    echo -e "${BOLD}${CYAN}========================================${RESET}"
    echo -e "${BOLD}${CYAN}$1${RESET}"
    echo -e "${BOLD}${CYAN}========================================${RESET}"
    echo ""
}

# Function to print success messages
print_success() {
    echo -e "${GREEN}✓ $1${RESET}"
}

# Function to print error messages
print_error() {
    echo -e "${RED}✗ $1${RESET}"
}

# Function to print info
print_info() {
    echo -e "${BLUE}ℹ $1${RESET}"
}

# Function to print warning
print_warning() {
    echo -e "${YELLOW}⚠ $1${RESET}"
}

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to prompt user for yes/no
prompt_yes_no() {
    local prompt="$1"
    local default="${2:-n}"
    local response
    
    if [ "$default" = "y" ]; then
        read -p "$(echo -e "${CYAN}$prompt [Y/n]: ${RESET}")" response
        response="${response:-y}"
    else
        read -p "$(echo -e "${CYAN}$prompt [y/N]: ${RESET}")" response
        response="${response:-n}"
    fi
    
    case "$response" in
        [yY]|[yY][eE][sS]) return 0 ;;
        *) return 1 ;;
    esac
}

# Function to wait for user
wait_for_user() {
    echo ""
    read -p "$(echo -e "${CYAN}Press Enter to continue...${RESET}")"
}

print_section "Welcome to Flock Build and Run Guide"
echo -e "${BOLD}This script will guide you through:${RESET}"
echo "  1. Checking prerequisites"
echo "  2. Setting up vcpkg"
echo "  3. Building the project"
echo "  4. Running DuckDB with Flock extension"
echo ""

if ! prompt_yes_no "Would you like to continue?" "y"; then
    echo "Exiting..."
    exit 0
fi

# Step 1: Check Prerequisites
print_section "Step 1: Checking Prerequisites"

MISSING_DEPS=()

if ! command_exists cmake; then
    print_error "CMake is not installed"
    MISSING_DEPS+=("cmake")
else
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    print_success "CMake is installed (version: $CMAKE_VERSION)"
fi

# Detect build system
GENERATOR=""
if command_exists ninja; then
    print_success "Ninja is installed"
    GENERATOR="Ninja"
elif command_exists make; then
    print_success "Make is installed"
    GENERATOR="Unix Makefiles"
else
    print_error "Neither Ninja nor Make is installed"
    MISSING_DEPS+=("ninja or make")
fi

# Check for C/C++ compilers
if ! command_exists gcc && ! command_exists clang && ! command_exists cc; then
    print_warning "C compiler not found (gcc/clang/cc)"
    print_info "On macOS, install Xcode Command Line Tools: xcode-select --install"
    print_info "On Linux, install build-essential or equivalent"
elif command_exists clang; then
    CLANG_VERSION=$(clang --version | head -n1)
    print_success "C/C++ compiler found: $CLANG_VERSION"
elif command_exists gcc; then
    GCC_VERSION=$(gcc --version | head -n1)
    print_success "C/C++ compiler found: $GCC_VERSION"
else
    print_success "C/C++ compiler found"
fi

if ! command_exists git; then
    print_error "Git is not installed"
    MISSING_DEPS+=("git")
else
    print_success "Git is installed"
fi

if ! command_exists python3; then
    print_warning "Python3 is not installed (optional, needed for integration tests)"
else
    PYTHON_VERSION=$(python3 --version | cut -d' ' -f2)
    print_success "Python3 is installed (version: $PYTHON_VERSION)"
fi

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    print_error "Missing required dependencies: ${MISSING_DEPS[*]}"
    echo "Please install the missing dependencies before continuing."
    exit 1
fi

wait_for_user

# Step 2: Setup vcpkg
print_section "Step 2: Setting up vcpkg"

if [ -d "vcpkg" ]; then
    print_info "vcpkg directory already exists"
    if prompt_yes_no "Do you want to re-run vcpkg setup?" "n"; then
        print_info "Running vcpkg setup..."
        bash scripts/setup_vcpkg.sh
    else
        print_success "Skipping vcpkg setup"
    fi
else
    print_info "vcpkg directory not found. Setting up vcpkg..."
    bash scripts/setup_vcpkg.sh
fi

# Export vcpkg toolchain path
if [ -f "vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then
    export VCPKG_TOOLCHAIN_PATH="$PROJ_DIR/vcpkg/scripts/buildsystems/vcpkg.cmake"
    print_success "VCPKG_TOOLCHAIN_PATH set to: $VCPKG_TOOLCHAIN_PATH"
else
    print_error "vcpkg.cmake not found. vcpkg setup may have failed."
    exit 1
fi

wait_for_user

# Step 3: Build Configuration
print_section "Step 3: Build Configuration"

BUILD_TYPE="release"
if prompt_yes_no "Do you want to build in Debug mode? (default: Release)" "n"; then
    BUILD_TYPE="debug"
fi

print_info "Selected build type: $BUILD_TYPE"

# Check if build directory exists
if [ -d "build/$BUILD_TYPE" ]; then
    if prompt_yes_no "Build directory exists. Do you want to clean and rebuild?" "n"; then
        print_info "Cleaning build directory..."
        rm -rf "build/$BUILD_TYPE"
        print_success "Build directory cleaned"
    fi
fi

wait_for_user

# Step 4: Building
print_section "Step 4: Building the Project"

# Verify build system is available
if [ -z "$GENERATOR" ]; then
    print_error "No build system available. Please install Ninja or Make."
    print_info "Install Ninja: brew install ninja (macOS) or apt-get install ninja-build (Linux)"
    print_info "Or use Make which is usually pre-installed"
    exit 1
fi

# Verify compiler is available
if ! command_exists gcc && ! command_exists clang && ! command_exists cc; then
    print_error "No C/C++ compiler found. Cannot proceed with build."
    print_info "On macOS: xcode-select --install"
    print_info "On Linux: sudo apt-get install build-essential (Ubuntu/Debian)"
    print_info "          or: sudo yum groupinstall 'Development Tools' (RHEL/CentOS)"
    exit 1
fi

print_info "Using build system: $GENERATOR"
print_info "Starting build process..."
print_info "This may take several minutes..."

# Build command with extension configuration
BUILD_CMD=""
if [ "$BUILD_TYPE" = "debug" ]; then
    mkdir -p build/debug
    BUILD_CMD="cmake -G \"$GENERATOR\" -DCMAKE_BUILD_TYPE=Debug"
else
    mkdir -p build/release
    BUILD_CMD="cmake -G \"$GENERATOR\" -DCMAKE_BUILD_TYPE=Release"
fi

# Add extension configuration flags
BUILD_CMD="$BUILD_CMD -DEXTENSION_STATIC_BUILD=1"
BUILD_CMD="$BUILD_CMD -DDUCKDB_EXTENSION_CONFIGS=\"$PROJ_DIR/extension_config.cmake\""

# Add vcpkg configuration if available
if [ -n "$VCPKG_TOOLCHAIN_PATH" ] && [ -f "$VCPKG_TOOLCHAIN_PATH" ]; then
    BUILD_CMD="$BUILD_CMD -DVCPKG_BUILD=1"
    BUILD_CMD="$BUILD_CMD -DCMAKE_TOOLCHAIN_FILE=\"$VCPKG_TOOLCHAIN_PATH\""
    BUILD_CMD="$BUILD_CMD -DVCPKG_MANIFEST_DIR=\"$PROJ_DIR\""
fi

# Configure
if [ "$BUILD_TYPE" = "debug" ]; then
    eval "$BUILD_CMD -S duckdb -B build/debug"
    CONFIGURE_RESULT=$?
else
    eval "$BUILD_CMD -S duckdb -B build/release"
    CONFIGURE_RESULT=$?
fi

if [ $CONFIGURE_RESULT -ne 0 ]; then
    print_error "CMake configuration failed."
    print_info "Common issues:"
    echo "  • Missing C/C++ compiler - install Xcode Command Line Tools (macOS) or build-essential (Linux)"
    echo "  • Missing build system - install Ninja or Make"
    echo "  • vcpkg issues - try running: bash scripts/setup_vcpkg.sh"
    exit 1
fi

# Build
if [ "$BUILD_TYPE" = "debug" ]; then
    cmake --build build/debug --config Debug
    BUILD_RESULT=$?
else
    cmake --build build/release --config Release
    BUILD_RESULT=$?
fi

if [ $BUILD_RESULT -eq 0 ]; then
    print_success "Build completed successfully!"
else
    print_error "Build failed. Please check the error messages above."
    exit 1
fi

wait_for_user

# Step 5: Verify Build
print_section "Step 5: Verifying Build"

DUCKDB_BINARY="build/$BUILD_TYPE/duckdb"
if [ ! -f "$DUCKDB_BINARY" ]; then
    # Try alternative path for Windows
    DUCKDB_BINARY="build/$BUILD_TYPE/$BUILD_TYPE/duckdb"
    if [ ! -f "$DUCKDB_BINARY" ]; then
        print_error "DuckDB binary not found at expected location"
        print_info "Looking for duckdb binary..."
        set +e  # Temporarily disable exit on error for find command
        FOUND_BINARY=$(find build/$BUILD_TYPE -name "duckdb" -type f 2>/dev/null | head -1)
        set -e  # Re-enable exit on error
        if [ -n "$FOUND_BINARY" ] && [ -f "$FOUND_BINARY" ]; then
            DUCKDB_BINARY="$FOUND_BINARY"
            print_success "Found DuckDB binary at: $DUCKDB_BINARY"
        else
            exit 1
        fi
    fi
fi

print_success "DuckDB binary found: $DUCKDB_BINARY"

# Check for extension (could be static or loadable)
EXTENSION_PATH="build/$BUILD_TYPE/extension/flock/flock.duckdb_extension"
EXTENSION_FOUND=false

if [ -f "$EXTENSION_PATH" ]; then
    print_success "Flock extension found: $EXTENSION_PATH"
    EXTENSION_FOUND=true
else
    # Extension might be statically built into DuckDB
    print_info "Checking if extension is statically built..."
    set +e  # Temporarily disable exit on error
    FOUND_EXTENSIONS=$(find build/$BUILD_TYPE -name "flock*.duckdb_extension" 2>/dev/null)
    set -e  # Re-enable exit on error
    
    if [ -n "$FOUND_EXTENSIONS" ]; then
        EXTENSION_PATH=$(echo "$FOUND_EXTENSIONS" | head -1)
        print_success "Flock extension found: $EXTENSION_PATH"
        EXTENSION_FOUND=true
    else
        print_info "Extension file not found - it may be statically built into DuckDB"
        print_info "This is normal when EXTENSION_STATIC_BUILD=1"
        print_info "The extension should be available directly in DuckDB without loading"
        EXTENSION_PATH=""
    fi
fi

wait_for_user

# Step 6: Running DuckDB
print_section "Step 6: Running DuckDB with Flock Extension"

print_success "Build completed! Flock extension is ready to use."
echo ""

# Highlight documentation link
echo -e "${BOLD}${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
echo -e "${BOLD}${CYAN}📚 IMPORTANT: Learn how to use Flock before running DuckDB${RESET}"
echo ""
echo -e "${BOLD}${GREEN}Documentation:${RESET}"
echo -e "  ${BOLD}${CYAN}https://dais-polymtl.github.io/flock/docs/what-is-flock${RESET}"
echo ""
echo -e "${BOLD}The documentation covers:${RESET}"
echo "  • Semantic functions (llm_complete, llm_filter, llm_embedding, etc.)"
echo "  • Image support capabilities"
echo "  • Hybrid search functions"
echo "  • Structured output"
echo "  • Resource management (models and prompts)"
echo "  • Getting started with different providers (OpenAI, Azure, Ollama)"
echo ""
echo -e "${BOLD}${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
echo ""

if [ "$EXTENSION_FOUND" = true ] && [ -n "$EXTENSION_PATH" ]; then
    print_info "Extension file available at: $EXTENSION_PATH"
    print_info "You can load it with: LOAD '$EXTENSION_PATH';"
else
    print_info "Extension is statically built - it's already available in DuckDB!"
fi

echo ""
print_info "Starting interactive DuckDB session..."
print_info "The Flock extension is ready to use. Try the examples from the documentation!"
echo ""

# Start DuckDB interactively
$DUCKDB_BINARY
