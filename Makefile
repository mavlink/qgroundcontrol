
# Enforce the presence of the GIT repository
#
# We depend on our submodules, so we have to prevent attempts to
# compile without it being present.
ifeq ($(wildcard .git),)
    $(error YOU HAVE TO USE GIT TO DOWNLOAD THIS REPOSITORY. ABORTING.)
endif


#  explicity set default build target
all: linux

# Parsing
# --------------------------------------------------------------------
# assume 1st argument passed is the main target, the
# rest are arguments to pass to the makefile generated
# by cmake in the subdirectory
FIRST_ARG := $(firstword $(MAKECMDGOALS))
ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
j ?= 4

NINJA_BIN := ninja
ifndef NO_NINJA_BUILD
	NINJA_BUILD := $(shell $(NINJA_BIN) --version 2>/dev/null)

	ifndef NINJA_BUILD
		NINJA_BIN := ninja-build
		NINJA_BUILD := $(shell $(NINJA_BIN) --version 2>/dev/null)
	endif
endif

ifdef NINJA_BUILD
	PX4_CMAKE_GENERATOR := Ninja
	PX4_MAKE := $(NINJA_BIN)

	ifdef VERBOSE
		PX4_MAKE_ARGS := -v
	else
		PX4_MAKE_ARGS :=
	endif
else
	ifdef SYSTEMROOT
		# Windows
		PX4_CMAKE_GENERATOR := "MSYS\ Makefiles"
	else
		PX4_CMAKE_GENERATOR := "Unix\ Makefiles"
	endif
	PX4_MAKE = $(MAKE)
	PX4_MAKE_ARGS = -j$(j) --no-print-directory
endif

CMAKE_BUILD_TYPE ?= RelWithDebInfo

SRC_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# Functions
# --------------------------------------------------------------------
# describe how to build a cmake config
define cmake-build
+@$(eval BUILD_DIR = $(SRC_DIR)/build/$@$(BUILD_DIR_SUFFIX))
+@if [ $(PX4_CMAKE_GENERATOR) = "Ninja" ] && [ -e $(BUILD_DIR)/Makefile ]; then rm -rf $(BUILD_DIR); fi
+@if [ ! -e $(BUILD_DIR)/CMakeCache.txt ]; then mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake $(2) -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -G"$(PX4_CMAKE_GENERATOR)" -DQT_MKSPEC=$(1) || (rm -rf $(BUILD_DIR)); fi
+@(cd $(BUILD_DIR) && $(PX4_MAKE) $(PX4_MAKE_ARGS) $(ARGS))
endef


# Qt mkspec
# android_armv7  android_x86  gcc_64

gcc_64:
	$(call cmake-build,$@,$(SRC_DIR))

android_armv7:
	$(call cmake-build,$@,$(SRC_DIR))

android_x86:
	$(call cmake-build,$@,$(SRC_DIR))

clang_64:
	$(call cmake-build,$@,$(SRC_DIR))

xcode:
	@mkdir -p build/xcode; cd build/xcode; cmake -GXcode -DCMAKE_BUILD_TYPE=RelWithDebInfo $(SRC_DIR)

linux: gcc_64

android: android_armv7

mac: clang_64


# Astyle
# --------------------------------------------------------------------
.PHONY: check_format format

check_format:
	$(call colorecho,"Checking formatting with astyle")
	@$(SRC_DIR)/Tools/astyle/check_code_style_all.sh
	@cd $(SRC_DIR) && git diff --check

format:
	$(call colorecho,"Formatting with astyle")
	@$(SRC_DIR)/Tools/astyle/check_code_style_all.sh --fix

# Testing
# --------------------------------------------------------------------
.PHONY: tests tests_coverage

tests:

tests_coverage:

# Cleanup
# --------------------------------------------------------------------
.PHONY: clean submodulesclean submodulesupdate distclean

clean:
	@rm -rf $(SRC_DIR)/build

submodulesclean:
	@git submodule foreach --quiet --recursive git clean -ff -x -d
	@git submodule update --quiet --init --recursive --force || true
	@git submodule sync --recursive
	@git submodule update --init --recursive --force

submodulesupdate:
	@git submodule update --quiet --init --recursive || true
	@git submodule sync --recursive
	@git submodule update --init --recursive

distclean:
	@git submodule deinit -f .
	@git clean -ff -x -d -e ".project" -e ".cproject" -e ".idea" -e ".settings" -e ".vscode"

