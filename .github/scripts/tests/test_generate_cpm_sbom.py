"""SBOM generation contracts for CPM dependencies."""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import TYPE_CHECKING

import generate_cpm_sbom as mod
from common.cmake import read_cache_dict

if TYPE_CHECKING:
    import pytest

SAMPLE_CACHE = """\
CMAKE_BUILD_TYPE:STRING=Debug
CPM_PACKAGES:INTERNAL=zlib;mavlink;earcut_hpp
CPM_PACKAGE_zlib_VERSION:INTERNAL=1.3.2
CPM_PACKAGE_zlib_SOURCE_DIR:PATH=/src/zlib
CPM_PACKAGE_mavlink_VERSION:INTERNAL=0
CPM_PACKAGE_mavlink_SOURCE_DIR:PATH=/src/mavlink
CPM_PACKAGE_earcut_hpp_VERSION:INTERNAL=0
CPM_PACKAGE_earcut_hpp_SOURCE_DIR:PATH=/src/earcut
"""


def _git_info(source_dir: Path) -> tuple[str, str]:
    return {
        "/src/zlib": (
            "https://github.com/madler/zlib.git",
            "da607da739fa6047df13e66a2af6b8bec7c2a498",
        ),
        "/src/mavlink": (
            "https://github.com/mavlink/mavlink.git",
            "b1fb5a1a32c41c6e46fea70600d626a0b5a8edbe",
        ),
        "/src/earcut": (
            "https://github.com/mapbox/earcut.hpp.git",
            "f36ced7e50254738c4e5af1a239f5fb7b1094007",
        ),
    }.get(str(source_dir), ("", ""))


def test_cache_parsing_url_normalization_and_purls(tmp_path: Path) -> None:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text(SAMPLE_CACHE)
    entries = read_cache_dict(str(cache))
    assert entries["CPM_PACKAGES"] == "zlib;mavlink;earcut_hpp"
    assert entries["CPM_PACKAGE_zlib_VERSION"] == "1.3.2"
    assert entries["CMAKE_BUILD_TYPE"] == "Debug"

    urls = {
        "https://github.com/madler/zlib.git": "https://github.com/madler/zlib",
        "git@github.com:madler/zlib.git": "https://github.com/madler/zlib",
        "https://gitlab.com/bzip2/bzip2.git": "https://gitlab.com/bzip2/bzip2",
        "https://example.com/repo": "https://example.com/repo",
    }
    for url, expected in urls.items():
        assert mod.normalize_git_url(url) == expected

    purls = [
        (
            ("zlib", "1.3.2", "https://github.com/madler/zlib.git", "abc123"),
            "pkg:github/madler/zlib@1.3.2",
        ),
        (
            ("mavlink", "0", "https://github.com/mavlink/mavlink.git", "abc123def456789"),
            "pkg:github/mavlink/mavlink@abc123def456",
        ),
        (
            ("bzip2", "1.0.8", "https://gitlab.com/bzip2/bzip2.git", "abc123"),
            "pkg:gitlab/bzip2/bzip2@1.0.8",
        ),
        (("something", "2.0", "", ""), "pkg:generic/something@2.0"),
        (("something", "0", "", ""), "pkg:generic/something"),
    ]
    for args, expected in purls:
        assert mod.make_purl(*args) == expected


def test_generate_sbom_captures_components_versions_hashes_and_empty_cache(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text(SAMPLE_CACHE)
    monkeypatch.setattr(mod, "git_info", _git_info)
    sbom = mod.generate_sbom(tmp_path)
    assert (sbom["bomFormat"], sbom["specVersion"]) == ("CycloneDX", "1.6")
    components = {component["name"]: component for component in sbom["components"]}
    assert set(components) == {"zlib", "mavlink", "earcut_hpp"}
    assert components["zlib"]["purl"] == "pkg:github/madler/zlib@1.3.2"
    assert components["zlib"]["hashes"][0]["content"] == _git_info(Path("/src/zlib"))[1]
    assert components["zlib"]["externalReferences"][0]["url"] == "https://github.com/madler/zlib"
    assert components["mavlink"]["version"] == "b1fb5a1a32c4"

    cache.write_text("CMAKE_BUILD_TYPE:STRING=Release\n")
    assert mod.generate_sbom(tmp_path)["components"] == []


def test_generate_spdx_captures_dependency_relationships_and_purls(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    (tmp_path / "CMakeCache.txt").write_text(SAMPLE_CACHE)
    monkeypatch.setattr(mod, "git_info", _git_info)

    sbom = mod.generate_spdx(tmp_path)

    assert sbom["spdxVersion"] == "SPDX-2.2"
    packages = {package["name"]: package for package in sbom["packages"]}
    assert set(packages) == {"QGroundControl", "zlib", "mavlink", "earcut_hpp"}
    required_package_fields = {"licenseConcluded", "licenseDeclared", "copyrightText"}
    assert all(required_package_fields <= package.keys() for package in packages.values())
    assert packages["zlib"]["externalRefs"][0]["referenceLocator"] == (
        "pkg:github/madler/zlib@1.3.2"
    )
    assert packages["zlib"]["externalRefs"][0]["referenceCategory"] == "PACKAGE_MANAGER"
    assert packages["mavlink"]["checksums"][0]["algorithm"] == "SHA1"
    dependency_ids = {
        relationship["relatedSpdxElement"]
        for relationship in sbom["relationships"]
        if relationship["relationshipType"] == "DEPENDS_ON"
    }
    assert dependency_ids == {
        package["SPDXID"] for name, package in packages.items() if name != "QGroundControl"
    }


def test_cli_writes_equivalent_stdout_and_file_output(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    (tmp_path / "CMakeCache.txt").write_text(SAMPLE_CACHE)
    monkeypatch.setattr(mod, "git_info", _git_info)
    monkeypatch.setattr(sys, "argv", ["prog", "--build-dir", str(tmp_path)])
    assert mod.main() == 0
    stdout_sbom = json.loads(capsys.readouterr().out)

    output = tmp_path / "sbom.json"
    monkeypatch.setattr(sys, "argv", ["prog", "--build-dir", str(tmp_path), "-o", str(output)])
    assert mod.main() == 0
    file_sbom = json.loads(output.read_text())
    assert file_sbom["bomFormat"] == stdout_sbom["bomFormat"]
    assert {component["name"] for component in file_sbom["components"]} == {
        component["name"] for component in stdout_sbom["components"]
    }

    spdx_output = tmp_path / "sbom.spdx.json"
    monkeypatch.setattr(
        sys,
        "argv",
        ["prog", "--build-dir", str(tmp_path), "--format", "spdx", "-o", str(spdx_output)],
    )
    assert mod.main() == 0
    assert json.loads(spdx_output.read_text())["spdxVersion"] == "SPDX-2.2"
