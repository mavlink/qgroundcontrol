"""Tests for android_apk_check.py on synthetic APKs, NDK trees, and llvm-readelf output."""

from __future__ import annotations

import zipfile
from pathlib import Path
from typing import TYPE_CHECKING

import pytest
from ci.android import android_apk_check as mod

from .._helpers import completed

if TYPE_CHECKING:
    import subprocess

ELF_STUB = b"\x7fELF\x02\x01\x01"

# llvm-readelf 18 (NDK r27) output for --program-headers --dynamic-table --wide, trimmed.
READELF_64 = """\
Elf file type is DYN (Shared object file)
Entry point 0x0
There are 9 program headers, starting at offset 64

Program Headers:
  Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align
  PHDR           0x000040 0x0000000000000040 0x0000000000000040 0x0001f8 0x0001f8 R   0x8
  LOAD           0x000000 0x0000000000000000 0x0000000000000000 0x613bf0 0x613bf0 R E {align}
  LOAD           0x613bf0 0x0000000000617bf0 0x0000000000617bf0 0x015418 0x016410 RW  {align}
  DYNAMIC        0x621518 0x0000000000625518 0x0000000000625518 0x000200 0x000200 RW  0x8
  GNU_RELRO      0x613bf0 0x0000000000617bf0 0x0000000000617bf0 0x015418 0x016410 R   0x1
  GNU_STACK      0x000000 0x0000000000000000 0x0000000000000000 0x000000 0x000000 RW  0x0

 Section to Segment mapping:
  Segment Sections...
   00
   01     .dynsym .text
   02     .data.rel.ro .dynamic .got
Dynamic section at offset 0x621518 contains 4 entries:
  Tag                Type           Name/Value
{needed}  0x000000000000000e (SONAME)       Library soname: [{name}]
  0x000000000000001e (FLAGS)        BIND_NOW
"""

READELF_32 = """\
Program Headers:
  Type           Offset   VirtAddr   PhysAddr   FileSiz  MemSiz   Flg Align
  LOAD           0x000000 0x00000000 0x00000000 0x43ce50 0x43ce50 R E 0x1000
  LOAD           0x43ce50 0x0043de50 0x0043de50 0x0ab7c 0x0b1b0 RW  0x1000
Dynamic section at offset 0x43c000 contains 2 entries:
  Tag        Type                 Name/Value
  0x00000001 (NEEDED)             Shared library: [libc.so]
  0x00000001 (NEEDED)             Shared library: [libc++_shared.so]
"""

ZIPALIGN_USAGE = (
    "Usage: zipalign [-f] [-p] [-P <pagesize_kb>] [-v] [-z] <align> infile.zip outfile.zip\n"
)
ZIPALIGN_OLD_USAGE = "Usage: zipalign [-f] [-p] [-v] [-z] <align> infile.zip outfile.zip\n"


def readelf_output(name: str, needed: tuple[str, ...] = ("libc.so",), align: str = "0x4000") -> str:
    lines = "".join(
        f"  0x0000000000000001 (NEEDED)       Shared library: [{library}]\n" for library in needed
    )
    return READELF_64.format(align=align, needed=lines, name=name)


@pytest.fixture
def ndk(tmp_path: Path) -> Path:
    """An NDK whose sysroot has API 28 and 29 stubs for arm64-v8a and armeabi-v7a."""
    root = tmp_path / "ndk"
    prebuilt = root / "toolchains/llvm/prebuilt/linux-x86_64"
    (prebuilt / "bin").mkdir(parents=True)
    (prebuilt / "bin/llvm-readelf").write_bytes(b"")
    for triple in ("aarch64-linux-android", "arm-linux-androideabi"):
        for api, stubs in (("28", ("libc", "liblog", "libEGL")), ("29", ("libc", "libamidi"))):
            directory = prebuilt / "sysroot/usr/lib" / triple / api
            directory.mkdir(parents=True)
            for stub in stubs:
                (directory / f"{stub}.so").write_bytes(ELF_STUB)
            # A linker script, not a library a device provides.
            (directory / "libc++.so").write_text("INPUT(-lc++_static -lc++abi)\n")
    return root


@pytest.fixture
def sdk(tmp_path: Path) -> Path:
    root = tmp_path / "sdk"
    (root / "build-tools/36.0.0").mkdir(parents=True)
    (root / "build-tools/36.0.0/zipalign").write_bytes(b"")
    return root


