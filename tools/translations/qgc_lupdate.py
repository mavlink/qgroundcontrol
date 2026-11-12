#!/usr/bin/env python3
"""Update Qt and JSON translation files (lupdate + JSON extractor + pseudo-loc).

Run from the repository root:
    python3 tools/translations/qgc_lupdate.py
    python3 tools/translations/qgc_lupdate.py --check  # only report tr() calls lupdate rejects

Honors QT_ROOT_DIR in CI; otherwise auto-detects lupdate via build-config.json.
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from common.build_config import get_build_config_value
from common.paths import REPO_ROOT
from common.qt import find_qt_tool

from translations import pseudo_loc, qgc_lupdate_json

_TR_NO_CONTEXT_MARKER = "tr() cannot be called without context"
_NO_QOBJECT_MARKER = "lacks Q_OBJECT macro"
_TR_ON_OBJECT_MARKER = "Cannot invoke tr() like this"
_TS_FILE = Path("translations/qgc.ts")


def resolve_lupdate() -> Path:
    """Return lupdate from $QT_ROOT_DIR, the configured Qt version's standard install, or PATH."""
    if found := find_qt_tool("lupdate", version=get_build_config_value("qt.version") or "*"):
        return found

    if found := shutil.which("lupdate"):
        return Path(found)

    raise FileNotFoundError("lupdate not found; set QT_ROOT_DIR or install Qt tools")


def collect_lupdate_errors(output: str) -> list[str]:
    """Return human-readable error blocks for the tr() calls lupdate rejects or cannot extract."""
    errors: list[str] = []
    no_context = [line for line in output.splitlines() if _TR_NO_CONTEXT_MARKER in line]
    if no_context:
        errors.append(
            "tr() called without a translation context:\n"
            + "\n".join(no_context)
            + '\n  Fix: Replace QT_TR_NOOP("...") with QT_TRANSLATE_NOOP("ClassName", "...")\n'
            '       Replace tr("...") with QCoreApplication::translate("ClassName", "...")'
        )
    no_qobject = [line for line in output.splitlines() if _NO_QOBJECT_MARKER in line]
    if no_qobject:
        errors.append(
            "Class uses tr() but is missing Q_OBJECT macro:\n"
            + "\n".join(no_qobject)
            + "\n  Fix: Add Q_OBJECT to the class declaration in the .h file"
            "\n       (Q_DECLARE_TR_FUNCTIONS(ClassName) when the class is not a QObject),"
            '\n       or replace tr("...") with QCoreApplication::translate("ClassName", "...")'
        )
    on_object = [line for line in output.splitlines() if _TR_ON_OBJECT_MARKER in line]
    if on_object:
        errors.append(
            "tr() called through an object, so lupdate does not extract the string:\n"
            + "\n".join(on_object)
            + '\n  Fix: Replace object->tr("...") with ClassName::tr("...")'
            '\n       or QCoreApplication::translate("ClassName", "...")'
        )
    return errors


def run_lupdate(lupdate: Path, ts_file: Path = _TS_FILE) -> str:
    """Run lupdate against src/ into ts_file, return combined stdout+stderr."""
    print(f"Using lupdate: {lupdate}")
    result = subprocess.run(
        [str(lupdate), "src", "-ts", str(ts_file), "-no-obsolete"],
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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="Only report tr() calls lupdate cannot extract; leave the translation files untouched",
    )
    args = parser.parse_args()

    try:
        lupdate = resolve_lupdate()
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    try:
        if args.check:
            # lupdate has no dry run, so it extracts into a scratch catalog.
            with tempfile.TemporaryDirectory(prefix="qgc-lupdate-") as scratch_dir:
                output = run_lupdate(lupdate, Path(scratch_dir) / _TS_FILE.name)
        else:
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

    if args.check:
        print("lupdate reported no tr() call it cannot extract")
        return 0

    print("Extracting JSON strings...")
    qgc_lupdate_json.main()

    print("Generating pseudo-localization files...")
    pseudo_loc.main()

    print("Translation files updated")
    return 0


if __name__ == "__main__":
    sys.exit(main())
