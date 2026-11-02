"""Tests for artifact_report.py."""

from __future__ import annotations

import io
import os
import subprocess
import sys
import zipfile
from pathlib import Path

import pytest
from ci.packaging import artifact_report
from ci.packaging.artifact_report import Listing

pytestmark = pytest.mark.windows

REPO = "mavlink/qgroundcontrol"
ARTIFACT = "QGroundControl-linux-arm64-v8a"
RUN_URL = "https://github.com/mavlink/qgroundcontrol/actions/runs/7"
ELF = b"\x7fELF" + b"\0" * 60
MACHO = b"\xcf\xfa\xed\xfe" + b"\0" * 60

SEVEN_ZIP_LISTING = """
7-Zip 23.01 (x64) : Copyright (c) 1999-2023 Igor Pavlov : 2023-06-20

Listing archive: QGroundControl-installer-AMD64.exe

--
Path = QGroundControl-installer-AMD64.exe
Type = Nsis
Physical Size = 116503507

----------
Path = $PLUGINSDIR\\System.dll
Size = 12288
Packed Size = 116395471
Attributes =

Path = bin
Folder = +
Size =

Path = bin\\QGroundControl.exe
Size = 51904512
Packed Size =
Attributes =

Path = bin\\x264-164.dll
Size = 2285056
Attributes =

Path = share\\empty.txt
Size =
Attributes =
"""


def _zip(path: Path, members: dict[str, bytes]) -> Path:
    """Write a stored (uncompressed) zip so sizes don't depend on the zlib build."""
    with zipfile.ZipFile(path, "w", zipfile.ZIP_STORED) as archive:
        for name, data in members.items():
            archive.writestr(zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0)), data)
    return path


def _apk(path: Path, members: dict[str, bytes] | None = None) -> Path:
    files = {"AndroidManifest.xml": b"<manifest/>", "lib/arm64-v8a/libQGroundControl.so": ELF}
    return _zip(path, {**files, **(members or {})})


def _run(run_id: int = 7, **fields: object) -> dict:
    run = {
        "id": run_id,
        "run_number": 1234,
        "event": "push",
        "head_branch": "master",
        "head_sha": "eec9b88901234567",
        "html_url": RUN_URL,
        "head_repository": {"full_name": REPO},
    }
    return {**run, **fields}


@pytest.fixture
def github(monkeypatch: pytest.MonkeyPatch, tmp_path: Path):
    """Fake GitHub API: workflow runs, their artifacts, and artifact zips holding a package."""

    class FakeGitHub:
        def __init__(self) -> None:
            self.runs: list[dict] = []
            self.artifacts: dict[int, list[dict]] = {}
            self.packages: dict[int, dict[str, bytes]] = {}
            self.queries: list[str] = []
            self.listed: list[int] = []

        def add(self, run: dict, artifact: str, package: dict[str, bytes] | None = None) -> None:
            artifact_id = 100 + len(self.runs)
            self.runs.append(run)
            self.artifacts[run["id"]] = [{"id": artifact_id, "name": artifact, "expired": False}]
            if package is not None:
                self.packages[artifact_id] = package

        def gh_api_json(self, path: str) -> dict:
            self.queries.append(path)
            return {"workflow_runs": self.runs}

        def list_run_artifacts(self, repo: str, run_id: int) -> list[dict]:
            assert repo == REPO
            self.listed.append(run_id)
            return self.artifacts.get(run_id, [])

        def download_artifact_zip(self, repo: str, artifact_id: int, destination: Path) -> None:
            if artifact_id not in self.packages:
                raise RuntimeError(f"Cannot download artifact {artifact_id}: HTTP 410")
            destination.parent.mkdir(parents=True, exist_ok=True)
            _zip(destination, self.packages[artifact_id])

    fake = FakeGitHub()
    monkeypatch.setattr(artifact_report, "gh_api_json", fake.gh_api_json)
    monkeypatch.setattr(artifact_report, "list_run_artifacts", fake.list_run_artifacts)
    monkeypatch.setattr(artifact_report, "download_artifact_zip", fake.download_artifact_zip)
    monkeypatch.setenv("GITHUB_REPOSITORY", REPO)
    monkeypatch.setenv(
        "GITHUB_WORKFLOW_REF", f"{REPO}/.github/workflows/android.yml@refs/pull/1/merge"
    )
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path / "runner-temp"))
    (tmp_path / "runner-temp").mkdir()
    return fake


