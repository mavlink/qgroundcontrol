"""Tests for the tools/moccache package — the AUTOMOC caching wrapper.

Covers the pure helper functions plus end-to-end behavior against a fake moc
that mimics the real one's caching-relevant quirks: it embeds a relative
#include back to the input header, honors --output-dep-file/--dep-file-path,
and derives its output from input + force-included content + args.
"""

from __future__ import annotations

import concurrent.futures
import os
import re
import shutil
import stat
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path

import pytest
from moccache import core, stats, trim

from ._helpers import REPO_ROOT, TOOLS_DIR

pytestmark = pytest.mark.windows

LAUNCHER = TOOLS_DIR / "moccache_launcher.py"

# Realistic epoch base so build-start plausibility checks behave as in production.
_T0 = 1_700_000_000.0


@pytest.fixture(autouse=True)
def _unix_ninja_log_mtimes(monkeypatch: pytest.MonkeyPatch) -> None:
    """Tests write Unix-ns .ninja_log mtimes regardless of the host platform."""
    monkeypatch.setattr(stats, "_NINJA_LOG_MTIME_IS_FILETIME", False)


FAKE_MOC = """#!/usr/bin/env python3
import hashlib
import os
import sys


def expand_response_files(argv):
    expanded = []
    for arg in argv:
        if arg.startswith("@") and os.path.isfile(arg[1:]):
            with open(arg[1:], encoding="utf-8") as response_file:
                expanded.extend(response_file.read().splitlines())
        else:
            expanded.append(arg)
    return expanded


def main() -> int:
    argv = expand_response_files(sys.argv[1:])
    if argv == ["--version"]:
        version_log = os.environ.get("FAKEMOC_VERSION_LOG")
        if version_log:
            with open(version_log, "a") as f:
                f.write("--version\\n")
        print("fake-moc " + os.environ.get("FAKEMOC_VERSION", "1.0"))
        return 0
    out = None
    dep_path = None
    wants_dep = False
    wants_json = False
    includes = []
    hash_args = []
    positional = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == "-o":
            i += 1
            out = argv[i]
        elif a == "--include":
            i += 1
            includes.append(argv[i])
        elif a == "--output-dep-file":
            wants_dep = True
        elif a == "--dep-file-path":
            i += 1
            dep_path = argv[i]
        elif a == "--output-json":
            wants_json = True
            hash_args.append(a)
        elif a.startswith("-"):
            hash_args.append(a)
        else:
            positional.append(a)
        i += 1
    inp = positional[-1]
    log = os.environ.get("FAKEMOC_LOG")
    if log:
        with open(log, "a") as f:
            f.write(inp + "\\n")
    if os.environ.get("FAKEMOC_FAIL"):
        sys.stderr.write("fake-moc: forced failure\\n")
        return 3
    deps = [inp] + includes
    sidecar = inp + ".deps"
    if os.path.exists(sidecar):
        with open(sidecar) as f:
            deps += [ln.strip() for ln in f if ln.strip()]
    h = hashlib.sha256()
    h.update(os.environ.get("FAKEMOC_VERSION", "1.0").encode())
    h.update(" ".join(hash_args).encode())
    for d in deps:
        with open(d, "rb") as f:
            h.update(f.read())
    rel = os.path.relpath(inp, os.path.dirname(os.path.abspath(out)) or ".")
    with open(out, "w") as f:
        f.write('#include "' + rel + '"\\n// fake-moc ' + h.hexdigest() + "\\n")
    if wants_dep:
        target = dep_path if dep_path else out + ".d"
        with open(target, "w") as f:
            body = " \\\\\\n  ".join(d.replace(" ", "\\\\ ") for d in deps)
            f.write(out.replace(" ", "\\\\ ") + ": \\\\\\n  " + body + "\\n")
    if wants_json:
        with open(out + ".json", "w") as f:
            f.write('{"inputFile": "' + inp + '"}\\n')
    return 0


if __name__ == "__main__":
    sys.exit(main())
"""


@dataclass
class Tree:
    """One simulated build tree (its own basedir, moc_predefs.h, output dirs)."""

    build: Path
    predefs: Path

    def out_path(self, name: str = "moc_Foo.cpp") -> Path:
        d = self.build / "src" / "Mod" / "Mod_autogen" / "XYZ"
        d.mkdir(parents=True, exist_ok=True)
        return d / name


class Harness:
    def __init__(self, root: Path, monkeypatch: pytest.MonkeyPatch) -> None:
        self.root = root
        self.monkeypatch = monkeypatch
        self.cache = root / "cache"
        self.src = root / "repo" / "src"
        self.src.mkdir(parents=True)
        self.input = self.src / "Foo.h"
        self.input.write_text("class Foo {};\n")
        self.dep_header = self.src / "Dep.h"
        self.dep_header.write_text("// dep v1\n")
        (self.src / "Foo.h.deps").write_text(f"{self.dep_header}\n")
        fake_moc_script = root / "fake-moc.py"
        fake_moc_script.write_text(FAKE_MOC)
        if sys.platform == "win32":
            self.fake_moc = root / "fake-moc.cmd"
            self.fake_moc.write_text(
                f'@echo off\n"{sys.executable}" "{fake_moc_script}" %*\n', encoding="utf-8"
            )
        else:
            self.fake_moc = fake_moc_script
            self.fake_moc.chmod(0o755)
        self.moc_log = root / "moc-invocations.log"
        self.version_log = root / "moc-version-queries.log"
        for var in ("MOCCACHE_DISABLE", "MOCCACHE_MOC", "MOCCACHE_BASEDIR"):
            monkeypatch.delenv(var, raising=False)
        monkeypatch.setenv("MOCCACHE_DIR", str(self.cache))
        monkeypatch.setenv("MOCCACHE_STATS", "1")
        monkeypatch.setenv("FAKEMOC_LOG", str(self.moc_log))
        monkeypatch.setenv("FAKEMOC_VERSION_LOG", str(self.version_log))

    def make_tree(self, name: str, root: Path | None = None) -> Tree:
        build = (root or self.root / "repo") / name
        build.mkdir(parents=True, exist_ok=True)
        predefs = build / "moc_predefs.h"
        predefs.write_text(f'#define QT_TESTCASE_BUILDDIR "{build}"\n#define __FAKE__ 1\n')
        return Tree(build=build, predefs=predefs)

    def run(
        self,
        tree: Tree,
        out: Path | None = None,
        extra_args: tuple[str, ...] = (),
        input_file: Path | None = None,
        argv_override: list[str] | None = None,
    ) -> int:
        out = out or tree.out_path()
        self.monkeypatch.setenv("MOCCACHE_BASEDIR", str(tree.build))
        if argv_override is not None:
            argv = argv_override
        else:
            argv = [
                "--include",
                str(tree.predefs),
                "-I" + str(self.src),
                *extra_args,
                str(input_file or self.input),
                "-o",
                str(out),
            ]
        self.monkeypatch.setattr(
            sys, "argv", ["moccache_launcher.py", "--real-moc", str(self.fake_moc), *argv]
        )
        return core.main()

    def stats(self) -> list[str]:
        """Event kinds in log order, with miss reasons collapsed to "miss"."""
        return ["miss" if k.startswith("miss-") else k for k, _ in self.stats_detailed()]

    def stats_detailed(self) -> list[tuple[str, str]]:
        """(kind, detail) pairs in log order; detail is "" when absent."""
        log = self.cache / "stats.log"
        if not log.is_file():
            return []
        out = []
        for line in log.read_text(encoding="utf-8").splitlines():
            fields = line.split("\t")
            out.append((fields[0], fields[4] if len(fields) > 4 else ""))
        return out

    def moc_runs(self) -> int:
        if not self.moc_log.is_file():
            return 0
        return len(self.moc_log.read_text().splitlines())

    def version_queries(self) -> int:
        """How many times `moc --version` was spawned."""
        if not self.version_log.is_file():
            return 0
        return len(self.version_log.read_text().splitlines())

    def upgrade_moc(self, version: str) -> None:
        """Swap in a different moc: new --version output and a new binary mtime."""
        self.monkeypatch.setenv("FAKEMOC_VERSION", version)
        st = self.fake_moc.stat()
        os.utime(self.fake_moc, ns=(st.st_atime_ns, st.st_mtime_ns + 1_000_000_000))

    def manifest(self) -> Path:
        (manifest,) = self.cache.glob("??/*/manifest")
        return manifest


