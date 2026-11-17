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
    metadata = build / "QGroundControl.AppImage.build.json"
    assert read_github_file(gh_output) == {
        "path": str(artifact),
        "checksum": str(build / "QGroundControl.AppImage.sha256"),
        "companion": "",
        "dependency-sbom": str(build / "QGroundControl-linux.dependencies.cdx.json"),
        "metadata": str(metadata),
        # What the provenance attestation covers, a path to a line.
        "provenance-subjects": f"{artifact}\n{metadata}",
        "sbom-path": str(tmp_path / "QGroundControl-linux.sbom.spdx.json"),
        "subject-name": "QGroundControl-linux",
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


def _metadata(build: Path) -> dict:
    return json.loads((build / "QGroundControl.AppImage.build.json").read_text())


@pytest.mark.parametrize(("flag", "marked"), [("true", True), ("false", False), ("", False)])
def test_publish_marks_the_package_with_its_name_and_its_workflow_s_packages(
    build: Path, gh_output: Path, monkeypatch: pytest.MonkeyPatch, flag: str, marked: bool
) -> None:
    monkeypatch.setenv("GITHUB_REF", "refs/heads/master")
    monkeypatch.setenv("GITHUB_WORKFLOW", "Linux")
    source = (build / "QGroundControl.AppImage").rename(build / "QGroundControl-x86_64.AppImage")
    argv = ["prepare", "--artifact-name", source.name, "--package-name", "QGroundControl-x86_64"]

    assert attest_helper.main([*argv, "--publish", flag, "--job-id", "130639116867"]) == 0

    metadata = json.loads(source.with_name(f"{source.name}.build.json").read_text())
    # Every package the Linux workflow marks in this checkout: the publisher holds the commit's
    # set complete when it has exactly these from the Linux run.
    marker = {
        "name": "QGroundControl-x86_64.AppImage",
        "workflow": "Linux",
        "packages": ["QGroundControl-aarch64.AppImage", "QGroundControl-x86_64.AppImage"],
    }
    assert metadata.get("publish") == (marker if marked else None)
    # What the publisher checks against the run it downloads the package from.
    assert (metadata["schema_version"], metadata["event"]) == (2, "push")
    assert (metadata["ref"], metadata["commit"]) == ("refs/heads/master", "abc")
    assert (metadata["workflow"], metadata["run_id"]) == ("Linux", "123")
    # The job the publisher looks up, and the attempt it ran in.
    assert (metadata["job_id"], metadata["run_attempt"]) == ("130639116867", "2")


def _linux_package(build: Path, monkeypatch: pytest.MonkeyPatch) -> tuple[Path, list[str]]:
    """The x86_64 AppImage of a Linux build, and the arguments its step prepares it with."""
    monkeypatch.setenv("GITHUB_WORKFLOW", "Linux")
    source = (build / "QGroundControl.AppImage").rename(build / "QGroundControl-x86_64.AppImage")
    argv = ["prepare", "--artifact-name", source.name, "--package-name", "QGroundControl-x86_64"]
    return source, [*argv, "--job-id", "130639116867"]


@pytest.mark.parametrize("publish", ["true", "false"])
def test_an_appimage_s_update_file_is_attested_uploaded_and_listed_in_its_marker(
    build: Path, gh_output: Path, gh_summary: Path, monkeypatch: pytest.MonkeyPatch, publish: str
) -> None:
    # appimagetool writes it beside an AppImage that embeds update information.
    source, argv = _linux_package(build, monkeypatch)
    zsync = source.with_name(f"{source.name}.zsync")
    zsync.write_bytes(b"zsync: 0.6.2\n\nblock sums")

    assert attest_helper.main([*argv, "--publish", publish, "--companion", str(zsync)]) == 0

    outputs = read_github_file(gh_output)
    assert outputs["companion"] == str(zsync)
    assert outputs["provenance-subjects"].splitlines() == [
        str(source),
        str(source.with_name(f"{source.name}.build.json")),
        str(zsync),
    ]
    metadata = json.loads(source.with_name(f"{source.name}.build.json").read_text())
    # Publish Builds requires the file that a marker lists, and none without a marker.
    listed = [
        {
            "name": "QGroundControl-x86_64.AppImage.zsync",
            "size": 24,
            "sha256": hashlib.sha256(zsync.read_bytes()).hexdigest(),
        }
    ]
    assert metadata.get("publish", {}).get("companions") == (listed if publish == "true" else None)
    assert "| Companion file | `QGroundControl-x86_64.AppImage.zsync` |" in (
        gh_summary.read_text(encoding="utf-8")
    )


@pytest.mark.parametrize(
    ("companion", "content", "reason"),
    [
        # Only the package's own update file, from the package's directory, with content.
        ("QGroundControl-aarch64.AppImage.zsync", b"x", "is not a file that is published beside"),
        ("QGroundControl-x86_64.AppImage.sha256", b"x", "is not a file that is published beside"),
        ("notes.txt", b"x", "only an AppImage has one, its <file>.zsync"),
        ("other/QGroundControl-x86_64.AppImage.zsync", b"x", "is not beside"),
        ("QGroundControl-x86_64.AppImage.zsync", b"", "is missing or empty"),
        ("QGroundControl-x86_64.AppImage.zsync", None, "is missing or empty"),
    ],
)
def test_a_companion_that_is_not_the_package_s_update_file_fails_the_build(
    build: Path,
    gh_output: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    companion: str,
    content: bytes | None,
    reason: str,
) -> None:
    source, argv = _linux_package(build, monkeypatch)
    path = build / companion
    path.parent.mkdir(exist_ok=True)
    if content is not None:
        path.write_bytes(content)

    with pytest.raises(SystemExit) as exit_:
        attest_helper.main([*argv, "--publish", "true", "--companion", str(path)])

    assert exit_.value.code == 1
    assert "::error::attest-and-upload: companion-path: " in (out := capsys.readouterr().out)
    assert reason in out
    assert gh_output.read_text() == ""
    assert not source.with_name(f"{source.name}.build.json").exists()


def test_only_an_appimage_has_a_companion_file(
    build: Path, gh_output: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture
) -> None:
    monkeypatch.setenv("GITHUB_WORKFLOW", "MacOS")
    source = (build / "QGroundControl.AppImage").rename(build / "QGroundControl.dmg")
    zsync = source.with_name("QGroundControl.dmg.zsync")
    zsync.write_bytes(b"x")
    argv = ["prepare", "--artifact-name", source.name, "--package-name", "QGroundControl"]

    with pytest.raises(SystemExit):
        attest_helper.main([*argv, "--publish", "true", "--companion", str(zsync)])

    assert "'QGroundControl.dmg.zsync' is not a file that is published beside" in (
        capsys.readouterr().out
    )


def test_packages_are_unmarked_unless_the_caller_asks(build: Path, gh_output: Path) -> None:
    _prepare()

    assert "publish" not in _metadata(build)


@pytest.mark.parametrize(
    ("artifact", "package", "reason"),
    [
        # Uploaded as the wrong name, so its sidecar would name another file.
        ("QGroundControl.dmg", "QGroundControl", "not artifact-name 'QGroundControl.dmg'"),
        ("QGroundControl.ipa", "QGroundControl-ios", "not a publishable QGroundControl package"),
        # The publisher only downloads distributable artifacts.
        ("QGroundControl.AppImage", "linux-package", "not a distributable artifact name"),
        # A package its own marker would not list: the publisher would hold the set wrong.
        (
            "QGroundControl.AppImage",
            "QGroundControl-x86_64",
            "PUBLISHED_BUILDS does not list 'QGroundControl.AppImage' as a package of the 'Linux'",
        ),
    ],
)
def test_publish_rejects_a_package_the_publisher_could_not_publish(
    build: Path,
    gh_output: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    artifact: str,
    package: str,
    reason: str,
) -> None:
    monkeypatch.setenv("GITHUB_WORKFLOW", "Linux")
    source = build / "QGroundControl.AppImage"
    if artifact.endswith(".ipa"):
        source = source.rename(build / artifact)
    argv = ["prepare", "--artifact-name", artifact, "--override", str(source)]

    with pytest.raises(SystemExit) as excinfo:
        attest_helper.main([*argv, "--package-name", package, "--publish", "true"])

    assert excinfo.value.code == 1
    assert reason in capsys.readouterr().out
    assert not list(build.glob("*.build.json"))
    assert gh_output.read_text() == ""


@pytest.mark.parametrize("job_id", [None, "", "build", "0", "1.30639116867E+11"])
def test_a_package_marked_without_a_usable_job_id_still_builds_and_warns(
    build: Path,
    gh_output: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    job_id: str | None,
) -> None:
    # A runner that does not tell the job its id is no fault of the build. The package is
    # marked without one, and the publisher refuses it in its own run.
    monkeypatch.setenv("GITHUB_WORKFLOW", "Linux")
    source = (build / "QGroundControl.AppImage").rename(build / "QGroundControl-x86_64.AppImage")
    argv = ["prepare", "--artifact-name", source.name, "--package-name", "QGroundControl-x86_64"]
    argv += [] if job_id is None else ["--job-id", job_id]

    assert attest_helper.main([*argv, "--publish", "true"]) == 0

    metadata = json.loads(source.with_name(f"{source.name}.build.json").read_text())
    assert metadata["publish"]["name"] == source.name
    assert metadata["job_id"] == ""
    assert read_github_file(gh_output)["metadata"] == str(build / f"{source.name}.build.json")
    out = capsys.readouterr().out
    assert out.count("::warning title=Package cannot be published from this run::") == 1
    assert f"job-id is '{job_id or ''}', not the number that `job.check_run_id` gives" in out
    assert "::error" not in out

    # A package that is not published needs none, and one that has it gets no warning.
    assert attest_helper.main([*argv, "--publish", "false"]) == 0
    assert attest_helper.main([*argv, "--publish", "true", "--job-id", "130639116867"]) == 0
    assert "::warning" not in capsys.readouterr().out


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
