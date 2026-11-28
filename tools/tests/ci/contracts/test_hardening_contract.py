"""Contracts for architecture-sensitive compiler hardening."""

from __future__ import annotations

import subprocess
from itertools import pairwise

import pytest

from ..._helpers import REPO_ROOT


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


def _hardening_flags(
    tmp_path, *, apple: bool, system_processor: str, osx_architectures: str
) -> list[str]:
    """Return the compile flags qgc_apply_global_hardening() applies when the compiler takes every flag."""
    script = tmp_path / "hardening-flags-contract.cmake"
    flags_path = tmp_path / "hardening-flags.txt"
    script.write_text(
        f"set(APPLE {'TRUE' if apple else 'FALSE'})\n"
        "set(MSVC FALSE)\n"
        f'set(CMAKE_CXX_COMPILER_ID "{"AppleClang" if apple else "Clang"}")\n'
        f'set(CMAKE_SYSTEM_PROCESSOR "{system_processor}")\n'
        f'set(CMAKE_OSX_ARCHITECTURES "{osx_architectures}")\n'
        f'include("{REPO_ROOT}/cmake/modules/Hardening.cmake")\n'
        "set(QGC_ENABLE_HARDENING ON)\n"
        "set(QGC_REPRODUCIBLE_BUILD OFF)\n"
        "function(check_compiler_flag language flag out_var)\n"
        "  set(${out_var} TRUE PARENT_SCOPE)\n"
        "endfunction()\n"
        "function(check_linker_flag language flag out_var)\n"
        "  set(${out_var} TRUE PARENT_SCOPE)\n"
        "endfunction()\n"
        "function(add_compile_options)\n"
        "endfunction()\n"
        "function(add_link_options)\n"
        "endfunction()\n"
        "qgc_apply_global_hardening()\n"
        f'file(WRITE "{flags_path}" "${{QGC_HARDENING_CXX_FLAGS}}")\n',
        encoding="utf-8",
    )

    subprocess.run(["cmake", "-P", str(script)], check=True, capture_output=True, text=True)
    return flags_path.read_text(encoding="utf-8").split(";")


def test_universal_apple_hardening_scopes_architecture_flags(tmp_path) -> None:
    flags = _hardening_flags(
        tmp_path, apple=True, system_processor="arm64", osx_architectures="x86_64h;arm64"
    )
    pairs = set(pairwise(flags))
    assert ("-Xarch_x86_64h", "-fcf-protection=full") in pairs
    assert ("-Xarch_arm64", "-mbranch-protection=bti") in pairs


@pytest.mark.parametrize(
    ("apple", "osx_architectures", "expected"),
    [
        # pac-ret, which "standard" includes, keeps an exception from being caught on macOS arm64.
        (True, "arm64", "-mbranch-protection=bti"),
        (True, "x86_64h;arm64", "-mbranch-protection=bti"),
        (False, "", "-mbranch-protection=standard"),
    ],
)
def test_only_apple_arm64_leaves_out_return_address_signing(
    tmp_path, apple: bool, osx_architectures: str, expected: str
) -> None:
    flags = _hardening_flags(
        tmp_path, apple=apple, system_processor="arm64", osx_architectures=osx_architectures
    )
    assert [flag for flag in flags if flag.startswith("-mbranch-protection")] == [expected]
