from __future__ import annotations

import json
import subprocess
from unittest.mock import patch

from android_collect_diagnostics import (
    collect_adb_diagnostics,
    collect_avd_files,
    collect_build_artifacts,
    emulator_online,
    extract_gstreamer_diagnostics,
    main,
)


class TestEmulatorOnline:
    def test_detects_online_emulator(self):
        out = "List of devices attached\nemulator-5554\tdevice\n"
        assert emulator_online(out) is True

    def test_offline_emulator_not_detected(self):
        out = "List of devices attached\nemulator-5554\toffline\n"
        assert emulator_online(out) is False

    def test_empty_output_not_detected(self):
        assert emulator_online("List of devices attached\n") is False


class TestCollectBuildArtifacts:
    def test_copies_present_files_and_skips_missing(self, tmp_path):
        build = tmp_path / "build"
        out = tmp_path / "out"
        build.mkdir()
        out.mkdir()
        (build / "qgc-build.log").write_text("build output", encoding="utf-8")
        (build / "android-QGroundControl-deployment-settings.json").write_text(
            '{"k": "v"}', encoding="utf-8")

        collect_build_artifacts(out, build, boot_log=None)

        assert (out / "qgc-build.log").read_text(encoding="utf-8") == "build output"
        assert not (out / "qgc-build-retry.log").exists()
        pretty = out / "android-QGroundControl-deployment-settings.pretty.json"
        assert pretty.exists()
        assert json.loads(pretty.read_text(encoding="utf-8")) == {"k": "v"}

    def test_pretty_print_failure_writes_error_file(self, tmp_path):
        build = tmp_path / "build"
        out = tmp_path / "out"
        build.mkdir()
        out.mkdir()
        (build / "android-QGroundControl-deployment-settings.json").write_text(
            "{not valid json}", encoding="utf-8")

        collect_build_artifacts(out, build, boot_log=None)

        assert (out / "android-QGroundControl-deployment-settings.error.txt").exists()
        assert not (out / "android-QGroundControl-deployment-settings.pretty.json").exists()

    def test_boot_log_is_copied_when_present(self, tmp_path):
        build = tmp_path / "build"
        out = tmp_path / "out"
        build.mkdir()
        out.mkdir()
        boot = tmp_path / "qgc_emulator_boot.log"
        boot.write_text("boot output", encoding="utf-8")

        collect_build_artifacts(out, build, boot_log=boot)

        assert (out / "qgc_emulator_boot.log").read_text(encoding="utf-8") == "boot output"


class TestExtractGstreamerDiagnostics:
    def test_filters_gst_and_load_error_lines(self, tmp_path):
        out = tmp_path
        logcat = tmp_path / "logcat.txt"
        logcat.write_text(
            "I activity unrelated line\n"
            "E gstreamer plugin load failure\n"
            "E dlopen failed for libgstcoreelements.so\n"
            "I irrelevant\n",
            encoding="utf-8",
        )

        extract_gstreamer_diagnostics(logcat, out)

        gst_lines = (out / "gstreamer-logcat.txt").read_text(encoding="utf-8")
        load_errors = (out / "gstreamer-load-errors.txt").read_text(encoding="utf-8")
        assert "gstreamer plugin load failure" in gst_lines
        assert "dlopen failed for libgstcoreelements.so" in gst_lines
        assert "unrelated line" not in gst_lines
        assert "dlopen failed for libgstcoreelements.so" in load_errors
        assert "gstreamer plugin load failure" not in load_errors

    def test_missing_logcat_is_noop(self, tmp_path):
        extract_gstreamer_diagnostics(tmp_path / "missing.txt", tmp_path)
        assert not (tmp_path / "gstreamer-logcat.txt").exists()


class TestCollectAvdFiles:
    def test_copies_log_and_ini_preserving_structure(self, tmp_path):
        avd_home = tmp_path / ".android" / "avd"
        (avd_home / "qgc-ci.avd").mkdir(parents=True)
        (avd_home / "qgc-ci.avd" / "config.ini").write_text("ini", encoding="utf-8")
        (avd_home / "qgc-ci.avd" / "emulator-output.log").write_text("log", encoding="utf-8")
        (avd_home / "qgc-ci.avd" / "userdata.img").write_bytes(b"binary")  # not copied
        out = tmp_path / "out"
        out.mkdir()

        collect_avd_files(out, avd_home)

        assert (out / "avd" / "qgc-ci.avd" / "config.ini").exists()
        assert (out / "avd" / "qgc-ci.avd" / "emulator-output.log").exists()
        assert not (out / "avd" / "qgc-ci.avd" / "userdata.img").exists()

    def test_missing_avd_home_is_noop(self, tmp_path):
        collect_avd_files(tmp_path / "out", tmp_path / "no-avd")


class TestCollectAdbDiagnostics:
    def test_no_adb_writes_skip_marker(self, tmp_path):
        with patch("android_collect_diagnostics.shutil.which", return_value=None):
            collect_adb_diagnostics(tmp_path)
        assert "adb not found" in (tmp_path / "adb-skipped.txt").read_text(encoding="utf-8")

    def test_offline_emulator_writes_skip_marker(self, tmp_path):
        offline = subprocess.CompletedProcess(
            [], 0, stdout="List of devices attached\nemulator-5554\toffline\n", stderr="")
        with patch("android_collect_diagnostics.shutil.which", return_value="/usr/bin/adb"), \
             patch("android_collect_diagnostics._run", return_value=offline):
            collect_adb_diagnostics(tmp_path)
        assert "No online emulator" in (tmp_path / "adb-skipped.txt").read_text(encoding="utf-8")


class TestMain:
    def test_main_succeeds_with_missing_build_artifacts(self, tmp_path):
        with patch("android_collect_diagnostics.shutil.which", return_value=None):
            rc = main([
                "--out-dir", str(tmp_path / "out"),
                "--build-dir", str(tmp_path / "build"),
                "--avd-home", str(tmp_path / "avd"),
            ])
        assert rc == 0
        assert (tmp_path / "out").is_dir()
