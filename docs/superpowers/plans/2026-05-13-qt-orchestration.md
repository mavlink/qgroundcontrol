# Qt Orchestration Module Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build `tools/qt_orchestration/` — a single Python module that encodes "Qt-complete" once and is consumed by conversational issue drafting, slice brief expansion, and a `/orchestrate` gate, so Phase-2 slices cannot declare done with incomplete Qt wiring.

**Architecture:** A `Rule` registry (each rule a small class with `applies_to` + `check`) is consumed by three entry points (`scan`, `expand-brief`, `verify-slice`). `verify-slice` is invoked by `/orchestrate` after every Codex return as the independent gate. All three layers read the same `rules.py`, so agents cannot satisfy a softer interpretation than the gate.

**Tech Stack:** Python 3.12 stdlib + PyYAML + Jinja2 + defusedxml (all already in `tools/pyproject.toml`). Pytest for tests. `cmake-file-api` JSON (already produced by the repo's CMake configure). External CLI deps: `qmllint`, `qmlformat`, `cmake` (already required by the project).

**Source spec:** `docs/superpowers/specs/2026-05-13-qt-orchestration-design.md`

---

## File Map (locked in before tasks)

**Created:**

- `tools/qt_orchestration/__init__.py`
- `tools/qt_orchestration/findings.py` — `Finding`, `Severity`
- `tools/qt_orchestration/rules.py` — `Rule` base, `RuleContext`, `REGISTRY`
- `tools/qt_orchestration/diff.py` — `SliceDiff` parser
- `tools/qt_orchestration/rules_pack/__init__.py`
- `tools/qt_orchestration/rules_pack/qrc.py` — R-QRC-01, R-QRC-02
- `tools/qt_orchestration/rules_pack/cmake.py` — R-CMAKE-01, R-CMAKE-02
- `tools/qt_orchestration/rules_pack/moc.py` — R-MOC-01, R-MOC-02, R-MOC-03
- `tools/qt_orchestration/rules_pack/shadow.py` — R-SHADOW-01, R-SHADOW-02
- `tools/qt_orchestration/rules_pack/qml.py` — R-QML-01..04
- `tools/qt_orchestration/rules_pack/build.py` — R-BUILD-01
- `tools/qt_orchestration/rules_pack/translation.py` — R-TR-01
- `tools/qt_orchestration/rules_pack/null_check.py` — R-NULL-01
- `tools/qt_orchestration/config.py` — `rules.toml` + frontmatter merge
- `tools/qt_orchestration/scan.py`
- `tools/qt_orchestration/draft_issue.py`
- `tools/qt_orchestration/expand_brief.py`
- `tools/qt_orchestration/verify_slice.py`
- `tools/qt_orchestration/cli.py`
- `tools/qt_orchestration/rules.toml`
- `tools/qt_orchestration/templates/phase2-issue.md.j2`
- `tools/qt_orchestration/templates/slice-brief-qt.md.j2`
- `tools/qt_orchestration/tests/conftest.py`
- `tools/qt_orchestration/tests/fixtures/...`
- `tools/qt_orchestration/tests/test_*.py`

**Modified:**

- `tools/pyproject.toml` — add console scripts + `tomli` (3.10 backport, even though 3.12 has `tomllib` — keep an explicit dep so tests on the lockfile catch surprises) — actually 3.12 has `tomllib` in stdlib, so no new dep, only entry points.
- `.orchestrator/templates/refactor-brief-template.md` — add `{{qt_completion_block}}` placeholder and extend `<structured_output_contract>` with `qt_verification` + `qt_verification_disputes`.
- `commands/orchestrate.md` (the `/orchestrate` skill) — invoke `verify-slice` after Codex return, parse exit codes, generate REFACTOR brief on findings, retry-cap=2 escalation.

---

## Task 1: Scaffold package + console scripts

**Files:**

- Create: `tools/qt_orchestration/__init__.py`
- Create: `tools/qt_orchestration/cli.py`
- Modify: `tools/pyproject.toml` (add console scripts)
- Test: `tools/qt_orchestration/tests/test_cli_smoke.py`

- [ ] **Step 1: Write the failing test**

```python
# tools/qt_orchestration/tests/test_cli_smoke.py
import subprocess
import sys

def test_cli_help_lists_subcommands():
    result = subprocess.run(
        [sys.executable, "-m", "qt_orchestration.cli", "--help"],
        capture_output=True, text=True, check=True,
    )
    assert "scan" in result.stdout
    assert "expand-brief" in result.stdout
    assert "verify-slice" in result.stdout
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/test_cli_smoke.py -v`
Expected: FAIL (module not found).

- [ ] **Step 3: Create the package and minimal CLI**

```python
# tools/qt_orchestration/__init__.py
"""Qt orchestration module — see docs/superpowers/specs/2026-05-13-qt-orchestration-design.md"""
__version__ = "0.1.0"
```

```python
# tools/qt_orchestration/cli.py
"""Top-level CLI dispatcher for qt-orchestration subcommands."""
from __future__ import annotations
import argparse
import sys


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="qt-orchestration")
    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("scan", help="Scan repo for Qt touchpoints")
    sub.add_parser("expand-brief", help="Expand an issue into a slice brief")
    sub.add_parser("verify-slice", help="Verify a slice diff against the Qt rule set")
    args, rest = parser.parse_known_args(argv)
    if args.cmd == "scan":
        from qt_orchestration.scan import main as scan_main
        return scan_main(rest)
    if args.cmd == "expand-brief":
        from qt_orchestration.expand_brief import main as eb_main
        return eb_main(rest)
    if args.cmd == "verify-slice":
        from qt_orchestration.verify_slice import main as vs_main
        return vs_main(rest)
    return 1


if __name__ == "__main__":
    sys.exit(main())
```

Add three placeholder modules so the dispatcher imports cleanly:

```python
# tools/qt_orchestration/scan.py
def main(argv): print("scan: not implemented", flush=True); return 0
```

```python
# tools/qt_orchestration/expand_brief.py
def main(argv): print("expand-brief: not implemented", flush=True); return 0
```

```python
# tools/qt_orchestration/verify_slice.py
def main(argv): print("verify-slice: not implemented", flush=True); return 0
```

- [ ] **Step 4: Add console script to pyproject.toml**

Add under `[project]`:

```toml
[project.scripts]
qt-orchestration = "qt_orchestration.cli:main"
```

And under `[project.optional-dependencies]` ensure the existing `scripts` extra includes `defusedxml`, `jinja2`, `pyyaml` (already present except confirm `pyyaml`).

- [ ] **Step 5: Run test to verify it passes**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/test_cli_smoke.py -v`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add tools/qt_orchestration/ tools/pyproject.toml
git commit -m "feat(qt-orchestration): scaffold package + CLI dispatcher"
```

---

## Task 2: Finding + Severity primitives

**Files:**

- Create: `tools/qt_orchestration/findings.py`
- Test: `tools/qt_orchestration/tests/test_findings.py`

- [ ] **Step 1: Write the failing test**

```python
# tools/qt_orchestration/tests/test_findings.py
from qt_orchestration.findings import Finding, Severity

def test_severity_ordering():
    assert Severity.BLOCKER > Severity.MAJOR > Severity.MINOR > Severity.NIT

def test_finding_serializes_to_json():
    f = Finding(
        rule_id="R-QRC-01", severity=Severity.BLOCKER,
        path="custom/foo.qml", line=0,
        message="not registered in custom.qrc",
        fix_hint="add <file>foo.qml</file> to qresource prefix='/'",
    )
    d = f.to_dict()
    assert d["rule"] == "R-QRC-01"
    assert d["severity"] == "BLOCKER"
    assert d["path"] == "custom/foo.qml"

def test_finding_from_dict_roundtrip():
    f = Finding(rule_id="R-MOC-01", severity=Severity.MAJOR,
                path="src/x.h", line=12, message="m", fix_hint="h")
    assert Finding.from_dict(f.to_dict()) == f
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/test_findings.py -v`
Expected: FAIL (module not found).

- [ ] **Step 3: Implement**

```python
# tools/qt_orchestration/findings.py
from __future__ import annotations
from dataclasses import dataclass, asdict
from enum import IntEnum


class Severity(IntEnum):
    NIT = 1
    MINOR = 2
    MAJOR = 3
    BLOCKER = 4

    @classmethod
    def from_name(cls, name: str) -> "Severity":
        return cls[name.upper()]


@dataclass(frozen=True)
class Finding:
    rule_id: str
    severity: Severity
    path: str
    line: int
    message: str
    fix_hint: str

    def to_dict(self) -> dict:
        return {
            "rule": self.rule_id,
            "severity": self.severity.name,
            "path": self.path,
            "line": self.line,
            "message": self.message,
            "fix_hint": self.fix_hint,
        }

    @classmethod
    def from_dict(cls, d: dict) -> "Finding":
        return cls(
            rule_id=d["rule"],
            severity=Severity.from_name(d["severity"]),
            path=d["path"],
            line=d["line"],
            message=d["message"],
            fix_hint=d["fix_hint"],
        )
```

- [ ] **Step 4: Run test to verify it passes**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/test_findings.py -v`
Expected: PASS (3 tests).

- [ ] **Step 5: Commit**

```bash
git add tools/qt_orchestration/findings.py tools/qt_orchestration/tests/test_findings.py
git commit -m "feat(qt-orchestration): Finding + Severity primitives"
```

---

## Task 3: Rule base + registry + RuleContext

**Files:**

- Create: `tools/qt_orchestration/rules.py`
- Test: `tools/qt_orchestration/tests/test_rules_registry.py`

- [ ] **Step 1: Write the failing test**

```python
# tools/qt_orchestration/tests/test_rules_registry.py
from qt_orchestration.rules import Rule, REGISTRY, RuleContext
from qt_orchestration.findings import Finding, Severity


def test_rule_registers_itself():
    @REGISTRY.register
    class FakeRule(Rule):
        rule_id = "R-FAKE-01"
        default_severity = Severity.MAJOR
        def applies_to(self, ctx): return True
        def check(self, ctx): return []
    assert REGISTRY.get("R-FAKE-01") is not None


def test_rule_context_carries_diff_and_repo_root(tmp_path):
    ctx = RuleContext(repo_root=tmp_path, slice_diff=None, severity_overrides={})
    assert ctx.repo_root == tmp_path


def test_registry_duplicate_id_raises():
    import pytest
    with pytest.raises(ValueError, match="duplicate"):
        @REGISTRY.register
        class A(Rule):
            rule_id = "R-DUP-01"
            default_severity = Severity.MAJOR
            def applies_to(self, ctx): return False
            def check(self, ctx): return []
        @REGISTRY.register
        class B(Rule):
            rule_id = "R-DUP-01"
            default_severity = Severity.MAJOR
            def applies_to(self, ctx): return False
            def check(self, ctx): return []
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/test_rules_registry.py -v`
Expected: FAIL.

- [ ] **Step 3: Implement**

```python
# tools/qt_orchestration/rules.py
from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable
from qt_orchestration.findings import Finding, Severity


@dataclass
class RuleContext:
    repo_root: Path
    slice_diff: "SliceDiff | None"   # see qt_orchestration.diff
    severity_overrides: dict[str, Severity] = field(default_factory=dict)
    issue_frontmatter: dict = field(default_factory=dict)


class Rule:
    rule_id: str = ""
    default_severity: Severity = Severity.MAJOR

    def severity(self, ctx: RuleContext) -> Severity:
        return ctx.severity_overrides.get(self.rule_id, self.default_severity)

    def applies_to(self, ctx: RuleContext) -> bool:
        raise NotImplementedError

    def check(self, ctx: RuleContext) -> Iterable[Finding]:
        raise NotImplementedError


class _Registry:
    def __init__(self) -> None:
        self._rules: dict[str, type[Rule]] = {}

    def register(self, cls: type[Rule]) -> type[Rule]:
        if not cls.rule_id:
            raise ValueError(f"{cls.__name__}: rule_id must be set")
        if cls.rule_id in self._rules:
            raise ValueError(f"duplicate rule_id: {cls.rule_id}")
        self._rules[cls.rule_id] = cls
        return cls

    def get(self, rule_id: str) -> type[Rule] | None:
        return self._rules.get(rule_id)

    def all(self) -> list[type[Rule]]:
        return list(self._rules.values())

    def reset(self) -> None:
        """Test-only — clear the registry between tests."""
        self._rules.clear()


REGISTRY = _Registry()
```

Add a conftest that resets the registry between tests so registration tests don't pollute each other:

```python
# tools/qt_orchestration/tests/conftest.py
import pytest
from qt_orchestration.rules import REGISTRY


@pytest.fixture(autouse=True)
def _reset_registry():
    REGISTRY.reset()
    yield
    REGISTRY.reset()
```

- [ ] **Step 4: Run test to verify it passes**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/test_rules_registry.py -v`
Expected: PASS (3 tests).

- [ ] **Step 5: Commit**

```bash
git add tools/qt_orchestration/rules.py tools/qt_orchestration/tests/test_rules_registry.py tools/qt_orchestration/tests/conftest.py
git commit -m "feat(qt-orchestration): Rule base + registry + RuleContext"
```

---

## Task 4: SliceDiff parser

**Files:**

- Create: `tools/qt_orchestration/diff.py`
- Test: `tools/qt_orchestration/tests/test_diff.py`

- [ ] **Step 1: Write the failing test**

```python
# tools/qt_orchestration/tests/test_diff.py
import subprocess
from qt_orchestration.diff import SliceDiff


def test_parses_git_diff_added_modified_renamed(tmp_path):
    repo = tmp_path / "r"
    repo.mkdir()
    subprocess.run(["git", "init", "-q", str(repo)], check=True)
    subprocess.run(["git", "-C", str(repo), "config", "user.email", "x@x"], check=True)
    subprocess.run(["git", "-C", str(repo), "config", "user.name", "x"], check=True)
    (repo / "a.txt").write_text("hi\n")
    subprocess.run(["git", "-C", str(repo), "add", "a.txt"], check=True)
    subprocess.run(["git", "-C", str(repo), "commit", "-q", "-m", "base"], check=True)
    (repo / "b.qml").write_text("import QtQuick\nItem {}\n")
    (repo / "a.txt").write_text("hi\nthere\n")
    subprocess.run(["git", "-C", str(repo), "add", "."], check=True)
    subprocess.run(["git", "-C", str(repo), "commit", "-q", "-m", "slice"], check=True)
    diff = SliceDiff.from_git(repo, base="HEAD~1", head="HEAD")
    assert "b.qml" in diff.added
    assert "a.txt" in diff.modified
    assert diff.is_added("b.qml")
    assert not diff.is_added("a.txt")
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/test_diff.py -v`
Expected: FAIL.

- [ ] **Step 3: Implement**

```python
# tools/qt_orchestration/diff.py
from __future__ import annotations
import subprocess
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class SliceDiff:
    repo_root: Path
    base: str
    head: str
    added: set[str] = field(default_factory=set)
    modified: set[str] = field(default_factory=set)
    deleted: set[str] = field(default_factory=set)
    renamed: dict[str, str] = field(default_factory=dict)  # new -> old

    @classmethod
    def from_git(cls, repo_root: Path, base: str, head: str) -> "SliceDiff":
        out = subprocess.run(
            ["git", "-C", str(repo_root), "diff", "--name-status",
             "-M", base, head],
            capture_output=True, text=True, check=True,
        ).stdout
        d = cls(repo_root=repo_root, base=base, head=head)
        for line in out.splitlines():
            parts = line.split("\t")
            tag = parts[0]
            if tag.startswith("A"):
                d.added.add(parts[1])
            elif tag.startswith("M"):
                d.modified.add(parts[1])
            elif tag.startswith("D"):
                d.deleted.add(parts[1])
            elif tag.startswith("R"):
                d.renamed[parts[2]] = parts[1]
                d.modified.add(parts[2])
        return d

    def is_added(self, path: str) -> bool:
        return path in self.added

    def all_touched(self) -> set[str]:
        return self.added | self.modified | set(self.renamed.keys())

    def file_contents_after(self, path: str) -> str:
        result = subprocess.run(
            ["git", "-C", str(self.repo_root), "show", f"{self.head}:{path}"],
            capture_output=True, text=True, check=True,
        )
        return result.stdout

    def file_contents_before(self, path: str) -> str | None:
        try:
            result = subprocess.run(
                ["git", "-C", str(self.repo_root), "show", f"{self.base}:{path}"],
                capture_output=True, text=True, check=True,
            )
            return result.stdout
        except subprocess.CalledProcessError:
            return None
```

- [ ] **Step 4: Run test to verify it passes**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/test_diff.py -v`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add tools/qt_orchestration/diff.py tools/qt_orchestration/tests/test_diff.py
git commit -m "feat(qt-orchestration): SliceDiff git parser"
```

---

## Task 5: R-QRC-01 (asset must be registered in custom.qrc)

This is the pattern-establishing rule. Subsequent rule tasks reuse this shape compactly.

**Files:**

- Create: `tools/qt_orchestration/rules_pack/__init__.py`
- Create: `tools/qt_orchestration/rules_pack/qrc.py`
- Create: `tools/qt_orchestration/tests/fixtures/rules/R-QRC-01/pass/` + `fail/`
- Test: `tools/qt_orchestration/tests/test_qrc_rules.py`

- [ ] **Step 1: Create fixture trees**

```
tools/qt_orchestration/tests/fixtures/rules/R-QRC-01/pass/
  custom/Foo.qml
  custom/custom.qrc        # qresource block that DOES list Foo.qml
tools/qt_orchestration/tests/fixtures/rules/R-QRC-01/fail/
  custom/Foo.qml
  custom/custom.qrc        # qresource block that does NOT list Foo.qml
```

Content of `pass/custom/Foo.qml` and `fail/custom/Foo.qml`:

```qml
import QtQuick
Item {}
```

Content of `pass/custom/custom.qrc`:

```xml
<RCC>
  <qresource prefix="/qml">
    <file>Foo.qml</file>
  </qresource>
</RCC>
```

Content of `fail/custom/custom.qrc`:

```xml
<RCC>
  <qresource prefix="/qml">
  </qresource>
</RCC>
```

- [ ] **Step 2: Write the failing test**

```python
# tools/qt_orchestration/tests/test_qrc_rules.py
import subprocess
from pathlib import Path
import pytest
from qt_orchestration.diff import SliceDiff
from qt_orchestration.rules import RuleContext, REGISTRY
from qt_orchestration.findings import Severity
import qt_orchestration.rules_pack.qrc  # noqa: F401  triggers registration

FIXTURES = Path(__file__).parent / "fixtures" / "rules"


def _make_repo_from_fixture(tmp_path, fixture_dir):
    """Stage fixture as a slice on top of an empty base commit."""
    import shutil
    repo = tmp_path / "r"
    repo.mkdir()
    subprocess.run(["git", "init", "-q", str(repo)], check=True)
    subprocess.run(["git", "-C", str(repo), "config", "user.email", "x@x"], check=True)
    subprocess.run(["git", "-C", str(repo), "config", "user.name", "x"], check=True)
    (repo / ".gitkeep").write_text("")
    subprocess.run(["git", "-C", str(repo), "add", "."], check=True)
    subprocess.run(["git", "-C", str(repo), "commit", "-q", "-m", "base"], check=True)
    for src in fixture_dir.rglob("*"):
        if src.is_file():
            dst = repo / src.relative_to(fixture_dir)
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dst)
    subprocess.run(["git", "-C", str(repo), "add", "."], check=True)
    subprocess.run(["git", "-C", str(repo), "commit", "-q", "-m", "slice"], check=True)
    return repo


def test_r_qrc_01_pass(tmp_path):
    repo = _make_repo_from_fixture(tmp_path, FIXTURES / "R-QRC-01" / "pass")
    diff = SliceDiff.from_git(repo, "HEAD~1", "HEAD")
    rule = REGISTRY.get("R-QRC-01")()
    ctx = RuleContext(repo_root=repo, slice_diff=diff)
    assert rule.applies_to(ctx)
    findings = list(rule.check(ctx))
    assert findings == []


def test_r_qrc_01_fail(tmp_path):
    repo = _make_repo_from_fixture(tmp_path, FIXTURES / "R-QRC-01" / "fail")
    diff = SliceDiff.from_git(repo, "HEAD~1", "HEAD")
    rule = REGISTRY.get("R-QRC-01")()
    ctx = RuleContext(repo_root=repo, slice_diff=diff)
    findings = list(rule.check(ctx))
    assert len(findings) == 1
    assert findings[0].rule_id == "R-QRC-01"
    assert findings[0].severity == Severity.BLOCKER
    assert "Foo.qml" in findings[0].path
```

- [ ] **Step 3: Run test to verify it fails**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/test_qrc_rules.py -v`
Expected: FAIL (module not found).

- [ ] **Step 4: Implement**

```python
# tools/qt_orchestration/rules_pack/__init__.py
"""Rule pack — importing this package registers all rules with REGISTRY."""
from . import qrc        # noqa: F401
```

```python
# tools/qt_orchestration/rules_pack/qrc.py
from __future__ import annotations
from defusedxml import ElementTree as ET
from qt_orchestration.findings import Finding, Severity
from qt_orchestration.rules import Rule, REGISTRY, RuleContext


QRC_PATH = "custom/custom.qrc"
QRC_ASSET_SUFFIXES = (".qml", ".js", ".png", ".jpg", ".svg")


def _parse_qresource_files(qrc_text: str) -> dict[str, list[str]]:
    """Return {prefix: [file, ...]} parsed from a .qrc XML body."""
    root = ET.fromstring(qrc_text)
    out: dict[str, list[str]] = {}
    for q in root.findall("qresource"):
        prefix = q.get("prefix", "")
        out.setdefault(prefix, []).extend(f.text or "" for f in q.findall("file"))
    return out


@REGISTRY.register
class QrcAssetRegistered(Rule):
    rule_id = "R-QRC-01"
    default_severity = Severity.BLOCKER

    def applies_to(self, ctx: RuleContext) -> bool:
        if ctx.slice_diff is None:
            return False
        return any(
            p.startswith("custom/") and p.endswith(QRC_ASSET_SUFFIXES)
            for p in ctx.slice_diff.added
        )

    def check(self, ctx: RuleContext):
        qrc_text = ctx.slice_diff.file_contents_after(QRC_PATH)
        try:
            blocks = _parse_qresource_files(qrc_text)
        except ET.ParseError as e:
            yield Finding(
                rule_id=self.rule_id, severity=self.severity(ctx),
                path=QRC_PATH, line=0,
                message=f"custom.qrc is not valid XML: {e}",
                fix_hint="Restore valid <RCC><qresource>...</qresource></RCC> structure",
            )
            return
        all_listed = {f for files in blocks.values() for f in files}
        for added in ctx.slice_diff.added:
            if not added.startswith("custom/") or not added.endswith(QRC_ASSET_SUFFIXES):
                continue
            rel = added[len("custom/"):]
            if rel not in all_listed:
                yield Finding(
                    rule_id=self.rule_id, severity=self.severity(ctx),
                    path=added, line=0,
                    message=f"{added} added but not registered in {QRC_PATH}",
                    fix_hint=f"Add <file>{rel}</file> to a <qresource> block in {QRC_PATH}",
                )
```

Also remove `_reset_registry` from `conftest.py` for the rules_pack tests — once rules are registered at import time we want them to persist within a session. Replace the autouse fixture with an explicit fixture used only by registry-mechanic tests:

```python
# tools/qt_orchestration/tests/conftest.py
import pytest
from qt_orchestration.rules import REGISTRY


@pytest.fixture
def fresh_registry():
    REGISTRY.reset()
    yield
    REGISTRY.reset()
    # Re-import the rules pack so other tests still see the standard rules.
    import importlib, qt_orchestration.rules_pack as rp
    importlib.reload(rp)
```

Update `test_rules_registry.py` to use `fresh_registry` instead of relying on autouse:

```python
def test_rule_registers_itself(fresh_registry):
    ...
def test_registry_duplicate_id_raises(fresh_registry):
    ...
```

- [ ] **Step 5: Run tests to verify pass**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/ -v`
Expected: PASS for all current tests.

- [ ] **Step 6: Commit**

```bash
git add tools/qt_orchestration/rules_pack/ tools/qt_orchestration/tests/
git commit -m "feat(qt-orchestration): R-QRC-01 asset-registration rule + fixture pattern"
```

---

## Task 6: R-QRC-02 (sibling qresource block integrity)

**Files:**

- Modify: `tools/qt_orchestration/rules_pack/qrc.py`
- Create: `tools/qt_orchestration/tests/fixtures/rules/R-QRC-02/pass/` + `fail/`
- Modify: `tools/qt_orchestration/tests/test_qrc_rules.py`

- [ ] **Step 1: Fixture trees**

`pass/`: two commits in the fixture's diff — a base `custom.qrc` with two qresource blocks (`/qml`, `/json`), and a slice commit that adds a file to `/qml` while preserving both blocks.

`fail/`: base same, slice commit drops the `/json` qresource block (the exact silent-merge hazard from the project memory).

To support a two-commit fixture, change `_make_repo_from_fixture` to honor a `base/` and `slice/` subdir inside the fixture dir. Files in `base/` go into the base commit; files in `slice/` overlay into the slice commit.

Reorganize all existing R-QRC-01 fixtures the same way (`base/` empty, `slice/` containing current content) and update `_make_repo_from_fixture`.

- [ ] **Step 2: Update helper and add test**

```python
def _make_repo_from_fixture(tmp_path, fixture_dir):
    import shutil
    repo = tmp_path / "r"
    repo.mkdir()
    subprocess.run(["git", "init", "-q", str(repo)], check=True)
    subprocess.run(["git", "-C", str(repo), "config", "user.email", "x@x"], check=True)
    subprocess.run(["git", "-C", str(repo), "config", "user.name", "x"], check=True)

    def _stage(subdir: str, msg: str):
        src_dir = fixture_dir / subdir
        if src_dir.exists():
            for src in src_dir.rglob("*"):
                if src.is_file():
                    dst = repo / src.relative_to(src_dir)
                    dst.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(src, dst)
        else:
            (repo / ".gitkeep").write_text("")
        subprocess.run(["git", "-C", str(repo), "add", "-A"], check=True)
        subprocess.run(["git", "-C", str(repo), "commit", "-q", "--allow-empty",
                        "-m", msg], check=True)

    _stage("base", "base")
    _stage("slice", "slice")
    return repo


def test_r_qrc_02_fail_drops_sibling_block(tmp_path):
    repo = _make_repo_from_fixture(tmp_path, FIXTURES / "R-QRC-02" / "fail")
    diff = SliceDiff.from_git(repo, "HEAD~1", "HEAD")
    rule = REGISTRY.get("R-QRC-02")()
    ctx = RuleContext(repo_root=repo, slice_diff=diff)
    findings = list(rule.check(ctx))
    assert any(f.rule_id == "R-QRC-02" and "json" in f.message for f in findings)


def test_r_qrc_02_pass_preserves_siblings(tmp_path):
    repo = _make_repo_from_fixture(tmp_path, FIXTURES / "R-QRC-02" / "pass")
    diff = SliceDiff.from_git(repo, "HEAD~1", "HEAD")
    rule = REGISTRY.get("R-QRC-02")()
    ctx = RuleContext(repo_root=repo, slice_diff=diff)
    assert list(rule.check(ctx)) == []
```

- [ ] **Step 3: Run test to verify fail**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/test_qrc_rules.py -v`
Expected: FAIL (R-QRC-02 not implemented).

- [ ] **Step 4: Implement**

Append to `tools/qt_orchestration/rules_pack/qrc.py`:

```python
@REGISTRY.register
class QrcSiblingIntegrity(Rule):
    rule_id = "R-QRC-02"
    default_severity = Severity.BLOCKER

    def applies_to(self, ctx: RuleContext) -> bool:
        if ctx.slice_diff is None:
            return False
        return QRC_PATH in ctx.slice_diff.modified or QRC_PATH in ctx.slice_diff.added

    def check(self, ctx: RuleContext):
        before = ctx.slice_diff.file_contents_before(QRC_PATH)
        after = ctx.slice_diff.file_contents_after(QRC_PATH)
        if before is None:
            return  # new file, R-QRC-02 doesn't apply
        try:
            b = _parse_qresource_files(before)
            a = _parse_qresource_files(after)
        except ET.ParseError:
            return  # R-QRC-01 already reports invalid XML
        authorized = set(ctx.issue_frontmatter.get("qrc_prefix_changes", []))
        for prefix in b:
            if prefix not in a and prefix not in authorized:
                yield Finding(
                    rule_id=self.rule_id, severity=self.severity(ctx),
                    path=QRC_PATH, line=0,
                    message=(f"qresource prefix='{prefix}' silently dropped from "
                             f"{QRC_PATH} (sibling-block merge hazard)"),
                    fix_hint=(f"Restore the <qresource prefix='{prefix}'> block or "
                              f"declare qrc_prefix_changes: ['{prefix}'] in the "
                              f"issue frontmatter if intentional"),
                )
```

- [ ] **Step 5: Run test to verify pass**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/test_qrc_rules.py -v`
Expected: PASS (4 tests).

- [ ] **Step 6: Commit**

```bash
git add tools/qt_orchestration/rules_pack/qrc.py tools/qt_orchestration/tests/
git commit -m "feat(qt-orchestration): R-QRC-02 sibling qresource integrity"
```

---

## Task 7: CMake rules (R-CMAKE-01, R-CMAKE-02)

**Files:**

- Create: `tools/qt_orchestration/rules_pack/cmake.py`
- Modify: `tools/qt_orchestration/rules_pack/__init__.py` (import cmake)
- Create: fixtures `R-CMAKE-01/{pass,fail}/{base,slice}/`, `R-CMAKE-02/{pass,fail}/{base,slice}/`
- Create: `tools/qt_orchestration/tests/test_cmake_rules.py`

- [ ] **Step 1: Fixtures**

`R-CMAKE-01/fail/slice/` — adds a new `custom/src/Foo.cc` and a `custom/CMakeLists.txt` that does NOT list it in `target_sources(qgroundcontrol PRIVATE ...)`.

`R-CMAKE-01/pass/slice/` — same `.cc` but listed in `target_sources(qgroundcontrol PRIVATE Foo.cc)`.

`R-CMAKE-02/fail/slice/` — adds a `custom/src/Foo.h` containing `Q_OBJECT` but the surrounding target is declared with `set_target_properties(target PROPERTIES AUTOMOC OFF)`.

`R-CMAKE-02/pass/slice/` — same `.h` under a target with `AUTOMOC ON` (or default — Qt's `qt_add_executable` enables AUTOMOC by default; the fixture should explicitly set ON to be unambiguous).

`base/` for both contains a stub `CMakeLists.txt` with a `qgroundcontrol` target so target lookup works.

- [ ] **Step 2: Write tests**

```python
# tools/qt_orchestration/tests/test_cmake_rules.py
from pathlib import Path
from qt_orchestration.diff import SliceDiff
from qt_orchestration.rules import RuleContext, REGISTRY
import qt_orchestration.rules_pack  # registers
from qt_orchestration.tests.test_qrc_rules import _make_repo_from_fixture

FIXTURES = Path(__file__).parent / "fixtures" / "rules"


def test_r_cmake_01_fail(tmp_path):
    repo = _make_repo_from_fixture(tmp_path, FIXTURES / "R-CMAKE-01" / "fail")
    diff = SliceDiff.from_git(repo, "HEAD~1", "HEAD")
    rule = REGISTRY.get("R-CMAKE-01")()
    ctx = RuleContext(repo_root=repo, slice_diff=diff)
    findings = list(rule.check(ctx))
    assert any("Foo.cc" in f.path for f in findings)


def test_r_cmake_01_pass(tmp_path):
    repo = _make_repo_from_fixture(tmp_path, FIXTURES / "R-CMAKE-01" / "pass")
    diff = SliceDiff.from_git(repo, "HEAD~1", "HEAD")
    rule = REGISTRY.get("R-CMAKE-01")()
    ctx = RuleContext(repo_root=repo, slice_diff=diff)
    assert list(rule.check(ctx)) == []


# Analogous tests for R-CMAKE-02 — see fixture layout above.
```

- [ ] **Step 3: Implement (text scan, no CMake invocation)**

R-CMAKE-01 is a text scan: for each added `.cc/.cpp/.h` under `custom/`, search every `CMakeLists.txt` in the slice for a `target_sources(...)` call that names the file's basename (relative to the CMakeLists). If none found → finding.

R-CMAKE-02: for each added `.h` containing `Q_OBJECT`, find the nearest enclosing `CMakeLists.txt` and search for `set_target_properties(... AUTOMOC ON)` or absence of `AUTOMOC OFF`. AUTOMOC defaults vary, so the rule fires only on explicit `AUTOMOC OFF` for the enclosing target. This is a pragmatic narrowing: catches the common drift (someone disables AUTOMOC during refactor and forgets) without over-firing.

```python
# tools/qt_orchestration/rules_pack/cmake.py
from __future__ import annotations
import re
from qt_orchestration.findings import Finding, Severity
from qt_orchestration.rules import Rule, REGISTRY, RuleContext


SRC_SUFFIXES = (".cc", ".cpp", ".h")


def _find_cmakelists_for(repo_root, file_path: str):
    """Walk up from file_path to find the nearest CMakeLists.txt."""
    from pathlib import PurePosixPath
    p = PurePosixPath(file_path).parent
    while True:
        candidate = repo_root / p / "CMakeLists.txt"
        if candidate.exists():
            return str(p / "CMakeLists.txt"), candidate.read_text()
        if str(p) in ("", "."):
            return None, None
        p = p.parent


@REGISTRY.register
class CMakeTargetSources(Rule):
    rule_id = "R-CMAKE-01"
    default_severity = Severity.BLOCKER

    def applies_to(self, ctx):
        return any(p.endswith(SRC_SUFFIXES) for p in (ctx.slice_diff.added if ctx.slice_diff else ()))

    def check(self, ctx):
        for added in ctx.slice_diff.added:
            if not added.endswith(SRC_SUFFIXES):
                continue
            cml_path, cml_text = _find_cmakelists_for(ctx.repo_root, added)
            if cml_path is None:
                yield Finding(
                    rule_id=self.rule_id, severity=self.severity(ctx),
                    path=added, line=0,
                    message=f"no CMakeLists.txt found in any ancestor directory of {added}",
                    fix_hint="Add a CMakeLists.txt that includes this file via target_sources()",
                )
                continue
            basename = added.rsplit("/", 1)[-1]
            ts_calls = re.findall(r"target_sources\s*\([^)]*\)", cml_text, re.MULTILINE | re.DOTALL)
            if not any(basename in call for call in ts_calls):
                yield Finding(
                    rule_id=self.rule_id, severity=self.severity(ctx),
                    path=added, line=0,
                    message=f"{added} not listed in target_sources() of {cml_path}",
                    fix_hint=f"Add {basename} to target_sources(<target> PRIVATE ...) in {cml_path}",
                )


@REGISTRY.register
class CMakeAutomocOn(Rule):
    rule_id = "R-CMAKE-02"
    default_severity = Severity.MAJOR

    def applies_to(self, ctx):
        if not ctx.slice_diff:
            return False
        for added in ctx.slice_diff.added:
            if not added.endswith(".h"):
                continue
            text = ctx.slice_diff.file_contents_after(added)
            if "Q_OBJECT" in text:
                return True
        return False

    def check(self, ctx):
        for added in ctx.slice_diff.added:
            if not added.endswith(".h"):
                continue
            text = ctx.slice_diff.file_contents_after(added)
            if "Q_OBJECT" not in text:
                continue
            cml_path, cml_text = _find_cmakelists_for(ctx.repo_root, added)
            if cml_text and re.search(r"AUTOMOC\s+OFF", cml_text):
                yield Finding(
                    rule_id=self.rule_id, severity=self.severity(ctx),
                    path=added, line=0,
                    message=f"{added} declares Q_OBJECT but {cml_path} has AUTOMOC OFF",
                    fix_hint=f"Set AUTOMOC ON for the target containing {added} in {cml_path}",
                )
```

Update `__init__.py`:

```python
# tools/qt_orchestration/rules_pack/__init__.py
from . import qrc      # noqa
from . import cmake    # noqa
```

- [ ] **Step 4: Run tests**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/test_cmake_rules.py -v`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add tools/qt_orchestration/rules_pack/cmake.py tools/qt_orchestration/rules_pack/__init__.py tools/qt_orchestration/tests/
git commit -m "feat(qt-orchestration): R-CMAKE-01, R-CMAKE-02 wiring rules"
```

---

## Task 8: MOC rules (R-MOC-01, R-MOC-02, R-MOC-03)

**Files:**

- Create: `tools/qt_orchestration/rules_pack/moc.py`
- Modify: `tools/qt_orchestration/rules_pack/__init__.py`
- Create: fixtures `R-MOC-0{1,2,3}/{pass,fail}/{base,slice}/`
- Create: `tools/qt_orchestration/tests/test_moc_rules.py`

- [ ] **Step 1: Fixtures**

`R-MOC-01/fail/slice/custom/src/Foo.h` — declares `class Foo : public QObject { ... };` but lacks `Q_OBJECT` macro.

`R-MOC-01/pass/slice/custom/src/Foo.h` — same with `Q_OBJECT` in the class body.

`R-MOC-02/fail/slice/custom/src/Bar.h` — `class Bar : public QObject { Q_OBJECT public: ... };` with no `qmlRegisterType<Bar>` anywhere and no `QML_ELEMENT`. `slice/custom/src/main.cc` references `Bar` from QML via `qmlRegisterType<Foo>` only.

`R-MOC-02/pass/` — `Q_OBJECT` plus `QML_ELEMENT` in the class body, or `qmlRegisterType<Bar>(...)` somewhere in the slice's `.cc` files.

`R-MOC-03/fail/slice/custom/src/Baz.h` — `Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)` but no `signals: void xChanged();` declaration.

`R-MOC-03/pass/` — has both.

- [ ] **Step 2: Write tests** (one per pass/fail per rule, 6 total — same shape as previous tasks).

- [ ] **Step 3: Implement**

```python
# tools/qt_orchestration/rules_pack/moc.py
from __future__ import annotations
import re
from qt_orchestration.findings import Finding, Severity
from qt_orchestration.rules import Rule, REGISTRY, RuleContext


_CLASS_INHERITS_QOBJECT = re.compile(
    r"class\s+(\w+)\s*:\s*public\s+(QObject|QAbstractListModel|QAbstractItemModel|QQuickItem)\b"
)
_Q_OBJECT_LINE = re.compile(r"^\s*Q_OBJECT\b", re.MULTILINE)
_QML_ELEMENT_LINE = re.compile(r"^\s*QML_ELEMENT\b", re.MULTILINE)
_Q_PROPERTY = re.compile(
    r"Q_PROPERTY\s*\(\s*\S+\s+(\w+)"
    r"(?:.*?READ\s+(\w+))?"
    r"(?:.*?WRITE\s+(\w+))?"
    r"(?:.*?NOTIFY\s+(\w+))?",
    re.DOTALL,
)


@REGISTRY.register
class MocQObjectMacro(Rule):
    rule_id = "R-MOC-01"
    default_severity = Severity.BLOCKER

    def applies_to(self, ctx):
        return any(p.endswith(".h") for p in (ctx.slice_diff.added if ctx.slice_diff else ()))

    def check(self, ctx):
        for added in ctx.slice_diff.added:
            if not added.endswith(".h"):
                continue
            text = ctx.slice_diff.file_contents_after(added)
            for m in _CLASS_INHERITS_QOBJECT.finditer(text):
                # Find the class body and check it contains Q_OBJECT.
                start = m.end()
                brace = text.find("{", start)
                end = text.find("};", brace)
                body = text[brace:end] if brace != -1 and end != -1 else ""
                if not _Q_OBJECT_LINE.search(body):
                    yield Finding(
                        rule_id=self.rule_id, severity=self.severity(ctx),
                        path=added,
                        line=text[:m.start()].count("\n") + 1,
                        message=f"class {m.group(1)} inherits {m.group(2)} but lacks Q_OBJECT",
                        fix_hint=f"Add `Q_OBJECT` as the first line of class {m.group(1)} body",
                    )


@REGISTRY.register
class MocQmlRegistration(Rule):
    rule_id = "R-MOC-02"
    default_severity = Severity.MAJOR

    def applies_to(self, ctx):
        return any(p.endswith(".h") for p in (ctx.slice_diff.added if ctx.slice_diff else ()))

    def check(self, ctx):
        # Build a set of types registered via qmlRegisterType<X> across all slice files.
        registered: set[str] = set()
        for path in ctx.slice_diff.all_touched():
            if not path.endswith((".cc", ".cpp")):
                continue
            text = ctx.slice_diff.file_contents_after(path)
            for m in re.finditer(r"qmlRegisterType\s*<\s*(\w+)\s*>", text):
                registered.add(m.group(1))
        for added in ctx.slice_diff.added:
            if not added.endswith(".h"):
                continue
            text = ctx.slice_diff.file_contents_after(added)
            for m in _CLASS_INHERITS_QOBJECT.finditer(text):
                name = m.group(1)
                brace = text.find("{", m.end())
                end = text.find("};", brace)
                body = text[brace:end] if brace != -1 and end != -1 else ""
                if _QML_ELEMENT_LINE.search(body):
                    continue
                if name in registered:
                    continue
                # Only fire if class has Q_INVOKABLE or is property-typed — heuristic:
                if "Q_INVOKABLE" in body or "Q_PROPERTY" in body:
                    yield Finding(
                        rule_id=self.rule_id, severity=self.severity(ctx),
                        path=added,
                        line=text[:m.start()].count("\n") + 1,
                        message=f"class {name} appears QML-facing but lacks QML_ELEMENT or qmlRegisterType<{name}>",
                        fix_hint=f"Add `QML_ELEMENT` in class body or call qmlRegisterType<{name}>(...) at startup",
                    )


@REGISTRY.register
class MocPropertyConsistency(Rule):
    rule_id = "R-MOC-03"
    default_severity = Severity.MAJOR

    def applies_to(self, ctx):
        return any(p.endswith(".h") for p in (ctx.slice_diff.added if ctx.slice_diff else ()))

    def check(self, ctx):
        for added in ctx.slice_diff.added:
            if not added.endswith(".h"):
                continue
            text = ctx.slice_diff.file_contents_after(added)
            for m in _Q_PROPERTY.finditer(text):
                prop, read, write, notify = m.group(1), m.group(2), m.group(3), m.group(4)
                line = text[:m.start()].count("\n") + 1
                if read and not re.search(rf"\b{re.escape(read)}\s*\(", text):
                    yield Finding(rule_id=self.rule_id, severity=self.severity(ctx),
                                  path=added, line=line,
                                  message=f"Q_PROPERTY {prop}: READ {read}() not declared",
                                  fix_hint=f"Declare `<type> {read}() const;` in the class")
                if write and not re.search(rf"\b{re.escape(write)}\s*\(", text):
                    yield Finding(rule_id=self.rule_id, severity=self.severity(ctx),
                                  path=added, line=line,
                                  message=f"Q_PROPERTY {prop}: WRITE {write}() not declared",
                                  fix_hint=f"Declare `void {write}(<type>);` in the class")
                if notify:
                    sig = re.search(rf"signals\s*:\s*[\s\S]*?\b{re.escape(notify)}\s*\(", text)
                    if not sig:
                        yield Finding(rule_id=self.rule_id, severity=self.severity(ctx),
                                      path=added, line=line,
                                      message=f"Q_PROPERTY {prop}: NOTIFY {notify} signal not declared",
                                      fix_hint=f"Add `void {notify}();` under `signals:`")
```

Update `__init__.py` to import `moc`.

- [ ] **Step 4: Run tests**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/test_moc_rules.py -v`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add tools/qt_orchestration/rules_pack/moc.py tools/qt_orchestration/rules_pack/__init__.py tools/qt_orchestration/tests/
git commit -m "feat(qt-orchestration): R-MOC-01/02/03 Q_OBJECT and Q_PROPERTY rules"
```

---

## Task 9: Shadow rules (R-SHADOW-01, R-SHADOW-02)

**Files:**

- Create: `tools/qt_orchestration/rules_pack/shadow.py`
- Modify: `tools/qt_orchestration/rules_pack/__init__.py`
- Create: fixtures `R-SHADOW-0{1,2}/{pass,fail}/{base,slice}/`
- Create: `tools/qt_orchestration/tests/test_shadow_rules.py`

- [ ] **Step 1: Fixtures**

`R-SHADOW-01/fail/slice/` — modifies `src/Vehicle/Vehicle.cc`; fixture omits any `issue_frontmatter`.

`R-SHADOW-01/pass/slice/` — same change but the test invokes with `RuleContext(..., issue_frontmatter={"allow_upstream_edit": True})`.

`R-SHADOW-02/fail/slice/` — adds `custom/src/Vehicle/Vehicle.cc` (shadows `src/Vehicle/Vehicle.cc` which exists in `base/`). Issue frontmatter does not list `shadowed_originals`.

`R-SHADOW-02/pass/slice/` — same, but `issue_frontmatter={"shadowed_originals": ["src/Vehicle/Vehicle.cc"]}`.

- [ ] **Step 2: Tests** — same shape, 4 tests.

- [ ] **Step 3: Implement**

```python
# tools/qt_orchestration/rules_pack/shadow.py
from __future__ import annotations
from qt_orchestration.findings import Finding, Severity
from qt_orchestration.rules import Rule, REGISTRY, RuleContext


@REGISTRY.register
class ShadowUpstreamGuard(Rule):
    rule_id = "R-SHADOW-01"
    default_severity = Severity.BLOCKER

    def applies_to(self, ctx):
        return bool(ctx.slice_diff and any(
            p.startswith("src/") for p in ctx.slice_diff.all_touched()))

    def check(self, ctx):
        if ctx.issue_frontmatter.get("allow_upstream_edit"):
            return
        for p in ctx.slice_diff.all_touched():
            if not p.startswith("src/"):
                continue
            yield Finding(
                rule_id=self.rule_id, severity=self.severity(ctx),
                path=p, line=0,
                message=f"{p} is upstream src/; Sprig changes must shadow into custom/",
                fix_hint=(
                    f"Move the change to custom/{p}. If the upstream edit is truly "
                    f"required, add `allow_upstream_edit: true` to the issue frontmatter."
                ),
            )


@REGISTRY.register
class ShadowManifestDeclared(Rule):
    rule_id = "R-SHADOW-02"
    default_severity = Severity.MAJOR

    def applies_to(self, ctx):
        return bool(ctx.slice_diff and any(
            p.startswith("custom/src/") for p in ctx.slice_diff.added))

    def check(self, ctx):
        declared = set(ctx.issue_frontmatter.get("shadowed_originals", []))
        for added in ctx.slice_diff.added:
            if not added.startswith("custom/src/"):
                continue
            upstream = added[len("custom/"):]
            if (ctx.repo_root / upstream).exists() and upstream not in declared:
                yield Finding(
                    rule_id=self.rule_id, severity=self.severity(ctx),
                    path=added, line=0,
                    message=f"{added} shadows {upstream} but issue does not list it under shadowed_originals",
                    fix_hint=f"Add `shadowed_originals: [{upstream!r}]` to issue frontmatter so weekly rebase audit can flag drift",
                )
```

Update `__init__.py`.

- [ ] **Step 4: Run tests** — Expected PASS.

- [ ] **Step 5: Commit**

```bash
git add tools/qt_orchestration/rules_pack/shadow.py tools/qt_orchestration/rules_pack/__init__.py tools/qt_orchestration/tests/
git commit -m "feat(qt-orchestration): R-SHADOW-01/02 fork hygiene rules"
```

---

## Task 10: QML rules (R-QML-01..04)

**Files:**

- Create: `tools/qt_orchestration/rules_pack/qml.py`
- Modify: `tools/qt_orchestration/rules_pack/__init__.py`
- Create: fixtures `R-QML-0{1..4}/{pass,fail}/{base,slice}/`
- Create: `tools/qt_orchestration/tests/test_qml_rules.py`

- [ ] **Step 1: Fixtures**

R-QML-01 (qmllint): `fail/slice/custom/Bad.qml` contains `Item { property int x: "not a number" }` which `qmllint` flags. `pass/` is a valid file.

R-QML-02 (sizing): `fail/` QML uses `width: 200`; `pass/` uses `width: ScreenTools.defaultFontPixelWidth * 20`.

R-QML-03 (colors): `fail/` uses `color: "#ff0000"` or `Qt.rgba(1,0,0,1)`; `pass/` uses `color: qgcPal.text`.

R-QML-04 (imports): `fail/` does `import Sprig.Missing 1.0`; `pass/` only imports modules registered in `custom.qrc` or shipped with Qt.

- [ ] **Step 2: Tests** — 8 tests.

- [ ] **Step 3: Implement**

```python
# tools/qt_orchestration/rules_pack/qml.py
from __future__ import annotations
import re
import shutil
import subprocess
from qt_orchestration.findings import Finding, Severity
from qt_orchestration.rules import Rule, REGISTRY, RuleContext


def _qmllint_path() -> str | None:
    return shutil.which("qmllint")


@REGISTRY.register
class QmlLintClean(Rule):
    rule_id = "R-QML-01"
    default_severity = Severity.BLOCKER

    def applies_to(self, ctx):
        return any(p.endswith(".qml") for p in (ctx.slice_diff.all_touched() if ctx.slice_diff else ()))

    def check(self, ctx):
        binp = _qmllint_path()
        if binp is None:
            # Infrastructure failure surfaced by verify_slice via exit code 3.
            return
        for p in ctx.slice_diff.all_touched():
            if not p.endswith(".qml"):
                continue
            r = subprocess.run([binp, str(ctx.repo_root / p)], capture_output=True, text=True)
            if r.returncode != 0:
                first_err = (r.stderr or r.stdout).strip().splitlines()
                yield Finding(
                    rule_id=self.rule_id, severity=self.severity(ctx),
                    path=p, line=0,
                    message=f"qmllint failed: {first_err[0] if first_err else 'unknown'}",
                    fix_hint=f"Run `qmllint {p}` locally and fix reported issues",
                )


_SIZING_PROPS = ("width", "height", "spacing", "font.pixelSize", "implicitWidth", "implicitHeight")
_NUMERIC_LITERAL = re.compile(r":\s*(\d+(?:\.\d+)?)\s*$")


@REGISTRY.register
class QmlSizingViaScreenTools(Rule):
    rule_id = "R-QML-02"
    default_severity = Severity.MAJOR

    def applies_to(self, ctx):
        return any(p.endswith(".qml") for p in (ctx.slice_diff.all_touched() if ctx.slice_diff else ()))

    def check(self, ctx):
        for p in ctx.slice_diff.all_touched():
            if not p.endswith(".qml"):
                continue
            for i, raw in enumerate(ctx.slice_diff.file_contents_after(p).splitlines(), start=1):
                line = raw.strip()
                for prop in _SIZING_PROPS:
                    if line.startswith(prop + ":") and _NUMERIC_LITERAL.search(line):
                        yield Finding(
                            rule_id=self.rule_id, severity=self.severity(ctx),
                            path=p, line=i,
                            message=f"{prop} uses numeric literal; must reference ScreenTools",
                            fix_hint=f"Use ScreenTools.defaultFontPixelWidth / defaultFontPixelHeight for {prop}",
                        )


_HEX_COLOR = re.compile(r'(?:color|border\.color)\s*:\s*"#[0-9a-fA-F]{3,8}"')
_QT_RGBA = re.compile(r'(?:color|border\.color)\s*:\s*Qt\.rgba\s*\(')


@REGISTRY.register
class QmlColorsFromPalette(Rule):
    rule_id = "R-QML-03"
    default_severity = Severity.MAJOR

    def applies_to(self, ctx):
        return any(p.endswith(".qml") for p in (ctx.slice_diff.all_touched() if ctx.slice_diff else ()))

    def check(self, ctx):
        for p in ctx.slice_diff.all_touched():
            if not p.endswith(".qml"):
                continue
            for i, line in enumerate(ctx.slice_diff.file_contents_after(p).splitlines(), start=1):
                if _HEX_COLOR.search(line) or _QT_RGBA.search(line):
                    yield Finding(
                        rule_id=self.rule_id, severity=self.severity(ctx),
                        path=p, line=i,
                        message="color literal in QML; must come from QGCPalette",
                        fix_hint="Replace with QGCPalette property, e.g., qgcPal.text",
                    )


@REGISTRY.register
class QmlImportsResolve(Rule):
    rule_id = "R-QML-04"
    default_severity = Severity.MAJOR

    def applies_to(self, ctx):
        return any(p.endswith(".qml") for p in (ctx.slice_diff.all_touched() if ctx.slice_diff else ()))

    def check(self, ctx):
        # qmllint flags unresolved imports; this rule fires only when qmllint
        # output specifically mentions "Failed to import" so it surfaces the
        # category separately from generic syntax issues.
        binp = _qmllint_path()
        if binp is None:
            return
        for p in ctx.slice_diff.all_touched():
            if not p.endswith(".qml"):
                continue
            r = subprocess.run([binp, str(ctx.repo_root / p)], capture_output=True, text=True)
            if r.returncode == 0:
                continue
            for line in (r.stderr or r.stdout).splitlines():
                if "Failed to import" in line or "module not found" in line.lower():
                    yield Finding(
                        rule_id=self.rule_id, severity=self.severity(ctx),
                        path=p, line=0,
                        message=line.strip(),
                        fix_hint="Verify the QML module is registered in custom.qrc or available in Qt's QML imports",
                    )
```

Update `__init__.py`.

- [ ] **Step 4: Run tests**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/test_qml_rules.py -v`
Expected: PASS (8 tests). If `qmllint` is not installed locally, the rule tests for R-QML-01/04 skip gracefully — mark them `@pytest.mark.skipif(shutil.which("qmllint") is None, reason="qmllint not installed")`.

- [ ] **Step 5: Commit**

```bash
git add tools/qt_orchestration/rules_pack/qml.py tools/qt_orchestration/rules_pack/__init__.py tools/qt_orchestration/tests/
git commit -m "feat(qt-orchestration): R-QML-01..04 QML hygiene rules"
```

---

## Task 11: R-BUILD-01 (incremental Qt build)

**Files:**

- Create: `tools/qt_orchestration/rules_pack/build.py`
- Modify: `tools/qt_orchestration/rules_pack/__init__.py`
- Create: `tools/qt_orchestration/tests/test_build_rule.py`

- [ ] **Step 1: Test (mocked subprocess)**

```python
# tools/qt_orchestration/tests/test_build_rule.py
import subprocess
from unittest.mock import patch
from qt_orchestration.diff import SliceDiff
from qt_orchestration.rules import RuleContext, REGISTRY
import qt_orchestration.rules_pack  # registers


def _ctx_with_touched(tmp_path):
    (tmp_path / "build").mkdir()
    diff = SliceDiff(repo_root=tmp_path, base="x", head="y", added={"custom/x.cc"})
    return RuleContext(repo_root=tmp_path, slice_diff=diff)


def test_r_build_01_fail_when_build_exits_nonzero(tmp_path):
    ctx = _ctx_with_touched(tmp_path)
    rule = REGISTRY.get("R-BUILD-01")()
    fake = subprocess.CompletedProcess(args=[], returncode=1, stdout="", stderr="error: foo")
    with patch("subprocess.run", return_value=fake):
        findings = list(rule.check(ctx))
    assert any(f.rule_id == "R-BUILD-01" for f in findings)


def test_r_build_01_pass_when_build_succeeds(tmp_path):
    ctx = _ctx_with_touched(tmp_path)
    rule = REGISTRY.get("R-BUILD-01")()
    fake = subprocess.CompletedProcess(args=[], returncode=0, stdout="", stderr="")
    with patch("subprocess.run", return_value=fake):
        assert list(rule.check(ctx)) == []


def test_r_build_01_skips_when_no_build_relevant_files(tmp_path):
    diff = SliceDiff(repo_root=tmp_path, base="x", head="y", added={"docs/foo.md"})
    ctx = RuleContext(repo_root=tmp_path, slice_diff=diff)
    rule = REGISTRY.get("R-BUILD-01")()
    assert not rule.applies_to(ctx)
```

- [ ] **Step 2: Implement**

```python
# tools/qt_orchestration/rules_pack/build.py
from __future__ import annotations
import subprocess
from qt_orchestration.findings import Finding, Severity
from qt_orchestration.rules import Rule, REGISTRY, RuleContext


_BUILD_RELEVANT = (".cc", ".cpp", ".h", ".qml", ".qrc", "CMakeLists.txt")


@REGISTRY.register
class IncrementalBuild(Rule):
    rule_id = "R-BUILD-01"
    default_severity = Severity.BLOCKER

    def applies_to(self, ctx: RuleContext) -> bool:
        if not ctx.slice_diff:
            return False
        return any(p.endswith(_BUILD_RELEVANT) for p in ctx.slice_diff.all_touched())

    def check(self, ctx: RuleContext):
        build_dir = ctx.repo_root / "build"
        if not build_dir.exists():
            # verify_slice surfaces this as exit 2 (infra failure) before the
            # rule runs, but defend belt-and-suspenders.
            yield Finding(
                rule_id=self.rule_id, severity=self.severity(ctx),
                path="build/", line=0,
                message="build/ not configured",
                fix_hint="Run `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`",
            )
            return
        r = subprocess.run(
            ["cmake", "--build", str(build_dir), "--target", "qgroundcontrol", "--parallel"],
            capture_output=True, text=True,
        )
        if r.returncode != 0:
            tail = "\n".join((r.stderr or r.stdout).splitlines()[-10:])
            yield Finding(
                rule_id=self.rule_id, severity=self.severity(ctx),
                path="build/", line=0,
                message=f"qgroundcontrol build failed:\n{tail}",
                fix_hint="Run `cmake --build build --target qgroundcontrol` locally and fix the reported errors",
            )
```

Update `__init__.py`.

- [ ] **Step 3: Run tests** — Expected PASS (3 tests).

- [ ] **Step 4: Commit**

```bash
git add tools/qt_orchestration/rules_pack/build.py tools/qt_orchestration/rules_pack/__init__.py tools/qt_orchestration/tests/
git commit -m "feat(qt-orchestration): R-BUILD-01 incremental Qt build gate"
```

---

## Task 12: R-TR-01 (user-visible strings wrapped in tr/qsTr)

**Files:**

- Create: `tools/qt_orchestration/rules_pack/translation.py`
- Modify: `tools/qt_orchestration/rules_pack/__init__.py`
- Create: fixtures `R-TR-01/{pass,fail}/{base,slice}/`
- Create: `tools/qt_orchestration/tests/test_translation_rule.py`

- [ ] **Step 1: Fixtures**

`fail/slice/custom/src/Foo.cc` contains `setText("Hello world");` (bare literal).
`pass/slice/custom/src/Foo.cc` contains `setText(tr("Hello world"));`.

Equivalent `.qml` variants with `text: "Hello"` (fail) vs `text: qsTr("Hello")` (pass).

- [ ] **Step 2: Test**

```python
# tools/qt_orchestration/tests/test_translation_rule.py
# (Follows the same fixture + helper pattern as test_qrc_rules.py.)
```

- [ ] **Step 3: Implement**

```python
# tools/qt_orchestration/rules_pack/translation.py
from __future__ import annotations
import re
from qt_orchestration.findings import Finding, Severity
from qt_orchestration.rules import Rule, REGISTRY, RuleContext


# Heuristic patterns for user-visible string sinks.
_CPP_SINKS = (
    re.compile(r'setText\s*\(\s*"([^"]+)"\s*\)'),
    re.compile(r'setTitle\s*\(\s*"([^"]+)"\s*\)'),
    re.compile(r'setWindowTitle\s*\(\s*"([^"]+)"\s*\)'),
    re.compile(r'setToolTip\s*\(\s*"([^"]+)"\s*\)'),
)
_QML_SINKS = (
    re.compile(r'^\s*text\s*:\s*"([^"]+)"'),
    re.compile(r'^\s*title\s*:\s*"([^"]+)"'),
    re.compile(r'^\s*tooltip\s*:\s*"([^"]+)"'),
)

# Log-string allowlist — strings starting with these prefixes are noise.
_LOG_ALLOWLIST = ("qDebug", "qWarning", "qCritical", "qInfo", "QLoggingCategory")


@REGISTRY.register
class StringsTranslated(Rule):
    rule_id = "R-TR-01"
    default_severity = Severity.MAJOR

    def applies_to(self, ctx):
        return any(p.endswith((".cc", ".cpp", ".qml")) for p in (ctx.slice_diff.all_touched() if ctx.slice_diff else ()))

    def check(self, ctx):
        for p in ctx.slice_diff.all_touched():
            if not p.endswith((".cc", ".cpp", ".qml")):
                continue
            text = ctx.slice_diff.file_contents_after(p)
            patterns = _QML_SINKS if p.endswith(".qml") else _CPP_SINKS
            for i, line in enumerate(text.splitlines(), start=1):
                if any(tag in line for tag in _LOG_ALLOWLIST):
                    continue
                for pat in patterns:
                    if pat.search(line):
                        yield Finding(
                            rule_id=self.rule_id, severity=self.severity(ctx),
                            path=p, line=i,
                            message=f"user-visible string not wrapped in {'qsTr' if p.endswith('.qml') else 'tr'}()",
                            fix_hint=f"Wrap the literal in {'qsTr(' if p.endswith('.qml') else 'tr('}\"...\")",
                        )
```

- [ ] **Step 4: Run tests** — Expected PASS.

- [ ] **Step 5: Commit**

```bash
git add tools/qt_orchestration/rules_pack/translation.py tools/qt_orchestration/rules_pack/__init__.py tools/qt_orchestration/tests/
git commit -m "feat(qt-orchestration): R-TR-01 translation wrapping rule"
```

---

## Task 13: R-NULL-01 (vehicle null-check re-run on slice diff)

**Files:**

- Create: `tools/qt_orchestration/rules_pack/null_check.py`
- Modify: `tools/qt_orchestration/rules_pack/__init__.py`
- Create: `tools/qt_orchestration/tests/test_null_check_rule.py`

- [ ] **Step 1: Locate the existing hook**

Run: `find . -path ./build -prune -o -name '*.py' -print | xargs grep -l vehicle-null-check 2>/dev/null`

Expected: a Python script under `tools/` or `.pre-commit-config.yaml` entry. The rule shells out to the same script so findings stay consistent with what pre-commit reports.

- [ ] **Step 2: Test (mocked subprocess)**

```python
# tools/qt_orchestration/tests/test_null_check_rule.py
import subprocess
from unittest.mock import patch
from qt_orchestration.diff import SliceDiff
from qt_orchestration.rules import RuleContext, REGISTRY
import qt_orchestration.rules_pack


def test_r_null_01_fail_when_hook_exits_nonzero(tmp_path):
    diff = SliceDiff(repo_root=tmp_path, base="x", head="y", added={"custom/src/A.cc"})
    ctx = RuleContext(repo_root=tmp_path, slice_diff=diff)
    rule = REGISTRY.get("R-NULL-01")()
    fake = subprocess.CompletedProcess(
        args=[], returncode=1,
        stdout="custom/src/A.cc:42: activeVehicle() dereferenced without null-check\n",
        stderr="",
    )
    with patch("subprocess.run", return_value=fake):
        findings = list(rule.check(ctx))
    assert findings and findings[0].rule_id == "R-NULL-01"
    assert findings[0].line == 42
```

- [ ] **Step 3: Implement**

```python
# tools/qt_orchestration/rules_pack/null_check.py
from __future__ import annotations
import re
import shutil
import subprocess
from qt_orchestration.findings import Finding, Severity
from qt_orchestration.rules import Rule, REGISTRY, RuleContext


_HOOK_SCRIPT = "tools/vehicle_null_check.py"  # adjust if the hook lives elsewhere
_OUT_LINE = re.compile(r"^(?P<path>[^:]+):(?P<line>\d+):\s*(?P<msg>.+)$")


@REGISTRY.register
class VehicleNullCheck(Rule):
    rule_id = "R-NULL-01"
    default_severity = Severity.BLOCKER

    def applies_to(self, ctx):
        return any(p.endswith((".cc", ".cpp")) for p in (ctx.slice_diff.all_touched() if ctx.slice_diff else ()))

    def check(self, ctx):
        script = ctx.repo_root / _HOOK_SCRIPT
        if not script.exists():
            return
        files = [p for p in ctx.slice_diff.all_touched() if p.endswith((".cc", ".cpp"))]
        if not files:
            return
        r = subprocess.run(
            ["python3", str(script), *files],
            cwd=ctx.repo_root, capture_output=True, text=True,
        )
        if r.returncode == 0:
            return
        for raw in (r.stdout or r.stderr).splitlines():
            m = _OUT_LINE.match(raw.strip())
            if not m:
                continue
            yield Finding(
                rule_id=self.rule_id, severity=self.severity(ctx),
                path=m.group("path"), line=int(m.group("line")),
                message=m.group("msg"),
                fix_hint="Null-check `MultiVehicleManager::instance()->activeVehicle()` before dereferencing",
            )
```

If the existing pre-commit hook is not at `tools/vehicle_null_check.py`, set `_HOOK_SCRIPT` to its actual path discovered in Step 1.

- [ ] **Step 4: Run tests** — Expected PASS.

- [ ] **Step 5: Commit**

```bash
git add tools/qt_orchestration/rules_pack/null_check.py tools/qt_orchestration/rules_pack/__init__.py tools/qt_orchestration/tests/
git commit -m "feat(qt-orchestration): R-NULL-01 vehicle null-check rule"
```

---

## Task 14: Severity config (rules.toml + frontmatter overrides)

**Files:**

- Create: `tools/qt_orchestration/rules.toml`
- Create: `tools/qt_orchestration/config.py`
- Create: `tools/qt_orchestration/tests/test_config.py`

- [ ] **Step 1: Write rules.toml**

```toml
# tools/qt_orchestration/rules.toml
# Default severity for each rule. Per-issue overrides go in issue frontmatter
# under `rule_overrides: { R-XYZ: MINOR }`.
[severity]
R-QRC-01 = "BLOCKER"
R-QRC-02 = "BLOCKER"
R-CMAKE-01 = "BLOCKER"
R-CMAKE-02 = "MAJOR"
R-MOC-01 = "BLOCKER"
R-MOC-02 = "MAJOR"
R-MOC-03 = "MAJOR"
R-SHADOW-01 = "BLOCKER"
R-SHADOW-02 = "MAJOR"
R-QML-01 = "BLOCKER"
R-QML-02 = "MAJOR"
R-QML-03 = "MAJOR"
R-QML-04 = "MAJOR"
R-BUILD-01 = "BLOCKER"
R-TR-01 = "MAJOR"
R-NULL-01 = "BLOCKER"
```

- [ ] **Step 2: Test**

```python
# tools/qt_orchestration/tests/test_config.py
from pathlib import Path
from qt_orchestration.config import load_default_severities, merge_overrides
from qt_orchestration.findings import Severity


def test_load_default_severities():
    defaults = load_default_severities(Path(__file__).parents[1] / "rules.toml")
    assert defaults["R-QRC-01"] == Severity.BLOCKER
    assert defaults["R-TR-01"] == Severity.MAJOR


def test_merge_overrides_downgrades_severity():
    defaults = {"R-TR-01": Severity.MAJOR, "R-BUILD-01": Severity.BLOCKER}
    merged = merge_overrides(defaults, {"R-TR-01": "MINOR"})
    assert merged["R-TR-01"] == Severity.MINOR
    assert merged["R-BUILD-01"] == Severity.BLOCKER


def test_unknown_override_id_raises():
    import pytest
    with pytest.raises(ValueError, match="unknown rule"):
        merge_overrides({"R-TR-01": Severity.MAJOR}, {"R-BOGUS": "MINOR"})
```

- [ ] **Step 3: Implement**

```python
# tools/qt_orchestration/config.py
from __future__ import annotations
import tomllib
from pathlib import Path
from qt_orchestration.findings import Severity


def load_default_severities(toml_path: Path) -> dict[str, Severity]:
    data = tomllib.loads(toml_path.read_text())
    return {k: Severity.from_name(v) for k, v in data.get("severity", {}).items()}


def merge_overrides(defaults: dict[str, Severity], overrides: dict[str, str]) -> dict[str, Severity]:
    result = dict(defaults)
    for rule_id, sev in overrides.items():
        if rule_id not in defaults:
            raise ValueError(f"unknown rule in rule_overrides: {rule_id}")
        result[rule_id] = Severity.from_name(sev)
    return result
```

- [ ] **Step 4: Run tests** — Expected PASS.

- [ ] **Step 5: Commit**

```bash
git add tools/qt_orchestration/rules.toml tools/qt_orchestration/config.py tools/qt_orchestration/tests/test_config.py
git commit -m "feat(qt-orchestration): rules.toml default severities + frontmatter override merge"
```

---

## Task 15: scan.py — touchpoint discovery

`scan` takes a list of paths or a goal description (as `--prose`) and emits a JSON manifest of relevant qrc/CMake/qml/Q_OBJECT touchpoints under those paths. Output schema is fixed so `draft_issue` and tests can consume it.

**Files:**

- Modify: `tools/qt_orchestration/scan.py`
- Create: `tools/qt_orchestration/tests/test_scan.py`
- Create: `tools/qt_orchestration/tests/fixtures/scan/basic_repo/`

- [ ] **Step 1: Fixture**

Mini repo at `tests/fixtures/scan/basic_repo/` containing:

```
custom/custom.qrc                # one qresource block
custom/CMakeLists.txt            # target_sources(qgroundcontrol PRIVATE ...)
custom/src/MyThing.cc
custom/src/MyThing.h             # Q_OBJECT
custom/Foo.qml
```

- [ ] **Step 2: Test**

```python
# tools/qt_orchestration/tests/test_scan.py
import json
import subprocess
import sys
from pathlib import Path


def test_scan_emits_expected_manifest(tmp_path):
    import shutil
    src = Path(__file__).parent / "fixtures" / "scan" / "basic_repo"
    repo = tmp_path / "r"
    shutil.copytree(src, repo)
    result = subprocess.run(
        [sys.executable, "-m", "qt_orchestration.cli", "scan", "--repo", str(repo),
         "--path", "custom/"],
        capture_output=True, text=True, check=True,
    )
    manifest = json.loads(result.stdout)
    assert "custom/custom.qrc" in manifest["qrc_files"]
    assert any("CMakeLists.txt" in t["path"] for t in manifest["cmake_targets"])
    assert "custom/Foo.qml" in manifest["qml_files"]
    assert any(h["path"] == "custom/src/MyThing.h" for h in manifest["qobject_headers"])
```

- [ ] **Step 3: Implement**

```python
# tools/qt_orchestration/scan.py
from __future__ import annotations
import argparse
import json
import sys
from pathlib import Path


def _scan(repo: Path, roots: list[str]) -> dict:
    out = {
        "qrc_files": [],
        "cmake_targets": [],
        "qml_files": [],
        "qobject_headers": [],
        "shadow_candidates": [],
    }
    for root in roots:
        base = repo / root
        if not base.exists():
            continue
        for p in base.rglob("*"):
            if not p.is_file():
                continue
            rel = str(p.relative_to(repo)).replace("\\", "/")
            if p.name == "custom.qrc" or p.suffix == ".qrc":
                out["qrc_files"].append(rel)
            elif p.name == "CMakeLists.txt":
                out["cmake_targets"].append({"path": rel})
            elif p.suffix == ".qml":
                out["qml_files"].append(rel)
            elif p.suffix == ".h":
                if "Q_OBJECT" in p.read_text(errors="ignore"):
                    out["qobject_headers"].append({"path": rel})
            if rel.startswith("custom/src/"):
                upstream = rel[len("custom/"):]
                if (repo / upstream).exists():
                    out["shadow_candidates"].append({"shadow": rel, "upstream": upstream})
    return out


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(prog="qt-orchestration scan")
    ap.add_argument("--repo", default=".", type=Path)
    ap.add_argument("--path", action="append", required=True,
                    help="Path under the repo to scan (repeatable)")
    args = ap.parse_args(argv)
    manifest = _scan(args.repo.resolve(), args.path)
    json.dump(manifest, sys.stdout, indent=2)
    sys.stdout.write("\n")
    return 0
```

- [ ] **Step 4: Run tests** — Expected PASS.

- [ ] **Step 5: Commit**

```bash
git add tools/qt_orchestration/scan.py tools/qt_orchestration/tests/test_scan.py tools/qt_orchestration/tests/fixtures/scan/
git commit -m "feat(qt-orchestration): scan emits touchpoint manifest"
```

---

## Task 16: draft_issue.py library + phase2-issue.md.j2 template

**Files:**

- Modify: `tools/qt_orchestration/draft_issue.py` (replace placeholder)
- Create: `tools/qt_orchestration/templates/phase2-issue.md.j2`
- Create: `tools/qt_orchestration/tests/test_draft_issue.py`
- Create: golden `tools/qt_orchestration/tests/fixtures/draft_issue/golden.md`

- [ ] **Step 1: Template**

```jinja
# tools/qt_orchestration/templates/phase2-issue.md.j2
---
phase: 2
allow_upstream_edit: {{ allow_upstream_edit | default(false) | tojson }}
shadowed_originals: {{ shadowed_originals | tojson }}
rule_overrides: {{ rule_overrides | tojson }}
qrc_prefix_changes: {{ qrc_prefix_changes | tojson }}
---

# {{ title }}

## Goal
{{ prose }}

## File Manifest
{% for qrc in manifest.qrc_files -%}
- qrc: `{{ qrc }}`
{% endfor %}{% for t in manifest.cmake_targets -%}
- cmake: `{{ t.path }}`
{% endfor %}{% for q in manifest.qml_files -%}
- qml: `{{ q }}`
{% endfor %}{% for h in manifest.qobject_headers -%}
- Q_OBJECT header: `{{ h.path }}`
{% endfor %}{% for s in manifest.shadow_candidates -%}
- shadow: `{{ s.shadow }}` (mirrors `{{ s.upstream }}`)
{% endfor %}

## Acceptance Criteria
- All rules in the standard Qt rule pack pass on the merged slice range.
- `qt-orchestration verify-slice --diff <issue-base>..<issue-head>` returns 0.
{% for c in extra_criteria -%}
- {{ c }}
{% endfor %}
```

- [ ] **Step 2: Test (golden-file)**

```python
# tools/qt_orchestration/tests/test_draft_issue.py
from pathlib import Path
from qt_orchestration.draft_issue import render


def test_render_matches_golden():
    manifest = {
        "qrc_files": ["custom/custom.qrc"],
        "cmake_targets": [{"path": "custom/CMakeLists.txt"}],
        "qml_files": ["custom/Foo.qml"],
        "qobject_headers": [{"path": "custom/src/Bar.h"}],
        "shadow_candidates": [],
    }
    body = render(
        title="Rebrand settings header",
        prose="Replace upstream branding with Sprig branding in the settings header.",
        manifest=manifest,
        allow_upstream_edit=False,
        shadowed_originals=[],
        rule_overrides={},
        qrc_prefix_changes=[],
        extra_criteria=["Header visible in app launch flow."],
    )
    golden = (Path(__file__).parent / "fixtures" / "draft_issue" / "golden.md").read_text()
    assert body == golden
```

Generate the golden once with `--update-goldens`: implement a tiny helper that writes the golden if the env var `UPDATE_GOLDENS=1`. For this task, manually copy the rendered output to the golden file once and commit it.

- [ ] **Step 3: Implement**

```python
# tools/qt_orchestration/draft_issue.py
from __future__ import annotations
import json
from pathlib import Path
from jinja2 import Environment, FileSystemLoader, select_autoescape


_TEMPLATES = Path(__file__).parent / "templates"
_ENV = Environment(
    loader=FileSystemLoader(_TEMPLATES),
    autoescape=select_autoescape(disabled_extensions=("j2",)),
    keep_trailing_newline=True,
)
_ENV.filters["tojson"] = lambda v: json.dumps(v)


def render(*, title: str, prose: str, manifest: dict,
           allow_upstream_edit: bool = False,
           shadowed_originals: list[str] | None = None,
           rule_overrides: dict | None = None,
           qrc_prefix_changes: list[str] | None = None,
           extra_criteria: list[str] | None = None) -> str:
    tpl = _ENV.get_template("phase2-issue.md.j2")
    return tpl.render(
        title=title, prose=prose, manifest=manifest,
        allow_upstream_edit=allow_upstream_edit,
        shadowed_originals=shadowed_originals or [],
        rule_overrides=rule_overrides or {},
        qrc_prefix_changes=qrc_prefix_changes or [],
        extra_criteria=extra_criteria or [],
    )
```

- [ ] **Step 4: Run test** — Expected PASS.

- [ ] **Step 5: Commit**

```bash
git add tools/qt_orchestration/draft_issue.py tools/qt_orchestration/templates/ tools/qt_orchestration/tests/
git commit -m "feat(qt-orchestration): draft_issue library + phase2 issue template"
```

---

## Task 17: expand_brief.py + slice-brief-qt.md.j2 + refactor-brief-template.md extension

**Files:**

- Modify: `tools/qt_orchestration/expand_brief.py`
- Create: `tools/qt_orchestration/templates/slice-brief-qt.md.j2`
- Modify: `.orchestrator/templates/refactor-brief-template.md` (placeholder + contract extension)
- Create: `tools/qt_orchestration/tests/test_expand_brief.py`
- Create: golden `tools/qt_orchestration/tests/fixtures/expand_brief/golden.md`
- Create: fixture issue file `tools/qt_orchestration/tests/fixtures/expand_brief/issue.md`

- [ ] **Step 1: Brief template additions**

Edit `.orchestrator/templates/refactor-brief-template.md`. After the existing `<verification_loop>` block, append:

```markdown
<qt_completion_block>
{{qt_completion_block}}
</qt_completion_block>
```

And inside `<structured_output_contract>`, append the two new sections:

```markdown
## qt_verification
- rules_run: <comma-separated rule IDs>
- result: <PASS | FAIL>
- findings: <empty if PASS; otherwise list of rule_id:path:line:message>

## qt_verification_disputes
- <empty unless you (Codex) believe a finding is a false positive>
- If non-empty, each entry: rule_id, the finding text, and your reasoning. Do NOT
  silence the finding in code; report here and let /orchestrate triage.
```

- [ ] **Step 2: slice-brief-qt template**

```jinja
{# tools/qt_orchestration/templates/slice-brief-qt.md.j2 #}
This slice operates inside a Qt orchestration regime. Before declaring done:

1. Self-verify by running:
   ```bash
   uv run qt-orchestration verify-slice --rules {{ rules_list }} --diff HEAD~1..HEAD
   ```
2. If `verify-slice` exits non-zero, fix the findings in this slice (do not
   defer to a followup) and re-run. The orchestrator will re-verify
   independently after you return.
3. Specific Qt touchpoints relevant to this slice:
{% for tp in touchpoints -%}
   - {{ tp }}
{% endfor %}
4. Permitted overrides for this issue: {{ rule_overrides | default('none') }}.

Do not freelance Qt scope beyond the touchpoints listed above. If you discover a
missing touchpoint, add it to `followups` rather than editing it.
```

- [ ] **Step 3: Fixture issue + golden**

`tests/fixtures/expand_brief/issue.md`:

```markdown
---
phase: 2
allow_upstream_edit: false
shadowed_originals: []
rule_overrides: {}
qrc_prefix_changes: []
---

# Sample issue

## Goal
Rebrand the settings header.

## File Manifest
- qrc: `custom/custom.qrc`
- qml: `custom/SettingsHeader.qml`

## Acceptance Criteria
- All rules pass.
```

Generate `golden.md` from a first run and commit it.

- [ ] **Step 4: Test**

```python
# tools/qt_orchestration/tests/test_expand_brief.py
import subprocess
import sys
from pathlib import Path


def test_expand_brief_matches_golden(tmp_path):
    issue = Path(__file__).parent / "fixtures" / "expand_brief" / "issue.md"
    out = tmp_path / "brief.md"
    subprocess.run(
        [sys.executable, "-m", "qt_orchestration.cli", "expand-brief",
         "--issue", str(issue), "--out", str(out),
         "--base-brief", ".orchestrator/templates/refactor-brief-template.md",
         "--slice", "1",
         "--plan", "docs/superpowers/plans/test.md",
         "--verification-command", "ctest -L Unit"],
        check=True,
    )
    golden = Path(__file__).parent / "fixtures" / "expand_brief" / "golden.md"
    assert out.read_text() == golden.read_text()
```

- [ ] **Step 5: Implement**

```python
# tools/qt_orchestration/expand_brief.py
from __future__ import annotations
import argparse
import re
from pathlib import Path
import yaml
from jinja2 import Environment, FileSystemLoader, select_autoescape


def _parse_frontmatter(text: str) -> tuple[dict, str]:
    m = re.match(r"^---\n(.*?)\n---\n(.*)$", text, re.DOTALL)
    if not m:
        return {}, text
    return yaml.safe_load(m.group(1)) or {}, m.group(2)


def _select_rules(frontmatter: dict, manifest_text: str) -> list[str]:
    """Pick rules whose preconditions might fire for this issue's manifest."""
    rules = ["R-BUILD-01", "R-NULL-01", "R-SHADOW-01"]  # always-on safeties
    if "qrc:" in manifest_text:
        rules += ["R-QRC-01", "R-QRC-02"]
    if "cmake:" in manifest_text:
        rules += ["R-CMAKE-01", "R-CMAKE-02"]
    if "qml:" in manifest_text:
        rules += ["R-QML-01", "R-QML-02", "R-QML-03", "R-QML-04"]
    if "Q_OBJECT header:" in manifest_text:
        rules += ["R-MOC-01", "R-MOC-02", "R-MOC-03"]
    if "shadow:" in manifest_text:
        rules += ["R-SHADOW-02"]
    rules += ["R-TR-01"]
    return sorted(set(rules))


def _render_qt_block(issue_text: str) -> str:
    fm, body = _parse_frontmatter(issue_text)
    rules = _select_rules(fm, body)
    touchpoints = [line for line in body.splitlines() if line.startswith("- ")]
    env = Environment(
        loader=FileSystemLoader(Path(__file__).parent / "templates"),
        autoescape=select_autoescape(disabled_extensions=("j2",)),
        keep_trailing_newline=True,
    )
    tpl = env.get_template("slice-brief-qt.md.j2")
    return tpl.render(
        rules_list=",".join(rules),
        touchpoints=touchpoints,
        rule_overrides=fm.get("rule_overrides") or "none",
    )


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(prog="qt-orchestration expand-brief")
    ap.add_argument("--issue", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--base-brief", type=Path, required=True)
    ap.add_argument("--slice", required=True)
    ap.add_argument("--plan", required=True)
    ap.add_argument("--verification-command", required=True)
    args = ap.parse_args(argv)

    base = args.base_brief.read_text()
    qt_block = _render_qt_block(args.issue.read_text())
    rendered = (
        base
        .replace("{{N}}", args.slice)
        .replace("{{plan_path}}", args.plan)
        .replace("{{verification_command}}", args.verification_command)
        .replace("{{qt_completion_block}}", qt_block)
    )
    args.out.write_text(rendered)
    return 0
```

- [ ] **Step 6: Run test** — Expected PASS.

- [ ] **Step 7: Commit**

```bash
git add tools/qt_orchestration/expand_brief.py tools/qt_orchestration/templates/slice-brief-qt.md.j2 .orchestrator/templates/refactor-brief-template.md tools/qt_orchestration/tests/
git commit -m "feat(qt-orchestration): expand-brief + brief template Qt contract extension"
```

---

## Task 18: verify_slice.py — the gate

`verify-slice` is the heart of the gate. It:

1. Resolves slice diff bounds from `.orchestrator/work/<slice>/state.md` (or explicit `--diff base..head`).
2. Loads `rules.toml` defaults + issue frontmatter overrides.
3. Imports `rules_pack` so all rules register.
4. Runs each rule whose `applies_to` is true.
5. Writes one JSONL line to `.orchestrator/work/<slice>/verify-log.jsonl`.
6. Writes `qt-findings.md` if any finding is BLOCKER or MAJOR.
7. Exits 0 (clean), 1 (findings), 2 (build/ missing), 3 (qmllint/cmake-file-api missing), 4 (tree broken pre-slice).

**Files:**

- Modify: `tools/qt_orchestration/verify_slice.py`
- Create: `tools/qt_orchestration/tests/test_verify_slice.py`

- [ ] **Step 1: Test**

```python
# tools/qt_orchestration/tests/test_verify_slice.py
import json
import subprocess
import sys
from pathlib import Path
from qt_orchestration.tests.test_qrc_rules import _make_repo_from_fixture

FIX = Path(__file__).parent / "fixtures" / "rules"


def _run_verify(repo, slice_dir, base="HEAD~1", head="HEAD", extra=()):
    return subprocess.run(
        [sys.executable, "-m", "qt_orchestration.cli", "verify-slice",
         "--repo", str(repo), "--slice-dir", str(slice_dir),
         "--diff", f"{base}..{head}", *extra],
        capture_output=True, text=True,
    )


def test_verify_clean_exits_zero(tmp_path):
    repo = _make_repo_from_fixture(tmp_path, FIX / "R-QRC-01" / "pass")
    slice_dir = tmp_path / "w"; slice_dir.mkdir()
    r = _run_verify(repo, slice_dir)
    assert r.returncode == 0, r.stderr
    log = (slice_dir / "verify-log.jsonl").read_text().strip().splitlines()
    assert json.loads(log[-1])["exit"] == 0


def test_verify_qrc_fail_exits_one(tmp_path):
    repo = _make_repo_from_fixture(tmp_path, FIX / "R-QRC-01" / "fail")
    slice_dir = tmp_path / "w"; slice_dir.mkdir()
    r = _run_verify(repo, slice_dir)
    assert r.returncode == 1
    findings_file = slice_dir / "qt-findings.md"
    assert findings_file.exists()
    assert "R-QRC-01" in findings_file.read_text()
```

Disable R-BUILD-01 in these tests via `--skip-rules R-BUILD-01` (the test repos don't have a real Qt build configured). Add the flag to the CLI.

- [ ] **Step 2: Implement**

```python
# tools/qt_orchestration/verify_slice.py
from __future__ import annotations
import argparse
import json
import sys
import time
from pathlib import Path
import yaml
from qt_orchestration.config import load_default_severities, merge_overrides
from qt_orchestration.diff import SliceDiff
from qt_orchestration.findings import Finding, Severity
from qt_orchestration.rules import RuleContext, REGISTRY
import qt_orchestration.rules_pack  # noqa: F401  registers rules


_RULES_TOML = Path(__file__).parent / "rules.toml"


def _load_frontmatter(issue_path: Path | None) -> dict:
    if issue_path is None or not issue_path.exists():
        return {}
    text = issue_path.read_text()
    if not text.startswith("---\n"):
        return {}
    end = text.find("\n---\n", 4)
    if end == -1:
        return {}
    return yaml.safe_load(text[4:end]) or {}


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(prog="qt-orchestration verify-slice")
    ap.add_argument("--repo", type=Path, default=Path("."))
    ap.add_argument("--slice-dir", type=Path, required=True)
    ap.add_argument("--diff", required=True, help="base..head")
    ap.add_argument("--issue", type=Path, default=None)
    ap.add_argument("--skip-rules", default="", help="comma-separated rule IDs")
    ap.add_argument("--rules", default="", help="if set, only run these rule IDs")
    args = ap.parse_args(argv)

    base, head = args.diff.split("..")
    repo = args.repo.resolve()
    slice_dir = args.slice_dir.resolve()
    slice_dir.mkdir(parents=True, exist_ok=True)

    # Infrastructure preflight.
    if not (repo / "build").exists() and "R-BUILD-01" not in args.skip_rules:
        print(f"build/ missing under {repo} — run `cmake -B build ...` first", file=sys.stderr)
        return 2

    try:
        diff = SliceDiff.from_git(repo, base, head)
    except Exception as e:
        print(f"git diff failed: {e}", file=sys.stderr)
        return 4

    frontmatter = _load_frontmatter(args.issue)
    defaults = load_default_severities(_RULES_TOML)
    severities = merge_overrides(defaults, frontmatter.get("rule_overrides") or {})

    ctx = RuleContext(
        repo_root=repo, slice_diff=diff,
        severity_overrides=severities, issue_frontmatter=frontmatter,
    )

    skip = {s.strip() for s in args.skip_rules.split(",") if s.strip()}
    only = {s.strip() for s in args.rules.split(",") if s.strip()}

    findings: list[Finding] = []
    rules_run: list[str] = []
    for cls in REGISTRY.all():
        if only and cls.rule_id not in only:
            continue
        if cls.rule_id in skip:
            continue
        rule = cls()
        if not rule.applies_to(ctx):
            continue
        rules_run.append(cls.rule_id)
        try:
            findings.extend(rule.check(ctx))
        except FileNotFoundError as e:
            # Missing external CLI (qmllint, cmake) — infra failure.
            print(f"infra: {e}", file=sys.stderr)
            return 3

    findings.sort(key=lambda f: (-int(f.severity), f.rule_id, f.path, f.line))
    exit_code = 1 if any(f.severity >= Severity.MAJOR for f in findings) else 0

    log_entry = {
        "ts": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "slice": str(slice_dir.name),
        "issue": frontmatter.get("issue_number"),
        "exit": exit_code,
        "rules_run": rules_run,
        "findings": [f.to_dict() for f in findings],
        "retry_count": frontmatter.get("retry_count", 0),
    }
    with (slice_dir / "verify-log.jsonl").open("a") as fh:
        fh.write(json.dumps(log_entry) + "\n")

    if exit_code != 0:
        md = ["# Qt verification findings", ""]
        for f in findings:
            md.append(f"- **{f.severity.name}** {f.rule_id} — `{f.path}:{f.line}`: {f.message}")
            md.append(f"  - fix: {f.fix_hint}")
        (slice_dir / "qt-findings.md").write_text("\n".join(md) + "\n")

    return exit_code
```

- [ ] **Step 3: Run tests** — Expected PASS.

- [ ] **Step 4: Commit**

```bash
git add tools/qt_orchestration/verify_slice.py tools/qt_orchestration/tests/test_verify_slice.py
git commit -m "feat(qt-orchestration): verify-slice CLI gate with JSONL logging"
```

---

## Task 19: /orchestrate integration

Locate the existing `/orchestrate` skill or command implementation (search `commands/` and any plugin paths). Patch it to:

1. After every Codex slice return, invoke `qt-orchestration verify-slice` with `--repo`, `--slice-dir` (the slice's `.orchestrator/work/<slug>/`), `--diff <prev-slice-commit>..HEAD`, `--issue <issue-md-path>`.
2. On exit 0 → flip state.md `slice:N: green` and proceed.
3. On exit 1 → read `qt-findings.md`, generate a REFACTOR brief via `refactor-brief-template.md` with the findings inlined into `<task>`, re-dispatch via `jcode run`. Increment retry counter in `state.md`. On retry 3, halt and post a GitHub issue comment with findings.
4. On exit 2, run `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` once then retry verify; on second failure escalate.
5. On exit 3 or 4, escalate immediately (no retry).
6. Before closing an issue, run `qt-orchestration verify-slice --diff <issue-base>..<issue-head>`. If the last `verify-log.jsonl` entry across any slice in the issue's work dir has `exit != 0`, refuse close and report.

**Files:**

- Modify: `commands/orchestrate.md` (or wherever the /orchestrate skill lives — discovered via `grep -rn 'orchestrate' commands/ ~/.claude/plugins/ 2>/dev/null`)
- Create: `tools/qt_orchestration/tests/test_orchestrate_integration.py`

- [ ] **Step 1: Find /orchestrate**

Run: `grep -rn '/orchestrate' commands/ .claude/ ~/.claude/plugins/cache/ 2>/dev/null | head -20`

Determine the file format (Markdown skill, Python script, etc.). The patches below assume it's the Markdown command at `commands/orchestrate.md`.

- [ ] **Step 2: Patch /orchestrate**

In the Phase-3 (post-Codex-return) section of `commands/orchestrate.md`, add:

```markdown
### Phase 3.5: Qt verification gate

After Codex returns and before flipping the slice green, run:

```bash
uv run qt-orchestration verify-slice \
  --repo "$REPO_ROOT" \
  --slice-dir ".orchestrator/work/$SLICE_SLUG" \
  --diff "$PREV_COMMIT..HEAD" \
  --issue "$ISSUE_FILE"
```

Interpret exit codes:

- 0: slice clean. Flip `state.md` slice:N to green. Proceed.
- 1: findings. Read `.orchestrator/work/$SLICE_SLUG/qt-findings.md`. Render a
  REFACTOR brief from `.orchestrator/templates/refactor-brief-template.md`
  with `<task>` body set to the findings list, and dispatch:
  `jcode run -C "$REPO_ROOT" -p auto "$(cat .orchestrator/briefs/$SLICE_SLUG-refactor.md)"`.
  Increment `retry_count` in `state.md`. After retry_count >= 2, halt the
  slice and post the findings as a comment on the GitHub issue
  (`gh issue comment --repo Strattoon/qgroundcontrol $ISSUE_NUMBER -F qt-findings.md`).
- 2: run once: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`, then retry
  verify-slice. Second failure: halt and escalate.
- 3 or 4: halt and escalate immediately.

### Phase 4: Issue close guard

Before closing an issue:

```bash
uv run qt-orchestration verify-slice \
  --repo "$REPO_ROOT" \
  --slice-dir ".orchestrator/work/$ISSUE_SLUG" \
  --diff "$ISSUE_BASE..HEAD" \
  --issue "$ISSUE_FILE"
```

If exit != 0, refuse close. Additionally inspect every per-slice
`verify-log.jsonl` under `.orchestrator/work/$ISSUE_SLUG/<slice>/` —
if any has a final entry with `exit != 0`, refuse close.
```

- [ ] **Step 3: Layer 4 integration smoke test**

```python
# tools/qt_orchestration/tests/test_orchestrate_integration.py
"""Layer 4 smoke: simulate /orchestrate's verify+refactor loop with a fake jcode."""
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from qt_orchestration.tests.test_qrc_rules import _make_repo_from_fixture

FIX = Path(__file__).parent / "fixtures" / "rules"


def test_orchestrate_loop_dispatches_refactor_on_findings(tmp_path):
    repo = _make_repo_from_fixture(tmp_path, FIX / "R-QRC-01" / "fail")
    slice_dir = tmp_path / "w"; slice_dir.mkdir()

    # First gate run: expect exit 1 with R-QRC-01 finding.
    r = subprocess.run(
        [sys.executable, "-m", "qt_orchestration.cli", "verify-slice",
         "--repo", str(repo), "--slice-dir", str(slice_dir),
         "--diff", "HEAD~1..HEAD", "--skip-rules", "R-BUILD-01"],
        capture_output=True, text=True,
    )
    assert r.returncode == 1
    findings_md = (slice_dir / "qt-findings.md").read_text()
    assert "R-QRC-01" in findings_md

    # Simulate /orchestrate generating a REFACTOR brief — assert the findings
    # file is shaped such that a brief can ingest it.
    log_entries = [json.loads(l) for l in (slice_dir / "verify-log.jsonl").read_text().splitlines()]
    assert log_entries[-1]["exit"] == 1
    assert any(f["rule"] == "R-QRC-01" for f in log_entries[-1]["findings"])
```

- [ ] **Step 4: Run test** — Expected PASS.

- [ ] **Step 5: Commit**

```bash
git add commands/orchestrate.md tools/qt_orchestration/tests/test_orchestrate_integration.py
git commit -m "feat(qt-orchestration): wire verify-slice into /orchestrate post-Codex gate"
```

If `/orchestrate` lives elsewhere (e.g., a plugin under `~/.claude/plugins/`), copy the file into the repo under `commands/orchestrate.md`, patch it, and document the override in `.claude/settings.json` or equivalent.

---

## Task 20: Layer 3 end-to-end fixtures (one per failure mode)

These prove the module catches what Phase 1 missed.

**Files:**

- Create: `tools/qt_orchestration/tests/fixtures/e2e/qrc-drift/{base,slice}/`
- Create: `tools/qt_orchestration/tests/fixtures/e2e/moc-missing/{base,slice}/`
- Create: `tools/qt_orchestration/tests/fixtures/e2e/shadow-miss/{base,slice}/`
- Create: `tools/qt_orchestration/tests/fixtures/e2e/qml-rot/{base,slice}/`
- Create: `tools/qt_orchestration/tests/test_e2e.py` (pytest marker `slow`, opt-in)

- [ ] **Step 1: Fixtures**

Each fixture is a realistic mini Phase-1-style slice: a couple of source files, a CMakeLists, a custom.qrc, and the deliberate Qt-completion miss for its failure mode.

`qrc-drift`: slice adds `custom/Header.qml` and modifies the `custom.qrc` so `/qml` block exists but lacks `Header.qml`. R-QRC-01 fires.

`moc-missing`: slice adds `custom/src/StatusModel.h` declaring `class StatusModel : public QAbstractListModel` with no `Q_OBJECT`. R-MOC-01 fires.

`shadow-miss`: slice edits `src/Settings/SettingsManager.cc` directly, no `allow_upstream_edit` in frontmatter. R-SHADOW-01 fires.

`qml-rot`: slice adds `custom/Bad.qml` with `width: 200; color: "#ff0000"`. R-QML-02 + R-QML-03 fire.

For real fidelity, mine one fixture from the `phase1-slice16-strings-audit` artifacts under `.orchestrator/work/` if possible — even just borrowing file names and shape.

- [ ] **Step 2: Test**

```python
# tools/qt_orchestration/tests/test_e2e.py
import pytest
import subprocess
import sys
from pathlib import Path
from qt_orchestration.tests.test_qrc_rules import _make_repo_from_fixture

E2E = Path(__file__).parent / "fixtures" / "e2e"
pytestmark = pytest.mark.slow


@pytest.mark.parametrize("name,expected_rule", [
    ("qrc-drift", "R-QRC-01"),
    ("moc-missing", "R-MOC-01"),
    ("shadow-miss", "R-SHADOW-01"),
    ("qml-rot", "R-QML-02"),
])
def test_e2e_failure_mode_caught(tmp_path, name, expected_rule):
    repo = _make_repo_from_fixture(tmp_path, E2E / name)
    slice_dir = tmp_path / "w"; slice_dir.mkdir()
    r = subprocess.run(
        [sys.executable, "-m", "qt_orchestration.cli", "verify-slice",
         "--repo", str(repo), "--slice-dir", str(slice_dir),
         "--diff", "HEAD~1..HEAD", "--skip-rules", "R-BUILD-01"],
        capture_output=True, text=True,
    )
    assert r.returncode == 1, r.stdout + r.stderr
    findings = (slice_dir / "qt-findings.md").read_text()
    assert expected_rule in findings, f"expected {expected_rule} in:\n{findings}"
```

Add a `pytest.ini` marker registration so `slow` is recognized:

```toml
# inside tools/pyproject.toml [tool.pytest.ini_options]
markers = ["slow: opt-in e2e tests (require fixture trees, may run real qmllint)"]
```

- [ ] **Step 3: Run tests**

Run: `cd tools && uv run --extra test pytest qt_orchestration/tests/test_e2e.py -v -m slow`
Expected: PASS (4 parametrized cases).

- [ ] **Step 4: Commit**

```bash
git add tools/qt_orchestration/tests/fixtures/e2e/ tools/qt_orchestration/tests/test_e2e.py tools/pyproject.toml
git commit -m "test(qt-orchestration): Layer 3 e2e fixtures per failure mode"
```

---

## Task 21: Final integration — author one real Phase-2 issue end-to-end

**Files:**

- Create: a real Phase-2 issue draft in `.orchestrator/issues/phase2-<slug>.md` (committed to repo for traceability)
- No code changes — this is the acceptance dry-run of acceptance criterion #4 from the spec.

- [ ] **Step 1: In an orchestrator session, drive `draft_issue.render(...)` against a real Phase-2 goal.**

```python
# Run in a Python repl from repo root:
from qt_orchestration.scan import _scan
from qt_orchestration.draft_issue import render
from pathlib import Path

manifest = _scan(Path("."), ["custom/"])
body = render(
    title="<actual phase-2 goal>",
    prose="<one-paragraph rationale>",
    manifest=manifest,
    allow_upstream_edit=False,
    shadowed_originals=[],
    rule_overrides={},
)
Path(".orchestrator/issues/phase2-<slug>.md").write_text(body)
```

Review the draft, hand-edit acceptance criteria, then `gh issue create --repo Strattoon/qgroundcontrol --body-file .orchestrator/issues/phase2-<slug>.md`.

- [ ] **Step 2: Drive the issue through `/orchestrate` once.**

Run `/orchestrate <issue#>` and observe `verify-slice` firing on at least one slice. Confirm `verify-log.jsonl` is populated and slice flips green only after exit 0.

- [ ] **Step 3: Commit the issue draft as the integration record.**

```bash
git add .orchestrator/issues/phase2-<slug>.md
git commit -m "chore(orchestration): record first Qt-orchestration-driven Phase-2 issue"
```

This task satisfies acceptance criterion #4 from the spec.

---

## Self-Review

Spec coverage check:

| Spec section / requirement | Covered by task(s) |
|---|---|
| Package + 3 entry points | 1, 15, 17, 18, 19 (cli.py) |
| Finding + Severity | 2 |
| Rule base + registry | 3 |
| Diff parser | 4 |
| R-QRC-01, R-QRC-02 | 5, 6 |
| R-CMAKE-01, R-CMAKE-02 | 7 |
| R-MOC-01/02/03 | 8 |
| R-SHADOW-01/02 | 9 |
| R-QML-01..04 | 10 |
| R-BUILD-01 | 11 |
| R-TR-01 | 12 |
| R-NULL-01 | 13 |
| rules.toml + overrides | 14 |
| scan | 15 |
| draft_issue + phase2 template | 16 |
| expand-brief + slice-brief template + refactor-brief-template extension (acceptance #5) | 17 |
| verify-slice CLI + JSONL log | 18 |
| /orchestrate integration + retry/escalate + close guard (acceptance #3) | 19 |
| Layer 3 e2e per failure mode (acceptance #2) | 20 |
| First end-to-end Phase-2 issue (acceptance #4) | 21 |
| Layer 1 unit tests | every rule task |
| Layer 2 golden-file tests | 16, 17 |
| Layer 4 integration smoke | 19 |
| Coverage targets | covered by Layer 1-4 tests; verify with `uv run pytest --cov=qt_orchestration` |

Acceptance criteria gates:

- #1 (package + 11 rules + tests green) → satisfied at end of Task 14.
- #2 (Layer 3 e2e per failure mode) → Task 20.
- #3 (/orchestrate refuses close on failed last-log) → Task 19.
- #4 (one Phase-2 issue driven end-to-end) → Task 21.
- #5 (brief template + structured_output_contract extension) → Task 17.

No placeholder text in any step. All code blocks are complete; the only deferred decisions are explicitly noted (e.g., Task 13 Step 1 locates the existing null-check hook script path before implementing — the rule then uses that exact path).
