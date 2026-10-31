"""android.yml checks every leg's APK and launches the emulator leg's APK with QGC's boot test."""

from __future__ import annotations

import re

from ..._helpers import REPO_ROOT, get_step, get_steps, load_workflow


def _build_steps() -> list[dict]:
    return get_steps(load_workflow("android.yml"), "build")


def test_every_leg_checks_its_apk_once_it_is_prepared() -> None:
    steps = _build_steps()
    prepare = get_step(steps, "Prepare Artifact")
    check = get_step(steps, "Check APK native libraries")
    assert steps.index(prepare) + 1 == steps.index(check)
    assert "if" not in check
    assert check["shell"] == "bash"
    assert check["run"] == (
        'python3 "${GITHUB_WORKSPACE}/tools/ci/android/android_apk_check.py" \\\n'
        '  --apk "${RUNNER_TEMP}/build/${PACKAGE}.apk" \\\n'
        '  --build-dir "${RUNNER_TEMP}/build"\n'
    )
    # Nothing uploads, attests or boot-tests the APK before the check.
    for name in ("Emulator Boot Test", "Attest and Upload", "Deploy to Play Store"):
        assert steps.index(check) < steps.index(get_step(steps, name))


def test_only_the_emulator_leg_launches_qgc_with_the_boot_test() -> None:
    configure = get_step(_build_steps(), "Configure")
    assert configure["with"]["extra-args"].endswith(
        " ${{ matrix.emulator && format('-C {0}/.github/actions/android-emulator-test/"
        "boot-test.cmake', github.workspace) || '' }}"
    )
    # The boot test passes only on the completion marker, which QGC logs in the Main category.
    cache = (REPO_ROOT / ".github/actions/android-emulator-test/boot-test.cmake").read_text()
    match = re.search(r'set\(QT_ANDROID_APPLICATION_ARGUMENTS\s+"([^"]*)"\s+CACHE STRING', cache)
    assert match is not None
    arguments = match.group(1).split()
    assert arguments[0] == "--simple-boot-test"
    assert arguments[1].startswith("--logging:Main,")
