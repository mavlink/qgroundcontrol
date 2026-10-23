"""Tests for tools/setup/qt_installer/compare.py."""

from __future__ import annotations

import sys
from typing import TYPE_CHECKING

import pytest
from setup.qt_installer.compare import compare_trees, render_markdown

from ._helpers import write_files

if TYPE_CHECKING:
    from pathlib import Path

pytestmark = pytest.mark.windows

POSIX_ONLY = pytest.mark.skipif(sys.platform == "win32", reason="POSIX modes and symlinks")
ARCH_ROOT = "6.12.0/gcc_64"


def _installs(tmp_path: Path, files: dict[str, str]) -> tuple[Path, Path]:
    """Write *files* into an aqt and a qgc output directory; ``{root}`` becomes each one."""
    roots = tmp_path / "aqt" / "Qt", tmp_path / "qgc" / "Qt"
    for root in roots:
        write_files(
            root / ARCH_ROOT,
            {name: text.replace("{root}", str(root)) for name, text in files.items()},
        )
    return roots


def _unexpected(reference: Path, candidate: Path) -> dict[str, list[str]]:
    result = compare_trees(reference, candidate, ARCH_ROOT)
    return {name: items for name, items in result.unexpected.items() if items}


def test_identical_trees_pass_with_the_installer_records(tmp_path: Path) -> None:
    aqt, qgc = _installs(tmp_path, {"bin/qmake": "x", "lib/libQt6Core.so": "core"})
    write_files(qgc / ARCH_ROOT, {".qgc-modules": "\n", ".qgc-qt-manifest.json": "{}\n"})

    result = compare_trees(aqt, qgc, ARCH_ROOT)

    assert result.ok
    assert result.entries == 4  # bin, lib and their files
    assert result.known["installer records"] == [".qgc-modules", ".qgc-qt-manifest.json"]


def test_install_root_differences_are_known(tmp_path: Path) -> None:
    aqt, qgc = _installs(
        tmp_path,
        {
            "bin/qmake": "{root}/6.12.0/gcc_64/bin/qmake6 $*\n",
            "lib/pkgconfig/Qt6Core.pc": "prefix={root}/6.12.0/gcc_64\nlibdir=${prefix}/lib\n",
        },
    )

    result = compare_trees(aqt, qgc, ARCH_ROOT)

    assert result.ok
    assert result.known["install root in text"] == ["bin/qmake", "lib/pkgconfig/Qt6Core.pc"]


def test_pkgconfig_prefix_lines_aqt_leaves_unpatched_are_known(tmp_path: Path) -> None:
    aqt, qgc = _installs(tmp_path, {"lib/pkgconfig/Qt6Core.pc": "prefix=/home/qt/work/install\n"})
    write_files(qgc / ARCH_ROOT, {"lib/pkgconfig/Qt6Core.pc": f"prefix={qgc}/6.12.0/gcc_64\n"})

    assert compare_trees(aqt, qgc, ARCH_ROOT).ok


def test_other_differences_fail(tmp_path: Path) -> None:
    aqt, qgc = _installs(
        tmp_path,
        {
            "bin/qmake": "x",
            "lib/pkgconfig/Qt6Core.pc": "prefix=/p\nLibs: -lQt6Core\n",
            "lib/only-aqt.so": "",
            "doc/file-or-dir": "",
        },
    )
    write_files(qgc / ARCH_ROOT, {"bin/qmake": "y", "lib/pkgconfig/Qt6Core.pc": "prefix=/q\n"})
    (qgc / ARCH_ROOT / "lib/only-aqt.so").unlink()
    (qgc / ARCH_ROOT / "doc/file-or-dir").unlink()
    write_files(qgc / ARCH_ROOT, {"doc/file-or-dir/x": "", "lib/extra.so": ""})

    assert _unexpected(aqt, qgc) == {
        "only in aqt": ["lib/only-aqt.so"],
        "only in qgc": ["doc/file-or-dir/x", "lib/extra.so"],
        "type": ["doc/file-or-dir: file vs dir"],
        "content": ["bin/qmake", "lib/pkgconfig/Qt6Core.pc"],
    }


@POSIX_ONLY
def test_modes_and_symlink_targets_must_match(tmp_path: Path) -> None:
    aqt, qgc = _installs(tmp_path, {"bin/tool": "", "lib/A/x": "", "lib/B/x": ""})
    (qgc / ARCH_ROOT / "bin/tool").chmod(0o755)
    (aqt / ARCH_ROOT / "bin/tool").chmod(0o644)
    (aqt / ARCH_ROOT / "lib/Current").symlink_to("A")
    (qgc / ARCH_ROOT / "lib/Current").symlink_to("B")

    assert _unexpected(aqt, qgc) == {
        "mode": ["bin/tool: 644 vs 755"],
        "symlink": ["lib/Current: A vs B"],
    }


def test_report_lists_each_difference(tmp_path: Path) -> None:
    aqt, qgc = _installs(tmp_path, {"bin/qmake": "x"})
    write_files(qgc / ARCH_ROOT, {"bin/qmake": "y", ".qgc-modules": "\n"})

    report = render_markdown("Qt 6.12.0 linux_gcc_64", compare_trees(aqt, qgc, ARCH_ROOT))

    assert report.startswith("### Qt installer parity: Qt 6.12.0 linux_gcc_64\n")
    assert "| Result | FAIL |" in report
    assert "| content | unexpected | 1 |" in report
    assert "| installer records | known | 1 |" in report
    assert "<details><summary>unexpected: content</summary>\n\n- `bin/qmake`\n" in report


def test_report_truncates_long_lists(tmp_path: Path) -> None:
    aqt, qgc = _installs(tmp_path, {f"lib/f{index:02}": "x" for index in range(60)})
    write_files(qgc / ARCH_ROOT, {f"lib/f{index:02}": "y" for index in range(60)})

    report = render_markdown("t", compare_trees(aqt, qgc, ARCH_ROOT))

    assert "- `lib/f49`" in report
    assert "- `lib/f50`" not in report
    assert "- …and 10 more" in report
