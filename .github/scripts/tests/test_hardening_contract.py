"""Contracts for architecture-sensitive compiler hardening."""

from __future__ import annotations

import subprocess

import pytest
from _helpers import REPO_ROOT


@pytest.mark.parametrize(
    ("system_processor", "osx_architectures", "aarch64_expected", "x86_expected"),
    [
        ("arm64", "", "TRUE", "FALSE"),
        ("aarch64", "", "TRUE", "FALSE"),
        ("arm64", "arm64", "TRUE", "FALSE"),
        ("arm64", "x86_64h;arm64", "FALSE", "FALSE"),
        ("x86_64", "", "FALSE", "TRUE"),
        ("x86_64", "x86_64h", "FALSE", "TRUE"),
        ("armv7", "", "FALSE", "FALSE"),
    ],
)
def test_architecture_hardening_requires_every_output_architecture(
    tmp_path,
    system_processor: str,
    osx_architectures: str,
    aarch64_expected: str,
    x86_expected: str,
) -> None:
    script = tmp_path / "hardening-contract.cmake"
    script.write_text(
        f'set(CMAKE_SYSTEM_PROCESSOR "{system_processor}")\n'
        f'set(CMAKE_OSX_ARCHITECTURES "{osx_architectures}")\n'
        f'include("{REPO_ROOT}/cmake/modules/Hardening.cmake")\n'
        '_qgc_target_architectures_match("^(aarch64|arm64)$" actual_aarch64)\n'
        '_qgc_target_architectures_match("^(x86_64h|x86_64|AMD64|i[0-9]86|x86)$" actual_x86)\n'
        f'if(NOT actual_aarch64 STREQUAL "{aarch64_expected}")\n'
        f'  message(FATAL_ERROR "expected aarch64={aarch64_expected}, got ${{actual_aarch64}}")\n'
        "endif()\n"
        f'if(NOT actual_x86 STREQUAL "{x86_expected}")\n'
        f'  message(FATAL_ERROR "expected x86={x86_expected}, got ${{actual_x86}}")\n'
        "endif()\n",
        encoding="utf-8",
    )

    subprocess.run(["cmake", "-P", str(script)], check=True, capture_output=True, text=True)