def write_apk(path: Path, libraries: dict[str, str]) -> Path:
    """Write an APK holding each ``lib/<abi>/<name>`` in *libraries*, whose content is its readelf output."""
    with zipfile.ZipFile(path, "w") as apk:
        apk.writestr("AndroidManifest.xml", "")
        apk.writestr("assets/fonts/font.ttf", "")
        for name, readelf in libraries.items():
            apk.writestr(name, readelf)
    return path


class FakeTools:
    """Fake llvm-readelf (prints the extracted library's content) and zipalign."""

    def __init__(self, zipalign_usage: str = ZIPALIGN_USAGE) -> None:
        self.zipalign_usage = zipalign_usage
        self.zipalign_check = completed(
            "Verifying alignment of app.apk (4)...\nVerification succesful\n"
        )
        self.calls: list[list[str]] = []

    def __call__(self, cmd: list[str], **_kwargs: object) -> subprocess.CompletedProcess:
        self.calls.append(cmd)
        tool = Path(cmd[0]).name
        if tool == "llvm-readelf":
            content = Path(cmd[-1]).read_text()
            if content == "corrupt":
                return completed(returncode=1, stderr="llvm-readelf: error: invalid ELF header")
            return completed(content)
        if tool == "zipalign" and len(cmd) == 1:
            return completed(stderr=self.zipalign_usage)
        return self.zipalign_check


@pytest.fixture
def tools(monkeypatch: pytest.MonkeyPatch) -> FakeTools:
    fake = FakeTools()
    monkeypatch.setattr(mod, "run_captured", fake)
    return fake


def good_libraries() -> dict[str, str]:
    return {
        "lib/arm64-v8a/libQGroundControl_arm64-v8a.so": readelf_output(
            "libQGroundControl_arm64-v8a.so",
            ("libgstreamer_android.so", "libc++_shared.so", "libEGL.so", "liblog.so", "libc.so"),
        ),
        "lib/arm64-v8a/libgstreamer_android.so": readelf_output(
            "libgstreamer_android.so", ("liblog.so", "libc.so")
        ),
        "lib/arm64-v8a/libc++_shared.so": readelf_output("libc++_shared.so"),
        "lib/armeabi-v7a/libgstreamer_android.so": READELF_32,
        "lib/armeabi-v7a/libc++_shared.so": READELF_32.replace("libc++_shared.so", "libc.so"),
    }


def run(apk: Path, ndk: Path, sdk: Path, *extra: str) -> int:
    return mod.main(
        [
            "--apk",
            str(apk),
            "--ndk-root",
            str(ndk),
            "--sdk-root",
            str(sdk),
            "--build-tools",
            "36.0.0",
            "--min-sdk",
            "28",
            *extra,
        ]
    )


def test_parse_readelf_reads_load_alignments_and_needed_libraries() -> None:
    info = mod.parse_readelf(readelf_output("libfoo.so", ("libc.so", "libbar.so"), align="0x1000"))
    assert info == mod.ElfInfo(load_alignments=(0x1000, 0x1000), needed=("libc.so", "libbar.so"))
    assert mod.parse_readelf(READELF_32) == mod.ElfInfo(
        (0x1000, 0x1000), ("libc.so", "libc++_shared.so")
    )


def test_system_libraries_are_the_elf_stubs_at_the_highest_level_up_to_the_api(ndk: Path) -> None:
    prebuilt = mod.ndk_prebuilt(ndk)
    assert mod.system_libraries(prebuilt, "aarch64-linux-android", 28) == {
        "libc.so",
        "liblog.so",
        "libEGL.so",
    }
    assert mod.system_libraries(prebuilt, "aarch64-linux-android", 35) == {"libc.so", "libamidi.so"}
    with pytest.raises(FileNotFoundError, match="API 27"):
        mod.system_libraries(prebuilt, "aarch64-linux-android", 27)


