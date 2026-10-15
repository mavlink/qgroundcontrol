"""Exercise profile selection and IPA validation without Apple tools."""

import json
import plistlib
import shlex
import subprocess
from pathlib import Path

import ios_package
import pytest


@pytest.fixture
def app(tmp_path, monkeypatch):
    app = tmp_path / "QGroundControl.app"
    app.mkdir()
    for relative in ("Assets.car", "QGroundControl", "embedded.mobileprovision"):
        (app / relative).touch()
    for relative in ("QGCLaunchScreen.storyboardc", "Frameworks/gstreamer_mobile.framework"):
        (app / relative).mkdir(parents=True)
    (app / "Info.plist").write_bytes(plistlib.dumps({"CFBundleIdentifier": "org.test.qgc"}))
    monkeypatch.setenv("APPSTORE_BUNDLE_ID", "org.test.qgc")
    monkeypatch.setenv("GITHUB_REF_TYPE", "tag")
    monkeypatch.setenv("GITHUB_REF_NAME", "v1.0")
    return app


@pytest.mark.parametrize(
    "profiles", [[], [{"name": "selected"}], [{"name": "selected", "udid": "id"}] * 2]
)
def test_profile_requires_one_match_with_uuid(monkeypatch, profiles):
    for key, value in {
        "TEAM_ID": "team",
        "PROFILE_NAME": "selected",
        "BUNDLE_ID": "org.test",
        "PROFILES": json.dumps(profiles),
    }.items():
        monkeypatch.setenv(key, value)
    with pytest.raises(ValueError):
        ios_package.select_profile()


def test_profile_preserves_argument_boundaries(monkeypatch):
    for key, value in {
        "TEAM_ID": "a team",
        "PROFILE_NAME": "selected",
        "BUNDLE_ID": "org.test",
        "PROFILES": '[{"name":"selected","udid":"a uuid"}]',
    }.items():
        monkeypatch.setenv(key, value)
    outputs = {}
    monkeypatch.setattr(ios_package, "write_github_output", outputs.update)
    ios_package.select_profile()
    assert "-DQGC_IOS_PROVISIONING_PROFILE=a uuid" in shlex.split(outputs["cmake_args"])


@pytest.mark.parametrize("failure", ["codesign", "unzip", "bundle-id", "profile", "assets"])
def test_failed_package_never_replaces_previous_artifact(app, monkeypatch, failure):
    output = app.with_suffix(".ipa")
    output.write_bytes(b"previous archive")
    calls = []

    def run(command, **kwargs):
        calls.append(command)
        if command[0] == failure:
            raise subprocess.CalledProcessError(1, command)
        if command[:2] == ["ditto", "-c"]:
            Path(command[-1]).write_bytes(b"new archive")
        return _completed(command)

    monkeypatch.setattr(ios_package.subprocess, "run", run)
    if failure == "bundle-id":
        monkeypatch.setenv("APPSTORE_BUNDLE_ID", "different.id")
    elif failure == "profile":
        (app / "embedded.mobileprovision").unlink()
    elif failure == "assets":
        (app / "Assets.car").unlink()
    with pytest.raises((ValueError, subprocess.CalledProcessError)):
        ios_package.package(app)
    assert output.read_bytes() == b"previous archive"
    assert not list(app.parent.glob(".qgc-ipa-*"))
    if failure in ("bundle-id", "profile", "assets"):
        assert not any(call[0] == "ditto" for call in calls)


def test_successful_signed_package_preserves_ditto_contract(app, monkeypatch):
    calls = []

    def run(command, **kwargs):
        calls.append(command)
        if command[:2] == ["ditto", "-c"]:
            Path(command[-1]).write_bytes(b"validated archive")
        return _completed(command)

    monkeypatch.setattr(ios_package.subprocess, "run", run)
    output = ios_package.package(app)
    assert output.read_bytes() == b"validated archive"
    assert calls[0][-2:] == ["-verify_arch", "arm64"]
    assert calls[1][:2] == ["vtool", "-show-build"]
    assert calls[2][:5] == ["codesign", "--verify", "--deep", "--strict", "--verbose=2"]
    assert calls[-2][:6] == [
        "ditto",
        "-c",
        "-k",
        "--sequesterRsrc",
        "--keepParent",
        str(Path(calls[-2][-1]).parent / "Payload"),
    ]
    assert calls[-1][0] == "unzip"


def _fake_tools(monkeypatch, vtool_output):
    def run(command, **kwargs):
        stdout = vtool_output if command[0] == "vtool" else ""
        return subprocess.CompletedProcess(command, 0, stdout=stdout)

    monkeypatch.setattr(ios_package.subprocess, "run", run)


def _build_version(platform):
    return (
        "QGroundControl (architecture arm64):\n"
        "Load command 9\n"
        "      cmd LC_BUILD_VERSION\n"
        "  cmdsize 32\n"
        f" platform {platform}\n"
        "    minos 15.0\n"
    )


def _completed(command, platform="IOS"):
    stdout = _build_version(platform) if command[0] == "vtool" else ""
    return subprocess.CompletedProcess(command, 0, stdout=stdout)


def test_package_rejects_simulator_binary(app, monkeypatch):
    output = app.with_suffix(".ipa")
    output.write_bytes(b"previous archive")
    calls = []

    def run(command, **kwargs):
        calls.append(command)
        return _completed(command, platform="IOSSIMULATOR")

    monkeypatch.setattr(ios_package.subprocess, "run", run)
    with pytest.raises(ValueError, match=r"Expected Mach-O platform IOS,"):
        ios_package.package(app)
    assert output.read_bytes() == b"previous archive"
    assert not any(call[0] == "ditto" for call in calls)


def test_platform_check_accepts_matching_platform(app, monkeypatch):
    _fake_tools(monkeypatch, _build_version("IOSSIMULATOR"))
    ios_package.verify_bundle(app, "arm64", platform="IOSSIMULATOR")


@pytest.mark.parametrize("output", [_build_version("IOS"), "QGroundControl:\n"])
def test_platform_check_rejects_other_or_missing_platform(app, monkeypatch, output):
    _fake_tools(monkeypatch, output)
    with pytest.raises(ValueError, match="IOSSIMULATOR"):
        ios_package.verify_bundle(app, "arm64", platform="IOSSIMULATOR")
