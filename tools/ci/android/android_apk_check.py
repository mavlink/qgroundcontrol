#!/usr/bin/env python3
"""Check the native libraries an Android APK ships.

For every ``lib/<abi>/*.so`` in the APK, reads the ELF program headers and dynamic section with
the NDK's llvm-readelf and requires:

- on the 64-bit ABIs, every PT_LOAD segment aligned to at least 16 KB: a device with 16 KB pages
  cannot load a library linked for 4 KB pages, and Google Play requires 16 KB support;
- every DT_NEEDED library packaged in the same ABI directory or a public NDK system library at
  the app's minimum API level (the NDK sysroot's stub libraries for it).

Each ABI directory must also hold ``libgstreamer_android.so`` when the build enables GStreamer.
Then runs ``zipalign -c -P 16 -v 4`` when the SDK's zipalign has ``-P``, else skips it with a
notice. Problems become error annotations; every run writes the job summary's APK check section.
"""

from __future__ import annotations

import argparse
import os
import shutil
import sys
import tempfile
import zipfile
from dataclasses import dataclass, field
from pathlib import Path, PurePosixPath

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.build_config import get_build_config_value
from common.cmake import read_cache_var
from common.format import format_count
from common.gh_actions import gh_error, gh_notice, write_step_summary
from common.markdown import code_block, code_span, more_items, summary_section
from common.proc import output_text, run_captured

ABI_TRIPLES = {
    "arm64-v8a": "aarch64-linux-android",
    "armeabi-v7a": "arm-linux-androideabi",
    "x86": "i686-linux-android",
    "x86_64": "x86_64-linux-android",
}
# Only 64-bit devices use 16 KB pages; Qt and the NDK align only these ABIs' libraries for them.
PAGE_ALIGNED_ABIS = frozenset({"arm64-v8a", "x86_64"})
MIN_LOAD_ALIGNMENT = 0x4000
ZIPALIGN_PAGE_SIZE_KB = "16"
GSTREAMER_LIBRARY = "libgstreamer_android.so"
GSTREAMER_OPTION = "QGC_ENABLE_GST_VIDEOSTREAMING"
MAX_ANNOTATIONS = 10
MAX_ZIPALIGN_LINES = 5
READELF_TIMEOUT_SECONDS = 120
ZIPALIGN_TIMEOUT_SECONDS = 300
ELF_MAGIC = b"\x7fELF"


@dataclass(frozen=True)
class ElfInfo:
    """The PT_LOAD alignments and DT_NEEDED entries of one library."""

    load_alignments: tuple[int, ...]
    needed: tuple[str, ...]


@dataclass
class ApkReport:
    """What the check found: the APK's ABIs and libraries, its problems, and zipalign's result."""

    abis: list[str] = field(default_factory=list)
    libraries: int = 0
    problems: list[str] = field(default_factory=list)
    zipalign: str = ""


def parse_readelf(output: str) -> ElfInfo:
    """Read the PT_LOAD alignments and DT_NEEDED names from ``llvm-readelf -l -d -W`` output."""
    alignments: list[int] = []
    needed: list[str] = []
    for line in output.splitlines():
        fields = line.split()
        if fields[:1] == ["LOAD"]:
            alignments.append(int(fields[-1], 16))
        elif "(NEEDED)" in fields:
            start, end = line.find("["), line.rfind("]")
            if 0 <= start < end:
                needed.append(line[start + 1 : end])
    return ElfInfo(tuple(alignments), tuple(needed))


def ndk_prebuilt(ndk_root: Path) -> Path:
    """Return the NDK's LLVM toolchain directory for this host (its only ``prebuilt`` entry)."""
    hosts = sorted(
        path for path in (ndk_root / "toolchains/llvm/prebuilt").glob("*") if path.is_dir()
    )
    if not hosts:
        raise FileNotFoundError(f"No LLVM toolchain under {ndk_root}/toolchains/llvm/prebuilt")
    return hosts[0]


def find_tool(directory: Path, name: str) -> Path | None:
    """Return *name* (``.exe`` on Windows) in *directory*, or None."""
    for candidate in (directory / name, directory / f"{name}.exe"):
        if candidate.is_file():
            return candidate
    return None


def system_libraries(prebuilt: Path, triple: str, api: int) -> frozenset[str]:
    """Return the public NDK libraries for *triple* at *api*: the sysroot's ELF stubs.

    Uses the highest stub level up to *api*. The stub directories also hold linker scripts, like
    ``libc++.so``, which no device provides.
    """
    root = prebuilt / "sysroot/usr/lib" / triple
    levels = sorted(
        int(path.name) for path in root.glob("*") if path.name.isdigit() and int(path.name) <= api
    )
    if not levels:
        raise FileNotFoundError(f"No NDK stub libraries for API {api} or lower under {root}")
    names: set[str] = set()
    for stub in (root / str(levels[-1])).glob("*.so"):
        with stub.open("rb") as handle:
            if handle.read(len(ELF_MAGIC)) == ELF_MAGIC:
                names.add(stub.name)
    return frozenset(names)