def _main(package: Path, *args: str, artifact: str = ARTIFACT) -> int:
    return artifact_report.main(["--package", str(package), "--artifact-name", artifact, *args])


def test_gpl_denylist_reads_the_plugin_policy() -> None:
    names = artifact_report.gpl_denylist()
    assert {"x264", "x265", "dvdread", "dvdnav", "faad", "a52dec", "mpeg2dec"} <= set(names)
    assert "openh264" not in names


def test_gpl_denylist_requires_the_list(tmp_path: Path) -> None:
    config = tmp_path / "build-config.json"
    config.write_text('{"gstreamer_plugin_policy": {"gpl_denylist": ["a52", "x264"]}}')
    assert artifact_report.gpl_denylist(config) == ["a52", "x264"]
    config.write_text('{"gstreamer_plugin_policy": {"optional_decoders": ["va"]}}')
    with pytest.raises(RuntimeError, match="gpl_denylist not found"):
        artifact_report.gpl_denylist(config)


@pytest.mark.parametrize(
    ("name", "matches"),
    [
        ("libx264.so.164", True),
        ("x264-164.dll", True),
        ("libgstx264.dylib", True),
        ("gstx265.dll", True),
        ("libgstdvdread.so", True),
        ("sid", True),
        ("libx2645.so", False),
        ("openh264-7.dll", False),
        ("sidebar.qml", False),
        ("gstmpegtsmux.dll", False),
    ],
)
def test_gpl_names_match_like_the_cmake_install_check(name: str, matches: bool) -> None:
    pattern = artifact_report.gpl_name_pattern(artifact_report.gpl_denylist())
    assert bool(pattern.match(name)) is matches


def test_parse_7z_listing_keeps_files_with_posix_paths() -> None:
    assert artifact_report.parse_7z_listing(SEVEN_ZIP_LISTING) == {
        "$PLUGINSDIR/System.dll": 12288,
        "bin/QGroundControl.exe": 51904512,
        "bin/x264-164.dll": 2285056,
        "share/empty.txt": 0,
    }
    with pytest.raises(RuntimeError, match="no file entries"):
        artifact_report.parse_7z_listing("Error: cannot open the file as archive")


def test_workflow_file_comes_from_the_workflow_ref() -> None:
    ref = "mavlink/qgroundcontrol/.github/workflows/windows.yml@refs/heads/master"
    assert artifact_report.workflow_file(ref) == "windows.yml"
    assert artifact_report.workflow_file("") == ""


def test_reports_the_diff_against_masters_artifact(github, tmp_path, gh_summary, capsys) -> None:
    app = "lib/arm64-v8a/libQGroundControl_arm64-v8a.so"
    baseline = _apk(tmp_path / "baseline.apk", {app: ELF * 100, "assets/old.qml": b"x" * 50})
    github.add(_run(), ARTIFACT, {"QGroundControl.apk": baseline.read_bytes()})
    package = _apk(
        tmp_path / "QGroundControl.apk",
        {app: ELF * 120, "lib/arm64-v8a/libgstreamer_android.so": ELF * 10},
    )

    assert _main(package) == 0

    assert github.queries == [
        f"repos/{REPO}/actions/workflows/android.yml/runs?branch=master&status=success&per_page=10"
    ]
    summary = gh_summary.read_text(encoding="utf-8")
    assert summary.startswith("### Artifact report\n\n| Item | Value |")
    assert "| Result | passed |" in summary
    assert f"| Baseline | [master run #1234]({RUN_URL}) at `eec9b8890` |" in summary
    assert "| Files | 1 added, 1 removed, 1 changed |" in summary
    assert "| Size | This build | master | Change |" in summary
    assert "**Added files** (1)\n\n| Path | Size |" in summary
    assert "| `lib/arm64-v8a/libgstreamer_android.so` | 640 B |" in summary
    assert "**Removed files** (1)" in summary
    assert "| `assets/old.qml` | 50 B |" in summary
    assert (
        "| `lib/arm64-v8a/libQGroundControl_arm64-v8a.so` | 6.2 KB | 7.5 KB | +1.2 KB (+20.0%) |"
        in summary
    )
    assert "<details>" not in summary
    out = capsys.readouterr().out
    assert f"Baseline: master run #1234 {RUN_URL}" in out
    assert "::warning title=Artifact size change::QGroundControl.apk package size changed +" in out
    assert "contents size changed +" in out
    assert not list((tmp_path / "runner-temp").iterdir())


