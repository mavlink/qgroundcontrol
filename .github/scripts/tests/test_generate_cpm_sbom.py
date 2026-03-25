"""Tests for generate_cpm_sbom.py."""

from __future__ import annotations

import json
from pathlib import Path
from unittest.mock import patch

from generate_cpm_sbom import (
    generate_sbom,
    make_purl,
    normalize_git_url,
    parse_cmake_cache,
)

SAMPLE_CACHE = """\
CMAKE_BUILD_TYPE:STRING=Debug
CPM_PACKAGES:INTERNAL=zlib;mavlink;earcut_hpp
CPM_PACKAGE_zlib_VERSION:INTERNAL=1.3.2
CPM_PACKAGE_zlib_SOURCE_DIR:PATH=/src/zlib
CPM_PACKAGE_zlib_BINARY_DIR:PATH=/build/zlib
CPM_PACKAGE_mavlink_VERSION:INTERNAL=0
CPM_PACKAGE_mavlink_SOURCE_DIR:PATH=/src/mavlink
CPM_PACKAGE_mavlink_BINARY_DIR:PATH=/build/mavlink
CPM_PACKAGE_earcut_hpp_VERSION:INTERNAL=0
CPM_PACKAGE_earcut_hpp_SOURCE_DIR:PATH=/src/earcut
CPM_PACKAGE_earcut_hpp_BINARY_DIR:PATH=/build/earcut
"""


def test_parse_cmake_cache(tmp_path: Path) -> None:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text(SAMPLE_CACHE)
    entries = parse_cmake_cache(cache)
    assert entries["CPM_PACKAGES"] == "zlib;mavlink;earcut_hpp"
    assert entries["CPM_PACKAGE_zlib_VERSION"] == "1.3.2"
    assert entries["CPM_PACKAGE_mavlink_VERSION"] == "0"
    assert entries["CMAKE_BUILD_TYPE"] == "Debug"
    assert entries["CPM_PACKAGE_zlib_SOURCE_DIR"] == "/src/zlib"


def test_normalize_git_url() -> None:
    assert normalize_git_url("https://github.com/madler/zlib.git") == "https://github.com/madler/zlib"
    assert normalize_git_url("git@github.com:madler/zlib.git") == "https://github.com/madler/zlib"
    assert normalize_git_url("https://gitlab.com/bzip2/bzip2.git") == "https://gitlab.com/bzip2/bzip2"
    assert normalize_git_url("https://example.com/repo") == "https://example.com/repo"


def test_make_purl_github_with_version() -> None:
    purl = make_purl("zlib", "1.3.2", "https://github.com/madler/zlib.git", "abc123")
    assert purl == "pkg:github/madler/zlib@1.3.2"


def test_make_purl_github_commit_only() -> None:
    purl = make_purl("mavlink", "0", "https://github.com/mavlink/mavlink.git", "abc123def456789")
    assert purl == "pkg:github/mavlink/mavlink@abc123def456"


def test_make_purl_gitlab() -> None:
    purl = make_purl("bzip2", "1.0.8", "https://gitlab.com/bzip2/bzip2.git", "abc123")
    assert purl == "pkg:gitlab/bzip2/bzip2@1.0.8"


def test_make_purl_generic_fallback() -> None:
    purl = make_purl("something", "2.0", "", "")
    assert purl == "pkg:generic/something@2.0"


def test_make_purl_generic_no_version() -> None:
    purl = make_purl("something", "0", "", "")
    assert purl == "pkg:generic/something"


def _mock_git_info(source_dir: Path) -> tuple[str, str]:
    """Return fake git info based on the source dir path."""
    mapping = {
        "/src/zlib": ("https://github.com/madler/zlib.git", "da607da739fa6047df13e66a2af6b8bec7c2a498"),
        "/src/mavlink": ("https://github.com/mavlink/mavlink.git", "b1fb5a1a32c41c6e46fea70600d626a0b5a8edbe"),
        "/src/earcut": ("https://github.com/mapbox/earcut.hpp.git", "f36ced7e50254738c4e5af1a239f5fb7b1094007"),
    }
    return mapping.get(str(source_dir), ("", ""))


def test_generate_sbom(tmp_path: Path) -> None:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text(SAMPLE_CACHE)

    with patch("generate_cpm_sbom.git_info", side_effect=_mock_git_info):
        sbom = generate_sbom(tmp_path)

    assert sbom["bomFormat"] == "CycloneDX"
    assert sbom["specVersion"] == "1.6"
    assert len(sbom["components"]) == 3

    names = {c["name"] for c in sbom["components"]}
    assert names == {"zlib", "mavlink", "earcut_hpp"}

    zlib = next(c for c in sbom["components"] if c["name"] == "zlib")
    assert zlib["version"] == "1.3.2"
    assert zlib["purl"] == "pkg:github/madler/zlib@1.3.2"
    assert zlib["hashes"][0]["content"] == "da607da739fa6047df13e66a2af6b8bec7c2a498"
    assert zlib["externalReferences"][0]["url"] == "https://github.com/madler/zlib"

    mavlink = next(c for c in sbom["components"] if c["name"] == "mavlink")
    assert mavlink["version"] == "b1fb5a1a32c4"


def test_generate_sbom_empty_cache(tmp_path: Path) -> None:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text("CMAKE_BUILD_TYPE:STRING=Release\n")

    sbom = generate_sbom(tmp_path)
    assert sbom["components"] == []


def test_main_stdout(tmp_path: Path, monkeypatch, capsys) -> None:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text(SAMPLE_CACHE)
    monkeypatch.setattr("sys.argv", ["prog", "--build-dir", str(tmp_path)])

    with patch("generate_cpm_sbom.git_info", side_effect=_mock_git_info):
        from generate_cpm_sbom import main
        ret = main()

    assert ret == 0
    output = capsys.readouterr().out
    sbom = json.loads(output)
    assert len(sbom["components"]) == 3


def test_main_file_output(tmp_path: Path, monkeypatch) -> None:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text(SAMPLE_CACHE)
    out_file = tmp_path / "sbom.json"
    monkeypatch.setattr("sys.argv", ["prog", "--build-dir", str(tmp_path), "-o", str(out_file)])

    with patch("generate_cpm_sbom.git_info", side_effect=_mock_git_info):
        from generate_cpm_sbom import main
        ret = main()

    assert ret == 0
    sbom = json.loads(out_file.read_text())
    assert len(sbom["components"]) == 3
