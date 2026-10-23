"""Tests for tools/setup/build_android_openssl.py and its CMake driver, AndroidOpenSSL.cmake."""

from __future__ import annotations

import io
import os
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path

import pytest
from setup import build_android_openssl as mod

from ._helpers import REPO_ROOT

VERSION = "3.5.6"


def _args(tmp_path: Path, *extra: str) -> list[str]:
    tarball, patch, ndk = tmp_path / "openssl.tar.gz", tmp_path / "ssl_3.patch", tmp_path / "ndk"
    return [
        "--version",
        VERSION,
        "--tarball",
        str(tarball),
        "--ndk",
        str(ndk),
        "--patch",
        str(patch),
        "--output-root",
        str(tmp_path / "out"),
        "--build-dir",
        str(tmp_path / "build"),
        *extra,
    ]


def _write_inputs(tmp_path: Path) -> None:
    """A source tarball with a header and a header template, an empty patch, and a Linux NDK."""
    with tarfile.open(tmp_path / "openssl.tar.gz", "w:gz") as tar:
        header = b"#define OPENSSL_VERSION_TEXT\n"
        for name, data in (
            (f"openssl-{VERSION}/include/openssl/opensslv.h", header),
            (f"openssl-{VERSION}/include/openssl/configuration.h.in", b""),
        ):
            info = tarfile.TarInfo(name)
            info.size = len(data)
            tar.addfile(info, io.BytesIO(data))
    (tmp_path / "ssl_3.patch").write_bytes(b"")
    (tmp_path / "ndk/toolchains/llvm/prebuilt/linux-x86_64/bin").mkdir(parents=True)


def test_qt_abi_names_arm_abis_like_qt() -> None:
    assert [mod.qt_abi(a) for a in ("arm64", "arm", "x86_64", "x86")] == [
        "arm64-v8a",
        "armeabi-v7a",
        "x86_64",
        "x86",
    ]


def test_find_toolchain_returns_the_first_prebuilt_host(tmp_path: Path) -> None:
    for host in ("darwin-arm64", "darwin-x86_64"):
        (tmp_path / "toolchains/llvm/prebuilt" / host / "bin").mkdir(parents=True)
    assert mod.find_toolchain(tmp_path) == tmp_path / "toolchains/llvm/prebuilt/darwin-x86_64/bin"
    with pytest.raises(FileNotFoundError, match="No LLVM toolchain"):
        mod.find_toolchain(tmp_path / "empty")


