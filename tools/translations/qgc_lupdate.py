#!/usr/bin/env python3
"""Update Qt and JSON translation files (lupdate + JSON extractor + pseudo-loc).

Run from the repository root:
    python3 tools/translations/qgc_lupdate.py

Honors QT_ROOT_DIR in CI; otherwise auto-detects lupdate via build-config.json.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

_TOOLS_DIR = Path(__file__).resolve().parents[1]
if str(_TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(_TOOLS_DIR))

from common.build_config import get_build_config_value  # noqa: E402

REPO_ROOT = _TOOLS_DIR.parent
QT_INSTALL_BASES = (Path.home() / "Qt", Path("/opt/Qt"))
QT_ARCH_CANDIDATES = ("gcc_64", "clang_64", "linux_gcc_64")

_TR_NO_CONTEXT_MARKER = "tr() cannot be called without context"
_NO_QOBJECT_MARKER = "lacks Q_OBJECT macro"


def resolve_lupdate() -> Path:
    """Return path to lupdate, preferring $QT_ROOT_DIR then standard Qt installs."""
    if qt_root := os.environ.get("QT_ROOT_DIR"):
        candidate = Path(qt_root) / "bin" / "lupdate"
        if candidate.is_file():
            return candidate

    qt_version = get_build_config_value("qt.version")
    if qt_version:
        for base in QT_INSTALL_BASES:
            for arch in QT_ARCH_CANDIDATES:
                candidate = base / qt_version / arch / "bin" / "lupdate"
                if candidate.is_file():
                    return candidate

    if found := shutil.which("lupdate"):
        return Path(found)

    raise FileNotFoundError("lupdate not found; set QT_ROOT_DIR or install Qt tools")


def collect_lupdate_errors(output: str) -> list[str]:
    """Return human-readable error blocks for tr()-without-context / missing Q_OBJECT."""
    errors: list[str] = []
    no_context = [line for line in output.splitlines() if _TR_NO_CONTEXT_MARKER in line]
    if no_context:
        errors.append(
            "tr() called without a translation context:\n"
            + "\n".join(no_context)
            + "\n  Fix: Replace QT_TR_NOOP(\"...\") with QT_TRANSLATE_NOOP(\"ClassName\", \"...\")\n"
            "       Replace tr(\"...\") with QCoreApplication::translate(\"ClassName\", \"...\")"
        )
    no_qobject = [line for line in output.splitlines() if _NO_QOBJECT_MARKER in line]
    if no_qobject:
        errors.append(
            "Class uses tr() but is missing Q_OBJECT macro:\n"
            + "\n".join(no_qobject)
            + "\n  Fix: Add Q_OBJECT to the class declaration in the .h file,"
            "\n       or replace tr(\"...\") with QCoreApplication::translate(\"ClassName\", \"...\")"
        )
    return errors


def run_lupdate(lupdate: Path) -> str:
    """Run lupdate against src/, return combined stdout+stderr."""
    print(f"Using lupdate: {lupdate}")
    result = subprocess.run(
        [str(lupdate), "src", "-ts", "translations/qgc.ts", "-no-obsolete"],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    output = (result.stdout or "") + (result.stderr or "")
    if output:
        print(output, end="" if output.endswith("\n") else "\n")
    if result.returncode != 0:
        raise subprocess.CalledProcessError(result.returncode, [str(lupdate)], output)
    return output


def run_pseudo_loc() -> None:
    """Invoke pseudo_loc.py as a subprocess (uses hyphens? no — has its own CLI flags)."""
    subprocess.run([sys.executable, str(_TOOLS_DIR / "pseudo_loc.py")], cwd=REPO_ROOT, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()

    try:
        lupdate = resolve_lupdate()
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    try:
        output = run_lupdate(lupdate)
    except subprocess.CalledProcessError as e:
        print(f"Error: lupdate failed with exit code {e.returncode}", file=sys.stderr)
        return 1

    if errors := collect_lupdate_errors(output):
        print("", file=sys.stderr)
        for block in errors:
            print(block, file=sys.stderr)
            print("", file=sys.stderr)
        return 1

    print("Extracting JSON strings...")
    import qgc_lupdate_json  # sibling module: tools/translations/ is on sys.path
    qgc_lupdate_json.main()

    print("Generating pseudo-localization files...")
    run_pseudo_loc()

    print("Translation files updated")
    return 0


if __name__ == "__main__":
    sys.exit(main())
