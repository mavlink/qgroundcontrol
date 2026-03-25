# QGroundControl Development Makefile
# Convenience wrapper for common development commands

.PHONY: help configure build test clean lint format analyze coverage docs docker submodules deps run check release

# Default target
help:
	@echo "QGroundControl Development Commands"
	@echo ""
	@echo "Setup:"
	@echo "  make deps       - Install system dependencies (Debian/Ubuntu)"
	@echo "  make submodules - Initialize git submodules"
	@echo ""
	@echo "Build:"
	@echo "  make configure  - Configure CMake build (Debug)"
	@echo "  make build      - Build the project"
	@echo "  make release    - Configure and build Release"
	@echo "  make clean      - Remove build directory"
	@echo "  make rebuild    - Clean, configure, and build"
	@echo ""
	@echo "Quality:"
	@echo "  make test       - Run unit tests"
	@echo "  make lint       - Run pre-commit checks"
	@echo "  make format     - Format code (check mode)"
	@echo "  make format-fix - Format code (apply fixes)"
	@echo "  make analyze    - Run static analysis"
	@echo "  make coverage   - Generate coverage report"
	@echo "  make check      - Run lint + test"
	@echo ""
	@echo "Other:"
	@echo "  make run        - Launch QGroundControl"
	@echo "  make docs       - Build documentation"
	@echo "  make docker     - Build using Docker (Ubuntu)"
	@echo ""
	@echo "Environment variables:"
	@echo "  QT_DIR     - Qt installation directory (default: ~/Qt/$(QT_VERSION)/gcc_64)"
	@echo "  BUILD_TYPE - CMake build type (default: Debug)"
	@echo ""
	@echo "Configuration from .github/build-config.json:"
	@echo "  Qt version: $(QT_VERSION)"

# Configuration - reads from centralized config
QT_VERSION := $(shell python3 ./tools/setup/read_config.py --get qt_version 2>/dev/null || echo "6.10.1")
QT_DIR ?= $(HOME)/Qt/$(QT_VERSION)/gcc_64
BUILD_TYPE ?= Debug
BUILD_DIR := build

# Setup targets
submodules:
	git submodule update --init --recursive

deps:
	@echo "Installing dependencies (requires sudo)..."
	python3 ./tools/setup/install_dependencies.py --platform debian

# Build targets
configure: submodules
	python3 ./tools/configure.py -B $(BUILD_DIR) -t $(BUILD_TYPE) --testing

build:
	cmake --build $(BUILD_DIR) --config $(BUILD_TYPE) --parallel

release:
	python3 ./tools/configure.py -B $(BUILD_DIR) --release
	cmake --build $(BUILD_DIR) --config Release --parallel

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean configure build

# Quality targets
test:
	cd $(BUILD_DIR) && ctest --output-on-failure

lint:
	pre-commit run --all-files

format:
	python3 ./tools/analyze.py --tool clang-format

format-fix:
	python3 ./tools/analyze.py --tool clang-format --fix

analyze:
	python3 ./tools/analyze.py

coverage:
	python3 ./tools/coverage.py

check: lint test

# Other targets
run:
	./$(BUILD_DIR)/$(BUILD_TYPE)/QGroundControl

docs:
	npm run docs:build

docker:
	./deploy/docker/run-docker-ubuntu.sh
