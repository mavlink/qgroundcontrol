#!/usr/bin/env python3
"""Generate API documentation using Doxygen."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from _bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.logging import log_error, log_info, log_ok, log_warn

DEFAULT_DOXYFILE = """# Doxyfile for QGroundControl

PROJECT_NAME           = "QGroundControl"
PROJECT_BRIEF          = "Ground Control Station for MAVLink Drones"
OUTPUT_DIRECTORY       = docs/api
INPUT                  = src
INPUT_ENCODING         = UTF-8
FILE_PATTERNS          = *.h *.cc *.cpp *.hpp *.qml *.md
RECURSIVE              = YES
EXCLUDE_PATTERNS       = */test/* */libs/* */_deps/* */build/*
GENERATE_HTML          = YES
HTML_OUTPUT            = html
GENERATE_LATEX         = NO
GENERATE_XML           = NO
HAVE_DOT               = YES
QUIET                  = NO
WARNINGS               = YES
"""


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(description="Generate QGroundControl API documentation.")
    parser.add_argument("-o", "--open", dest="open_docs", action="store_true", help="Open docs after generating")
    parser.add_argument("--pdf", action="store_true", help="Generate PDF output")
    parser.add_argument("-c", "--clean", action="store_true", help="Clean generated docs")
    parser.add_argument("--output-dir", default="docs/api", help="Output directory (default: docs/api)")
    parser.add_argument("--doxyfile", default="Doxyfile", help="Path to Doxyfile (default: Doxyfile)")
    parser.add_argument("--check-deps", action="store_true", help="Check required tools, then exit")
    return parser.parse_args(argv)


def build_doxyfile_text(base_text: str, output_dir: Path, *, generate_pdf: bool) -> str:
    """Return a Doxygen config with safe per-run overrides appended."""
    latex_value = "YES" if generate_pdf else "NO"
    override = (
        "\n"
        f'OUTPUT_DIRECTORY       = "{output_dir.as_posix()}"\n'
        f"GENERATE_LATEX         = {latex_value}\n"
    )
    return base_text.rstrip() + override


def check_dependencies(*, generate_pdf: bool) -> None:
    """Ensure the required tooling is installed."""
    if shutil.which("doxygen") is None:
        raise RuntimeError("doxygen not found. Install with: sudo apt install doxygen")
    if shutil.which("dot") is None:
        log_warn("graphviz not found. Diagrams will be disabled.")
    if generate_pdf and shutil.which("pdflatex") is None:
        raise RuntimeError("pdflatex not found. Install with: sudo apt install texlive-latex-base")


def clean_docs(output_dir: Path) -> None:
    """Remove generated documentation."""
    log_info("Cleaning generated documentation...")
    shutil.rmtree(output_dir, ignore_errors=True)
    log_ok("Documentation cleaned")


def load_base_doxyfile(doxyfile_path: Path) -> str:
    """Load the repo Doxyfile or fall back to a generated default."""
    if doxyfile_path.exists():
        return doxyfile_path.read_text(encoding="utf-8")
    log_warn(f"{doxyfile_path.name} not found. Using generated default configuration.")
    return DEFAULT_DOXYFILE


def generate_docs(repo_root: Path, output_dir: Path, doxyfile_path: Path, *, generate_pdf: bool) -> None:
    """Generate HTML docs and optionally PDF docs without mutating tracked files."""
    log_info("Generating documentation...")
    output_dir.mkdir(parents=True, exist_ok=True)

    base_text = load_base_doxyfile(doxyfile_path)
    effective_text = build_doxyfile_text(base_text, output_dir, generate_pdf=generate_pdf)

    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".doxyfile", delete=False) as handle:
        handle.write(effective_text)
        temp_doxyfile = Path(handle.name)

    try:
        subprocess.run(["doxygen", str(temp_doxyfile)], cwd=repo_root, check=True)
    finally:
        temp_doxyfile.unlink(missing_ok=True)

    log_ok(f"Documentation generated: {output_dir / 'html' / 'index.html'}")

    if generate_pdf:
        latex_dir = output_dir / "latex"
        log_info("Generating PDF...")
        subprocess.run(["make", "pdf"], cwd=latex_dir, check=True)
        log_ok(f"PDF generated: {latex_dir / 'refman.pdf'}")


def open_docs(output_dir: Path) -> None:
    """Open the generated HTML documentation."""
    index = output_dir / "html" / "index.html"
    if not index.exists():
        raise FileNotFoundError(f"Documentation not found: {index}")

    log_info("Opening documentation...")
    opener = shutil.which("xdg-open") or shutil.which("open")
    if opener is None:
        raise RuntimeError(f"Could not find a browser opener. Docs at: {index}")
    subprocess.run([opener, str(index)], check=False)


def main(argv: list[str] | None = None) -> int:
    """Run the requested documentation workflow."""
    args = parse_args(argv)

    if args.check_deps:
        from common.deps import check_and_report
        check_and_report(["doxygen"])
        return 0

    repo_root = Path(__file__).resolve().parent.parent
    output_dir = Path(args.output_dir)
    doxyfile_path = Path(args.doxyfile)
    if not output_dir.is_absolute():
        output_dir = repo_root / output_dir
    if not doxyfile_path.is_absolute():
        doxyfile_path = repo_root / doxyfile_path

    try:
        if args.clean:
            clean_docs(output_dir)
            return 0

        check_dependencies(generate_pdf=args.pdf)
        generate_docs(repo_root, output_dir, doxyfile_path, generate_pdf=args.pdf)
        if args.open_docs:
            open_docs(output_dir)
        return 0
    except (RuntimeError, FileNotFoundError, subprocess.CalledProcessError) as exc:
        log_error(str(exc))
        return 1


if __name__ == "__main__":
    sys.exit(main())