def test_run_reports_the_output_of_a_failed_command(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    mod.run([sys.executable, "-c", "print('ok')"], cwd=tmp_path, env=dict(os.environ))
    with pytest.raises(subprocess.CalledProcessError) as failure:
        mod.run(
            [sys.executable, "-c", "print('boom'); raise SystemExit(3)"],
            cwd=tmp_path,
            env=dict(os.environ),
        )
    assert failure.value.returncode == 3
    assert "boom" in capsys.readouterr().err


@pytest.mark.parametrize(
    ("arch", "build_type", "expected"),
    [
        (
            "arm64",
            "",
            [
                "shared",
                "android-arm64",
                "-U__ANDROID_API__",
                "-D__ANDROID_API__=28",
                "-Wl,-z,max-page-size=16384",
                "-Wl,-z,common-page-size=16384",
            ],
        ),
        (
            "x86",
            "no-asm",
            ["no-asm", "shared", "android-x86", "-U__ANDROID_API__", "-D__ANDROID_API__=28"],
        ),
    ],
)
def test_configure_params_page_aligns_only_64_bit_abis_that_need_it(
    arch: str, build_type: str, expected: list[str]
) -> None:
    assert mod.configure_params(arch, build_type, 28) == expected


def test_stage_output_replaces_the_abi_dir_and_links_unversioned_names(tmp_path: Path) -> None:
    src, out = tmp_path / "src", tmp_path / "out"
    src.mkdir()
    for name in ("libcrypto_3.so", "libssl_3.so", "libcrypto.a", "libssl.a", "libother.so"):
        (src / name).write_text(name, encoding="utf-8")
    out.mkdir()
    (out / "stale.so").touch()

    mod.stage_output(src, out)

    assert sorted(p.name for p in out.iterdir()) == [
        "libcrypto.a",
        "libcrypto.so",
        "libcrypto_3.so",
        "libssl.a",
        "libssl.so",
        "libssl_3.so",
    ]
    assert (out / "libssl.so").readlink() == Path("libssl_3.so")
    assert (out / "libcrypto.so").readlink() == Path("libcrypto_3.so")


def test_copy_headers_strips_templates_and_keeps_an_existing_copy(tmp_path: Path) -> None:
    src, root = tmp_path / "src", tmp_path / "ssl_3"
    (src / "include/openssl").mkdir(parents=True)
    for name in ("ssl.h", "configuration.h.in", "exports.def"):
        (src / "include/openssl" / name).touch()

    mod.copy_headers(src, root)
    (src / "include/openssl/new.h").touch()
    mod.copy_headers(src, root)

    assert sorted(p.name for p in (root / "include/openssl").iterdir()) == ["ssl.h"]


@pytest.mark.parametrize(
    ("extra", "variants"),
    [([], ["asm", "no-asm"]), (["--variants", "no-asm"], ["no-asm"])],
)
def test_parse_args_variants(tmp_path: Path, extra: list[str], variants: list[str]) -> None:
    assert mod.parse_args(_args(tmp_path, *extra)).variants == variants


def test_parse_args_rejects_unknown_variants(tmp_path: Path) -> None:
    with pytest.raises(SystemExit):
        mod.parse_args(_args(tmp_path, "--variants", "fast"))


@pytest.mark.parametrize("missing", ["openssl.tar.gz", "ssl_3.patch", "ndk"])
def test_main_rejects_missing_inputs(tmp_path: Path, missing: str) -> None:
    _write_inputs(tmp_path)
    path = tmp_path / missing
    if path.is_dir():
        shutil.rmtree(path)
    else:
        path.unlink()
    assert mod.main(_args(tmp_path)) == 1
    assert not (tmp_path / "build").exists()


def test_main_builds_only_the_requested_variants_per_arch(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _write_inputs(tmp_path)
    (tmp_path / "build").mkdir()
    (tmp_path / "build/stale").touch()
    builds: list[tuple[str, str]] = []
    monkeypatch.setattr(
        mod, "build_one", lambda cfg, build_type, arch, env: builds.append((build_type, arch))
    )

    assert mod.main(_args(tmp_path, "--archs", "arm64", "x86", "--variants", "no-asm")) == 0

    assert builds == [("no-asm", "arm64"), ("no-asm", "x86")]
    assert list((tmp_path / "build").iterdir()) == []


@pytest.mark.parametrize(("build_type", "root"), [("", "out"), ("no-asm", "out/no-asm")])
def test_build_one_stages_libraries_and_headers_into_the_variant_root(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, build_type: str, root: str
) -> None:
    _write_inputs(tmp_path)
    cfg = mod.parse_args(_args(tmp_path, "--api", "28"))
    cfg.build_dir.mkdir()
    commands: list[list[str]] = []

    def fake_run(cmd: list[str], cwd: Path, env: dict[str, str], stdin: int | None = None) -> None:
        commands.append(cmd)
        if cmd[-1] == "build_libs":
            for name in ("libcrypto_3.so", "libssl_3.so"):
                (cwd / name).touch()

    monkeypatch.setattr(mod, "run", fake_run)

    mod.build_one(cfg, build_type, "x86", {})

    assert [c[0] for c in commands] == ["patch", "./Configure", "make", "make", "llvm-strip"]
    assert commands[1][1:] == mod.configure_params("x86", build_type, 28)
    ssl_root = tmp_path / root / "ssl_3"
    assert (ssl_root / "x86/libssl_3.so").is_file()
    assert (ssl_root / "x86/libssl.so").is_symlink()
    assert sorted(p.name for p in (ssl_root / "include/openssl").iterdir()) == ["opensslv.h"]
    assert list(cfg.build_dir.iterdir()) == []


# AndroidOpenSSL.cmake run in script mode with its downloads, CPM, and the build stubbed out.
_CMAKE_STUBS = """\
function(qgc_resilient_download)
    cmake_parse_arguments(ARG "" "FILENAME;DESTINATION_DIR;RESULT_VAR" "" ${ARGN})
    set(${ARG_RESULT_VAR} "${ARG_DESTINATION_DIR}/${ARG_FILENAME}" PARENT_SCOPE)
endfunction()
macro(CPMAddPackage)
endmacro()
macro(qgc_require_cpm_added)
endmacro()
function(execute_process)
    cmake_parse_arguments(ARG "" "RESULT_VARIABLE" "COMMAND" ${ARGN})
    list(JOIN ARG_COMMAND " " command)
    file(APPEND "${QGC_TEST_LOG}" "${command}\\n")
    set(${ARG_RESULT_VARIABLE} 0 PARENT_SCOPE)
endfunction()
"""


def _configure_openssl(
    tmp_path: Path, build_type: str, abis: str = "arm64-v8a;x86_64"
) -> list[str]:
    """Run AndroidOpenSSL.cmake; return the build commands it would run."""
    stubs = tmp_path / "stubs"
    stubs.mkdir(exist_ok=True)
    (stubs / "Download.cmake").write_text(_CMAKE_STUBS, encoding="utf-8")
    source = tmp_path / "android_openssl"
    source.mkdir(exist_ok=True)
    (source / "android_openssl.cmake").write_text(
        "function(add_android_openssl_libraries)\nendfunction()\n", encoding="utf-8"
    )
    log = tmp_path / "commands.log"
    log.write_text("", encoding="utf-8")
    module = REPO_ROOT / "cmake/modules/AndroidOpenSSL.cmake"
    subprocess.run(
        [
            "cmake",
            f"-DCMAKE_MODULE_PATH={stubs.as_posix()}",
            f"-DCMAKE_BUILD_TYPE={build_type}",
            f"-DQT_ANDROID_ABIS={abis}",
            f"-Dandroid_openssl_SOURCE_DIR={source.as_posix()}",
            f"-DQGC_TEST_LOG={log.as_posix()}",
            "-DPython3_EXECUTABLE=python3",
            "-DQGC_QT_ANDROID_MIN_SDK_VERSION=28",
            "-P",
            str(module),
        ],
        cwd=tmp_path,
        check=True,
        capture_output=True,
        text=True,
    )
    return log.read_text(encoding="utf-8").splitlines()


def _built(source: Path, variant_root: str, abis: tuple[str, ...]) -> None:
    root = source / variant_root
    for abi in abis:
        (root / "ssl_3" / abi).mkdir(parents=True, exist_ok=True)
        (root / "ssl_3" / abi / "libssl_3.so").touch()
    (root / ".qgc_openssl_version").write_text(f"{VERSION}\n", encoding="utf-8")


@pytest.mark.parametrize(
    ("build_type", "variant", "stamp"),
    [
        ("Debug", "no-asm", "no-asm/.qgc_openssl_version"),
        ("Release", "asm", ".qgc_openssl_version"),
        ("RelWithDebInfo", "asm", ".qgc_openssl_version"),
    ],
)
def test_cmake_builds_only_the_variant_android_openssl_links(
    tmp_path: Path, build_type: str, variant: str, stamp: str
) -> None:
    (command,) = _configure_openssl(tmp_path, build_type)

    assert command.endswith(f"--archs arm64 x86_64 --variants {variant}")
    source = tmp_path / "android_openssl"
    assert (source / stamp).read_text(encoding="utf-8") == f"{VERSION}\n"
    assert [p.relative_to(source).as_posix() for p in source.rglob(".qgc_openssl_version")] == [
        stamp
    ]


def test_cmake_skips_a_variant_already_built_at_this_version(tmp_path: Path) -> None:
    source = tmp_path / "android_openssl"
    _built(source, ".", ("arm64-v8a", "x86_64"))

    assert _configure_openssl(tmp_path, "Release") == []
    (command,) = _configure_openssl(tmp_path, "Debug")
    assert command.endswith("--archs arm64 x86_64 --variants no-asm")


def test_cmake_rebuilds_only_the_missing_abis(tmp_path: Path) -> None:
    _built(tmp_path / "android_openssl", "no-asm", ("arm64-v8a",))

    (command,) = _configure_openssl(tmp_path, "Debug")

    assert command.endswith("--archs x86_64 --variants no-asm")