def test_long_lists_are_cut_to_the_top_files_and_kept_in_full_collapsed(
    github, tmp_path, gh_summary
) -> None:
    github.add(_run(), ARTIFACT, {"QGroundControl.apk": _apk(tmp_path / "b.apk").read_bytes()})
    added = {f"assets/file{index}.qml": b"q" * (index + 1) for index in range(4)}
    package = _apk(tmp_path / "QGroundControl.apk", added)

    assert _main(package, "--top", "2") == 0

    summary = gh_summary.read_text(encoding="utf-8")
    top, _, collapsed = summary.partition("<details><summary>All 4 added files</summary>")
    assert "| `assets/file3.qml` | 4 B |\n| `assets/file2.qml` | 3 B |\n\n…and 2 more" in top
    assert "assets/file1.qml" not in top
    assert "| `assets/file0.qml` | 1 B |" in collapsed


@pytest.mark.parametrize(
    ("threshold", "new_size", "warnings"),
    [
        (10.0, 110, []),
        (10.0, 111, ["package size changed +11.0%"]),
        (10.0, 85, ["package size changed -15.0%"]),
        (20.0, 85, []),
    ],
)
def test_size_changes_warn_beyond_the_threshold(capsys, threshold, new_size, warnings) -> None:
    current = Listing(size=new_size, files={"a": 1000})
    baseline = Listing(size=100, files={"a": 1000})

    artifact_report.warn_size_changes("QGC.apk", current, baseline, threshold, "master")

    out = capsys.readouterr().out
    expected = [
        f"QGC.apk {warning} vs master (0.00 MB -> 0.00 MB), beyond the ±{threshold:g}% threshold"
        for warning in warnings
    ]
    assert [line for line in out.splitlines() if line.startswith("::warning")] == [
        f"::warning title=Artifact size change::{message.replace('%', '%25')}"
        for message in expected
    ]


def test_contents_size_change_warns_too(capsys) -> None:
    current = Listing(size=100, files={"a": 700, "b": 600})
    artifact_report.warn_size_changes("QGC.apk", current, Listing(100, {"a": 1000}), 10, "master")
    assert "QGC.apk contents size changed +30.0%25 vs master" in capsys.readouterr().out


def test_threshold_comes_from_the_action_input(github, tmp_path, monkeypatch, capsys) -> None:
    baseline = _apk(tmp_path / "baseline.apk")
    github.add(_run(), ARTIFACT, {"QGroundControl.apk": baseline.read_bytes()})
    package = _apk(tmp_path / "QGroundControl.apk", {"assets/big.bin": b"b" * 400})
    monkeypatch.setenv("INPUT_PACKAGE", str(package))
    monkeypatch.setenv("INPUT_ARTIFACT_NAME", ARTIFACT)
    monkeypatch.setenv("INPUT_SIZE_THRESHOLD", "1000")

    assert artifact_report.main([]) == 0

    assert "::warning" not in capsys.readouterr().out


def test_rejects_a_non_positive_threshold(tmp_path, capsys) -> None:
    with pytest.raises(SystemExit) as raised:
        _main(_apk(tmp_path / "QGroundControl.apk"), "--size-threshold", "0")
    assert raised.value.code == 2
    assert "must be positive" in capsys.readouterr().err


