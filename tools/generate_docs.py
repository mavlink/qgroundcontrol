#!/usr/bin/env python3
"""
Generate API documentation using Doxygen.

Usage:
    ./tools/generate_docs.py              # Generate HTML docs
    ./tools/generate_docs.py --open       # Generate and open in browser
    ./tools/generate_docs.py --pdf        # Generate PDF (requires LaTeX)
    ./tools/generate_docs.py --clean      # Clean generated docs

Requirements:
    - doxygen
    - graphviz (for diagrams)
    - Optional: texlive (for PDF output)
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import webbrowser
from pathlib import Path

from common import find_repo_root, Logger, log_info, log_ok, log_warn, log_error

DOXYFILE_TEMPLATE = """\
# Doxyfile for QGroundControl

PROJECT_NAME           = "QGroundControl"
PROJECT_BRIEF          = "Ground Control Station for MAVLink Drones"
PROJECT_NUMBER         =
OUTPUT_DIRECTORY       = {output_dir}

# Input
INPUT                  = src
INPUT_ENCODING         = UTF-8
FILE_PATTERNS          = *.h *.cc *.cpp *.hpp *.qml *.md
RECURSIVE              = YES
EXCLUDE_PATTERNS       = */test/* */libs/* */_deps/* */build/*

# Output formats
GENERATE_HTML          = YES
HTML_OUTPUT            = html
GENERATE_LATEX         = {generate_latex}
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
HAVE_DOT               = {have_dot}
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
QUIET                  = {quiet}
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
"""


class DoxygenGenerator:
    """Generate Doxygen documentation for QGroundControl."""

    def __init__(
        self,
        repo_root: Path,
        output_dir: Path,
        *,
        generate_pdf: bool = False,
        quiet: bool = False,
    ) -> None:
        self.repo_root = repo_root.resolve()
        self.output_dir = output_dir.resolve()
        self.doxyfile = self.repo_root / "Doxyfile"
        self.generate_pdf = generate_pdf
        self.quiet = quiet
        self._has_graphviz: bool | None = None

    def check_dependencies(self) -> bool:
        """Check required dependencies, return True if all present."""
        if not shutil.which("doxygen"):
            log_error("doxygen not found. Install with: sudo apt install doxygen")
            return False

        if not shutil.which("dot"):
            log_warn("graphviz not found. Diagrams will be disabled.")
            log_info("Install with: sudo apt install graphviz")
            self._has_graphviz = False
        else:
            self._has_graphviz = True

        if self.generate_pdf and not shutil.which("pdflatex"):
            log_error(
                "pdflatex not found. Install with: sudo apt install texlive-latex-base"
            )
            return False

        return True

    def generate_doxyfile(self) -> Path:
        """Generate Doxyfile from template, return path to generated file."""
        log_info("Generating Doxyfile...")

        relative_output = self.output_dir.relative_to(self.repo_root)

        content = DOXYFILE_TEMPLATE.format(
            output_dir=relative_output,
            generate_latex="YES" if self.generate_pdf else "NO",
            have_dot="YES" if self._has_graphviz else "NO",
            quiet="YES" if self.quiet else "NO",
        )

        self.doxyfile.write_text(content)
        log_ok("Doxyfile generated")
        return self.doxyfile

    def run_doxygen(self) -> bool:
        """Run doxygen to generate documentation, return True on success."""
        log_info("Generating documentation...")

        self.output_dir.mkdir(parents=True, exist_ok=True)

        if not self.doxyfile.exists():
            self.generate_doxyfile()

        try:
            result = subprocess.run(
                ["doxygen", str(self.doxyfile)],
                cwd=self.repo_root,
                check=True,
                capture_output=self.quiet,
            )
            index_path = self.output_dir / "html" / "index.html"
            log_ok(f"Documentation generated: {index_path}")
            return result.returncode == 0
        except subprocess.CalledProcessError as e:
            log_error(f"Doxygen failed with exit code {e.returncode}")
            return False

    def generate_pdf(self) -> bool:
        """Generate PDF from LaTeX output, return True on success."""
        if not self.generate_pdf:
            return True

        log_info("Generating PDF...")
        latex_dir = self.output_dir / "latex"

        if not latex_dir.exists():
            log_error("LaTeX output not found. Run doxygen with --pdf first.")
            return False

        try:
            subprocess.run(
                ["make", "pdf"],
                cwd=latex_dir,
                check=True,
                capture_output=self.quiet,
            )
            pdf_path = latex_dir / "refman.pdf"
            log_ok(f"PDF generated: {pdf_path}")
            return True
        except subprocess.CalledProcessError as e:
            log_error(f"PDF generation failed with exit code {e.returncode}")
            return False

    def open_docs(self) -> None:
        """Open generated documentation in browser."""
        index_path = self.output_dir / "html" / "index.html"

        if not index_path.exists():
            log_error("Documentation not found. Generate first.")
            sys.exit(1)

        log_info("Opening documentation...")
        webbrowser.open(index_path.as_uri())

    def clean(self) -> None:
        """Remove generated documentation and Doxyfile."""
        log_info("Cleaning generated documentation...")

        if self.output_dir.exists():
            shutil.rmtree(self.output_dir)

        if self.doxyfile.exists():
            self.doxyfile.unlink()

        log_ok("Documentation cleaned")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate API documentation using Doxygen",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""\
Examples:
  %(prog)s                    Generate HTML documentation
  %(prog)s --open             Generate and open in browser
  %(prog)s --pdf              Generate PDF (requires LaTeX)
  %(prog)s --clean            Clean generated documentation
  %(prog)s -o /tmp/docs       Output to custom directory
""",
    )

    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="Output directory (default: docs/api)",
    )
    parser.add_argument(
        "--pdf",
        action="store_true",
        help="Generate PDF documentation (requires texlive)",
    )
    parser.add_argument(
        "--open",
        action="store_true",
        dest="open_docs",
        help="Open documentation in browser after generation",
    )
    parser.add_argument(
        "-c",
        "--clean",
        action="store_true",
        help="Clean generated documentation",
    )
    parser.add_argument(
        "-q",
        "--quiet",
        action="store_true",
        help="Suppress doxygen output",
    )
    parser.add_argument(
        "--no-color",
        action="store_true",
        help="Disable colored output",
    )

    args = parser.parse_args()

    try:
        repo_root = find_repo_root(Path(__file__).parent)
    except RuntimeError as e:
        log_error(str(e))
        return 1

    output_dir = args.output or (repo_root / "docs" / "api")

    generator = DoxygenGenerator(
        repo_root=repo_root,
        output_dir=output_dir,
        generate_pdf=args.pdf,
        quiet=args.quiet,
    )

    if args.clean:
        generator.clean()
        return 0

    if not generator.check_dependencies():
        return 1

    if not generator.run_doxygen():
        return 1

    if args.pdf and not generator.generate_pdf():
        return 1

    if args.open_docs:
        generator.open_docs()

    return 0


if __name__ == "__main__":
    sys.exit(main())
