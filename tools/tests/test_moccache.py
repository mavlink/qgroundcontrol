"""Tests for tools/moccache.py — the AUTOMOC caching wrapper.

Covers the pure helper functions plus end-to-end behavior against a fake moc
that mimics the real one's caching-relevant quirks: it embeds a relative
#include back to the input header, honors --output-dep-file/--dep-file-path,
and derives its output from input + force-included content + args.
"""

from __future__ import annotations

import concurrent.futures
import os
import shutil
import stat
import sys
from dataclasses import dataclass
from pathlib import Path

import moccache
import pytest

# The harness relies on POSIX semantics (shebang execution of the fake moc,
# chmod-based permission tests), matching CI, which only enables moccache on
# non-Windows runners.
pytestmark = pytest.mark.skipif(sys.platform == "win32", reason="moccache is POSIX-only")

FAKE_MOC = """#!/usr/bin/env python3
import hashlib
import os
import sys


def main() -> int:
    argv = sys.argv[1:]
    if argv == ["--version"]:
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
        self.fake_moc = root / "fake-moc"
        self.fake_moc.write_text(FAKE_MOC)
        self.fake_moc.chmod(0o755)
        self.moc_log = root / "moc-invocations.log"
        for var in ("MOCCACHE_DISABLE", "MOCCACHE_MOC", "MOCCACHE_BASEDIR"):
            monkeypatch.delenv(var, raising=False)
        monkeypatch.setenv("MOCCACHE_DIR", str(self.cache))
        monkeypatch.setenv("MOCCACHE_STATS", "1")
        monkeypatch.setenv("FAKEMOC_LOG", str(self.moc_log))

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
            sys, "argv", ["moccache.py", "--real-moc", str(self.fake_moc), *argv]
        )
        return moccache.main()

    def stats(self) -> list[str]:
        log = self.cache / "stats.log"
        if not log.is_file():
            return []
        return [line.split("\t")[0] for line in log.read_text().splitlines()]

    def moc_runs(self) -> int:
        if not self.moc_log.is_file():
            return 0
        return len(self.moc_log.read_text().splitlines())


@pytest.fixture
def harness(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Harness:
    return Harness(tmp_path, monkeypatch)


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

    def test_stats_log_records_input(self, harness: Harness) -> None:
        tree = harness.make_tree("build")
        harness.run(tree)
        log = (harness.cache / "stats.log").read_text()
        assert str(harness.input) in log


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
        harness.monkeypatch.setenv("FAKEMOC_VERSION", "2.0")
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
        deps = moccache._parse_dep_file(dep_text)
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
            sys, "argv", ["moccache.py", "--real-moc", str(harness.root / "nope"), str(tree.build)]
        )
        assert moccache.main() == 2

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
            import subprocess

            return subprocess.run(
                [
                    sys.executable,
                    str(Path(moccache.__file__)),
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
        # Warm run afterwards must hit.
        harness.run(tree, out=tree.out_path("warm.cpp"))
        assert harness.stats()[-1] == "hit"


# ---------------------------------------------------------------------------
# Pure helper functions
# ---------------------------------------------------------------------------


class TestParseDepFile:
    def test_simple(self) -> None:
        assert moccache._parse_dep_file("out.cpp: a.h b.h\n") == ["a.h", "b.h"]

    def test_line_continuations(self) -> None:
        text = "out.cpp: \\\n  a.h \\\n  b.h\n"
        assert moccache._parse_dep_file(text) == ["a.h", "b.h"]

    def test_crlf_continuations(self) -> None:
        text = "out.cpp: \\\r\n  a.h \\\r\n  b.h\r\n"
        assert moccache._parse_dep_file(text) == ["a.h", "b.h"]

    def test_escaped_spaces_in_paths(self) -> None:
        text = "out.cpp: /p/My\\ Dir/a.h b.h\n"
        assert moccache._parse_dep_file(text) == ["/p/My Dir/a.h", "b.h"]

    def test_colon_in_dep_path_not_split(self) -> None:
        # Windows-style drive letters must not be treated as the rule separator.
        text = "out.cpp: C:/x/a.h\n"
        assert moccache._parse_dep_file(text) == ["C:/x/a.h"]

    def test_empty(self) -> None:
        assert moccache._parse_dep_file("") == []


class TestBasedirNormalization:
    def test_round_trip(self, monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
        monkeypatch.setenv("MOCCACHE_BASEDIR", str(tmp_path / "bld"))
        prefixes = moccache._basedir_prefixes()
        s = f"{tmp_path}/bld/_deps/x.h"
        normalized = moccache._normalize_basedir(s, prefixes)
        assert str(tmp_path / "bld") not in normalized
        assert moccache._denormalize_basedir(normalized) == s

    def test_no_basedir_is_identity(self, monkeypatch: pytest.MonkeyPatch) -> None:
        monkeypatch.delenv("MOCCACHE_BASEDIR", raising=False)
        assert moccache._basedir_prefixes() == []
        assert moccache._denormalize_basedir("<<MOCCACHE_BASEDIR>>/x") == "<<MOCCACHE_BASEDIR>>/x"

    def test_trailing_slash_stripped(self, monkeypatch: pytest.MonkeyPatch) -> None:
        monkeypatch.setenv("MOCCACHE_BASEDIR", "/a/bld/")
        normalized = moccache._normalize_basedir("/a/bld/x.h", moccache._basedir_prefixes())
        assert normalized == "<<MOCCACHE_BASEDIR>>/x.h"

    def test_sibling_dir_with_prefix_name_not_rewritten(self) -> None:
        # /a/bld2 is a different directory that merely starts with /a/bld.
        assert moccache._normalize_basedir("/a/bld2/x.h", ["/a/bld"]) == "/a/bld2/x.h"
        assert moccache._normalize_basedir("/a/bld-old/x.h", ["/a/bld"]) == "/a/bld-old/x.h"
        assert moccache._normalize_basedir("/a/bld_2/x.h", ["/a/bld"]) == "/a/bld_2/x.h"
        assert moccache._normalize_basedir("/a/bld.bak/x.h", ["/a/bld"]) == "/a/bld.bak/x.h"
        # Legal-but-uncommon filename chars must also extend the name.
        assert moccache._normalize_basedir("/a/bld+1/x.h", ["/a/bld"]) == "/a/bld+1/x.h"
        assert moccache._normalize_basedir("/a/bld@2/x.h", ["/a/bld"]) == "/a/bld@2/x.h"

    def test_exact_and_slash_boundaries_rewritten(self) -> None:
        assert moccache._normalize_basedir("/a/bld", ["/a/bld"]) == "<<MOCCACHE_BASEDIR>>"
        assert (
            moccache._normalize_basedir("-I/a/bld/inc", ["/a/bld"]) == "-I<<MOCCACHE_BASEDIR>>/inc"
        )


class TestNormalizedContentHash:
    def test_basedir_content_hashes_equal(self, tmp_path: Path) -> None:
        a = tmp_path / "a.h"
        b = tmp_path / "b.h"
        a.write_text('#define BUILDDIR "/repo/build"\n')
        b.write_text('#define BUILDDIR "/repo/build2"\n')
        ha = moccache._sha256_file_normalized(a, ["/repo/build"])
        hb = moccache._sha256_file_normalized(b, ["/repo/build2"])
        assert ha == hb

    def test_real_difference_hashes_differ(self, tmp_path: Path) -> None:
        a = tmp_path / "a.h"
        b = tmp_path / "b.h"
        a.write_text('#define BUILDDIR "/repo/build"\n#define X 1\n')
        b.write_text('#define BUILDDIR "/repo/build2"\n#define X 2\n')
        ha = moccache._sha256_file_normalized(a, ["/repo/build"])
        hb = moccache._sha256_file_normalized(b, ["/repo/build2"])
        assert ha != hb

    def test_no_prefixes_matches_plain_hash(self, tmp_path: Path) -> None:
        a = tmp_path / "a.h"
        a.write_text("content\n")
        assert moccache._sha256_file_normalized(a, []) == moccache._sha256_file(a)

    def test_prefix_of_other_path_not_rewritten(self, tmp_path: Path) -> None:
        # Content references /repo/build2; basedir /repo/build must not munge it.
        a = tmp_path / "a.h"
        a.write_text('#define OTHER_DIR "/repo/build2/foo.h"\n')
        assert moccache._sha256_file_normalized(a, ["/repo/build"]) == moccache._sha256_file(a)

    def test_quoted_and_eof_boundaries_rewritten(self, tmp_path: Path) -> None:
        a = tmp_path / "a.h"
        b = tmp_path / "b.h"
        a.write_text('#define BUILDDIR "/repo/build"\n#include "/repo/build/x.h"\n')
        b.write_text('#define BUILDDIR "/repo/bld2"\n#include "/repo/bld2/x.h"\n')
        ha = moccache._sha256_file_normalized(a, ["/repo/build"])
        hb = moccache._sha256_file_normalized(b, ["/repo/bld2"])
        assert ha == hb


class TestDepFileWriting:
    def test_round_trip_with_spaces(self, tmp_path: Path) -> None:
        dep = tmp_path / "out.d"
        deps = ["/p/My Dir/a.h", "/p/b.h"]
        moccache._write_dep_file(dep, "/p/out.cpp", deps)
        assert moccache._parse_dep_file(dep.read_text()) == deps


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
        assert moccache._parse_size("1234") == 1234

    def test_suffixes(self) -> None:
        assert moccache._parse_size("2K") == 2 * 1024
        assert moccache._parse_size("3M") == 3 * 1024**2
        assert moccache._parse_size("1G") == 1024**3
        assert moccache._parse_size("1T") == 1024**4

    def test_surrounding_whitespace_stripped(self) -> None:
        assert moccache._parse_size(" 256M ") == 256 * 1024**2

    def test_case_and_b_suffix(self) -> None:
        assert moccache._parse_size("2kb") == 2 * 1024
        assert moccache._parse_size("1gB") == 1024**3

    def test_invalid_raises(self) -> None:
        with pytest.raises(ValueError):
            moccache._parse_size("abc")
        with pytest.raises(ValueError):
            moccache._parse_size("")


class TestTrim:
    def test_under_limit_removes_nothing(self, tmp_path: Path) -> None:
        entries = [_fake_entry(tmp_path, f"{i:02x}entry", 100, age=i) for i in range(3)]
        assert moccache._trim(tmp_path, max_bytes=10_000) == 0
        assert all(e.is_dir() for e in entries)

    def test_over_limit_evicts_oldest_first(self, tmp_path: Path) -> None:
        old = _fake_entry(tmp_path, "aa" + "0" * 6, 500, age=1000)
        mid = _fake_entry(tmp_path, "bb" + "0" * 6, 500, age=500)
        new = _fake_entry(tmp_path, "cc" + "0" * 6, 500, age=0)
        removed = moccache._trim(tmp_path, max_bytes=1000)
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
            moccache.shutil,
            "rmtree",
            lambda p, **kw: None if p == old else real_rmtree(p, **kw),
        )
        removed = moccache._trim(tmp_path, max_bytes=1000)
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
        monkeypatch.setattr(sys, "argv", ["moccache.py", "--trim", "--max-size", "1"])
        assert moccache.main() == 0
        assert not any(p for p in harness.cache.glob("??/*") if p.is_dir())

    def test_trim_cli_bad_size_errors(self, monkeypatch: pytest.MonkeyPatch) -> None:
        monkeypatch.setattr(sys, "argv", ["moccache.py", "--trim", "--max-size", "bogus"])
        assert moccache.main() == 2

    def test_trim_cli_max_size_missing_value_errors(
        self, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
    ) -> None:
        monkeypatch.setattr(sys, "argv", ["moccache.py", "--trim", "--max-size"])
        assert moccache.main() == 2
        assert "--max-size requires a value" in capsys.readouterr().err

    def test_trim_cli_unknown_argument_errors(
        self, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
    ) -> None:
        monkeypatch.setattr(sys, "argv", ["moccache.py", "--trim", "--frobnicate"])
        assert moccache.main() == 2
        assert "unknown --trim argument: --frobnicate" in capsys.readouterr().err

    def test_trim_cli_no_size_from_anywhere_errors(
        self, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
    ) -> None:
        monkeypatch.delenv("MOCCACHE_MAX_SIZE", raising=False)
        monkeypatch.setattr(sys, "argv", ["moccache.py", "--trim"])
        assert moccache.main() == 2
        assert "requires --max-size or MOCCACHE_MAX_SIZE" in capsys.readouterr().err

    def test_auto_trim_on_miss_when_max_size_set(
        self, harness: Harness, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        _fake_entry(harness.cache, "ee" + "0" * 6, 100_000, age=99_999)
        monkeypatch.setenv("MOCCACHE_MAX_SIZE", "1K")
        tree = harness.make_tree("build")
        harness.run(tree)
        assert harness.stats() == ["miss"]
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
        moccache._maybe_auto_trim(tmp_path)
        assert big.is_dir()  # no trim, but also no crash
        assert "bad-max-size\tbogus" in (tmp_path / "stats.log").read_text()

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
        moccache._maybe_auto_trim(tmp_path)
        assert not big.is_dir()
        assert (tmp_path / "trim.stamp").is_file()

    def test_auto_trim_skipped_while_lock_held(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        monkeypatch.setenv("MOCCACHE_MAX_SIZE", "1K")
        big = _fake_entry(tmp_path, "ee" + "0" * 6, 100_000, age=99_999)
        (tmp_path / "trim.lock").touch()  # another process is trimming
        moccache._maybe_auto_trim(tmp_path)
        assert big.is_dir()
        assert (tmp_path / "trim.lock").is_file()  # not ours to remove

    def test_stale_lock_recovered(self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
        monkeypatch.setenv("MOCCACHE_MAX_SIZE", "1K")
        big = _fake_entry(tmp_path, "ee" + "0" * 6, 100_000, age=99_999)
        lock = tmp_path / "trim.lock"
        lock.touch()
        stale = os.stat(lock).st_mtime - 10_000  # crashed holder, long past interval
        os.utime(lock, (stale, stale))
        moccache._maybe_auto_trim(tmp_path)  # reclaims the stale lock
        assert not lock.is_file()
        moccache._maybe_auto_trim(tmp_path)  # next miss can trim
        assert not big.is_dir()

    def test_lock_released_after_trim(
        self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        monkeypatch.setenv("MOCCACHE_MAX_SIZE", "1K")
        big = _fake_entry(tmp_path, "ee" + "0" * 6, 100_000, age=99_999)
        moccache._maybe_auto_trim(tmp_path)
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
        real_trim = moccache._trim

        def boom(cache_dir: Path, max_bytes: int) -> int:
            raise OSError("simulated crash mid-trim")

        monkeypatch.setattr(moccache, "_trim", boom)
        moccache._maybe_auto_trim(tmp_path)
        assert not (tmp_path / "trim.stamp").exists()  # no completed trim
        assert not (tmp_path / "trim.lock").exists()  # lock still released
        monkeypatch.setattr(moccache, "_trim", real_trim)
        moccache._maybe_auto_trim(tmp_path)  # next miss retries immediately
        assert not big.is_dir()
        assert (tmp_path / "trim.stamp").is_file()
