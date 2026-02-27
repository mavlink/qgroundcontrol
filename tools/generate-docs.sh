#!/usr/bin/env bash
#
# Generate API documentation using Doxygen
#
# Usage:
#   ./tools/generate-docs.sh              # Generate HTML docs
#   ./tools/generate-docs.sh --open       # Generate and open in browser
#   ./tools/generate-docs.sh --pdf        # Generate PDF (requires LaTeX)
#   ./tools/generate-docs.sh --clean      # Clean generated docs
#
# Requirements:
#   - doxygen
#   - graphviz (for diagrams)
#   - Optional: texlive (for PDF output)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info()  { echo -e "${BLUE}[INFO]${NC} $*"; }
log_ok()    { echo -e "${GREEN}[OK]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

# Defaults
OUTPUT_DIR="$REPO_ROOT/docs/api"
DOXYFILE="$REPO_ROOT/Doxyfile"
GENERATE_PDF=false
OPEN_DOCS=false
CLEAN_ONLY=false

show_help() {
    head -14 "$0" | tail -12
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            ;;
        -o|--open)
            OPEN_DOCS=true
            shift
            ;;
        --pdf)
            GENERATE_PDF=true
            shift
            ;;
        -c|--clean)
            CLEAN_ONLY=true
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

check_dependencies() {
    if ! command -v doxygen &> /dev/null; then
        log_error "doxygen not found. Install with: sudo apt install doxygen"
        exit 1
    fi

    if ! command -v dot &> /dev/null; then
        log_warn "graphviz not found. Diagrams will be disabled."
        log_info "Install with: sudo apt install graphviz"
    fi

    if [[ "$GENERATE_PDF" == true ]] && ! command -v pdflatex &> /dev/null; then
        log_error "pdflatex not found. Install with: sudo apt install texlive-latex-base"
        exit 1
    fi
}

clean_docs() {
    log_info "Cleaning generated documentation..."
    rm -rf "$OUTPUT_DIR"
    log_ok "Documentation cleaned"
}

generate_doxyfile() {
    log_info "Generating Doxyfile..."

    cat > "$DOXYFILE" << 'DOXYFILE_CONTENT'
# Doxyfile for QGroundControl

PROJECT_NAME           = "QGroundControl"
PROJECT_BRIEF          = "Ground Control Station for MAVLink Drones"
PROJECT_NUMBER         =
OUTPUT_DIRECTORY       = docs/api

# Input
INPUT                  = src
INPUT_ENCODING         = UTF-8
FILE_PATTERNS          = *.h *.cc *.cpp *.hpp *.qml *.md
RECURSIVE              = YES
EXCLUDE_PATTERNS       = */test/* */libs/* */_deps/* */build/*

# Output formats
GENERATE_HTML          = YES
HTML_OUTPUT            = html
GENERATE_LATEX         = NO
GENERATE_XML           = NO

# HTML settings
HTML_COLORSTYLE_HUE    = 220
HTML_COLORSTYLE_SAT    = 100
HTML_COLORSTYLE_GAMMA  = 80
HTML_DYNAMIC_SECTIONS  = YES
DISABLE_INDEX          = NO
GENERATE_TREEVIEW      = YES
FULL_SIDEBAR           = YES

# Extraction settings
EXTRACT_ALL            = YES
EXTRACT_PRIVATE        = NO
EXTRACT_STATIC         = YES
EXTRACT_LOCAL_CLASSES  = YES

# Source browser
SOURCE_BROWSER         = YES
INLINE_SOURCES         = NO
STRIP_CODE_COMMENTS    = YES
REFERENCED_BY_RELATION = YES
REFERENCES_RELATION    = YES

# Diagrams (requires graphviz)
HAVE_DOT               = YES
DOT_NUM_THREADS        = 0
CLASS_DIAGRAMS         = YES
COLLABORATION_GRAPH    = YES
GROUP_GRAPHS           = YES
INCLUDE_GRAPH          = YES
INCLUDED_BY_GRAPH      = YES
CALL_GRAPH             = NO
CALLER_GRAPH           = NO
GRAPHICAL_HIERARCHY    = YES
DIRECTORY_GRAPH        = YES
DOT_IMAGE_FORMAT       = svg
INTERACTIVE_SVG        = YES

# Preprocessing
ENABLE_PREPROCESSING   = YES
MACRO_EXPANSION        = YES
EXPAND_ONLY_PREDEF     = NO
PREDEFINED             = Q_OBJECT Q_PROPERTY Q_INVOKABLE Q_SIGNAL Q_SLOT

# Warnings
QUIET                  = NO
WARNINGS               = YES
WARN_IF_UNDOCUMENTED   = NO
WARN_IF_DOC_ERROR      = YES

# Other
ALPHABETICAL_INDEX     = YES
GENERATE_TODOLIST      = YES
GENERATE_BUGLIST       = YES
SHOW_USED_FILES        = YES
SHOW_FILES             = YES
SHOW_NAMESPACES        = YES
DOXYFILE_CONTENT

    log_ok "Doxyfile generated"
}

generate_docs() {
    log_info "Generating documentation..."

    mkdir -p "$OUTPUT_DIR"

    # Generate Doxyfile if it doesn't exist
    if [[ ! -f "$DOXYFILE" ]]; then
        generate_doxyfile
    fi

    # Enable LaTeX if PDF requested
    if [[ "$GENERATE_PDF" == true ]]; then
        sed -i 's/GENERATE_LATEX.*= NO/GENERATE_LATEX = YES/' "$DOXYFILE"
    fi

    # Run doxygen
    cd "$REPO_ROOT"
    doxygen "$DOXYFILE"

    log_ok "Documentation generated: $OUTPUT_DIR/html/index.html"

    # Generate PDF if requested
    if [[ "$GENERATE_PDF" == true ]]; then
        log_info "Generating PDF..."
        cd "$OUTPUT_DIR/latex"
        make pdf
        log_ok "PDF generated: $OUTPUT_DIR/latex/refman.pdf"
    fi
}

open_docs() {
    local index="$OUTPUT_DIR/html/index.html"
    if [[ ! -f "$index" ]]; then
        log_error "Documentation not found. Generate first."
        exit 1
    fi

    log_info "Opening documentation..."

    if command -v xdg-open &> /dev/null; then
        xdg-open "$index"
    elif command -v open &> /dev/null; then
        open "$index"
    else
        log_warn "Could not open browser. Docs at: $index"
    fi
}

# Main
cd "$REPO_ROOT"

if [[ "$CLEAN_ONLY" == true ]]; then
    clean_docs
    exit 0
fi

check_dependencies
generate_docs

if [[ "$OPEN_DOCS" == true ]]; then
    open_docs
fi
