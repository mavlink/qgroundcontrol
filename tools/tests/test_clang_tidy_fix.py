"""clang-tidy --fix: exported fixes are merged by clang-apply-replacements, never replayed."""

from __future__ import annotations

import json
import re
import sys
from typing import TYPE_CHECKING
from unittest.mock import patch

import analyze
import pytest
from analyzers.clang_tidy import FIX_HEADER_DIRS, ClangTidyAnalyzer
from analyzers.result_cache import ResultCache

from ._helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path

pytestmark = pytest.mark.skipif(sys.platform == "win32", reason="fake tools are POSIX scripts")

# Like clang-tidy, the fake exports fixes even when compilation fails.
FAKE_CLANG_TIDY = """
import json, pathlib, sys
arguments = sys.argv[1:]
with open(pathlib.Path(sys.argv[0]).with_suffix(".log"), "a") as log:
    log.write(json.dumps(arguments) + "\\n")
source = pathlib.Path(arguments[-1])
text = source.read_text()
for argument in arguments:
    if argument.startswith("--export-fixes=") and "NULL" in text:
        fix = {"file": str(source), "old": "NULL", "new": "nullptr"}
        pathlib.Path(argument.split("=", 1)[1]).write_text(json.dumps(fix))
if "BROKEN" in text:
    print(f"{source}:1:1: error: unknown type name 'BROKEN' [clang-diagnostic-error]")
    sys.exit(1)
if "NULL" in text:
    print(f"{source}:1:10: warning: use nullptr [modernize-use-nullptr]")
"""

FAKE_APPLY_REPLACEMENTS = """
import json, pathlib, sys
with open(pathlib.Path(sys.argv[0]).with_suffix(".log"), "a") as log:
    log.write(json.dumps(sys.argv[1:]) + "\\n")
for exported in sorted(pathlib.Path(sys.argv[-1]).glob("*.yaml")):
    fix = json.loads(exported.read_text())
    path = pathlib.Path(fix["file"])
    path.write_text(path.read_text().replace(fix["old"], fix["new"]))
"""


def _tool(path: Path, body: str) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(f"#!{sys.executable}\n{body}", encoding="utf-8")
    path.chmod(0o755)
    return path


def _calls(tool: Path) -> list[list[str]]:
    log = tool.with_suffix(".log")
    if not log.exists():
        return []
    return [json.loads(line) for line in log.read_text(encoding="utf-8").splitlines()]


