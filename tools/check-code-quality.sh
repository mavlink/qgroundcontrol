#!/bin/bash
# Code quality checking script for QGroundControl
# Runs various linters and formatters in check mode

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

ERRORS=0
WARNINGS=0

echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}QGroundControl Code Quality Checks${NC}"
echo -e "${BLUE}======================================${NC}"
echo ""

# Check if we're in the root directory
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}ERROR: Must be run from repository root${NC}"
    exit 1
fi

# Function to check command
check_command() {
    if command -v "$1" >/dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

# Parse arguments
CHECK_FORMAT=true
CHECK_CMAKE=true
CHECK_YAML=true
CHECK_CLANG_TIDY=false
FIX_ISSUES=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --fix)
            FIX_ISSUES=true
            shift
            ;;
        --clang-tidy)
            CHECK_CLANG_TIDY=true
            shift
            ;;
        --format-only)
            CHECK_CMAKE=false
            CHECK_YAML=false
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --fix           Fix issues automatically"
            echo "  --clang-tidy    Run clang-tidy (slow)"
            echo "  --format-only   Only check code formatting"
            echo "  -h, --help      Show this help"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# 1. Check C++ formatting with clang-format
if [ "$CHECK_FORMAT" = true ] && check_command clang-format; then
    echo -e "${BLUE}[1/5] Checking C++ formatting...${NC}"

    if [ "$FIX_ISSUES" = true ]; then
        find src -name "*.cc" -o -name "*.h" | xargs clang-format -i
        echo -e "${GREEN}✓ C++ files formatted${NC}"
    else
        UNFORMATTED=$(find src -name "*.cc" -o -name "*.h" | xargs clang-format --dry-run --Werror 2>&1 | grep -c "error:" || true)
        if [ "$UNFORMATTED" -gt 0 ]; then
            echo -e "${RED}✗ Found $UNFORMATTED formatting issues${NC}"
            echo -e "  Run with ${YELLOW}--fix${NC} to auto-format"
            ERRORS=$((ERRORS + 1))
        else
            echo -e "${GREEN}✓ All C++ files properly formatted${NC}"
        fi
    fi
else
    echo -e "${YELLOW}⊘ Skipping C++ formatting check (clang-format not found)${NC}"
fi
echo ""

# 2. Check CMake formatting
if [ "$CHECK_CMAKE" = true ] && check_command cmake-format; then
    echo -e "${BLUE}[2/5] Checking CMake formatting...${NC}"

    if [ "$FIX_ISSUES" = true ]; then
        find . -name "CMakeLists.txt" -o -name "*.cmake" | grep -v "build/" | xargs cmake-format -i
        echo -e "${GREEN}✓ CMake files formatted${NC}"
    else
        if find . -name "CMakeLists.txt" -o -name "*.cmake" | grep -v "build/" | xargs cmake-format --check 2>&1 | grep -q "would be reformatted"; then
            echo -e "${RED}✗ CMake files need formatting${NC}"
            echo -e "  Run with ${YELLOW}--fix${NC} to auto-format"
            ERRORS=$((ERRORS + 1))
        else
            echo -e "${GREEN}✓ All CMake files properly formatted${NC}"
        fi
    fi
else
    echo -e "${YELLOW}⊘ Skipping CMake formatting check (cmake-format not found)${NC}"
fi
echo ""

# 3. Check YAML files
if [ "$CHECK_YAML" = true ]; then
    echo -e "${BLUE}[3/5] Checking YAML files...${NC}"

    YAML_ERRORS=0
    for file in $(find .github -name "*.yml" -o -name "*.yaml"); do
        if ! python3 -c "import yaml; yaml.safe_load(open('$file'))" 2>/dev/null; then
            echo -e "${RED}✗ Invalid YAML: $file${NC}"
            YAML_ERRORS=$((YAML_ERRORS + 1))
        fi
    done

    if [ "$YAML_ERRORS" -eq 0 ]; then
        echo -e "${GREEN}✓ All YAML files valid${NC}"
    else
        echo -e "${RED}✗ Found $YAML_ERRORS invalid YAML files${NC}"
        ERRORS=$((ERRORS + 1))
    fi
fi
echo ""

# 4. Check for common issues
echo -e "${BLUE}[4/5] Checking for common issues...${NC}"

# Check for trailing whitespace
TRAILING_WS=$(find src -name "*.cc" -o -name "*.h" | xargs grep -n "[[:space:]]$" | wc -l)
if [ "$TRAILING_WS" -gt 0 ]; then
    echo -e "${YELLOW}⚠ Found $TRAILING_WS lines with trailing whitespace${NC}"
    WARNINGS=$((WARNINGS + 1))
else
    echo -e "${GREEN}✓ No trailing whitespace${NC}"
fi

# Check for mixed line endings
CRLF_FILES=$(find src -name "*.cc" -o -name "*.h" | xargs file | grep -c "CRLF" || true)
if [ "$CRLF_FILES" -gt 0 ]; then
    echo -e "${YELLOW}⚠ Found $CRLF_FILES files with CRLF line endings${NC}"
    WARNINGS=$((WARNINGS + 1))
else
    echo -e "${GREEN}✓ All files use LF line endings${NC}"
fi

echo ""

# 5. Run clang-tidy (optional, slow)
if [ "$CHECK_CLANG_TIDY" = true ] && check_command clang-tidy; then
    echo -e "${BLUE}[5/5] Running clang-tidy (this may take a while)...${NC}"

    if [ ! -f "build/compile_commands.json" ]; then
        echo -e "${YELLOW}⚠ compile_commands.json not found${NC}"
        echo -e "  Build with: cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    else
        # Run on a subset of files for speed
        SAMPLE_FILES=$(find src/Vehicle src/FactSystem -name "*.cc" | head -5)
        TIDY_WARNINGS=0
        for file in $SAMPLE_FILES; do
            if ! clang-tidy "$file" -p build/ 2>&1 | grep -q "warning:"; then
                TIDY_WARNINGS=$((TIDY_WARNINGS + 1))
            fi
        done

        if [ "$TIDY_WARNINGS" -gt 0 ]; then
            echo -e "${YELLOW}⚠ Found warnings in sampled files${NC}"
            WARNINGS=$((WARNINGS + 1))
        else
            echo -e "${GREEN}✓ No clang-tidy warnings in sample${NC}"
        fi
    fi
else
    echo -e "${YELLOW}⊘ Skipping clang-tidy (use --clang-tidy to enable)${NC}"
fi
echo ""

# Summary
echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}Summary${NC}"
echo -e "${BLUE}======================================${NC}"

if [ "$ERRORS" -eq 0 ] && [ "$WARNINGS" -eq 0 ]; then
    echo -e "${GREEN}✓ All checks passed!${NC}"
    exit 0
elif [ "$ERRORS" -eq 0 ]; then
    echo -e "${YELLOW}⚠ $WARNINGS warning(s) found${NC}"
    exit 0
else
    echo -e "${RED}✗ $ERRORS error(s) and $WARNINGS warning(s) found${NC}"
    echo ""
    echo -e "Run with ${YELLOW}--fix${NC} to automatically fix formatting issues"
    exit 1
fi