def read_elf(readelf: Path, library: Path) -> ElfInfo:
    """Run llvm-readelf on *library*; raise RuntimeError when it fails."""
    result = run_captured(
        [str(readelf), "--program-headers", "--dynamic-table", "--wide", str(library)],
        timeout=READELF_TIMEOUT_SECONDS,
    )
    if result.returncode != 0:
        raise RuntimeError(output_text(result.stderr).strip() or f"exit code {result.returncode}")
    return parse_readelf(result.stdout)


def native_libraries(apk: zipfile.ZipFile) -> dict[str, list[str]]:
    """Map each ABI directory of *apk* to the names of the ``.so`` files directly in it."""
    libraries: dict[str, list[str]] = {}
    for name in apk.namelist():
        path = PurePosixPath(name)
        if len(path.parts) == 3 and path.parts[0] == "lib" and path.suffix == ".so":
            libraries.setdefault(path.parts[1], []).append(path.name)
    return {abi: sorted(names) for abi, names in sorted(libraries.items())}


def library_problems(
    abi: str, name: str, info: ElfInfo, packaged: set[str], system: frozenset[str], api: int
) -> list[str]:
    """Return the problems of library *name* of *abi*, described by *info*."""
    problems: list[str] = []
    if abi in PAGE_ALIGNED_ABIS:
        if not info.load_alignments:
            problems.append(f"lib/{abi}/{name} has no PT_LOAD segment")
        small = sorted({align for align in info.load_alignments if align < MIN_LOAD_ALIGNMENT})
        if small:
            problems.append(
                f"lib/{abi}/{name} has PT_LOAD segments aligned to "
                f"{', '.join(hex(align) for align in small)}, below 16 KB ({hex(MIN_LOAD_ALIGNMENT)}): "
                "devices with 16 KB pages cannot load it (link it with -z max-page-size=16384)"
            )
    for needed in info.needed:
        if needed not in packaged and needed not in system:
            problems.append(
                f"lib/{abi}/{name} needs {needed}, which is neither packaged in lib/{abi}/ nor an "
                f"NDK system library at API {api}"
            )
    return problems


def check_libraries(
    apk_path: Path, readelf: Path, prebuilt: Path, api: int, required: list[str]
) -> ApkReport:
    """Check every native library of the APK at *apk_path*; *required* names libraries every ABI needs."""
    report = ApkReport()
    with zipfile.ZipFile(apk_path) as apk, tempfile.TemporaryDirectory() as scratch:
        libraries = native_libraries(apk)
        report.abis = list(libraries)
        if not libraries:
            report.problems.append(f"{apk_path.name} has no native libraries (lib/<abi>/*.so)")
        for abi, names in libraries.items():
            packaged = set(names)
            report.problems += [
                f"lib/{abi}/{library} is missing" for library in required if library not in packaged
            ]
            triple = ABI_TRIPLES.get(abi)
            if triple is None:
                report.problems.append(f"lib/{abi}/ is not an ABI Qt for Android builds")
                continue
            system = system_libraries(prebuilt, triple, api)
            for name in names:
                report.libraries += 1
                extracted = Path(scratch, name)
                with apk.open(f"lib/{abi}/{name}") as source, extracted.open("wb") as target:
                    shutil.copyfileobj(source, target)
                try:
                    info = read_elf(readelf, extracted)
                except RuntimeError as error:
                    report.problems.append(f"llvm-readelf cannot read lib/{abi}/{name}: {error}")
                    continue
                finally:
                    extracted.unlink()
                report.problems += library_problems(abi, name, info, packaged, system, api)
    return report


