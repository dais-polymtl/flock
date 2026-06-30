# Define the project directory
PROJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Configuration of extension
EXT_NAME = flock
EXT_CONFIG = ${PROJ_DIR}extension_config.cmake

# Use the local vcpkg toolchain when available so tidy-check can resolve curl/json deps.
ifneq ($(wildcard ${PROJ_DIR}vcpkg/scripts/buildsystems/vcpkg.cmake),)
VCPKG_TOOLCHAIN_PATH ?= ${PROJ_DIR}vcpkg/scripts/buildsystems/vcpkg.cmake
endif

# Include the Makefile from extension-ci-tools
include extension-ci-tools/makefiles/duckdb_extension.Makefile

# Upstream tidy-check omits VCPKG_MANIFEST_FLAGS, which flock needs for curl/nlohmann-json.
tidy-check:
	mkdir -p ./build/tidy
	cmake $(GENERATOR) $(BUILD_FLAGS) $(EXT_DEBUG_FLAGS) $(VCPKG_MANIFEST_FLAGS) -DDISABLE_UNITY=1 -DCLANG_TIDY=1 -S $(DUCKDB_SRCDIR) -B build/tidy
	cp duckdb/.clang-tidy build/tidy/.clang-tidy
	cd build/tidy && python3 ../../duckdb/scripts/run-clang-tidy.py '$(PROJ_DIR)src/.*/' -header-filter '$(PROJ_DIR)src/.*/' -quiet ${TIDY_THREAD_PARAMETER} ${TIDY_BINARY_PARAMETER} ${TIDY_PERFORM_CHECKS}

# Color codes
RESET = \033[0m
BOLD = \033[1m
GREEN = \033[32m
RED = \033[31m
YELLOW = \033[33m
CYAN = \033[36m

# Target to setup the project
lf_setup:
	@. scripts/setup_vcpkg.sh

# Target to setup the project and install pre-commit, clang-format, and cmake-format
lf_setup_dev: lf_setup
	@echo -e "$(CYAN)Starting development environment setup...$(RESET)"
	
	# Check if Python is installed
	@echo -e "$(CYAN)Checking if Python is installed...$(RESET)"
	@command -v python3 >/dev/null 2>&1 && { echo -e "$(GREEN)Python is already installed.$(RESET)"; } || { echo -e "$(RED)Python is not installed. Please install Python.$(RESET)"; exit 1; }
	
	# Install pre-commit using pip
	@echo -e "$(CYAN)Installing pre-commit using pip...$(RESET)"
	@python3 -m pip install --user pre-commit
	@echo -e "$(GREEN)pre-commit installed successfully.$(RESET)"

	# Install pre-commit hooks
	@echo -e "$(CYAN)Installing pre-commit hooks...$(RESET)"
	@pre-commit install
	@echo -e "$(GREEN)pre-commit hooks installed successfully.$(RESET)"

	# Check if clang-format is installed
	@echo -e "$(CYAN)Checking if clang-format is installed...$(RESET)"
	@command -v clang-format >/dev/null 2>&1 && { echo -e "$(GREEN)clang-format is already installed.$(RESET)"; } || { echo -e "$(YELLOW)clang-format is not installed. Installing...$(RESET)"; pip install clang-format; echo -e "$(GREEN)clang-format installed successfully.$(RESET)"; }

	# Check if cmake-format is installed
	@echo -e "$(CYAN)Checking if cmake-format is installed...$(RESET)"
	@command -v cmake-format >/dev/null 2>&1 && { echo -e "$(GREEN)cmake-format is already installed.$(RESET)"; } || { echo -e "$(YELLOW)cmake-format is not installed. Installing...$(RESET)"; pip install cmakelang; echo -e "$(GREEN)cmake-format installed successfully.$(RESET)"; }

	@echo -e "$(CYAN)Development setup complete.$(RESET)"
