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
QT_VERSION := $(shell ./tools/setup/read-config.sh qt_version 2>/dev/null || echo "6.10.1")
QT_DIR ?= $(HOME)/Qt/$(QT_VERSION)/gcc_64
BUILD_TYPE ?= Debug
BUILD_DIR := build

# Setup targets
submodules:
	git submodule update --init --recursive

deps:
	@echo "Installing dependencies (requires sudo)..."
	sudo ./tools/setup/install-dependencies-debian.sh

# Build targets
configure: submodules
	$(QT_DIR)/bin/qt-cmake -B $(BUILD_DIR) -G Ninja \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DQGC_BUILD_TESTING=ON

build:
	cmake --build $(BUILD_DIR) --config $(BUILD_TYPE) --parallel

release:
	$(QT_DIR)/bin/qt-cmake -B $(BUILD_DIR) -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DQGC_BUILD_TESTING=OFF
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
	./tools/analyze.sh --tool clang-format

format-fix:
	./tools/analyze.sh --tool clang-format --fix

analyze:
	./tools/analyze.sh

coverage:
	./tools/coverage.sh

check: lint test

# Other targets
run:
	./$(BUILD_DIR)/staging/QGroundControl

docs:
	npm run docs:build

docker:
	./deploy/docker/run-docker-ubuntu.sh