def test_gpl_files_and_binary_markers_fail(tmp_path, gh_summary, capsys, monkeypatch) -> None:
    monkeypatch.delenv("GITHUB_REPOSITORY", raising=False)
    package = _apk(
        tmp_path / "QGroundControl.apk",
        {
            "lib/arm64-v8a/libgstx264.so": ELF,
            "lib/arm64-v8a/libgstreamer_android.so": ELF + b"..x264_encoder_open_164..",
            "assets/notes.txt": b"x264_encoder_open is mentioned in text, not linked",
        },
    )

    assert _main(package) == 1

    out = capsys.readouterr().out
    assert out.count("::error title=GPL check::") == 2
    assert (
        "::error title=GPL check::QGroundControl.apk ships GPL-licensed code: "
        "lib/arm64-v8a/libgstx264.so (named after GPL x264)"
    ) in out
    assert "lib/arm64-v8a/libgstreamer_android.so (contains x264_encoder_open)" in out
    summary = gh_summary.read_text(encoding="utf-8")
    assert "| Result | failed: 2 GPL findings |" in summary
    assert "| `lib/arm64-v8a/libgstx264.so` | named after GPL `x264` |" in summary
    assert "| `lib/arm64-v8a/libgstreamer_android.so` | contains `x264_encoder_open` |" in summary
    assert "notes.txt" not in summary


def test_ipa_mach_o_binaries_are_scanned_across_chunks(tmp_path, monkeypatch) -> None:
    monkeypatch.setattr(artifact_report, "SCAN_CHUNK_BYTES", 7)
    package = _zip(
        tmp_path / "QGroundControl.ipa",
        {
            "Payload/QGroundControl.app/QGroundControl": MACHO
            + b"http://www.videolan.org/x264.html",
            "Payload/QGroundControl.app/Frameworks/gst.framework/gst": MACHO + b"(c) Multicoreware",
            "Payload/QGroundControl.app/Info.plist": b"x265_encoder_open",
        },
    )

    assert artifact_report.scan_binaries(package) == [
        ("Payload/QGroundControl.app/QGroundControl", "videolan.org/x264.html"),
        ("Payload/QGroundControl.app/Frameworks/gst.framework/gst", "Multicoreware"),
    ]


@pytest.mark.parametrize(
    ("setup", "reason"),
    [
        (lambda gh: None, "no successful master run of android.yml has a live"),
        (lambda gh: gh.add(_run(event="pull_request"), ARTIFACT), "no successful master run"),
        (
            lambda gh: gh.add(_run(head_repository={"full_name": "fork/qgc"}), ARTIFACT),
            "no successful master run",
        ),
        (lambda gh: gh.add(_run(), "QGroundControl-mac-arm64-v8a"), "no successful master run"),
        (
            lambda gh: gh.add(_run(), ARTIFACT),
            f"cannot read master's {ARTIFACT}: Cannot download artifact 100",
        ),
        (
            lambda gh: gh.add(_run(), ARTIFACT, {"Other.apk": b"x"}),
            "the artifact has no QGroundControl.apk",
        ),
    ],
    ids=["no-runs", "pull-request-run", "fork-run", "other-artifact", "expired", "no-package"],
)
def test_missing_baseline_reports_without_a_diff(
    github, tmp_path, gh_summary, capsys, setup, reason
) -> None:
    setup(github)
    package = _apk(tmp_path / "QGroundControl.apk")

    assert _main(package) == 0

    summary = gh_summary.read_text(encoding="utf-8")
    assert "| Result | passed |" in summary
    assert "| Baseline | none: " in summary
    assert "| Files |" not in summary
    assert "| Size | This build |\n" in summary
    out = capsys.readouterr().out
    assert reason in out
    assert "::warning" not in out


