"""Tests for generate_cpm_sbom.py."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING
from unittest.mock import patch

import pytest
from ci.security.generate_cpm_sbom import (
    generate_sbom,
    generate_spdx,
    git_exact_tag,
    gstreamer_component,
    main,
    make_purl,
    normalize_git_url,
)
from common.cmake import read_cache_dict

from .._helpers import git

if TYPE_CHECKING:
    from pathlib import Path

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


def test_spdx_snapshot_preserves_dependency_identity_and_relationships(tmp_path: Path) -> None:
    (tmp_path / "CMakeCache.txt").write_text(SAMPLE_CACHE)
    with patch("ci.security.generate_cpm_sbom.git_info", side_effect=_mock_git_info):
        sbom = generate_spdx(tmp_path)
    assert sbom["spdxVersion"] == "SPDX-2.2"
    assert sbom["creationInfo"]["created"].endswith("Z")
    root, *dependencies = sbom["packages"]
    assert len(dependencies) == 3
    assert len({p["SPDXID"] for p in sbom["packages"]}) == 4
    assert dependencies[0]["externalRefs"][0]["referenceLocator"] == "pkg:github/madler/zlib@1.3.2"
    assert "checksums" not in dependencies[0]  # A Git commit is not a package archive checksum.
    assert sbom["relationships"][0]["relationshipType"] == "DESCRIBES"
    assert sbom["relationships"][1:] == [
        {
            "spdxElementId": root["SPDXID"],
            "relationshipType": "DEPENDS_ON",
            "relatedSpdxElement": p["SPDXID"],
        }
        for p in dependencies
    ]


def test_gstreamer_component_names_the_bundled_sdk_and_its_packages() -> None:
    cache = {
        "QGC_ENABLE_GST_VIDEOSTREAMING": "ON",
        "QGC_GST_RESOLVED_SDK_VERSION": "1.28.4",
        "QGC_GST_RESOLVED_PACKAGES": "https://example.invalid/runtime.pkg|0a1b;"
        "https://example.invalid/devel.pkg|2c3d",
    }
    component = gstreamer_component(cache)
    assert component is not None
    assert component["purl"] == "pkg:generic/gstreamer@1.28.4"
    assert component["licenses"] == [{"expression": "LGPL-2.1-or-later"}]
    distributions = [r for r in component["externalReferences"] if r["type"] == "distribution"]
    assert [(r["url"], r["hashes"][0]["content"]) for r in distributions] == [
        ("https://example.invalid/runtime.pkg", "0a1b"),
        ("https://example.invalid/devel.pkg", "2c3d"),
    ]
    assert component["cpe"] == "cpe:2.3:a:gstreamer:gstreamer:1.28.4:*:*:*:*:*:*:*"
    # A system or user-provided GStreamer has no packages; its source location remains.
    system = gstreamer_component({**cache, "QGC_GST_RESOLVED_PACKAGES": ""})
    assert system is not None
    assert [r["type"] for r in system["externalReferences"]] == ["vcs", "website"]
    # NVD's version ranges do not know the fixes a distribution backports into its GStreamer.
    assert "cpe" not in system
    assert gstreamer_component({**cache, "QGC_ENABLE_GST_VIDEOSTREAMING": "OFF"}) is None


def test_gstreamer_component_links_a_nano_version_to_its_release() -> None:
    cache = {
        "QGC_ENABLE_GST_VIDEOSTREAMING": "ON",
        "QGC_GST_RESOLVED_SDK_VERSION": "1.28.8.1",
        "QGC_GST_RESOLVED_PACKAGES": "https://example.invalid/runtime.pkg|0a1b",
    }
    component = gstreamer_component(cache)
    assert component is not None
    assert component["version"] == "1.28.8.1"
    (vcs,) = [r for r in component["externalReferences"] if r["type"] == "vcs"]
    assert vcs["url"] == "https://gitlab.freedesktop.org/gstreamer/gstreamer/-/tree/1.28.8"
    assert component["cpe"] == "cpe:2.3:a:gstreamer:gstreamer:1.28.8:*:*:*:*:*:*:*"


DECODER_CACHE = """\
CPM_PACKAGES:INTERNAL=ZLIB;LibLZMA;LibArchive;zstd;lz4;BZip2;mavlink
CPM_PACKAGE_ZLIB_VERSION:INTERNAL=1.2.11
CPM_PACKAGE_LibLZMA_VERSION:INTERNAL=5.6.0
CPM_PACKAGE_LibArchive_VERSION:INTERNAL=3.7.2
CPM_PACKAGE_zstd_VERSION:INTERNAL=1.4.8
CPM_PACKAGE_lz4_VERSION:INTERNAL=1.9.2
CPM_PACKAGE_BZip2_VERSION:INTERNAL=1.0.6
CPM_PACKAGE_mavlink_VERSION:INTERNAL=0
CPM_PACKAGE_mavlink_SOURCE_DIR:PATH=/src/mavlink
"""


def test_components_name_the_nvd_product_scanners_match(tmp_path: Path) -> None:
    (tmp_path / "CMakeCache.txt").write_text(DECODER_CACHE)
    with patch("ci.security.generate_cpm_sbom.git_info", side_effect=_mock_git_info):
        components = {c["name"]: c for c in generate_sbom(tmp_path)["components"]}
    assert {name: c.get("cpe") for name, c in components.items()} == {
        "ZLIB": "cpe:2.3:a:zlib:zlib:1.2.11:*:*:*:*:*:*:*",
        # NVD files liblzma under xz: a CPE made from the CPM name misses CVE-2024-3094.
        "LibLZMA": "cpe:2.3:a:tukaani:xz:5.6.0:*:*:*:*:*:*:*",
        "LibArchive": "cpe:2.3:a:libarchive:libarchive:3.7.2:*:*:*:*:*:*:*",
        "zstd": "cpe:2.3:a:facebook:zstandard:1.4.8:*:*:*:*:*:*:*",
        "lz4": "cpe:2.3:a:lz4_project:lz4:1.9.2:*:*:*:*:*:*:*",
        "BZip2": "cpe:2.3:a:bzip:bzip2:1.0.6:*:*:*:*:*:*:*",
        # A commit is no version NVD lists.
        "mavlink": None,
    }
    assert components["mavlink"]["version"] == "b1fb5a1a32c4"


def test_spdx_carries_the_cpe_as_a_security_reference(tmp_path: Path) -> None:
    (tmp_path / "CMakeCache.txt").write_text(DECODER_CACHE)
    with patch("ci.security.generate_cpm_sbom.git_info", side_effect=_mock_git_info):
        packages = {p["name"]: p for p in generate_spdx(tmp_path)["packages"]}
    assert packages["LibLZMA"]["externalRefs"][1:] == [
        {
            "referenceCategory": "SECURITY",
            "referenceType": "cpe23Type",
            "referenceLocator": "cpe:2.3:a:tukaani:xz:5.6.0:*:*:*:*:*:*:*",
        }
    ]
    assert [r["referenceCategory"] for r in packages["mavlink"]["externalRefs"]] == [
        "PACKAGE_MANAGER"
    ]


def test_a_tag_gives_the_version_cpm_does_not_record(git_repo: Path) -> None:
    git(git_repo, "commit", "-q", "--allow-empty", "-m", "release")
    git(git_repo, "tag", "R_2_7_5")
    assert git_exact_tag(git_repo) == "R_2_7_5"
    (git_repo / "CMakeCache.txt").write_text(
        "CPM_PACKAGES:INTERNAL=EXPAT\n"
        f"CPM_PACKAGE_EXPAT_VERSION:INTERNAL=\nCPM_PACKAGE_EXPAT_SOURCE_DIR:PATH={git_repo}\n"
    )
    (expat,) = generate_sbom(git_repo)["components"]
    assert expat["version"] == "2.7.5"
    assert expat["cpe"] == "cpe:2.3:a:libexpat_project:libexpat:2.7.5:*:*:*:*:*:*:*"


def test_a_checkout_at_a_commit_gets_no_cpe_whatever_its_version(git_repo: Path) -> None:
    git(git_repo, "commit", "-q", "--allow-empty", "-m", "after 1.0.8")
    assert git_exact_tag(git_repo) == ""
    # bzip2 is pinned to a commit after 1.0.8, and its CPMAddPackage() states VERSION 1.0.8.
    (git_repo / "CMakeCache.txt").write_text(
        "CPM_PACKAGES:INTERNAL=BZip2\n"
        f"CPM_PACKAGE_BZip2_VERSION:INTERNAL=1.0.8\nCPM_PACKAGE_BZip2_SOURCE_DIR:PATH={git_repo}\n"
    )
    (bzip2,) = generate_sbom(git_repo)["components"]
    assert bzip2["version"] == "1.0.8"
    assert "cpe" not in bzip2


def test_spdx_declares_the_gstreamer_license(tmp_path: Path) -> None:
    cache = SAMPLE_CACHE + (
        "QGC_ENABLE_GST_VIDEOSTREAMING:BOOL=ON\nQGC_GST_RESOLVED_SDK_VERSION:INTERNAL=1.24.2\n"
    )
    (tmp_path / "CMakeCache.txt").write_text(cache)
    with patch("ci.security.generate_cpm_sbom.git_info", side_effect=_mock_git_info):
        sbom = generate_spdx(tmp_path)
    gstreamer = sbom["packages"][-1]
    assert gstreamer["name"] == "GStreamer"
    assert gstreamer["licenseDeclared"] == "LGPL-2.1-or-later"
    assert sbom["packages"][1]["licenseDeclared"] == "NOASSERTION"


def test_read_cache_dict(tmp_path: Path) -> None:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text(SAMPLE_CACHE)
    entries = read_cache_dict(str(cache))
    assert entries["CPM_PACKAGES"] == "zlib;mavlink;earcut_hpp"
    assert entries["CPM_PACKAGE_zlib_VERSION"] == "1.3.2"
    assert entries["CPM_PACKAGE_mavlink_VERSION"] == "0"
    assert entries["CMAKE_BUILD_TYPE"] == "Debug"
    assert entries["CPM_PACKAGE_zlib_SOURCE_DIR"] == "/src/zlib"


def test_normalize_git_url() -> None:
    assert (
        normalize_git_url("https://github.com/madler/zlib.git") == "https://github.com/madler/zlib"
    )
    assert normalize_git_url("git@github.com:madler/zlib.git") == "https://github.com/madler/zlib"
    assert (
        normalize_git_url("https://gitlab.com/bzip2/bzip2.git") == "https://gitlab.com/bzip2/bzip2"
    )
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
    }
    return mapping.get(str(source_dir), ("", ""))


def test_generate_sbom(tmp_path: Path) -> None:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text(SAMPLE_CACHE)

    with patch("ci.security.generate_cpm_sbom.git_info", side_effect=_mock_git_info):
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


def test_main_stdout(tmp_path: Path, capsys) -> None:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text(SAMPLE_CACHE)

    with patch("ci.security.generate_cpm_sbom.git_info", side_effect=_mock_git_info):
        ret = main(["--build-dir", str(tmp_path)])

    assert ret == 0
    output = capsys.readouterr().out
    sbom = json.loads(output)
    assert len(sbom["components"]) == 3


def test_main_file_output(tmp_path: Path) -> None:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text(SAMPLE_CACHE)
    out_file = tmp_path / "sbom.json"

    with patch("ci.security.generate_cpm_sbom.git_info", side_effect=_mock_git_info):
        ret = main(["--build-dir", str(tmp_path), "-o", str(out_file)])

    assert ret == 0
    sbom = json.loads(out_file.read_text())
    assert len(sbom["components"]) == 3


def test_main_requires_components(tmp_path: Path) -> None:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text(SAMPLE_CACHE)
    out_file = tmp_path / "sbom.json"
    argv = ["--build-dir", str(tmp_path), "--output", str(out_file), "--require-components"]

    with patch("ci.security.generate_cpm_sbom.git_info", side_effect=_mock_git_info):
        ret = main(argv)

    assert ret == 0
    assert len(json.loads(out_file.read_text())["components"]) == 3


@pytest.mark.parametrize("format_name", ["cyclonedx", "spdx"])
def test_main_rejects_empty_required_components(tmp_path: Path, capsys, format_name: str) -> None:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text("CMAKE_BUILD_TYPE:STRING=Release\n")
    out_file = tmp_path / "sbom.json"
    argv = ["--build-dir", str(tmp_path), "--output", str(out_file), "--require-components"]

    assert main([*argv, "--format", format_name]) == 1
    assert "contains no components" in capsys.readouterr().err
    assert not out_file.exists()