@pytest.fixture
def checkout(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    (tmp_path / ".clang-tidy").write_text("Checks: modernize-use-nullptr\n", encoding="utf-8")
    sources = {"src/good.cc": "int *p = NULL;\n", "src/broken.cc": "BROKEN int *q = NULL;\n"}
    for name, text in sources.items():
        (tmp_path / name).parent.mkdir(parents=True, exist_ok=True)
        (tmp_path / name).write_text(text, encoding="utf-8")
    build = tmp_path / "build"
    build.mkdir()
    (build / "compile_commands.json").write_text(
        json.dumps(
            [
                {"directory": str(tmp_path), "file": name, "command": f"c++ -c {name}"}
                for name in sources
            ]
        ),
        encoding="utf-8",
    )
    bin_dir = tmp_path / "bin"
    _tool(bin_dir / "clang-tidy", FAKE_CLANG_TIDY)
    monkeypatch.setenv("PATH", str(bin_dir))
    return tmp_path


def test_fix_applies_fixes_from_compiled_units_without_the_result_cache(checkout: Path) -> None:
    apply_replacements = _tool(checkout / "bin/clang-apply-replacements", FAKE_APPLY_REPLACEMENTS)
    analyzer = ClangTidyAnalyzer(checkout, checkout / "build")
    analyzer.result_cache = ResultCache(checkout / "cache")
    with (
        patch("analyzers.compiler.scan_dependencies", side_effect=AssertionError("cache scan")),
        patch.object(ResultCache, "load", side_effect=AssertionError("cache replay")),
        patch.object(ResultCache, "store", side_effect=AssertionError("cache store")),
    ):
        result = analyzer.run([checkout / "src/good.cc", checkout / "src/broken.cc"], fix=True)

    assert (checkout / "src/good.cc").read_text(encoding="utf-8") == "int *p = nullptr;\n"
    assert "NULL" in (checkout / "src/broken.cc").read_text(encoding="utf-8")
    assert result.execution_error
    assert sorted(result.files_with_issues) == ["src/broken.cc", "src/good.cc"]
    assert analyzer.fixes_dir is None

    fixes_dir = checkout / "build/clang-tidy-fixes"
    assert _calls(apply_replacements) == [["--ignore-insert-conflict", str(fixes_dir)]]
    tidy_calls = _calls(checkout / "bin/clang-tidy")
    assert len(tidy_calls) == 2
    for arguments in tidy_calls:
        header_filter = next(arg for arg in arguments if arg.startswith("--header-filter="))
        pattern = re.compile(header_filter.split("=", 1)[1])
        assert pattern.match(f"{checkout.resolve().as_posix()}/src/Vehicle/Vehicle.h")
        assert not pattern.match(f"{checkout.resolve().as_posix()}/.cache/CPM/sdl3/1/src/a.h")
        assert not pattern.match(f"{checkout.resolve().as_posix()}/build/src/generated.h")
        assert any(arg.startswith(f"--export-fixes={fixes_dir}/") for arg in arguments)
    timings = json.loads((checkout / "build/clang-tidy-timings.json").read_text(encoding="utf-8"))
    assert timings["result_cache"] == {}


def test_missing_apply_replacements_fails_before_analysis(checkout: Path) -> None:
    result = ClangTidyAnalyzer(checkout, checkout / "build").run(
        [checkout / "src/good.cc"], fix=True
    )
    assert result.execution_error
    assert _calls(checkout / "bin/clang-tidy") == []
    assert "NULL" in (checkout / "src/good.cc").read_text(encoding="utf-8")


def test_apply_replacements_failure_is_an_execution_error(checkout: Path) -> None:
    _tool(checkout / "bin/clang-apply-replacements", "import sys\nsys.exit(3)\n")
    result = ClangTidyAnalyzer(checkout, checkout / "build").run(
        [checkout / "src/good.cc"], fix=True
    )
    assert result.execution_error
    assert list((checkout / "build/clang-tidy-fixes").glob("*.yaml"))


def test_apply_replacements_beside_resolved_clang_tidy_wins(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    llvm_bin = tmp_path / "llvm/bin"
    _tool(llvm_bin / "clang-tidy", "")
    beside = _tool(llvm_bin / "clang-apply-replacements", "")
    _tool(tmp_path / "other/clang-apply-replacements", "")
    (tmp_path / "bin").mkdir()
    (tmp_path / "bin/clang-tidy").symlink_to(llvm_bin / "clang-tidy")
    monkeypatch.setenv("PATH", f"{tmp_path / 'bin'}:{tmp_path / 'other'}")
    analyzer = ClangTidyAnalyzer(tmp_path, tmp_path)
    assert analyzer._apply_replacements_executable() == str(beside)


def test_fix_mode_is_never_cached(tmp_path: Path) -> None:
    analyzer = ClangTidyAnalyzer(tmp_path, tmp_path)
    assert analyzer._cacheable()
    analyzer.fixes_dir = tmp_path / "fixes"
    assert not analyzer._cacheable()


def test_fix_header_filter_anchors_the_configured_header_filter() -> None:
    config = (REPO_ROOT / ".clang-tidy").read_text(encoding="utf-8")
    match = re.search(
        r"^HeaderFilterRegex: '\^\.\*/\(([^)]*)\)/\.\*\\\.\(h\|hpp\)\$'$", config, re.M
    )
    assert match is not None
    assert tuple(match.group(1).split("|")) == FIX_HEADER_DIRS


@pytest.mark.parametrize(
    "arguments",
    [
        ["--tool", "clazy", "--fix"],
        ["--tool", "clang-tidy", "--tool", "clazy", "--fix"],
        ["--tool", "qmllint", "--fix"],
    ],
)
def test_fix_is_limited_to_clang_format_and_clang_tidy(arguments) -> None:
    with pytest.raises(SystemExit) as exc:
        analyze.parse_args(arguments)
    assert exc.value.code == 2


@pytest.mark.parametrize("variable", ["CI", "GITHUB_ACTIONS"])
def test_clang_tidy_fix_is_refused_in_ci(monkeypatch, variable: str) -> None:
    assert analyze.parse_args(["--tool", "clang-tidy", "--fix"]).fix
    monkeypatch.setenv(variable, "true")
    with pytest.raises(SystemExit) as exc:
        analyze.parse_args(["--tool", "clang-tidy", "--fix"])
    assert exc.value.code == 2
    assert analyze.parse_args(["--tool", "clang-format", "--fix"]).fix