def test_api_failures_and_unknown_workflows_leave_the_diff_out(
    github, tmp_path, gh_summary, monkeypatch, capsys
) -> None:
    def fail(path: str) -> dict:
        raise subprocess.CalledProcessError(1, ["gh", "api", path], stderr="HTTP 403")

    monkeypatch.setattr(artifact_report, "gh_api_json", fail)
    package = _apk(tmp_path / "QGroundControl.apk")
    assert _main(package) == 0
    assert "No baseline: cannot read master's" in capsys.readouterr().out

    monkeypatch.delenv("GITHUB_WORKFLOW_REF")
    assert _main(package) == 0
    assert "No baseline: the repository or workflow is unknown" in capsys.readouterr().out
    assert gh_summary.read_text(encoding="utf-8").count("| Baseline | none: ") == 2


def test_baseline_is_the_newest_master_push_run_with_the_artifact(github) -> None:
    github.add(_run(1, event="pull_request"), ARTIFACT)
    github.add(_run(2, head_branch="feature"), ARTIFACT)
    github.add(_run(3), "QGroundControl-mac-arm64-v8a")
    github.add(_run(4, event="workflow_dispatch"), ARTIFACT)
    github.add(_run(5), ARTIFACT)

    found = artifact_report.find_baseline_artifact(REPO, "android.yml", "master", ARTIFACT)

    assert found is not None
    run, artifact = found
    assert run["id"] == 4
    assert artifact["name"] == ARTIFACT
    assert github.listed == [3, 4]


def test_nsis_installers_are_listed_with_7zip(tmp_path, monkeypatch, gh_summary, capsys) -> None:
    commands = []

    def run_captured(cmd, **kwargs):
        commands.append(cmd)
        return subprocess.CompletedProcess(cmd, 0, stdout=SEVEN_ZIP_LISTING, stderr="")

    monkeypatch.setattr(artifact_report, "_seven_zip", lambda: "7z")
    monkeypatch.setattr(artifact_report, "run_captured", run_captured)
    package = tmp_path / "QGroundControl-installer-AMD64.exe"
    package.write_bytes(b"MZ" + b"\0" * 98)

    assert _main(package, artifact="QGroundControl-installer-AMD64") == 1

    assert commands == [["7z", "l", "-slt", str(package)]]
    assert "bin/x264-164.dll (named after GPL x264)" in capsys.readouterr().out
    assert "| Contents | 51.69 MB in 4 files |" in gh_summary.read_text(encoding="utf-8")


def test_missing_7zip_fails_because_the_gpl_check_cannot_run(tmp_path, monkeypatch, capsys) -> None:
    monkeypatch.setattr(artifact_report.shutil, "which", lambda name: None)
    monkeypatch.setenv("PROGRAMFILES", str(tmp_path))
    package = tmp_path / "QGroundControl-installer-AMD64.exe"
    package.write_bytes(b"MZ")

    assert _main(package, artifact="QGroundControl-installer-AMD64") == 1

    assert (
        "::error title=Artifact report::Cannot list QGroundControl-installer-AMD64.exe, "
        "so its GPL check did not run: 7-Zip (7z) is not installed"
    ) in capsys.readouterr().out


class FakeDmgTools:
    """hdiutil that mounts a fake image's files at the requested mount point."""

    def __init__(self, attach_rc: int = 0, detach_rcs: tuple[int, ...] = (0,)) -> None:
        self.attach_rc = attach_rc
        self.detach_rcs = list(detach_rcs)
        self.commands: list[list[str]] = []

    def __call__(self, cmd, **kwargs):
        self.commands.append(cmd)
        if cmd[1] == "attach":
            if self.attach_rc == 0:
                mount = Path(cmd[cmd.index("-mountpoint") + 1])
                binary = mount / "QGroundControl.app" / "Contents" / "MacOS" / "QGroundControl"
                binary.parent.mkdir(parents=True)
                binary.write_bytes(b"q" * 300)
                plugin = (
                    mount / "QGroundControl.app" / "Contents" / "Frameworks" / "libgstx265.dylib"
                )
                plugin.parent.mkdir(parents=True)
                plugin.write_bytes(b"x" * 20)
                if sys.platform != "win32":
                    os.symlink("/Applications", mount / "Applications")
            return subprocess.CompletedProcess(cmd, self.attach_rc, stdout="", stderr="no image")
        return subprocess.CompletedProcess(cmd, self.detach_rcs.pop(0), stdout="", stderr="busy")


