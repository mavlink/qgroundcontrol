# QGroundControl Development Makefile
# Convenience wrapper for common commands

.PHONY: help configure build test clean lint format docker submodules

# Default target
help:
	@echo "QGroundControl Development Commands"
	@echo ""
	@echo "  make configure  - Configure CMake build (Debug)"
	@echo "  make build      - Build the project"
	@echo "  make test       - Run unit tests"
	@echo "  make clean      - Remove build directory"
	@echo "  make lint       - Run pre-commit checks"
	@echo "  make format     - Format C++ code with clang-format"
	@echo "  make docker     - Build using Docker (Ubuntu)"
	@echo "  make submodules - Initialize git submodules"
	@echo ""
	@echo "Environment variables:"
	@echo "  QT_DIR     - Qt installation directory (default: ~/Qt/<version>/gcc_64)"
	@echo "  BUILD_TYPE - CMake build type (default: Debug)"
	@echo ""
	@echo "Configuration from .github/build-config.json:"
	@echo "  Qt version: $(QT_VERSION)"

# Configuration - reads Qt version from centralized config
QT_VERSION := $(shell ./tools/setup/read-config.sh qt_version 2>/dev/null || echo "6.10.1")
QT_DIR ?= $(HOME)/Qt/$(QT_VERSION)/gcc_64
BUILD_TYPE ?= Debug
BUILD_DIR := build

submodules:
	git submodule update --init --recursive

configure: submodules
	$(QT_DIR)/bin/qt-cmake -B $(BUILD_DIR) -G Ninja -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build:
	cmake --build $(BUILD_DIR) --config $(BUILD_TYPE) --parallel

test:
	./$(BUILD_DIR)/$(BUILD_TYPE)/QGroundControl --unittest

clean:
	rm -rf $(BUILD_DIR)

lint:
	pre-commit run --all-files

format:
	find src -name "*.h" -o -name "*.cc" -o -name "*.cpp" | xargs clang-format -i

docker:
	./deploy/docker/run-docker-ubuntu.sh
