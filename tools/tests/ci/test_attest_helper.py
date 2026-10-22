"""Tests for attest_helper.py."""

from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path

import pytest
from ci.publish import attest_helper

from .._helpers import REPO_ROOT

CACHE = """\
CMAKE_BUILD_TYPE:STRING=Release
MY_TOKEN:STRING=secret
CPM_PACKAGES:INTERNAL=zlib
CPM_PACKAGE_zlib_VERSION:INTERNAL=1.3.2
"""


def _outputs(path: Path) -> dict[str, str]:
    return dict(
        line.split("=", 1) for line in path.read_text(encoding="utf-8").splitlines() if line
    )


def _prepare(tmp_path: Path, *extra: str, build_dir: Path | None = None) -> None:
    attest_helper.main(
        [
            "prepare",
            "--default",
            str(tmp_path / "build" / "QGroundControl.AppImage"),
            "--build-dir",
            str(build_dir or tmp_path / "build"),
            "--subject-name",
            "QGroundControl-linux",
            "--runner-temp",
            str(tmp_path),
            *extra,
        ]
    )


@pytest.fixture
def build(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    (build_dir / "CMakeCache.txt").write_text(CACHE, encoding="utf-8")
    (build_dir / "QGroundControl.AppImage").write_bytes(b"artifact")
    monkeypatch.setenv("GITHUB_EVENT_NAME", "push")
    monkeypatch.setenv("GITHUB_SHA", "abc")
    monkeypatch.setenv("GITHUB_RUN_ID", "123")
    monkeypatch.setenv("GITHUB_RUN_ATTEMPT", "2")
    return build_dir


def test_prepare_emits_every_path_the_action_consumes(
    tmp_path: Path, build: Path, gh_output: Path
) -> None:
    _prepare(tmp_path)

    artifact = build / "QGroundControl.AppImage"
    assert _outputs(gh_output) == {
        "path": str(artifact),
        "parent": str(build),
        "checksum": str(build / "QGroundControl.AppImage.sha256"),
        "dependency-sbom": f"{build}/QGroundControl-linux.dependencies.cdx.json",
        "metadata": str(build / "QGroundControl.AppImage.build.json"),
        "skip": "false",
        "scan-path": str(build),
        "sbom-path": str(tmp_path / "QGroundControl-linux.sbom.spdx.json"),
    }
    assert (
        (build / "QGroundControl.AppImage.sha256")
        .read_text(encoding="utf-8")
        .endswith("  QGroundControl.AppImage\n")
    )


def test_prepare_skips_attestation_for_pull_requests(
    tmp_path: Path, build: Path, gh_output: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setenv("GITHUB_EVENT_NAME", "pull_request")

    _prepare(tmp_path)

    outputs = _outputs(gh_output)
    assert outputs["skip"] == "true"
    # PR artifacts still carry the checksum, dependency SBOM and build identity.
    for key in ("checksum", "dependency-sbom", "metadata"):
        assert Path(outputs[key]).is_file()


def test_prepare_prefers_override(tmp_path: Path, build: Path, gh_output: Path) -> None:
    override_dir = build / "Release"
    override_dir.mkdir()
    override = override_dir / "QGroundControl.ipa"
    override.write_bytes(b"ipa")

    _prepare(tmp_path, "--override", str(override))

    outputs = _outputs(gh_output)
    assert outputs["path"] == str(override)
    assert outputs["parent"] == str(override_dir)
    assert (
        outputs["dependency-sbom"] == f"{override_dir}/QGroundControl-linux.dependencies.cdx.json"
    )
    assert outputs["scan-path"] == str(build)


def test_prepare_fails_when_artifact_is_missing(
    tmp_path: Path, build: Path, gh_output: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    (build / "QGroundControl.AppImage").unlink()

    with pytest.raises(SystemExit) as excinfo:
        _prepare(tmp_path)

    assert excinfo.value.code == 1
    assert "artifact not found" in capsys.readouterr().out
    assert gh_output.read_text() == ""


def test_prepare_rejects_checksum_mismatch(tmp_path: Path, build: Path, gh_output: Path) -> None:
    (build / "QGroundControl.AppImage.sha256").write_text(
        f"{'0' * 64}  QGroundControl.AppImage\n", encoding="utf-8"
    )

    with pytest.raises(SystemExit) as excinfo:
        _prepare(tmp_path)

    assert excinfo.value.code == 1
    assert gh_output.read_text() == ""


def test_prepare_requires_dependency_components(
    tmp_path: Path, build: Path, gh_output: Path
) -> None:
    (build / "CMakeCache.txt").write_text("CMAKE_BUILD_TYPE:STRING=Release\n", encoding="utf-8")

    with pytest.raises(SystemExit) as excinfo:
        _prepare(tmp_path)

    assert excinfo.value.code == 1
    assert not (build / "QGroundControl-linux.dependencies.cdx.json").exists()
    assert gh_output.read_text() == ""


def test_prepare_requires_cmake_cache(tmp_path: Path, build: Path, gh_output: Path) -> None:
    with pytest.raises(SystemExit) as excinfo:
        _prepare(tmp_path, build_dir=tmp_path / "missing")

    assert excinfo.value.code == 1
    assert gh_output.read_text() == ""


def test_dependency_sbom_matches_generate_cpm_sbom(
    tmp_path: Path, build: Path, gh_output: Path
) -> None:
    _prepare(tmp_path)

    sbom = json.loads((build / "QGroundControl-linux.dependencies.cdx.json").read_text())
    assert sbom["bomFormat"] == "CycloneDX"
    assert [component["name"] for component in sbom["components"]] == ["zlib"]
    expected = attest_helper.generate_sbom(build)
    for volatile in ("serialNumber", "metadata"):
        sbom.pop(volatile)
        expected.pop(volatile)
    assert sbom == expected


def test_package_manifest_records_producer_and_checksum_without_arbitrary_cache_values(
    tmp_path: Path, build: Path, gh_output: Path
) -> None:
    _prepare(tmp_path)

    manifest = json.loads((build / "QGroundControl.AppImage.build.json").read_text())
    assert manifest["artifact"] == {
        "name": "QGroundControl.AppImage",
        "size": len(b"artifact"),
        "sha256": hashlib.sha256(b"artifact").hexdigest(),
    }
    assert manifest["commit"] == "abc"
    assert manifest["run_id"] == "123"
    assert manifest["run_attempt"] == "2"
    assert manifest["configuration"] == {"CMAKE_BUILD_TYPE": "Release"}


def test_action_consumes_only_outputs_prepare_emits(
    tmp_path: Path, build: Path, gh_output: Path
) -> None:
    _prepare(tmp_path)

    action = (REPO_ROOT / ".github/actions/attest-and-upload/action.yml").read_text()
    consumed = set(re.findall(r"steps\.prepare\.outputs\.([\w-]+)", action))
    assert consumed
    assert consumed <= set(_outputs(gh_output))


def _checksum(artifact: Path, *extra: str) -> None:
    attest_helper.main(["checksum", "--source-path", str(artifact), *extra])


@pytest.fixture
def package(tmp_path: Path) -> Path:
    path = tmp_path / "qgroundcontrol_5.0.0_amd64.deb"
    path.write_bytes(b"package")
    return path


def test_checksum_writes_missing_sidecar(package: Path, gh_output: Path) -> None:
    _checksum(package)

    sidecar = package.with_name(f"{package.name}.sha256")
    assert sidecar.read_text(encoding="utf-8") == (
        f"{hashlib.sha256(b'package').hexdigest()}  {package.name}\n"
    )
    assert _outputs(gh_output) == {"checksum": str(sidecar)}


def test_checksum_require_existing_fails_without_sidecar(
    package: Path, gh_output: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    with pytest.raises(SystemExit) as excinfo:
        _checksum(package, "--require-existing")

    assert excinfo.value.code == 1
    assert "Checksum sidecar not found" in capsys.readouterr().out
    assert not package.with_name(f"{package.name}.sha256").exists()
    assert gh_output.read_text() == ""


@pytest.mark.parametrize(
    "entry",
    [
        "{digest}  {name}\n",
        # Windows tools: BOM, uppercase digest, binary-mode marker, CRLF.
        "\ufeff{upper} *{name}\r\n",
    ],
)
def test_checksum_require_existing_accepts_and_canonicalizes_sidecar(
    package: Path, gh_output: Path, entry: str
) -> None:
    digest = hashlib.sha256(b"package").hexdigest()
    sidecar = package.with_name(f"{package.name}.sha256")
    sidecar.write_text(
        entry.format(digest=digest, upper=digest.upper(), name=package.name), encoding="utf-8"
    )

    _checksum(package, "--require-existing")

    assert sidecar.read_text(encoding="utf-8") == f"{digest}  {package.name}\n"
    assert _outputs(gh_output) == {"checksum": str(sidecar)}


@pytest.mark.parametrize(
    "entry",
    [
        f"{'0' * 64}  {{name}}\n",
        f"{hashlib.sha256(b'package').hexdigest()}  other.deb\n",
        "not a checksum\n",
    ],
)
def test_checksum_rejects_invalid_sidecar(package: Path, gh_output: Path, entry: str) -> None:
    sidecar = package.with_name(f"{package.name}.sha256")
    sidecar.write_text(entry.format(name=package.name), encoding="utf-8")

    for extra in ((), ("--require-existing",)):
        with pytest.raises(SystemExit) as excinfo:
            _checksum(package, *extra)
        assert excinfo.value.code == 1
    assert sidecar.read_text(encoding="utf-8") == entry.format(name=package.name)
    assert gh_output.read_text() == ""


def test_checksum_fails_when_artifact_is_missing(tmp_path: Path, gh_output: Path) -> None:
    with pytest.raises(SystemExit) as excinfo:
        _checksum(tmp_path / "missing.rpm")

    assert excinfo.value.code == 1
    assert not (tmp_path / "missing.rpm.sha256").exists()
    assert gh_output.read_text() == ""