def test_dmg_contents_are_listed_from_the_mounted_image(tmp_path, monkeypatch, capsys) -> None:
    tools = FakeDmgTools(detach_rcs=(1, 0))
    monkeypatch.setattr(artifact_report, "run_captured", tools)
    package = tmp_path / "QGroundControl.dmg"
    package.write_bytes(b"dmg")

    listing = artifact_report.list_package(package, tmp_path / "work")

    assert listing == Listing(
        size=3,
        files={
            "QGroundControl.app/Contents/MacOS/QGroundControl": 300,
            "QGroundControl.app/Contents/Frameworks/libgstx265.dylib": 20,
        },
    )
    mount = str(tmp_path / "work" / "mount")
    assert tools.commands == [
        ["hdiutil", "attach", "-nobrowse", "-readonly", "-mountpoint", mount, str(package)],
        ["hdiutil", "detach", mount],
        ["hdiutil", "detach", "-force", mount],
    ]
    assert "::warning" not in capsys.readouterr().out


def test_dmg_attach_failure_fails_without_detaching(tmp_path, monkeypatch, capsys) -> None:
    tools = FakeDmgTools(attach_rc=1)
    monkeypatch.setattr(artifact_report, "run_captured", tools)
    package = tmp_path / "QGroundControl.dmg"
    package.write_bytes(b"dmg")

    assert _main(package, artifact="QGroundControl") == 1

    assert [cmd[1] for cmd in tools.commands] == ["attach"]
    assert "hdiutil failed with exit code 1: no image" in capsys.readouterr().out


def test_appimages_are_listed_from_their_extracted_squashfs(tmp_path, monkeypatch) -> None:
    calls = []

    def run_captured(cmd, *, cwd, **kwargs):
        calls.append((cmd, cwd))
        root = Path(cwd) / "squashfs-root"
        (root / "usr" / "bin").mkdir(parents=True)
        (root / "usr" / "bin" / "QGroundControl").write_bytes(b"q" * 42)
        (root / "AppRun").write_bytes(b"#!/bin/sh\n")
        return subprocess.CompletedProcess(cmd, 0, stdout="", stderr="")

    monkeypatch.setattr(artifact_report, "run_captured", run_captured)
    package = tmp_path / "QGroundControl-x86_64.AppImage"
    package.write_bytes(b"\x7fELF")

    listing = artifact_report.list_package(package, tmp_path / "work")

    assert listing.files == {"usr/bin/QGroundControl": 42, "AppRun": 10}
    assert calls == [([str(package.resolve()), "--appimage-extract"], tmp_path / "work")]
    assert not (tmp_path / "work" / "squashfs-root").exists()
    if sys.platform != "win32":
        assert os.access(package, os.X_OK)