@pytest.fixture
def harness(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Harness:
    return Harness(tmp_path, monkeypatch)


def write_ninja_log(build_dir: Path, *edges: tuple[int, int, int]) -> None:
    """Write a .ninja_log with (start_ms, end_ms, mtime_ns) edges."""
    build_dir.mkdir(parents=True, exist_ok=True)
    body = "# ninja log v7\n" + "".join(
        f"{s}\t{e}\t{m}\tout{i}\t{i:016x}\n" for i, (s, e, m) in enumerate(edges)
    )
    (build_dir / ".ninja_log").write_text(body, encoding="utf-8")


def show_stats(harness: Harness, *extra: str) -> int:
    """Run --show-stats against a build that began an hour ago (before every log entry)."""
    build_dir = harness.root / "bld"
    write_ninja_log(build_dir, (0, 1000, int((time.time() - 3600) * 1e9)))
    harness.monkeypatch.setattr(
        sys, "argv", ["moccache_launcher.py", "--show-stats", "--build-dir", str(build_dir), *extra]
    )
    return core.main()


# ---------------------------------------------------------------------------
# Basic hit/miss behavior
# ---------------------------------------------------------------------------


class TestBasicCaching:
    def test_first_run_misses_second_hits(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        assert harness.run(tree, out=tree.out_path("a.cpp")) == 0
        assert harness.run(tree, out=tree.out_path("b.cpp")) == 0
        assert harness.stats() == ["miss", "hit"]

    def test_hit_output_byte_identical(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        out_a = tree.out_path("a.cpp")
        out_b = tree.out_path("b.cpp")
        harness.run(tree, out=out_a)
        harness.run(tree, out=out_b)
        assert out_a.read_bytes() == out_b.read_bytes()

    def test_hit_does_not_invoke_moc(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        runs_after_miss = harness.moc_runs()
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.moc_runs() == runs_after_miss

    def test_hit_skips_spawn_and_copy_imports(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        out = tree.out_path("b.cpp")
        argv = ["--real-moc", str(harness.fake_moc), "--include", str(tree.predefs)]
        argv += ["-I" + str(harness.src), str(harness.input), "-o", str(out)]
        # Baseline after moccache's own top-level stdlib imports, so a Python whose
        # pathlib or hashlib pulls in shutil does not fail this test
        probe = (
            "import sys\n"
            "import contextlib, functools, hashlib, os, re, stat, time, pathlib\n"
            "before = set(sys.modules)\n"
            "from moccache.core import main\n"
            "sys.argv = ['moccache_launcher.py', *sys.argv[1:]]\n"
            "rc = main()\n"
            "new = set(sys.modules) - before\n"
            "lazy = {'shutil', 'subprocess', 'tempfile', 'moccache.stats', 'moccache.trim'}\n"
            "print(rc, sorted(new & lazy))\n"
        )
        result = subprocess.run(
            [sys.executable, "-c", probe, *argv],
            env={
                **os.environ,
                "PYTHONPATH": str(TOOLS_DIR),
                "MOCCACHE_BASEDIR": str(tree.build),
            },
            capture_output=True,
            text=True,
            check=True,
        )
        assert result.stdout.split() == ["0", "[]"]
        assert harness.stats() == ["miss", "hit"]

    def test_launcher_stub_runs_main(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        out = tree.out_path("a.cpp")
        rc = subprocess.run(
            [
                sys.executable,
                str(LAUNCHER),
                "--real-moc",
                str(harness.fake_moc),
                "--include",
                str(tree.predefs),
                str(harness.input),
                "-o",
                str(out),
            ],
            env={**os.environ, "MOCCACHE_BASEDIR": str(tree.build)},
            check=False,
        ).returncode
        assert rc == 0
        assert out.is_file()
        assert harness.stats() == ["miss"]

    def test_launcher_runs_stats_and_trim_subcommands(self, harness: Harness) -> None:
        harness.run(harness.make_tree("build"))
        launch = [sys.executable, str(LAUNCHER)]
        shown = subprocess.run(
            [*launch, "--show-stats", "--build-dir", str(harness.root / "bld")],
            capture_output=True,
            text=True,
            check=True,
        ).stdout
        assert "misses   1" in shown
        trimmed = subprocess.run(
            [*launch, "--trim", "--max-size", "1G"], capture_output=True, text=True, check=True
        ).stdout
        assert trimmed.startswith("moccache: trimmed 0 entries")

    def test_cache_dir_defaults_to_source_tree(self, monkeypatch: pytest.MonkeyPatch) -> None:
        monkeypatch.delenv("MOCCACHE_DIR", raising=False)
        assert core._cache_dir() == REPO_ROOT / ".cache" / "moccache"

    def test_stats_log_records_input(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.monkeypatch.setattr(core.time, "time", lambda: 1700000000.1234567)
        harness.run(tree)
        line = (harness.cache / "stats.log").read_text(encoding="utf-8").splitlines()[0]
        what, input_file, when, basedir, detail = line.split("\t")
        assert what == "miss-first-seen"
        assert input_file == str(harness.input)
        assert when == "1700000000.123"
        assert basedir == str(tree.build)
        assert detail == ""

    def test_stats_log_fields_cannot_break_the_tab_format(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        monkeypatch.setenv("MOCCACHE_STATS", "1")
        monkeypatch.setenv("MOCCACHE_BASEDIR", "/b\tdir")
        core._log_stat(tmp_path, "miss-args-changed", "/src/we\tird.h", "-DX=a\tb -> -DX=c\nd")
        line = (tmp_path / "stats.log").read_text(encoding="utf-8")
        assert line.count("\n") == 1
        fields = line.rstrip("\n").split("\t")
        assert len(fields) == 5
        assert fields[1] == "/src/we ird.h"
        assert fields[3] == "/b dir"
        assert fields[4] == "-DX=a b -> -DX=c d"

    def test_cmake_response_file_misses_then_hits(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        response_dir = harness.root / "response files"
        response_dir.mkdir()
        response_file = response_dir / "moc.rsp"

        def write_response(output: Path) -> None:
            response_file.write_text(
                "\n".join(
                    [
                        "--include",
                        str(tree.predefs),
                        "-I" + str(harness.src),
                        "--output-dep-file",
                        "-o",
                        str(output),
                        str(harness.input),
                    ]
                )
                + "\n"
            )

        write_response(tree.out_path("a.cpp"))
        assert harness.run(tree, argv_override=[f"@{response_file}"]) == 0
        write_response(tree.out_path("b.cpp"))
        assert harness.run(tree, argv_override=[f"@{response_file}"]) == 0
        assert harness.stats() == ["miss", "hit"]
        assert harness.moc_runs() == 1


# ---------------------------------------------------------------------------
# Invalidation
# ---------------------------------------------------------------------------


class TestInvalidation:
    def test_input_edit_invalidates_revert_restores(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        original = harness.input.read_text()
        harness.run(tree, out=tree.out_path("a.cpp"))
        harness.input.write_text("class Foo { int changed; };\n")
        harness.run(tree, out=tree.out_path("b.cpp"))
        harness.input.write_text(original)
        harness.run(tree, out=tree.out_path("c.cpp"))
        assert harness.stats() == ["miss", "miss", "hit"]

    def test_transitive_dep_edit_invalidates(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        harness.dep_header.write_text("// dep v2\n")
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats() == ["miss", "miss"]

    def test_transitive_dep_deleted_invalidates(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        harness.dep_header.unlink()
        (harness.src / "Foo.h.deps").write_text("")
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats() == ["miss", "miss"]

    def test_include_real_content_change_invalidates(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        tree.predefs.write_text(tree.predefs.read_text() + "#define EXTRA 1\n")
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats() == ["miss", "miss"]

    def test_arg_change_invalidates(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"), extra_args=("-DFOO=1",))
        harness.run(tree, out=tree.out_path("b.cpp"), extra_args=("-DFOO=2",))
        assert harness.stats() == ["miss", "miss"]

    def test_moc_version_change_invalidates(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        harness.upgrade_moc("2.0")
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats() == ["miss", "miss"]


# ---------------------------------------------------------------------------
# Cross-tree sharing (the whole point of the cache)
# ---------------------------------------------------------------------------


class TestCrossTree:
    def test_same_depth_sibling_tree_hits(self, harness: Harness) -> None:
        tree_a = harness.make_tree("build")
        tree_b = harness.make_tree("build2")
        harness.run(tree_a)
        harness.run(tree_b)
        assert harness.stats() == ["miss", "hit"]

    def test_predefs_differing_only_in_basedir_hits(self, harness: Harness) -> None:
        tree_a = harness.make_tree("build")
        tree_b = harness.make_tree("build2")
        assert tree_a.predefs.read_text() != tree_b.predefs.read_text()
        harness.run(tree_a)
        harness.run(tree_b)
        assert harness.stats() == ["miss", "hit"]

    def test_different_depth_tree_misses_with_correct_output(self, harness: Harness) -> None:
        # moc embeds a relative #include from the output dir back to the input;
        # a tree at a different filesystem depth must NOT reuse the entry.
        tree_a = harness.make_tree("build")
        deep_root = harness.root / "a" / "b" / "c"
        tree_b = harness.make_tree("build", root=deep_root)
        out_b = tree_b.out_path()
        harness.run(tree_a)
        harness.run(tree_b, out=out_b)
        assert harness.stats() == ["miss", "miss"]
        rel = os.path.relpath(harness.input, out_b.parent)
        assert f'#include "{rel}"' in out_b.read_text()

    def test_hit_output_compilable_include_path(self, harness: Harness) -> None:
        # The relative #include served from cache must resolve in the new tree.
        tree_a = harness.make_tree("build")
        tree_b = harness.make_tree("build2")
        out_b = tree_b.out_path()
        harness.run(tree_a)
        harness.run(tree_b, out=out_b)
        assert harness.stats() == ["miss", "hit"]
        first_line = out_b.read_text().splitlines()[0]
        rel = first_line.split('"')[1]
        assert (out_b.parent / rel).resolve() == harness.input.resolve()

    def test_dep_file_synthesized_for_current_tree(self, harness: Harness) -> None:
        tree_a = harness.make_tree("build")
        tree_b = harness.make_tree("build2")
        out_b = tree_b.out_path()
        harness.run(tree_a, extra_args=("--output-dep-file",))
        harness.run(tree_b, out=out_b, extra_args=("--output-dep-file",))
        assert harness.stats() == ["miss", "hit"]
        dep_text = Path(str(out_b) + ".d").read_text()
        assert str(tree_b.predefs) in dep_text.replace("\\ ", " ")
        assert str(tree_a.build) + "/" not in dep_text  # no originating-tree paths
        deps = core._parse_dep_file(dep_text)
        assert str(harness.input) in deps
        assert str(harness.dep_header) in deps


# ---------------------------------------------------------------------------
# Passthrough and failure modes
# ---------------------------------------------------------------------------


class TestFailureModes:
    def test_disable_env_passes_through(self, harness: Harness) -> None:
        harness.monkeypatch.setenv("MOCCACHE_DISABLE", "1")
        tree = harness.make_tree("build")
        assert harness.run(tree) == 0
        assert harness.stats() == []
        assert tree.out_path().is_file()

    def test_moc_failure_propagates_and_is_not_cached(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.monkeypatch.setenv("FAKEMOC_FAIL", "1")
        assert harness.run(tree) == 3
        harness.monkeypatch.delenv("FAKEMOC_FAIL")
        harness.run(tree)
        assert harness.stats() == ["miss"]  # failed run logs nothing

    def test_missing_input_passes_through(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        rc = harness.run(tree, input_file=harness.src / "DoesNotExist.h")
        assert rc != 0  # fake moc fails opening it; wrapper must not mask that
        assert harness.stats() == []

    def test_no_output_arg_passes_through(self, harness: Harness) -> None:
        # A bare --version has no -o/input; the wrapper must pass it straight
        # through to the real moc without any cache activity.
        tree = harness.make_tree("build")
        rc = harness.run(tree, argv_override=["--version"])
        assert rc == 0
        assert harness.stats() == []

    def test_missing_real_moc_errors(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.monkeypatch.setattr(
            sys,
            "argv",
            ["moccache_launcher.py", "--real-moc", str(harness.root / "nope"), str(tree.build)],
        )
        assert core.main() == 2

    def test_corrupted_manifest_recovers(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        manifest = next(harness.cache.rglob("manifest"))
        manifest.write_text("garbage without tabs\nmore garbage\n")
        out_b = tree.out_path("b.cpp")
        assert harness.run(tree, out=out_b) == 0
        assert out_b.is_file()
        assert harness.stats() == ["miss", "miss"]

    def test_missing_cached_output_recovers(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        next(harness.cache.rglob("output.cpp")).unlink()
        out_b = tree.out_path("b.cpp")
        assert harness.run(tree, out=out_b) == 0
        assert out_b.is_file()

    @pytest.mark.skipif(sys.platform == "win32", reason="Windows does not use POSIX mode bits")
    def test_readonly_cache_does_not_fail_build(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.cache.mkdir(parents=True, exist_ok=True)
        harness.cache.chmod(stat.S_IRUSR | stat.S_IXUSR)
        try:
            out = tree.out_path()
            assert harness.run(tree, out=out) == 0
            assert out.is_file()
        finally:
            harness.cache.chmod(stat.S_IRWXU)


# ---------------------------------------------------------------------------
# JSON sidecar output
# ---------------------------------------------------------------------------


class TestJsonOutput:
    def test_json_cached_and_restored(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        out_a = tree.out_path("a.cpp")
        out_b = tree.out_path("b.cpp")
        harness.run(tree, out=out_a, extra_args=("--output-json",))
        harness.run(tree, out=out_b, extra_args=("--output-json",))
        assert harness.stats() == ["miss", "hit"]
        assert Path(str(out_b) + ".json").read_bytes() == Path(str(out_a) + ".json").read_bytes()

    def test_json_requested_but_not_cached_misses(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))  # no json cached
        out_b = tree.out_path("b.cpp")
        harness.run(tree, out=out_b, extra_args=("--output-json",))
        assert Path(str(out_b) + ".json").is_file()


# ---------------------------------------------------------------------------
# Concurrency
# ---------------------------------------------------------------------------


class TestParallel:
    def test_parallel_cold_start_no_corruption(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        outs = [tree.out_path(f"p{i}.cpp") for i in range(8)]

        def one(out: Path) -> int:
            return subprocess.run(
                [
                    sys.executable,
                    str(LAUNCHER),
                    "--real-moc",
                    str(harness.fake_moc),
                    "--include",
                    str(tree.predefs),
                    "-I" + str(harness.src),
                    str(harness.input),
                    "-o",
                    str(out),
                ],
                env={
                    **os.environ,
                    "MOCCACHE_DIR": str(harness.cache),
                    "MOCCACHE_BASEDIR": str(tree.build),
                },
                check=False,
            ).returncode

        with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
            assert all(rc == 0 for rc in pool.map(one, outs))
        contents = {o.read_bytes() for o in outs}
        assert len(contents) == 1
        # Racing version-memo writers leave one complete memo and no temp files.
        memo = core._read_moc_versions(harness.cache / core._MOC_VERSIONS_FILE)
        assert memo[os.path.realpath(harness.fake_moc)][2] == "fake-moc 1.0"
        assert not list(harness.cache.glob(".moccache-*"))
        # Warm run afterwards must hit.
        harness.run(tree, out=tree.out_path("warm.cpp"))
        assert harness.stats()[-1] == "hit"


# ---------------------------------------------------------------------------
# Hits skip re-hashing deps whose stat matches the manifest
# ---------------------------------------------------------------------------


def _settle(harness: Harness) -> None:
    """Write manifests as if long after every file last changed, past the racy window.

    ctime cannot be backdated, so the manifest writer's clock is advanced instead.
    """
    real_time_ns = time.time_ns
    harness.monkeypatch.setattr(core.time, "time_ns", lambda: real_time_ns() + 3600 * 10**9)


def _manifest_fields(harness: Harness) -> dict[str, list[str]]:
    """Manifest lines keyed by their (possibly tokenized) path."""
    lines = harness.manifest().read_text(encoding="utf-8").splitlines()
    return {line.split("\t")[0]: line.split("\t")[1:] for line in lines}


class TestDepStatShortcut:
    @staticmethod
    def _record_hashes(harness: Harness) -> list[Path]:
        hashed: list[Path] = []
        real = core._sha256_file

        def recording(path: Path) -> str:
            hashed.append(Path(path))
            return real(path)

        harness.monkeypatch.setattr(core, "_sha256_file", recording)
        return hashed

    def test_manifest_records_stat_of_settled_deps(self, harness: Harness) -> None:
        _settle(harness)
        harness.run(harness.make_tree("build"))
        # Python 3.12 on Windows reports a different st_ctime_ns from os.stat than from os.fstat
        st = core._dep_stat(str(harness.dep_header))
        assert _manifest_fields(harness)[str(harness.dep_header)] == [
            core._sha256_file(harness.dep_header),
            str(st.st_mtime_ns),
            str(st.st_ctime_ns),
            str(st.st_size),
        ]

    def test_stat_match_skips_hashing(self, harness: Harness) -> None:
        _settle(harness)
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        hashed = self._record_hashes(harness)
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats() == ["miss", "hit"]
        assert hashed == [harness.input]  # for the cache key only

    def test_same_size_edit_with_new_mtime_is_rehashed_and_detected(self, harness: Harness) -> None:
        _settle(harness)
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        before = harness.dep_header.stat()
        harness.dep_header.write_text("// dep v2\n")
        assert harness.dep_header.stat().st_size == before.st_size
        # Still well in the past, as when an older copy is restored with its mtime.
        os.utime(harness.dep_header, ns=(before.st_atime_ns, before.st_mtime_ns + 1_000_000_000))
        hashed = self._record_hashes(harness)
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.dep_header in hashed
        assert harness.stats_detailed()[-1] == ("miss-include-changed", str(harness.dep_header))

    @pytest.mark.skipif(sys.platform == "win32", reason="st_ctime_ns is the creation time")
    def test_rewrite_with_restored_mtime_and_size_is_caught_by_ctime(
        self, harness: Harness
    ) -> None:
        # E.g. an archive re-extracted over the tree: new content, old mtime.
        _settle(harness)
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        _, mtime_ns, ctime_ns, size = _manifest_fields(harness)[str(harness.dep_header)]
        before = harness.dep_header.stat()
        harness.dep_header.write_text("// dep v2\n")
        os.utime(harness.dep_header, ns=(before.st_atime_ns, int(mtime_ns)))
        after = harness.dep_header.stat()
        assert (str(after.st_mtime_ns), str(after.st_size)) == (mtime_ns, size)
        assert str(after.st_ctime_ns) != ctime_ns
        hashed = self._record_hashes(harness)
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.dep_header in hashed
        assert harness.stats_detailed()[-1] == ("miss-include-changed", str(harness.dep_header))

    def test_racy_dep_is_rehashed(self, harness: Harness) -> None:
        # Changed just before the manifest was written: an edit within the
        # same timestamp tick can keep its whole stat, so none is recorded.
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        assert len(_manifest_fields(harness)[str(harness.dep_header)]) == 1
        hashed = self._record_hashes(harness)
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats() == ["miss", "hit"]
        assert harness.dep_header in hashed
        before = harness.dep_header.stat()
        harness.dep_header.write_text("// dep v2\n")
        os.utime(harness.dep_header, ns=(before.st_atime_ns, before.st_mtime_ns))
        assert harness.dep_header.stat().st_size == before.st_size
        harness.run(tree, out=tree.out_path("c.cpp"))
        assert harness.stats_detailed()[-1] == ("miss-include-changed", str(harness.dep_header))

    def test_stat_recorded_only_for_absolute_settled_paths(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        dep = tmp_path / "Dep.h"
        dep.write_text("// dep\n")
        old = time.time() - 3600
        os.utime(dep, (old, old))  # extracted with an old mtime; ctime is now
        # The same fstat _manifest_line reads: on Windows os.stat and os.fstat can report different ctimes.
        st = core._sha256_file_stat(dep)[1]
        assert st.st_ctime_ns > st.st_mtime_ns
        assert len(core._manifest_line(str(dep), [], st.st_ctime_ns + 1).split("\t")) == 5
        assert len(core._manifest_line(str(dep), [], st.st_ctime_ns).split("\t")) == 2
        # A relative path names a different file in each working directory.
        monkeypatch.chdir(tmp_path)
        assert len(core._manifest_line("Dep.h", [], st.st_ctime_ns + 1).split("\t")) == 2

    def test_old_format_manifest_still_hits(self, harness: Harness) -> None:
        _settle(harness)
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        manifest = harness.manifest()
        old_format = "".join(
            "\t".join(line.split("\t")[:2]) + "\n"
            for line in manifest.read_text(encoding="utf-8").splitlines()
        )
        manifest.write_text(old_format, encoding="utf-8")
        hashed = self._record_hashes(harness)
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats() == ["miss", "hit"]
        assert harness.dep_header in hashed

    def test_unparsable_stat_fields_fall_back_to_hashing(self, harness: Harness) -> None:
        _settle(harness)
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        manifest = harness.manifest()
        junk = "".join(
            "\t".join([*line.split("\t")[:2], "soon", "-", "?"]) + "\n"
            for line in manifest.read_text(encoding="utf-8").splitlines()
        )
        manifest.write_text(junk, encoding="utf-8")
        hashed = self._record_hashes(harness)
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats() == ["miss", "hit"]
        assert harness.dep_header in hashed

    def test_mtime_size_only_line_is_malformed(self, harness: Harness) -> None:
        _settle(harness)
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        manifest = harness.manifest()
        records = [line.split("\t") for line in manifest.read_text(encoding="utf-8").splitlines()]
        assert all(len(fields) == 5 for fields in records)
        no_ctime = "".join("\t".join([*fields[:3], fields[4]]) + "\n" for fields in records)
        manifest.write_text(no_ctime, encoding="utf-8")
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats_detailed()[-1] == ("miss-cache-unreadable", "manifest malformed")

    def test_build_dir_dep_stat_never_vouches_across_trees(self, harness: Harness) -> None:
        # Sibling trees share entries through the basedir token, so a build-dir
        # header's stat describes another tree's file; even a matching stat
        # must not stand in for its content.
        tree_a = harness.make_tree("build")
        tree_b = harness.make_tree("build2")
        gen_a = tree_a.build / "gen" / "Gen.h"
        gen_b = tree_b.build / "gen" / "Gen.h"
        for gen, value in ((gen_a, 1), (gen_b, 0)):
            gen.parent.mkdir()
            gen.write_text(f"#define FEATURE {value}\n")
        st = gen_a.stat()
        os.utime(gen_b, ns=(st.st_atime_ns, st.st_mtime_ns))
        _settle(harness)
        deps_list = harness.src / "Foo.h.deps"
        deps_list.write_text(f"{harness.dep_header}\n{gen_a}\n")
        harness.run(tree_a)
        fields = _manifest_fields(harness)
        gen_entry = core._normalize_basedir(str(gen_a), core._basedir_prefixes())
        assert gen_entry.startswith(core._BASEDIR_TOKEN)
        assert len(fields[gen_entry]) == 1
        assert len(fields[str(harness.dep_header)]) == 4
        deps_list.write_text(f"{harness.dep_header}\n{gen_b}\n")
        harness.run(tree_b)
        assert harness.stats_detailed()[-1] == ("miss-include-changed", str(gen_b))


# ---------------------------------------------------------------------------
# moc --version memo
# ---------------------------------------------------------------------------


class TestMocVersionMemo:
    @staticmethod
    def _identity(harness: Harness, moc: Path | None = None) -> str:
        return core._moc_identity(moc or harness.fake_moc, harness.cache)

    def test_hits_reuse_memoized_version(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats() == ["miss", "hit"]
        assert harness.version_queries() == 1
        assert (harness.cache / core._MOC_VERSIONS_FILE).is_file()

    def test_memo_reused_while_moc_stat_unchanged(self, harness: Harness) -> None:
        assert self._identity(harness) == "fake-moc 1.0"
        harness.monkeypatch.setenv("FAKEMOC_VERSION", "2.0")  # binary itself untouched
        assert self._identity(harness) == "fake-moc 1.0"
        assert harness.version_queries() == 1

    def test_memo_refreshed_when_moc_mtime_changes(self, harness: Harness) -> None:
        assert self._identity(harness) == "fake-moc 1.0"
        harness.upgrade_moc("2.0")
        assert self._identity(harness) == "fake-moc 2.0"
        assert self._identity(harness) == "fake-moc 2.0"
        assert harness.version_queries() == 2

    def test_memo_refreshed_when_moc_size_changes(self, harness: Harness) -> None:
        assert self._identity(harness) == "fake-moc 1.0"
        harness.monkeypatch.setenv("FAKEMOC_VERSION", "2.0")
        before = harness.fake_moc.stat()
        comment = "rem padding\r\n" if harness.fake_moc.suffix == ".cmd" else "# padding\n"
        with harness.fake_moc.open("a", encoding="utf-8") as f:
            f.write(comment)
        os.utime(harness.fake_moc, ns=(before.st_atime_ns, before.st_mtime_ns))
        assert self._identity(harness) == "fake-moc 2.0"
        assert harness.version_queries() == 2

    def test_missing_memo_asks_moc_again(self, harness: Harness) -> None:
        assert self._identity(harness) == "fake-moc 1.0"
        (harness.cache / core._MOC_VERSIONS_FILE).unlink()
        assert self._identity(harness) == "fake-moc 1.0"
        assert harness.version_queries() == 2

    @pytest.mark.parametrize(
        "memo",
        [
            pytest.param(b"\xff\xfe not utf-8", id="not-utf8"),
            pytest.param(b"", id="empty"),
            pytest.param(b"garbage\n", id="garbage"),
            pytest.param("{path}\tsoon\t{size}\tfake-moc 9.9\n", id="bad-number"),
            pytest.param("{path}\t{mtime}\t{size}\t\n", id="empty-version"),
            pytest.param("{path}\t{mtime}\t{size}\tfake-moc 9.9", id="unterminated"),
        ],
    )
    def test_corrupt_memo_falls_back_to_moc_and_is_repaired(
        self, harness: Harness, memo: bytes | str
    ) -> None:
        if isinstance(memo, str):
            st = harness.fake_moc.stat()
            memo = memo.format(
                path=os.path.realpath(harness.fake_moc), mtime=st.st_mtime_ns, size=st.st_size
            ).encode()
        harness.cache.mkdir(parents=True)
        (harness.cache / core._MOC_VERSIONS_FILE).write_bytes(memo)
        assert self._identity(harness) == "fake-moc 1.0"
        assert self._identity(harness) == "fake-moc 1.0"
        assert harness.version_queries() == 1

    def test_unusable_memo_path_does_not_fail_build(self, harness: Harness) -> None:
        (harness.cache / core._MOC_VERSIONS_FILE).mkdir(parents=True)
        tree = harness.make_tree("build")
        assert harness.run(tree, out=tree.out_path("a.cpp")) == 0
        assert harness.run(tree, out=tree.out_path("b.cpp")) == 0
        assert harness.stats() == ["miss", "hit"]
        assert harness.version_queries() == 2

    def test_failed_version_query_is_not_memoized(self, harness: Harness) -> None:
        def no_spawn(*args: object, **kwargs: object) -> object:
            raise OSError("cannot spawn")

        harness.monkeypatch.setattr(subprocess, "run", no_spawn)
        st = harness.fake_moc.stat()
        assert self._identity(harness) == f"{st.st_mtime_ns}:{st.st_size}"
        assert not (harness.cache / core._MOC_VERSIONS_FILE).exists()

    def test_memo_keeps_most_recent_mocs(self, harness: Harness) -> None:
        harness.monkeypatch.setattr(core, "_MOC_VERSIONS_MAX", 2)
        mocs = []
        for i in range(3):
            moc = harness.root / f"moc{i}{harness.fake_moc.suffix}"
            shutil.copy2(harness.fake_moc, moc)
            mocs.append(moc)
            self._identity(harness, moc)
        memo = core._read_moc_versions(harness.cache / core._MOC_VERSIONS_FILE)
        assert list(memo) == [os.path.realpath(m) for m in mocs[1:]]

    def test_memo_is_not_a_cache_entry(
        self, harness: Harness, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        memo = harness.cache / core._MOC_VERSIONS_FILE
        assert memo.is_file()
        entries = trim.scan_entries(harness.cache)
        assert [p.parent.parent for _, _, p in entries] == [harness.cache]
        assert show_stats(harness) == 0
        monkeypatch.setattr(sys, "argv", ["moccache_launcher.py", "--trim", "--max-size", "1"])
        assert core.main() == 0
        assert trim.scan_entries(harness.cache) == []
        monkeypatch.setattr(sys, "argv", ["moccache_launcher.py", "--zero-stats"])
        assert core.main() == 0
        assert memo.is_file()
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.version_queries() == 1


# ---------------------------------------------------------------------------
# Pure helper functions
# ---------------------------------------------------------------------------


class TestParseDepFile:
    def test_simple(self) -> None:
        assert core._parse_dep_file("out.cpp: a.h b.h\n") == ["a.h", "b.h"]

    def test_line_continuations(self) -> None:
        text = "out.cpp: \\\n  a.h \\\n  b.h\n"
        assert core._parse_dep_file(text) == ["a.h", "b.h"]

    def test_crlf_continuations(self) -> None:
        text = "out.cpp: \\\r\n  a.h \\\r\n  b.h\r\n"
        assert core._parse_dep_file(text) == ["a.h", "b.h"]

    def test_escaped_spaces_in_paths(self) -> None:
        text = "out.cpp: /p/My\\ Dir/a.h b.h\n"
        assert core._parse_dep_file(text) == ["/p/My Dir/a.h", "b.h"]

    def test_colon_in_dep_path_not_split(self) -> None:
        # Windows-style drive letters must not be treated as the rule separator.
        text = "out.cpp: C:/x/a.h\n"
        assert core._parse_dep_file(text) == ["C:/x/a.h"]

    def test_empty(self) -> None:
        assert core._parse_dep_file("") == []


class TestBasedirNormalization:
    def test_round_trip(self, monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
        monkeypatch.setenv("MOCCACHE_BASEDIR", str(tmp_path / "bld"))
        prefixes = core._basedir_prefixes()
        s = f"{tmp_path}/bld/_deps/x.h"
        normalized = core._normalize_basedir(s, prefixes)
        assert str(tmp_path / "bld") not in normalized
        assert core._denormalize_basedir(normalized) == s

    def test_no_basedir_is_identity(self, monkeypatch: pytest.MonkeyPatch) -> None:
        monkeypatch.delenv("MOCCACHE_BASEDIR", raising=False)
        assert core._basedir_prefixes() == []
        assert core._denormalize_basedir("<<MOCCACHE_BASEDIR>>/x") == "<<MOCCACHE_BASEDIR>>/x"

    def test_trailing_slash_stripped(self, monkeypatch: pytest.MonkeyPatch) -> None:
        monkeypatch.setenv("MOCCACHE_BASEDIR", "/a/bld/")
        normalized = core._normalize_basedir("/a/bld/x.h", core._basedir_prefixes())
        assert normalized == "<<MOCCACHE_BASEDIR>>/x.h"

    def test_trailing_backslash_stripped(self, monkeypatch: pytest.MonkeyPatch) -> None:
        monkeypatch.setenv("MOCCACHE_BASEDIR", "C:\\repo\\build\\")
        normalized = core._normalize_basedir("C:\\repo\\build\\x.h", core._basedir_prefixes())
        assert normalized == "<<MOCCACHE_BASEDIR>>\\x.h"

    def test_sibling_dir_with_prefix_name_not_rewritten(self) -> None:
        # /a/bld2 is a different directory that merely starts with /a/bld.
        assert core._normalize_basedir("/a/bld2/x.h", ["/a/bld"]) == "/a/bld2/x.h"
        assert core._normalize_basedir("/a/bld-old/x.h", ["/a/bld"]) == "/a/bld-old/x.h"
        assert core._normalize_basedir("/a/bld_2/x.h", ["/a/bld"]) == "/a/bld_2/x.h"
        assert core._normalize_basedir("/a/bld.bak/x.h", ["/a/bld"]) == "/a/bld.bak/x.h"
        # Legal-but-uncommon filename chars must also extend the name.
        assert core._normalize_basedir("/a/bld+1/x.h", ["/a/bld"]) == "/a/bld+1/x.h"
        assert core._normalize_basedir("/a/bld@2/x.h", ["/a/bld"]) == "/a/bld@2/x.h"

    def test_exact_and_slash_boundaries_rewritten(self) -> None:
        assert core._normalize_basedir("/a/bld", ["/a/bld"]) == "<<MOCCACHE_BASEDIR>>"
        assert core._normalize_basedir("-I/a/bld/inc", ["/a/bld"]) == "-I<<MOCCACHE_BASEDIR>>/inc"


class TestNormalizedContentHash:
    def test_basedir_content_hashes_equal(self, tmp_path: Path) -> None:
        a = tmp_path / "a.h"
        b = tmp_path / "b.h"
        a.write_text('#define BUILDDIR "/repo/build"\n')
        b.write_text('#define BUILDDIR "/repo/build2"\n')
        ha = core._sha256_file_normalized(a, ["/repo/build"])
        hb = core._sha256_file_normalized(b, ["/repo/build2"])
        assert ha == hb

    def test_real_difference_hashes_differ(self, tmp_path: Path) -> None:
        a = tmp_path / "a.h"
        b = tmp_path / "b.h"
        a.write_text('#define BUILDDIR "/repo/build"\n#define X 1\n')
        b.write_text('#define BUILDDIR "/repo/build2"\n#define X 2\n')
        ha = core._sha256_file_normalized(a, ["/repo/build"])
        hb = core._sha256_file_normalized(b, ["/repo/build2"])
        assert ha != hb

    def test_no_prefixes_matches_plain_hash(self, tmp_path: Path) -> None:
        a = tmp_path / "a.h"
        a.write_text("content\n")
        assert core._sha256_file_normalized(a, []) == core._sha256_file(a)

    def test_prefix_of_other_path_not_rewritten(self, tmp_path: Path) -> None:
        # Content references /repo/build2; basedir /repo/build must not munge it.
        a = tmp_path / "a.h"
        a.write_text('#define OTHER_DIR "/repo/build2/foo.h"\n')
        assert core._sha256_file_normalized(a, ["/repo/build"]) == core._sha256_file(a)

    def test_quoted_and_eof_boundaries_rewritten(self, tmp_path: Path) -> None:
        a = tmp_path / "a.h"
        b = tmp_path / "b.h"
        a.write_text('#define BUILDDIR "/repo/build"\n#include "/repo/build/x.h"\n')
        b.write_text('#define BUILDDIR "/repo/bld2"\n#include "/repo/bld2/x.h"\n')
        ha = core._sha256_file_normalized(a, ["/repo/build"])
        hb = core._sha256_file_normalized(b, ["/repo/bld2"])
        assert ha == hb


class TestDepFileWriting:
    def test_round_trip_with_spaces(self, tmp_path: Path) -> None:
        dep = tmp_path / "out.d"
        deps = ["/p/My Dir/a.h", "/p/b.h"]
        core._write_dep_file(dep, "/p/out.cpp", deps)
        assert core._parse_dep_file(dep.read_text()) == deps


class TestAtomicWrite:
    def test_replaces_with_utf8_lf_and_leaves_no_temp(self, tmp_path: Path) -> None:
        dst = tmp_path / "sub" / "manifest"
        core._atomic_write(dst, "old\n")
        core._atomic_write(dst, "caf\u00e9\n")
        assert dst.read_bytes() == "caf\u00e9\n".encode()
        assert [p.name for p in dst.parent.iterdir()] == ["manifest"]

    def test_existing_temp_name_fails_without_touching_destination(self, tmp_path: Path) -> None:
        dst = tmp_path / "manifest"
        dst.write_text("keep\n")
        stale = tmp_path / f".moccache-{os.getpid()}-manifest"
        stale.write_text("other writer\n")
        with pytest.raises(FileExistsError):
            core._atomic_write(dst, "new\n")
        assert dst.read_text() == "keep\n"
        assert stale.read_text() == "other writer\n"


# ---------------------------------------------------------------------------
# Trimming / eviction
# ---------------------------------------------------------------------------


def _fake_entry(cache_dir: Path, name: str, size: int, age: float) -> Path:
    """Create a synthetic cache entry of roughly `size` bytes, `age` seconds old."""
    mdir = cache_dir / name[:2] / name
    mdir.mkdir(parents=True)
    (mdir / "output.cpp").write_bytes(b"x" * size)
    (mdir / "manifest").write_bytes(b"")
    when = os.stat(mdir).st_mtime - age
    os.utime(mdir, (when, when))
    return mdir


class TestParseSize:
    def test_plain_bytes(self) -> None:
        assert trim.parse_size("1234") == 1234

    def test_suffixes(self) -> None:
        assert trim.parse_size("2K") == 2 * 1024
        assert trim.parse_size("3M") == 3 * 1024**2
        assert trim.parse_size("1G") == 1024**3
        assert trim.parse_size("1T") == 1024**4

    def test_surrounding_whitespace_stripped(self) -> None:
        assert trim.parse_size(" 256M ") == 256 * 1024**2

    def test_case_and_b_suffix(self) -> None:
        assert trim.parse_size("2kb") == 2 * 1024
        assert trim.parse_size("1gB") == 1024**3

    def test_invalid_raises(self) -> None:
        with pytest.raises(ValueError):
            trim.parse_size("abc")
        with pytest.raises(ValueError):
            trim.parse_size("")


class TestTrim:
    def test_under_limit_removes_nothing(self, tmp_path: Path) -> None:
        entries = [_fake_entry(tmp_path, f"{i:02x}entry", 100, age=i) for i in range(3)]
        assert trim.trim(tmp_path, max_bytes=10_000) == 0
        assert all(e.is_dir() for e in entries)

    def test_over_limit_evicts_oldest_first(self, tmp_path: Path) -> None:
        old = _fake_entry(tmp_path, "aa" + "0" * 6, 500, age=1000)
        mid = _fake_entry(tmp_path, "bb" + "0" * 6, 500, age=500)
        new = _fake_entry(tmp_path, "cc" + "0" * 6, 500, age=0)
        removed = trim.trim(tmp_path, max_bytes=1000)
        assert removed == 2
        assert not old.is_dir()
        # Trims to the low-water mark (80% of max), so mid goes too.
        assert not mid.is_dir()
        assert new.is_dir()

    def test_failed_removal_not_counted(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        old = _fake_entry(tmp_path, "aa" + "0" * 6, 500, age=1000)
        mid = _fake_entry(tmp_path, "bb" + "0" * 6, 500, age=500)
        new = _fake_entry(tmp_path, "cc" + "0" * 6, 500, age=0)
        # Simulate rmtree silently failing on the oldest entry (permissions,
        # concurrent writer, ...): it must not count toward `removed` or the
        # freed-size accounting, so the loop keeps evicting until the real
        # on-disk total reaches the target.
        real_rmtree = shutil.rmtree
        monkeypatch.setattr(
            shutil,
            "rmtree",
            lambda p, **kw: None if p == old else real_rmtree(p, **kw),
        )
        removed = trim.trim(tmp_path, max_bytes=1000)
        assert removed == 2  # mid and new actually went away; old did not
        assert old.is_dir()
        assert not mid.is_dir()
        assert not new.is_dir()

    def test_hit_refreshes_entry_mtime_for_lru(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        entry = next(p for p in harness.cache.glob("??/*") if p.is_dir())
        stale = os.stat(entry).st_mtime - 10_000
        os.utime(entry, (stale, stale))
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats() == ["miss", "hit"]
        assert os.stat(entry).st_mtime > stale + 5000

    def test_trim_cli(self, harness: Harness, monkeypatch: pytest.MonkeyPatch) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        assert any(harness.cache.glob("??/*"))
        monkeypatch.setattr(sys, "argv", ["moccache_launcher.py", "--trim", "--max-size", "1"])
        assert core.main() == 0
        assert not any(p for p in harness.cache.glob("??/*") if p.is_dir())

    def test_trim_cli_bad_size_errors(self, monkeypatch: pytest.MonkeyPatch) -> None:
        monkeypatch.setattr(sys, "argv", ["moccache_launcher.py", "--trim", "--max-size", "bogus"])
        assert core.main() == 2

    def test_trim_cli_max_size_missing_value_errors(
        self, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
    ) -> None:
        monkeypatch.setattr(sys, "argv", ["moccache_launcher.py", "--trim", "--max-size"])
        assert core.main() == 2
        assert "--max-size requires a value" in capsys.readouterr().err

    def test_trim_cli_unknown_argument_errors(
        self, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
    ) -> None:
        monkeypatch.setattr(sys, "argv", ["moccache_launcher.py", "--trim", "--frobnicate"])
        assert core.main() == 2
        assert "unknown --trim argument: --frobnicate" in capsys.readouterr().err

    def test_trim_cli_no_size_from_anywhere_errors(
        self, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
    ) -> None:
        monkeypatch.delenv("MOCCACHE_MAX_SIZE", raising=False)
        monkeypatch.setattr(sys, "argv", ["moccache_launcher.py", "--trim"])
        assert core.main() == 2
        assert "requires --max-size or MOCCACHE_MAX_SIZE" in capsys.readouterr().err


# ---------------------------------------------------------------------------
# Miss reasons: every miss says what invalidated it so cache-design problems
# (evictions, arg churn, relpath) can be told apart from legitimate misses.
# ---------------------------------------------------------------------------


class TestKeyIndex:
    def test_read_returns_newest_distinct_keys_oldest_first(self, tmp_path: Path) -> None:
        index = tmp_path / "idx"
        for key in ("k1", "k2", "k1", "k3"):
            core._index_add(index, key)
        assert core._index_read(index) == ["k2", "k1", "k3"]

    def test_read_caps_at_max_keys(self, tmp_path: Path) -> None:
        index = tmp_path / "idx"
        for i in range(core._INDEX_MAX_KEYS + 3):
            core._index_add(index, f"k{i}")
        keys = core._index_read(index)
        assert len(keys) == core._INDEX_MAX_KEYS
        assert keys[-1] == f"k{core._INDEX_MAX_KEYS + 2}"

    def test_add_appends_without_rewriting(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        # Concurrent writers must never clobber each other, so a normal add
        # may only append; rewriting is reserved for compaction.
        def no_rewrite(*args: object, **kwargs: object) -> None:
            raise AssertionError("index add must not rewrite the file")

        monkeypatch.setattr(core, "_atomic_write", no_rewrite)
        index = tmp_path / "idx"
        core._index_add(index, "a")
        core._index_add(index, "b")
        assert index.read_text().split() == ["a", "b"]

    def test_add_is_a_single_append_syscall(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        # One write(2) on an O_APPEND fd is what makes concurrent appends safe;
        # buffered text I/O gives no such guarantee.
        writes: list[bytes] = []
        real_write = os.write

        def counting_write(fd: int, data: bytes) -> int:
            writes.append(bytes(data))
            return real_write(fd, data)

        monkeypatch.setattr(core.os, "write", counting_write)
        core._index_add(tmp_path / "idx", "a" * 64)
        assert writes == [b"a" * 64 + b"\n"]

    def test_add_compacts_oversized_file(self, tmp_path: Path) -> None:
        index = tmp_path / "idx"
        for i in range(core._INDEX_COMPACT_LINES + 1):
            core._index_add(index, f"{i:064x}")
        assert len(index.read_text().split()) <= core._INDEX_MAX_KEYS
        assert core._index_read(index)[-1] == f"{core._INDEX_COMPACT_LINES:064x}"

    def test_read_missing_file_is_empty(self, tmp_path: Path) -> None:
        assert core._index_read(tmp_path / "nope") == []


class TestMissReasons:
    def test_first_ever_input_is_miss_new(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        assert harness.stats_detailed() == [("miss-first-seen", "")]

    def test_changed_input_is_miss_input(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        harness.input.write_text("class Foo { int v2; };\n")
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats_detailed()[-1] == ("miss-header-changed", "")

    def test_changed_dependency_is_miss_dep_with_path(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        harness.dep_header.write_text("// dep v2\n")
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats_detailed()[-1] == ("miss-include-changed", str(harness.dep_header))

    def test_changed_args_is_miss_args_with_diff(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        harness.run(tree, out=tree.out_path("b.cpp"), extra_args=("-DEXTRA=1",))
        kind, detail = harness.stats_detailed()[-1]
        assert kind == "miss-args-changed"
        assert "-DEXTRA=1" in detail

    def test_changed_predefs_is_miss_predefs(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        tree.predefs.write_text(tree.predefs.read_text() + "#define __NEW__ 1\n")
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats_detailed()[-1] == ("miss-predefs-changed", "")

    def test_changed_moc_is_miss_moc(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        harness.upgrade_moc("2.0")
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats_detailed()[-1][0] == "miss-moc-version-changed"

    def test_different_output_depth_is_miss_relpath(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        deeper = tree.build / "a" / "b" / "c" / "d" / "e"
        deeper.mkdir(parents=True)
        harness.run(tree, out=deeper / "moc_Foo.cpp")
        assert harness.stats_detailed()[-1][0] == "miss-output-depth-differs"

    def test_evicted_entry_is_miss_evicted(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        for shard in harness.cache.glob("??"):
            shutil.rmtree(shard)
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert harness.stats_detailed()[-1] == ("miss-cache-evicted", "")

    def test_unreadable_cached_output_is_not_reported_as_evicted(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        (cached_out,) = harness.cache.glob("??/*/output.cpp")
        real_read_bytes = Path.read_bytes

        def failing_read_bytes(path: Path) -> bytes:
            if path == cached_out:
                raise OSError("cached output unreadable")
            return real_read_bytes(path)

        harness.monkeypatch.setattr(Path, "read_bytes", failing_read_bytes)
        assert harness.run(tree, out=tree.out_path("b.cpp")) == 0
        kind, detail = harness.stats_detailed()[-1]
        assert kind == "miss-cache-unreadable"
        assert detail == ""

    def test_incomplete_cache_entry_is_not_reported_as_evicted(self, harness: Harness) -> None:
        # A crash between writing output.cpp and the manifest leaves the entry
        # dir behind; that is not an eviction and must not point at MAX_SIZE.
        tree = harness.make_tree("build")
        harness.run(tree)
        (cached_out,) = harness.cache.glob("??/*/output.cpp")
        cached_out.unlink()
        assert harness.run(tree, out=tree.out_path("b.cpp")) == 0
        assert harness.stats_detailed()[-1] == ("miss-cache-unreadable", "entry incomplete")

    def test_corrupt_manifest_is_cache_unreadable_not_include_changed(
        self, harness: Harness
    ) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        (manifest,) = harness.cache.glob("??/*/manifest")
        manifest.write_bytes(b"\xff\xfe not utf-8")
        assert harness.run(tree, out=tree.out_path("b.cpp")) == 0
        assert harness.stats_detailed()[-1] == ("miss-cache-unreadable", "manifest unreadable")

    def test_malformed_manifest_line_is_cache_unreadable(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        (manifest,) = harness.cache.glob("??/*/manifest")
        manifest.write_text(f"{harness.dep_header}\n", encoding="utf-8")  # no tab / hash
        assert harness.run(tree, out=tree.out_path("b.cpp")) == 0
        assert harness.stats_detailed()[-1] == ("miss-cache-unreadable", "manifest malformed")

    def test_corrupt_index_degrades_to_miss(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        (index,) = harness.cache.glob("index/??/*")
        index.write_bytes(b"\xff\xfe not utf-8")
        assert harness.run(tree, out=tree.out_path("b.cpp"), extra_args=("-DEXTRA=1",)) == 0
        assert harness.stats()[-1] == "miss"

    def test_uncacheable_miss_is_logged_on_fresh_cache(self, harness: Harness) -> None:
        # Nothing else has created MOCCACHE_DIR yet when moc yields no dep file.
        real_run = subprocess.run

        def run_and_drop_dep_file(cmd: list[str], *args: object, **kwargs: object) -> object:
            result = real_run(cmd, *args, **kwargs)  # type: ignore[arg-type]
            if "--dep-file-path" in cmd:
                Path(cmd[cmd.index("--dep-file-path") + 1]).unlink()
            return result

        harness.monkeypatch.setattr(subprocess, "run", run_and_drop_dep_file)
        tree = harness.make_tree("build")
        assert not harness.cache.exists()
        assert harness.run(tree) == 0
        assert harness.stats_detailed() == [("miss-uncacheable", "")]

    def test_miss_reason_not_computed_when_stats_disabled(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        harness.monkeypatch.delenv("MOCCACHE_STATS")

        def explode(*args: object) -> tuple[str, str]:
            raise AssertionError("_explain_key_miss must not run with stats off")

        harness.monkeypatch.setattr(core, "_explain_key_miss", explode)
        harness.run(tree, out=tree.out_path("b.cpp"), extra_args=("-DEXTRA=1",))
        # The index still learns the new key so a later stats-on run can diff against it.
        (index,) = harness.cache.glob("index/??/*")
        assert len(index.read_text(encoding="utf-8").splitlines()) == 2

    def test_auto_trim_logs_eviction_count(
        self, harness: Harness, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        _fake_entry(harness.cache, "ee" + "0" * 6, 100_000, age=99_999)
        monkeypatch.setenv("MOCCACHE_MAX_SIZE", "1K")
        tree = harness.make_tree("build")
        harness.run(tree)
        trims = [d for k, d in harness.stats_detailed() if k == "cache-trimmed"]
        assert trims and int(trims[0]) >= 1

    def test_show_stats_breaks_misses_down_by_reason(
        self, harness: Harness, capsys: pytest.CaptureFixture[str]
    ) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        harness.run(tree, out=tree.out_path("b.cpp"))
        harness.dep_header.write_text("// dep v2\n")
        harness.run(tree, out=tree.out_path("c.cpp"))
        assert show_stats(harness) == 0
        out = capsys.readouterr().out
        assert "hits     1" in out
        assert "misses   2" in out
        assert "hit rate 33.3%" in out
        assert re.search(r"miss-first-seen\s+1  \(first time this header was moc'd\)", out)
        assert re.search(r"miss-include-changed\s+1  \(a header it includes changed\)", out)

    def test_show_stats_verbose_lists_miss_details(
        self, harness: Harness, capsys: pytest.CaptureFixture[str]
    ) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        harness.dep_header.write_text("// dep v2\n")
        harness.run(tree, out=tree.out_path("b.cpp"))
        assert show_stats(harness, "--verbose") == 0
        out = capsys.readouterr().out
        assert f"miss-include-changed  {harness.input}  {harness.dep_header}" in out
        assert f"miss-first-seen  {harness.input}" in out


# ---------------------------------------------------------------------------
# Stats CLI (--show-stats / --zero-stats)
# ---------------------------------------------------------------------------


class TestStatsCli:
    def test_show_stats_counts_hits_and_misses(
        self, harness: Harness, capsys: pytest.CaptureFixture[str]
    ) -> None:
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        harness.run(tree, out=tree.out_path("b.cpp"))
        harness.run(tree, out=tree.out_path("c.cpp"))
        assert show_stats(harness) == 0
        out = capsys.readouterr().out
        assert "hits     2" in out
        assert "misses   1" in out
        assert "hit rate 66.7%" in out

    def test_show_stats_without_log_reports_zero(
        self, harness: Harness, capsys: pytest.CaptureFixture[str]
    ) -> None:
        assert show_stats(harness) == 0
        out = capsys.readouterr().out
        assert "no stats recorded" in out

    def test_collect_stats_reports_counts_and_size(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree, out=tree.out_path("a.cpp"))
        harness.run(tree, out=tree.out_path("b.cpp"))
        harness.run(tree, out=tree.out_path("c.cpp"))
        harness.monkeypatch.setenv("MOCCACHE_MAX_SIZE", "1G")
        build_dir = harness.root / "bld"
        write_ninja_log(build_dir, (0, 1000, int((time.time() - 3600) * 1e9)))
        result = stats.collect_stats(build_dir)
        assert (result.scope, result.note) == ("this build", "")
        assert (result.hits, result.misses) == (2, 1)
        assert result.counts == {"miss-first-seen": 1}
        assert re.fullmatch(r"\d+ MiB of 1\.0 GiB", result.size())
        assert stats.KIND_DESCRIPTIONS["miss-first-seen"] == "first time this header was moc'd"

    def test_collect_stats_without_ninja_log_reports_everything(self, harness: Harness) -> None:
        result = stats.collect_stats(harness.root / "missing")
        assert result.scope == "all recorded"
        assert "build start unknown" in result.note
        assert (result.hits, result.misses, result.counts) == (0, 0, {})

    def test_show_stats_rejects_markdown(self, harness: Harness) -> None:
        assert show_stats(harness, "--markdown") == 2

    def test_zero_stats_removes_log(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        assert (harness.cache / "stats.log").is_file()
        harness.monkeypatch.setattr(sys, "argv", ["moccache_launcher.py", "--zero-stats"])
        assert core.main() == 0
        assert not (harness.cache / "stats.log").exists()

    def test_zero_stats_without_log_succeeds(
        self, harness: Harness, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        monkeypatch.setattr(sys, "argv", ["moccache_launcher.py", "--zero-stats"])
        assert core.main() == 0

    @pytest.mark.parametrize("flag", ["--show-stats", "--zero-stats"])
    def test_stats_flags_reject_extra_arguments(
        self, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str], flag: str
    ) -> None:
        monkeypatch.setattr(sys, "argv", ["moccache_launcher.py", flag, "--frobnicate"])
        assert core.main() == 2
        assert f"{flag}: unknown argument: --frobnicate" in capsys.readouterr().err

    def test_show_stats_requires_build_dir(
        self, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
    ) -> None:
        monkeypatch.setattr(sys, "argv", ["moccache_launcher.py", "--show-stats"])
        assert core.main() == 2
        assert "--show-stats requires --build-dir" in capsys.readouterr().err

    def test_show_stats_build_dir_requires_value(
        self, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
    ) -> None:
        monkeypatch.setattr(sys, "argv", ["moccache_launcher.py", "--show-stats", "--build-dir"])
        assert core.main() == 2
        assert "--build-dir requires a path" in capsys.readouterr().err

    # Mixed log: two builds of /b1 (old at T0+100, current at T0+200), a concurrent
    # build of /b2, and a legacy line without time/basedir fields.
    _MIXED_LOG = (
        f"hit\ta.h\t{_T0 + 100:.1f}\t/b1\n"
        f"miss\tb.h\t{_T0 + 100:.1f}\t/b1\n"
        f"hit\tc.h\t{_T0 + 200:.1f}\t/b1\n"
        f"hit\td.h\t{_T0 + 200:.1f}\t/b1\n"
        f"miss\te.h\t{_T0 + 200:.1f}\t/b2\n"
        "hit\tf.h\n"
    )

    def _write_mixed_log(self, harness: Harness) -> None:
        harness.cache.mkdir(parents=True, exist_ok=True)
        (harness.cache / "stats.log").write_text(self._MIXED_LOG, encoding="utf-8")

    def _show_current_build(self, harness: Harness) -> int:
        """--show-stats at T0+300 for a /b1 build that began at T0+150 (+slack)."""
        build_dir = harness.root / "bld"
        start_ns = int((_T0 + 150.0 + stats._BUILD_START_SLACK_SECONDS + 5.0) * 1e9)
        write_ninja_log(build_dir, (0, 5000, start_ns))
        harness.monkeypatch.setattr(stats.time, "time", lambda: _T0 + 300.0)
        harness.monkeypatch.setenv("MOCCACHE_BASEDIR", "/b1")
        harness.monkeypatch.setattr(
            sys, "argv", ["moccache_launcher.py", "--show-stats", "--build-dir", str(build_dir)]
        )
        return core.main()

    @staticmethod
    def _expected_start(started_at: float) -> float:
        return started_at - stats._BUILD_START_SLACK_SECONDS

    def test_build_start_derived_from_last_ninja_log_edge(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        # Last edge finished 5 s into the build at absolute T0+155 -> build began at T0+150.
        monkeypatch.setattr(stats.time, "time", lambda: _T0 + 300.0)
        write_ninja_log(
            tmp_path, (0, 1000, int((_T0 + 100) * 1e9)), (2000, 5000, int((_T0 + 155) * 1e9))
        )
        assert stats.build_start(tmp_path) == pytest.approx(self._expected_start(_T0 + 150))

    def test_build_start_skips_edges_without_output_mtime(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        monkeypatch.setattr(stats.time, "time", lambda: _T0 + 300.0)
        write_ninja_log(tmp_path, (2000, 5000, int((_T0 + 155) * 1e9)), (5000, 6000, 0))
        assert stats.build_start(tmp_path) == pytest.approx(self._expected_start(_T0 + 150))

    def test_build_start_ignores_restat_edge_with_stale_output_mtime(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        # A restat custom command that left its output untouched logs the old
        # mtime, which would place the build start far in the past on its own.
        monkeypatch.setattr(stats.time, "time", lambda: _T0 + 300.0)
        write_ninja_log(
            tmp_path,
            (0, 5000, int((_T0 + 155) * 1e9)),
            (5000, 6000, int((_T0 - 5000) * 1e9)),
        )
        assert stats.build_start(tmp_path) == pytest.approx(self._expected_start(_T0 + 150))

    def test_build_start_decodes_windows_filetime_mtimes(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        # ninja on Windows logs FILETIME ticks (100 ns since 1601-01-01) minus
        # its own 12622770400 s rebase constant (src/disk_interface.cc).
        monkeypatch.setattr(stats, "_NINJA_LOG_MTIME_IS_FILETIME", True)
        monkeypatch.setattr(stats.time, "time", lambda: _T0 + 300.0)
        filetime_ticks = int((_T0 + 155 + 11_644_473_600) * 10**7)
        ninja_mtime = filetime_ticks - 12_622_770_400 * 10**7
        write_ninja_log(tmp_path, (0, 5000, ninja_mtime))
        assert stats.build_start(tmp_path) == pytest.approx(self._expected_start(_T0 + 150))

    @pytest.mark.parametrize("started_at", [_T0 + 400.0, _T0 - 2 * 86400.0])
    def test_build_start_none_when_implausible(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch, started_at: float
    ) -> None:
        # In the future or days old means the log is not from this build (or
        # its units are not what we assume): fall back rather than misreport.
        monkeypatch.setattr(stats.time, "time", lambda: _T0 + 300.0)
        write_ninja_log(tmp_path, (0, 5000, int((started_at + 5) * 1e9)))
        assert stats.build_start(tmp_path) is None

    def test_build_start_none_without_ninja_log(self, tmp_path: Path) -> None:
        assert stats.build_start(tmp_path) is None
        write_ninja_log(tmp_path)
        assert stats.build_start(tmp_path) is None

    def test_show_stats_reports_only_current_build(
        self, harness: Harness, capsys: pytest.CaptureFixture[str]
    ) -> None:
        self._write_mixed_log(harness)
        assert self._show_current_build(harness) == 0
        out = capsys.readouterr().out
        assert "this build" in out
        assert "hits     2" in out
        assert "misses   0" in out

    def test_show_stats_prunes_entries_older_than_this_build(self, harness: Harness) -> None:
        # Older lines are dead weight; the concurrent /b2 build's line is kept
        # because its own post-build report still needs it.
        self._write_mixed_log(harness)
        assert self._show_current_build(harness) == 0
        remaining = (harness.cache / "stats.log").read_text(encoding="utf-8").splitlines()
        assert remaining == [
            f"hit\tc.h\t{_T0 + 200:.1f}\t/b1",
            f"hit\td.h\t{_T0 + 200:.1f}\t/b1",
            f"miss\te.h\t{_T0 + 200:.1f}\t/b2",
        ]

    def test_show_stats_keeps_older_entries_of_other_build_dirs(self, harness: Harness) -> None:
        # /b2 began before this /b1 build and may still be running; only its
        # own report may prune its records.
        harness.cache.mkdir(parents=True, exist_ok=True)
        (harness.cache / "stats.log").write_text(
            f"hit\ta.h\t{_T0 + 100:.1f}\t/b2\n"
            f"hit\tb.h\t{_T0 + 100:.1f}\t/b1\n"
            f"hit\tc.h\t{_T0 + 200:.1f}\t/b1\n",
            encoding="utf-8",
        )
        assert self._show_current_build(harness) == 0
        remaining = (harness.cache / "stats.log").read_text(encoding="utf-8").splitlines()
        assert remaining == [
            f"hit\ta.h\t{_T0 + 100:.1f}\t/b2",
            f"hit\tc.h\t{_T0 + 200:.1f}\t/b1",
        ]

    def test_show_stats_prunes_to_empty_when_build_logged_nothing(
        self, harness: Harness, capsys: pytest.CaptureFixture[str]
    ) -> None:
        # A no-op build still clears the previous build's entries.
        harness.cache.mkdir(parents=True, exist_ok=True)
        (harness.cache / "stats.log").write_text(
            f"hit\ta.h\t{_T0 + 100:.1f}\t/b1\n", encoding="utf-8"
        )
        assert self._show_current_build(harness) == 0
        assert "no stats recorded" in capsys.readouterr().out
        assert (harness.cache / "stats.log").read_text(encoding="utf-8") == ""

    def test_show_stats_without_ninja_log_falls_back_and_keeps_log(
        self, harness: Harness, capsys: pytest.CaptureFixture[str]
    ) -> None:
        self._write_mixed_log(harness)
        build_dir = harness.root / "bld"
        build_dir.mkdir()
        harness.monkeypatch.setenv("MOCCACHE_BASEDIR", "/b1")
        harness.monkeypatch.setattr(
            sys, "argv", ["moccache_launcher.py", "--show-stats", "--build-dir", str(build_dir)]
        )
        assert core.main() == 0
        out = capsys.readouterr().out
        assert "build start unknown" in out
        assert "hits     4" in out
        assert "misses   2" in out
        assert (harness.cache / "stats.log").read_text(encoding="utf-8") == self._MIXED_LOG

    def test_show_stats_without_ninja_log_caps_log_growth(
        self, harness: Harness, capsys: pytest.CaptureFixture[str]
    ) -> None:
        # With no build boundary to prune at, the log must still be bounded.
        harness.monkeypatch.setattr(stats, "_STATS_LOG_MAX_LINES", 3)
        harness.cache.mkdir(parents=True, exist_ok=True)
        lines = [f"hit\t{i}.h\t{_T0 + i}\t/b1" for i in range(5)]
        (harness.cache / "stats.log").write_text("".join(f"{s}\n" for s in lines), encoding="utf-8")
        build_dir = harness.root / "bld"
        build_dir.mkdir()
        harness.monkeypatch.setattr(
            sys, "argv", ["moccache_launcher.py", "--show-stats", "--build-dir", str(build_dir)]
        )
        assert core.main() == 0
        assert "hits     5" in capsys.readouterr().out
        remaining = (harness.cache / "stats.log").read_text(encoding="utf-8").splitlines()
        assert remaining == lines[-3:]

    def test_show_stats_with_only_other_events_still_prints_counts(
        self, harness: Harness, capsys: pytest.CaptureFixture[str]
    ) -> None:
        harness.cache.mkdir(parents=True, exist_ok=True)
        (harness.cache / "stats.log").write_text(
            f"bad-max-size\t\t{time.time():.3f}\t\tx\n", encoding="utf-8"
        )
        assert show_stats(harness) == 0
        out = capsys.readouterr().out
        assert "no stats recorded" not in out
        assert "hits     0" in out
        assert "misses   0" in out
        assert "hit rate" not in out
        assert re.search(r"bad-max-size\s+1  \(MOCCACHE_MAX_SIZE is not a valid size", out)

    def test_zero_then_show_reports_zero(
        self, harness: Harness, capsys: pytest.CaptureFixture[str]
    ) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        harness.monkeypatch.setattr(sys, "argv", ["moccache_launcher.py", "--zero-stats"])
        assert core.main() == 0
        assert show_stats(harness) == 0
        assert "no stats recorded" in capsys.readouterr().out

    def test_auto_trim_on_miss_when_max_size_set(
        self, harness: Harness, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        _fake_entry(harness.cache, "ee" + "0" * 6, 100_000, age=99_999)
        monkeypatch.setenv("MOCCACHE_MAX_SIZE", "1K")
        tree = harness.make_tree("build")
        harness.run(tree)
        assert harness.stats() == ["miss", "cache-trimmed"]
        assert not (harness.cache / "ee" / ("ee" + "0" * 6)).is_dir()

    def test_auto_trim_respects_stamp_interval(
        self, harness: Harness, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        monkeypatch.setenv("MOCCACHE_MAX_SIZE", "1G")
        tree = harness.make_tree("build")
        harness.run(tree)  # cold miss creates the trim stamp
        stamp = harness.cache / "trim.stamp"
        assert stamp.is_file()
        big = _fake_entry(harness.cache, "ee" + "0" * 6, 100_000, age=99_999)
        monkeypatch.setenv("MOCCACHE_MAX_SIZE", "1K")
        harness.input.write_text("class Foo { int v2; };\n")
        harness.run(tree, out=tree.out_path("b.cpp"))  # miss, but stamp is fresh
        assert big.is_dir()

    def test_no_auto_trim_without_max_size(self, harness: Harness) -> None:
        big = _fake_entry(harness.cache, "ee" + "0" * 6, 100_000, age=99_999)
        tree = harness.make_tree("build")
        harness.run(tree)
        assert big.is_dir()

    def test_invalid_max_size_logged_and_ignored(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        monkeypatch.setenv("MOCCACHE_MAX_SIZE", "bogus")
        monkeypatch.setenv("MOCCACHE_STATS", "1")
        big = _fake_entry(tmp_path, "ee" + "0" * 6, 100_000, age=99_999)
        trim.maybe_auto_trim(tmp_path)
        assert big.is_dir()  # no trim, but also no crash
        line = (tmp_path / "stats.log").read_text(encoding="utf-8").splitlines()[0].split("\t")
        assert line[0] == "bad-max-size"
        assert line[1] == ""  # not a moc input
        assert line[4] == "bogus"

    def test_stamp_vanishing_mid_check_still_trims(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        # TOCTTOU: another process removes the stamp between the existence
        # check and the stat; the trim must proceed, not be silently skipped.
        monkeypatch.setenv("MOCCACHE_MAX_SIZE", "1K")
        big = _fake_entry(tmp_path, "ee" + "0" * 6, 100_000, age=99_999)
        real_exists = Path.exists

        def stamp_seems_present(self: Path, **kwargs: bool) -> bool:
            if self.name == "trim.stamp":
                return True  # vanishes before the subsequent stat()
            return real_exists(self, **kwargs)

        monkeypatch.setattr(Path, "exists", stamp_seems_present)
        trim.maybe_auto_trim(tmp_path)
        assert not big.is_dir()
        assert (tmp_path / "trim.stamp").is_file()

    def test_auto_trim_skipped_while_lock_held(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        monkeypatch.setenv("MOCCACHE_MAX_SIZE", "1K")
        big = _fake_entry(tmp_path, "ee" + "0" * 6, 100_000, age=99_999)
        (tmp_path / "trim.lock").touch()  # another process is trimming
        trim.maybe_auto_trim(tmp_path)
        assert big.is_dir()
        assert (tmp_path / "trim.lock").is_file()  # not ours to remove

    def test_stale_lock_recovered(self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
        monkeypatch.setenv("MOCCACHE_MAX_SIZE", "1K")
        big = _fake_entry(tmp_path, "ee" + "0" * 6, 100_000, age=99_999)
        lock = tmp_path / "trim.lock"
        lock.touch()
        stale = os.stat(lock).st_mtime - 10_000  # crashed holder, long past interval
        os.utime(lock, (stale, stale))
        trim.maybe_auto_trim(tmp_path)  # reclaims the stale lock
        assert not lock.is_file()
        trim.maybe_auto_trim(tmp_path)  # next miss can trim
        assert not big.is_dir()

    def test_lock_released_after_trim(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        monkeypatch.setenv("MOCCACHE_MAX_SIZE", "1K")
        big = _fake_entry(tmp_path, "ee" + "0" * 6, 100_000, age=99_999)
        trim.maybe_auto_trim(tmp_path)
        assert not big.is_dir()
        assert not (tmp_path / "trim.lock").exists()
        assert (tmp_path / "trim.stamp").is_file()

    def test_failed_trim_does_not_refresh_stamp(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        # The stamp means "a trim completed"; a trim that dies partway must
        # not suppress retries for the whole interval.
        monkeypatch.setenv("MOCCACHE_MAX_SIZE", "1K")
        big = _fake_entry(tmp_path, "ee" + "0" * 6, 100_000, age=99_999)
        real_trim = trim.trim

        def boom(cache_dir: Path, max_bytes: int) -> int:
            raise OSError("simulated crash mid-trim")

        monkeypatch.setattr(trim, "trim", boom)
        trim.maybe_auto_trim(tmp_path)
        assert not (tmp_path / "trim.stamp").exists()  # no completed trim
        assert not (tmp_path / "trim.lock").exists()  # lock still released
        monkeypatch.setattr(trim, "trim", real_trim)
        trim.maybe_auto_trim(tmp_path)  # next miss retries immediately
        assert not big.is_dir()
        assert (tmp_path / "trim.stamp").is_file()


def test_dep_stat_uses_fstat_on_windows(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    """The check stats a dependency the same way _manifest_line recorded it (os.fstat) on Windows."""
    dep = tmp_path / "Dep.h"
    dep.write_text("// dep\n", encoding="utf-8")
    monkeypatch.setattr(core.os, "name", "nt")
    calls: list[int] = []
    real_fstat = core.os.fstat
    monkeypatch.setattr(core.os, "fstat", lambda fd: calls.append(fd) or real_fstat(fd))
    assert core._dep_stat(str(dep)).st_size == dep.stat().st_size
    assert calls