def test_passes_a_well_formed_apk(
    tmp_path: Path,
    ndk: Path,
    sdk: Path,
    tools: FakeTools,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    apk = write_apk(tmp_path / "app.apk", good_libraries())
    assert run(apk, ndk, sdk) == 0
    out = capsys.readouterr().out
    assert "::error" not in out
    summary = gh_summary.read_text()
    assert "### APK check" in summary
    assert "| Result | passed |" in summary
    assert "| Native libraries | 5 |" in summary
    assert "| zipalign | passed |" in summary
    assert [
        str(sdk / "build-tools/36.0.0/zipalign"),
        "-c",
        "-P",
        "16",
        "-v",
        "4",
        str(apk),
    ] in tools.calls


def test_fails_a_4kb_aligned_64_bit_library(
    tmp_path: Path,
    ndk: Path,
    sdk: Path,
    tools: FakeTools,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    libraries = good_libraries()
    libraries["lib/arm64-v8a/libgstreamer_android.so"] = readelf_output(
        "libgstreamer_android.so", align="0x1000"
    )
    apk = write_apk(tmp_path / "app.apk", libraries)
    assert run(apk, ndk, sdk) == 1
    out = capsys.readouterr().out
    assert (
        "::error title=APK check::lib/arm64-v8a/libgstreamer_android.so has PT_LOAD segments aligned to 0x1000, "
        "below 16 KB (0x4000)"
    ) in out
    # 32-bit ABIs keep 4 KB pages.
    assert "armeabi-v7a" not in out.split("### APK check")[0]
    assert "| Result | failed |" in gh_summary.read_text()


@pytest.mark.parametrize(
    ("needed", "reason"),
    [
        (
            "libmissing.so",
            "libmissing.so, which is neither packaged in lib/arm64-v8a/ nor an NDK system library",
        ),
        # Only API 29 and later provide it.
        (
            "libamidi.so",
            "libamidi.so, which is neither packaged in lib/arm64-v8a/ nor an NDK system library at API 28",
        ),
        # The sysroot's libc++.so is a linker script; apps package libc++_shared.so.
        ("libc++.so", "libc++.so, which is neither packaged"),
    ],
)
def test_fails_a_needed_library_neither_packaged_nor_in_the_ndk(
    tmp_path: Path,
    ndk: Path,
    sdk: Path,
    tools: FakeTools,
    needed: str,
    reason: str,
    capsys: pytest.CaptureFixture[str],
) -> None:
    libraries = good_libraries()
    libraries["lib/arm64-v8a/libgstreamer_android.so"] = readelf_output(
        "libgstreamer_android.so", ("libc.so", needed)
    )
    assert run(write_apk(tmp_path / "app.apk", libraries), ndk, sdk) == 1
    assert f"lib/arm64-v8a/libgstreamer_android.so needs {reason}" in capsys.readouterr().out


def test_a_library_packaged_for_another_abi_does_not_satisfy_a_needed_entry(
    tmp_path: Path, ndk: Path, sdk: Path, tools: FakeTools, capsys: pytest.CaptureFixture[str]
) -> None:
    libraries = good_libraries()
    del libraries["lib/armeabi-v7a/libc++_shared.so"]
    assert run(write_apk(tmp_path / "app.apk", libraries), ndk, sdk) == 1
    assert (
        "lib/armeabi-v7a/libgstreamer_android.so needs libc++_shared.so" in capsys.readouterr().out
    )


@pytest.mark.parametrize(("option", "required"), [("ON", True), ("OFF", False), (None, False)])
def test_requires_the_gstreamer_library_when_the_build_enables_gstreamer(
    tmp_path: Path,
    ndk: Path,
    sdk: Path,
    tools: FakeTools,
    option: str | None,
    required: bool,
    capsys: pytest.CaptureFixture[str],
) -> None:
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    if option is not None:
        (build_dir / "CMakeCache.txt").write_text(f"QGC_ENABLE_GST_VIDEOSTREAMING:BOOL={option}\n")
    libraries = {
        name: content
        for name, content in good_libraries().items()
        if "libgstreamer_android" not in name
    }
    libraries["lib/arm64-v8a/libQGroundControl_arm64-v8a.so"] = readelf_output(
        "libQGroundControl_arm64-v8a.so"
    )
    exit_code = run(
        write_apk(tmp_path / "app.apk", libraries), ndk, sdk, "--build-dir", str(build_dir)
    )
    out = capsys.readouterr().out
    assert exit_code == int(required)
    for abi in ("arm64-v8a", "armeabi-v7a"):
        assert (
            f"::error title=APK check::lib/{abi}/libgstreamer_android.so is missing" in out
        ) is required


def test_fails_an_apk_without_native_libraries(
    tmp_path: Path, ndk: Path, sdk: Path, tools: FakeTools, capsys: pytest.CaptureFixture[str]
) -> None:
    assert run(write_apk(tmp_path / "app.apk", {}), ndk, sdk) == 1
    assert "app.apk has no native libraries" in capsys.readouterr().out


def test_reports_a_library_llvm_readelf_cannot_read(
    tmp_path: Path, ndk: Path, sdk: Path, tools: FakeTools, capsys: pytest.CaptureFixture[str]
) -> None:
    libraries = good_libraries()
    libraries["lib/arm64-v8a/libc++_shared.so"] = "corrupt"
    assert run(write_apk(tmp_path / "app.apk", libraries), ndk, sdk) == 1
    assert (
        "llvm-readelf cannot read lib/arm64-v8a/libc++_shared.so: llvm-readelf: error: invalid ELF header"
        in capsys.readouterr().out
    )


def test_fails_when_zipalign_finds_misaligned_entries(
    tmp_path: Path,
    ndk: Path,
    sdk: Path,
    tools: FakeTools,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    bad = [f"   {offset} res/icon{offset}.png (BAD - 1)" for offset in range(1, 8)]
    tools.zipalign_check = completed(
        "Verifying alignment of app.apk (4)...\n"
        "   49152 lib/arm64-v8a/libgstreamer_android.so (BAD - 4096)\n"
        + "\n".join(bad)
        + "\nVerification FAILED\n",
        returncode=1,
    )
    assert run(write_apk(tmp_path / "app.apk", good_libraries()), ndk, sdk) == 1
    assert (
        "zipalign -c -P 16 -v 4 failed (exit code 1): "
        "49152 lib/arm64-v8a/libgstreamer_android.so (BAD - 4096); 1 res/icon1.png (BAD - 1); "
        "2 res/icon2.png (BAD - 1); 3 res/icon3.png (BAD - 1); 4 res/icon4.png (BAD - 1); …and 3 more"
    ) in capsys.readouterr().out
    assert "| zipalign | failed |" in gh_summary.read_text()


@pytest.mark.parametrize(
    ("usage", "installed", "result"),
    [
        (ZIPALIGN_OLD_USAGE, True, "skipped: no -P"),
        (ZIPALIGN_USAGE, False, "skipped: zipalign not found"),
    ],
)
def test_skips_zipalign_with_a_notice_without_page_alignment_support(
    tmp_path: Path,
    ndk: Path,
    sdk: Path,
    tools: FakeTools,
    gh_summary: Path,
    usage: str,
    installed: bool,
    result: str,
    capsys: pytest.CaptureFixture[str],
) -> None:
    tools.zipalign_usage = usage
    if not installed:
        (sdk / "build-tools/36.0.0/zipalign").unlink()
    assert run(write_apk(tmp_path / "app.apk", good_libraries()), ndk, sdk) == 0
    assert "::notice::" in capsys.readouterr().out
    assert f"| zipalign | {result} |" in gh_summary.read_text()
    assert not any("-c" in call for call in tools.calls)


def test_annotates_only_the_first_problems(
    tmp_path: Path, ndk: Path, sdk: Path, tools: FakeTools, capsys: pytest.CaptureFixture[str]
) -> None:
    libraries = good_libraries()
    for index in range(mod.MAX_ANNOTATIONS + 2):
        libraries[f"lib/arm64-v8a/libextra{index:02}.so"] = readelf_output(
            f"libextra{index:02}.so", align="0x1000"
        )
    assert run(write_apk(tmp_path / "app.apk", libraries), ndk, sdk) == 1
    out = capsys.readouterr().out
    assert out.count("::error") == mod.MAX_ANNOTATIONS
    assert "lib/arm64-v8a/libextra11.so has PT_LOAD segments aligned to 0x1000" in out


def test_requires_an_ndk(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.delenv("ANDROID_NDK_ROOT", raising=False)
    with pytest.raises(SystemExit):
        mod.main(["--apk", str(tmp_path / "app.apk")])


def test_fails_without_the_apk_or_llvm_readelf(
    tmp_path: Path, ndk: Path, sdk: Path, tools: FakeTools, capsys: pytest.CaptureFixture[str]
) -> None:
    assert run(tmp_path / "missing.apk", ndk, sdk) == 1
    assert "::error title=APK check::APK not found" in capsys.readouterr().out
    (mod.ndk_prebuilt(ndk) / "bin/llvm-readelf").unlink()
    assert run(write_apk(tmp_path / "app.apk", good_libraries()), ndk, sdk) == 1
    assert "llvm-readelf not found" in capsys.readouterr().out