def _gpl_appimage(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    """An AppImage whose extracted tree ships libavcodec's GPL x264 and x265."""

    def run_captured(cmd, *, cwd, **kwargs):
        lib = Path(cwd) / "squashfs-root" / "usr" / "lib"
        lib.mkdir(parents=True)
        for name in ("libavcodec.so.60", "libx264.so.164", "libx265.so.199"):
            (lib / name).write_bytes(b"\x7fELF")
        return subprocess.CompletedProcess(cmd, 0, stdout="", stderr="")

    monkeypatch.setattr(artifact_report, "run_captured", run_captured)
    package = tmp_path / "QGroundControl-x86_64.AppImage"
    package.write_bytes(b"\x7fELF")
    return package


def test_warn_mode_reports_appimage_gpl_findings_as_a_known_issue(
    tmp_path, monkeypatch, gh_summary, capsys
) -> None:
    package = _gpl_appimage(tmp_path, monkeypatch)
    monkeypatch.setenv("INPUT_GPL_FINDINGS", "warn")

    assert _main(package, artifact="QGroundControl-x86_64") == 0

    out = capsys.readouterr().out
    assert "::error" not in out
    warnings = [line for line in out.splitlines() if line.startswith("::warning")]
    assert warnings == [
        "::warning title=GPL check (known issue)::QGroundControl-x86_64.AppImage ships "
        f"GPL-licensed code: usr/lib/{name} (named after GPL {gpl}). Known issue: Ubuntu's "
        "libavcodec, which the bundled gst-libav plugin (libgstlibav.so) loads, links the GPL x264 "
        "and x265 encoders, so the AppImage's dependency deployment bundles libx264 and libx265"
        for name, gpl in (("libx264.so.164", "x264"), ("libx265.so.199", "x265"))
    ]
    summary = gh_summary.read_text(encoding="utf-8")
    assert "| Result | passed with 2 GPL findings (known issue, warnings only) |" in summary
    assert (
        "**GPL-licensed files: known issue, reported as warnings** (QGC's GStreamer notice covers "
        "LGPL only). Ubuntu's `libavcodec`, which the bundled gst-libav plugin (`libgstlibav.so`) "
        "loads, links the GPL x264 and x265 encoders"
    ) in summary
    assert "| `usr/lib/libx265.so.199` | named after GPL `x265` |" in summary
    assert summary.index("GPL-licensed files") < summary.index("| Size | This build |")


def test_fail_mode_is_the_default_for_every_package_type(tmp_path, monkeypatch, capsys) -> None:
    package = _gpl_appimage(tmp_path, monkeypatch)

    assert _main(package, artifact="QGroundControl-x86_64") == 1
    assert _main(package, "--gpl-findings", "fail", artifact="QGroundControl-x86_64") == 1

    out = capsys.readouterr().out
    assert out.count("::error title=GPL check::") == 4
    assert "Known issue" not in out
    assert "::warning" not in out


def test_warn_mode_without_a_known_issue_still_warns(tmp_path, gh_summary, capsys) -> None:
    package = _apk(tmp_path / "QGroundControl.apk", {"lib/arm64-v8a/libgstx264.so": ELF})

    assert _main(package, "--gpl-findings", "warn") == 0

    assert (
        "::warning title=GPL check (known issue)::QGroundControl.apk ships GPL-licensed code: "
        "lib/arm64-v8a/libgstx264.so (named after GPL x264), reported as a warning"
    ) in capsys.readouterr().out
    summary = gh_summary.read_text(encoding="utf-8")
    assert "reported as warnings** (QGC's GStreamer notice covers LGPL only)\n\n" in summary


def test_rejects_an_unknown_gpl_findings_mode(tmp_path, monkeypatch, capsys) -> None:
    monkeypatch.setenv("INPUT_GPL_FINDINGS", "ignore")
    with pytest.raises(SystemExit) as raised:
        _main(_apk(tmp_path / "QGroundControl.apk"))
    assert raised.value.code == 2
    assert "must be one of fail, warn: ignore" in capsys.readouterr().err


def test_unsupported_or_missing_packages_fail(tmp_path, capsys) -> None:
    (tmp_path / "QGroundControl.tar").write_bytes(b"tar")
    assert _main(tmp_path / "QGroundControl.tar") == 1
    assert _main(tmp_path / "missing.apk") == 1
    out = capsys.readouterr().out
    assert out.count("::error title=Artifact report::Package not found or unsupported") == 2


def test_corrupt_zip_packages_fail(tmp_path, capsys) -> None:
    package = tmp_path / "QGroundControl.apk"
    package.write_bytes(b"not a zip")
    assert _main(package) == 1
    assert "Cannot list QGroundControl.apk" in capsys.readouterr().out


def test_requires_the_package_and_artifact_name(capsys) -> None:
    with pytest.raises(SystemExit) as raised:
        artifact_report.main([])
    assert raised.value.code == 2
    assert "--package and --artifact-name" in capsys.readouterr().err


def test_marker_scan_reads_streams_in_overlapping_chunks(monkeypatch) -> None:
    monkeypatch.setattr(artifact_report, "SCAN_CHUNK_BYTES", 5)
    stream = io.BytesIO(b"....x265_encoder_open....")
    assert artifact_report._find_markers(stream, b"\x7fELF") == [b"x265_encoder_open"]
