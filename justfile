# QGroundControl Development Commands
# Wrapper for Makefile - install: cargo install just, brew install just, or apt install just

# Default: show available commands
default:
    @just --list

# Show Makefile help
help:
    @make help

# Initialize git submodules
submodules:
    make submodules

# Configure CMake build
configure:
    make configure

# Build the project
build:
    make build

# Run unit tests
test:
    make test

# Clean build directory
clean:
    make clean

# Run pre-commit checks
lint:
    make lint

# Format C++ code
format:
    make format

# Build with Docker (Ubuntu)
docker:
    make docker

# Full rebuild
rebuild: clean configure build

# Override build type: just release-build
release-build:
    make configure BUILD_TYPE=Release
    make build BUILD_TYPE=Release
