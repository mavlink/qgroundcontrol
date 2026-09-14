"""Coverage must survive a ccache hit, including instrumented static libraries."""

import gzip
import json
import os
import shutil
import subprocess
from pathlib import Path

import pytest


def test_coverage_report_after_cached_rebuild(tmp_path: Path) -> None:
    ccache = shutil.which("ccache")
    compiler = shutil.which("g++", path="/usr/bin:/bin")
    if not ccache or not compiler or not shutil.which("ninja"):
        pytest.skip("GCC, ccache and Ninja are required")
    major = subprocess.check_output([compiler, "-dumpversion"], text=True).strip().split(".")[0]
    gcov = shutil.which(f"gcov-{major}")
    if not gcov:
        pytest.skip("Matching gcov is required")
    repo = Path(__file__).resolve().parents[2]
    env = {key: value for key, value in os.environ.items() if not key.startswith("CCACHE_")}
    env.update(
        CCACHE_DIR=str(tmp_path / "cache"),
        CCACHE_CONFIGPATH=str(repo / "tools/configs/ccache.conf"),
        CCACHE_BASEDIR=str(tmp_path),
    )
    source = tmp_path / "source"
    source.mkdir()
    (source / "main.cpp").write_text(
        """#include <atomic>
#include <thread>
#include <vector>
int classify(int);
int main() {
    std::atomic<bool> start{false};
    std::vector<std::thread> workers;
    for (int i = 0; i < 8; ++i) {
        workers.emplace_back([&start] {
            while (!start.load()) {
                std::this_thread::yield();
            }
            for (int j = 0; j < 100000; ++j) {
                classify(1);
            }
        });
    }
    start.store(true);
    for (auto &worker : workers) {
        worker.join();
    }
}
"""
    )
    (source / "library.cpp").write_text("int classify(int value) { return value > 0 ? 0 : 1; }\n")
    (source / "CMakeLists.txt").write_text(
        f"""cmake_minimum_required(VERSION 3.25)
project(CoverageProbe LANGUAGES CXX)
find_package(Threads REQUIRED)
set(QGC_ENABLE_COVERAGE ON)
set(QGC_COVERAGE_LINE_THRESHOLD 30)
set(QGC_COVERAGE_BRANCH_THRESHOLD 20)
include("{repo.as_posix()}/cmake/modules/Coverage.cmake")
add_library(probe STATIC library.cpp)
add_executable(CoverageProbe main.cpp)
target_link_libraries(CoverageProbe PRIVATE probe Threads::Threads)
target_precompile_headers(probe PRIVATE <vector>)
target_compile_options(probe PRIVATE -fpch-preprocess)
qgc_apply_coverage_to_target(probe)
qgc_apply_coverage_to_target(CoverageProbe)
"""
    )
    build = tmp_path / "build"
    subprocess.run(
        [
            "cmake",
            "-S",
            str(source),
            "-B",
            str(build),
            "-G",
            "Ninja",
            "-DCMAKE_BUILD_TYPE=Debug",
            f"-DCMAKE_CXX_COMPILER={compiler}",
            f"-DCMAKE_CXX_COMPILER_LAUNCHER={ccache}",
        ],
        env=env,
        check=True,
    )
    subprocess.run(["cmake", "--build", str(build)], env=env, check=True)
    notes = {path: path.read_bytes() for path in build.glob("**/*.gcno")}
    assert len(notes) == 3
    for path in [*notes, *build.glob("**/*.o"), *build.glob("**/*.gch")]:
        path.unlink()
    subprocess.run(["cmake", "--build", str(build)], env=env, check=True)
    assert all(path.read_bytes() == data for path, data in notes.items())
    stats = subprocess.check_output([ccache, "--print-stats"], env=env, text=True)
    counters = dict(line.split() for line in stats.splitlines())
    assert int(counters["direct_cache_hit"]) + int(counters["preprocessed_cache_hit"]) >= 2
    subprocess.run([str(build / "CoverageProbe")], cwd=build, env=env, check=True)
    for path in notes:
        if "cmake_pch" in path.name:
            continue
        subprocess.run([gcov, "--json-format", str(path)], cwd=build, env=env, check=True)
    reports = list(build.glob("*.gcov.json.gz"))
    assert len(reports) == 2
    classify_counts = []
    for report in reports:
        with gzip.open(report, "rt") as stream:
            data = json.load(stream)
        assert any(line["count"] > 0 for file in data["files"] for line in file["lines"])
        classify_counts.extend(
            function["execution_count"]
            for file in data["files"]
            for function in file["functions"]
            if function["demangled_name"] == "classify(int)"
        )
    assert classify_counts == [800000]