def check_zipalign(zipalign: Path | None, apk_path: Path, report: ApkReport) -> None:
    """Run ``zipalign -c -P 16 -v 4`` on the APK when *zipalign* supports ``-P``."""
    if zipalign is None:
        report.zipalign = "skipped: zipalign not found"
        gh_notice(
            "zipalign not found in the Android SDK build-tools; skipping the APK alignment check"
        )
        return
    usage = run_captured([str(zipalign)], timeout=ZIPALIGN_TIMEOUT_SECONDS)
    if "-P <pagesize_kb>" not in usage.stdout + usage.stderr:
        report.zipalign = "skipped: no -P"
        gh_notice(
            f"{zipalign} has no -P (build-tools 35 or later); skipping the APK alignment check"
        )
        return
    command = [str(zipalign), "-c", "-P", ZIPALIGN_PAGE_SIZE_KB, "-v", "4", str(apk_path)]
    result = run_captured(command, timeout=ZIPALIGN_TIMEOUT_SECONDS)
    if result.returncode == 0:
        report.zipalign = "passed"
        return
    report.zipalign = "failed"
    output = (result.stdout + result.stderr).splitlines()
    bad = [line.strip() for line in output if "(BAD" in line] or output[-MAX_ZIPALIGN_LINES:]
    print("\n".join(bad))
    if len(bad) > MAX_ZIPALIGN_LINES:
        bad = [*bad[:MAX_ZIPALIGN_LINES], more_items(len(bad) - MAX_ZIPALIGN_LINES)]
    report.problems.append(
        f"zipalign -c -P {ZIPALIGN_PAGE_SIZE_KB} -v 4 failed (exit code {result.returncode}): "
        + "; ".join(bad)
    )


def write_summary(apk_path: Path, report: ApkReport) -> None:
    """Write the job summary's APK check section."""
    section = summary_section(
        "APK check",
        [
            ("Result", "failed" if report.problems else "passed"),
            ("APK", code_span(apk_path.name)),
            ("ABIs", ", ".join(code_span(abi) for abi in report.abis)),
            ("Native libraries", format_count(report.libraries)),
            ("Problems", format_count(len(report.problems)) if report.problems else ""),
            ("zipalign", report.zipalign),
        ],
        collapsed=[("Problems", code_block("\n".join(report.problems)) if report.problems else "")],
    )
    print(section)
    write_step_summary(section)


def gstreamer_enabled(build_dir: Path) -> bool:
    """Report whether the CMake build in *build_dir* enables GStreamer."""
    value = read_cache_var(build_dir / "CMakeCache.txt", GSTREAMER_OPTION) or ""
    return value.upper() in {"1", "ON", "TRUE", "Y", "YES"}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apk", type=Path, required=True, help="APK to check")
    parser.add_argument(
        "--build-dir",
        type=Path,
        help=f"CMake build directory; requires {GSTREAMER_LIBRARY} when its cache enables {GSTREAMER_OPTION}",
    )
    parser.add_argument(
        "--ndk-root",
        type=Path,
        default=os.environ.get("ANDROID_NDK_ROOT") or None,
        help="Android NDK (default: $ANDROID_NDK_ROOT)",
    )
    parser.add_argument(
        "--sdk-root",
        type=Path,
        default=os.environ.get("ANDROID_SDK_ROOT") or os.environ.get("ANDROID_HOME") or None,
        help="Android SDK whose build-tools hold zipalign (default: $ANDROID_SDK_ROOT, else $ANDROID_HOME)",
    )
    parser.add_argument(
        "--build-tools",
        default=get_build_config_value("android.build_tools"),
        help="Build-tools version holding zipalign (default: build-config.json's android.build_tools)",
    )
    parser.add_argument(
        "--min-sdk",
        type=int,
        default=int(get_build_config_value("android.min_sdk", "0")),
        help="Minimum API level whose NDK system libraries may be needed (default: build-config.json's)",
    )
    args = parser.parse_args(argv)
    if args.ndk_root is None:
        parser.error("--ndk-root or ANDROID_NDK_ROOT is required")
    if args.min_sdk <= 0:
        parser.error("--min-sdk needs a build-config.json android.min_sdk value")
    if not args.apk.is_file():
        gh_error(f"APK not found: {args.apk}", title="APK check")
        return 1

    prebuilt = ndk_prebuilt(args.ndk_root)
    readelf = find_tool(prebuilt / "bin", "llvm-readelf")
    if readelf is None:
        gh_error(f"llvm-readelf not found in {prebuilt / 'bin'}", title="APK check")
        return 1
    required = [GSTREAMER_LIBRARY] if args.build_dir and gstreamer_enabled(args.build_dir) else []

    report = check_libraries(args.apk, readelf, prebuilt, args.min_sdk, required)
    zipalign = None
    if args.sdk_root and args.build_tools:
        zipalign = find_tool(args.sdk_root / "build-tools" / args.build_tools, "zipalign")
    check_zipalign(zipalign, args.apk, report)

    for problem in report.problems[:MAX_ANNOTATIONS]:
        gh_error(problem, title="APK check")
    for problem in report.problems[MAX_ANNOTATIONS:]:
        print(problem)
    write_summary(args.apk, report)
    return 1 if report.problems else 0


if __name__ == "__main__":
    raise SystemExit(main())
