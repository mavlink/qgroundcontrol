"""Tests for attest_helper.py."""

from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path

import pytest
from ci.publish import attest_helper

from .._helpers import REPO_ROOT, read_github_file

pytestmark = pytest.mark.windows

CACHE = """\
CMAKE_BUILD_TYPE:STRING=Release
MY_TOKEN:STRING=secret
CPM_PACKAGES:INTERNAL=zlib
CPM_PACKAGE_zlib_VERSION:INTERNAL=1.3.2
"""


def _prepare(*extra: str) -> None:
    artifact = ["--artifact-name", "QGroundControl.AppImage"]
    names = [
        "--package-name",
        "QGroundControl-linux-x86_64",
        "--subject-name",
        "QGroundControl-linux",
    ]
    assert attest_helper.main(["prepare", *artifact, *names, *extra]) == 0


@pytest.fixture
def build(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    (build_dir / "CMakeCache.txt").write_text(CACHE, encoding="utf-8")
    (build_dir / "QGroundControl.AppImage").write_bytes(b"artifact")
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    monkeypatch.setenv("GITHUB_EVENT_NAME", "push")
    monkeypatch.setenv("GITHUB_SHA", "abc")
    monkeypatch.setenv("GITHUB_RUN_ID", "123")
    monkeypatch.setenv("GITHUB_RUN_ATTEMPT", "2")
    return build_dir


def test_prepare_emits_every_path_the_action_consumes(
    tmp_path: Path, build: Path, gh_output: Path
) -> None:
    _prepare()

    artifact = build / "QGroundControl.AppImage"
    assert read_github_file(gh_output) == {
        "path": str(artifact),
        "checksum": str(build / "QGroundControl.AppImage.sha256"),
        "dependency-sbom": str(build / "QGroundControl-linux.dependencies.cdx.json"),
        "metadata": str(build / "QGroundControl.AppImage.build.json"),
        "sbom-path": str(tmp_path / "QGroundControl-linux.sbom.spdx.json"),
        "subject-name": "QGroundControl-linux",
        "publish-aws": "false",
    }
    sidecar = build / "QGroundControl.AppImage.sha256"
    assert sidecar.read_bytes().endswith(b"  QGroundControl.AppImage\n")


def test_subject_defaults_to_the_package_name(build: Path, gh_output: Path) -> None:
    artifact = ["--artifact-name", "QGroundControl.AppImage"]
    assert (
        attest_helper.main(["prepare", *artifact, "--package-name", "QGC", "--subject-name", ""])
        == 0
    )

    outputs = read_github_file(gh_output)
    assert outputs["subject-name"] == "QGC"
    assert outputs["dependency-sbom"] == str(build / "QGC.dependencies.cdx.json")


RELEASE_ENV = {
    "GITHUB_EVENT_NAME": "push",
    "GITHUB_REF_TYPE": "branch",
    "GITHUB_REPOSITORY_OWNER": "mavlink",
    "UPLOAD_AWS": "true",
    "HAS_AWS_CREDENTIALS": "true",
}


@pytest.mark.parametrize(
    ("change", "publish"),
    [
        ({}, "true"),
        ({"GITHUB_REF_TYPE": "tag"}, "true"),
        ({"GITHUB_EVENT_NAME": "workflow_dispatch", "GITHUB_REF_TYPE": "tag"}, "true"),
        ({"GITHUB_EVENT_NAME": "workflow_dispatch"}, "false"),
        ({"GITHUB_EVENT_NAME": "pull_request"}, "false"),
        ({"GITHUB_EVENT_NAME": "merge_group"}, "false"),
        ({"GITHUB_REPOSITORY_OWNER": "someone"}, "false"),
        ({"UPLOAD_AWS": "false"}, "false"),
        ({"HAS_AWS_CREDENTIALS": "false"}, "false"),
        ({"UPLOAD_AWS": ""}, "false"),
    ],
)
def test_prepare_publishes_to_aws_only_for_credentialed_upstream_releases(
    build: Path, gh_output: Path, monkeypatch: pytest.MonkeyPatch, change: dict, publish: str
) -> None:
    for name, value in (RELEASE_ENV | change).items():
        monkeypatch.setenv(name, value)

    _prepare()

    assert read_github_file(gh_output)["publish-aws"] == publish


def test_prepare_adds_an_artifact_section_to_the_job_summary(
    build: Path, gh_output: Path, gh_summary: Path
) -> None:
    _prepare()

    artifact = build / "QGroundControl.AppImage"
    digest = (build / "QGroundControl.AppImage.sha256").read_text().split()[0]
    size = artifact.stat().st_size
    assert gh_summary.read_text(encoding="utf-8") == (
        "### Artifact\n\n| Item | Value |\n| --- | --- |\n"
        "| File | `QGroundControl.AppImage` |\n"
        f"| Size | 0.00 MB ({size} bytes) |\n"
        f"| SHA-256 | `{digest}` |\n"
        "| Attestation subject | `QGroundControl-linux` |\n\n"
    )


def test_pull_request_artifacts_keep_checksum_dependency_sbom_and_identity(
    build: Path, gh_output: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setenv("GITHUB_EVENT_NAME", "pull_request")

    _prepare()

    outputs = read_github_file(gh_output)
    for key in ("checksum", "dependency-sbom", "metadata"):
        assert Path(outputs[key]).is_file()


def test_prepare_requires_the_runner_temp_directory(
    build: Path, gh_output: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.delenv("RUNNER_TEMP")
    with pytest.raises(SystemExit) as excinfo:
        _prepare()
    assert excinfo.value.code == 2

    _prepare("--runner-temp", str(build.parent))
    assert read_github_file(gh_output)["path"] == str(build / "QGroundControl.AppImage")


def test_prepare_prefers_override(build: Path, gh_output: Path) -> None:
    override_dir = build / "Release"
    override_dir.mkdir()
    override = override_dir / "QGroundControl.ipa"
    override.write_bytes(b"ipa")

    _prepare("--override", str(override))

    outputs = read_github_file(gh_output)
    assert outputs["path"] == str(override)
    assert outputs["dependency-sbom"] == str(
        override_dir / "QGroundControl-linux.dependencies.cdx.json"
    )


def test_prepare_fails_when_artifact_is_missing(
    build: Path, gh_output: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    (build / "QGroundControl.AppImage").unlink()

    with pytest.raises(SystemExit) as excinfo:
        _prepare()

    assert excinfo.value.code == 1
    assert "artifact not found" in capsys.readouterr().out
    assert gh_output.read_text() == ""


def test_prepare_rejects_checksum_mismatch(build: Path, gh_output: Path) -> None:
    (build / "QGroundControl.AppImage.sha256").write_text(
        f"{'0' * 64}  QGroundControl.AppImage\n", encoding="utf-8"
    )

    with pytest.raises(SystemExit) as excinfo:
        _prepare()

    assert excinfo.value.code == 1
    assert gh_output.read_text() == ""


def test_prepare_requires_dependency_components(build: Path, gh_output: Path) -> None:
    (build / "CMakeCache.txt").write_text("CMAKE_BUILD_TYPE:STRING=Release\n", encoding="utf-8")

    with pytest.raises(SystemExit) as excinfo:
        _prepare()

    assert excinfo.value.code == 1
    assert not (build / "QGroundControl-linux.dependencies.cdx.json").exists()
    assert gh_output.read_text() == ""


def test_prepare_requires_cmake_cache(build: Path, gh_output: Path) -> None:
    (build / "CMakeCache.txt").unlink()
    with pytest.raises(SystemExit) as excinfo:
        _prepare()

    assert excinfo.value.code == 1
    assert gh_output.read_text() == ""


def test_dependency_sbom_matches_generate_cpm_sbom(build: Path, gh_output: Path) -> None:
    _prepare()

    sbom = json.loads((build / "QGroundControl-linux.dependencies.cdx.json").read_text())
    assert sbom["bomFormat"] == "CycloneDX"
    assert [component["name"] for component in sbom["components"]] == ["zlib"]
    expected = attest_helper.generate_sbom(build)
    for volatile in ("serialNumber", "metadata"):
        sbom.pop(volatile)
        expected.pop(volatile)
    assert sbom == expected


def test_package_manifest_records_producer_and_checksum_without_arbitrary_cache_values(
    build: Path, gh_output: Path
) -> None:
    _prepare()

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


def test_action_consumes_only_outputs_prepare_emits(build: Path, gh_output: Path) -> None:
    _prepare()

    action = (REPO_ROOT / ".github/actions/attest-and-upload/action.yml").read_text()
    consumed = set(re.findall(r"steps\.prepare\.outputs\.([\w-]+)", action))
    assert consumed
    assert consumed <= set(read_github_file(gh_output))


def _checksum(artifact: Path) -> None:
    assert attest_helper.main(["checksum", "--source-path", str(artifact)]) == 0


@pytest.fixture(params=["qgroundcontrol_5.0.0_amd64.deb", "qgroundcontrol-5.0.0-1.x86_64.rpm"])
def package(tmp_path: Path, request: pytest.FixtureRequest) -> Path:
    path = tmp_path / request.param
    path.write_bytes(b"package")
    return path


def test_checksum_writes_missing_arch_package_sidecar(tmp_path: Path, gh_output: Path) -> None:
    # makepkg emits no checksum, unlike CPack's DEB and RPM generators.
    package = tmp_path / "qgroundcontrol-5.0.0-1-x86_64.pkg.tar.zst"
    package.write_bytes(b"package")

    _checksum(package)

    sidecar = package.with_name(f"{package.name}.sha256")
    assert sidecar.read_bytes() == (
        f"{hashlib.sha256(b'package').hexdigest()}  {package.name}\n".encode()
    )
    assert read_github_file(gh_output) == {"checksum": str(sidecar)}


def test_checksum_requires_cpack_package_sidecars(
    package: Path, gh_output: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    with pytest.raises(SystemExit) as excinfo:
        _checksum(package)

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
def test_checksum_accepts_and_canonicalizes_cpack_sidecars(
    package: Path, gh_output: Path, entry: str
) -> None:
    digest = hashlib.sha256(b"package").hexdigest()
    sidecar = package.with_name(f"{package.name}.sha256")
    sidecar.write_bytes(
        entry.format(digest=digest, upper=digest.upper(), name=package.name).encode()
    )

    _checksum(package)

    assert sidecar.read_bytes() == f"{digest}  {package.name}\n".encode()
    assert read_github_file(gh_output) == {"checksum": str(sidecar)}


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

    with pytest.raises(SystemExit) as excinfo:
        _checksum(package)
    assert excinfo.value.code == 1
    assert sidecar.read_text(encoding="utf-8") == entry.format(name=package.name)
    assert gh_output.read_text() == ""


def test_checksum_fails_when_artifact_is_missing(tmp_path: Path, gh_output: Path) -> None:
    with pytest.raises(SystemExit) as excinfo:
        _checksum(tmp_path / "missing.rpm")

    assert excinfo.value.code == 1
    assert not (tmp_path / "missing.rpm.sha256").exists()
    assert gh_output.read_text() == ""
