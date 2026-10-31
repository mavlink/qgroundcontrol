#!/usr/bin/env python3
"""Audit a macOS app bundle's code signature and the libraries its Mach-O files load.

Verifies a Developer ID signature with ``codesign --verify --deep --strict`` (an ad-hoc signed or
unsigned bundle, as pull requests build, is skipped with a notice). Then reads every Mach-O file's
load commands with ``otool -l`` and fails when a library one loads resolves neither inside the
bundle, through @rpath (its own and the main executable's LC_RPATHs), @loader_path or
@executable_path, nor to macOS (/usr/lib, /System). Absolute paths anywhere else, such as Homebrew's
/opt/homebrew and /usr/local, /Library/Frameworks/GStreamer.framework or the build machine's
directories, are errors in load commands, and in the LC_RPATHs dyld searches: the main
executable's, and those of files that load libraries through @rpath.

verify_executable.py --type dmg audits the mounted image's bundle before its boot test.
"""

from __future__ import annotations

import argparse
import functools
import os
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import gh_error, gh_notice, write_step_summary
from common.markdown import code_block, code_span, more_items, summary_section
from common.proc import run_captured

TOOL_TIMEOUT_SECONDS = 120
SYSTEM_PREFIXES = ("/usr/lib/", "/System/")
MACHO_MAGICS = frozenset(
    bytes.fromhex(magic)
    for magic in ("feedface", "cefaedfe", "feedfacf", "cffaedfe", "cafebabe", "bebafeca")
)
MH_EXECUTE = 2
DYLIB_COMMANDS = frozenset(
    {
        "LC_LOAD_DYLIB",
        "LC_LOAD_WEAK_DYLIB",
        "LC_REEXPORT_DYLIB",
        "LC_LAZY_LOAD_DYLIB",
        "LC_LOAD_UPWARD_DYLIB",
    }
)
SUMMARY_PROBLEMS = 30


@dataclass(frozen=True)
class LoadCommands:
    """What one Mach-O file's load commands say about the libraries it loads."""

    executable: bool
    dylibs: tuple[str, ...]
    rpaths: tuple[str, ...]


def _strip_offset(value: str) -> str:
    """Drop otool's `` (offset N)`` suffix from a load command's name or path."""
    head, separator, _ = value.rpartition(" (offset ")
    return head if separator else value


def parse_load_commands(text: str) -> LoadCommands:
    """Parse ``otool -l -arch all`` output; a universal file lists each architecture's commands."""
    executable = False
    dylibs: dict[str, None] = {}
    rpaths: dict[str, None] = {}
    lines = text.splitlines()
    command = ""
    for index, line in enumerate(lines):
        fields = line.split(maxsplit=1)
        if line.strip() == "Mach header" and index + 2 < len(lines):
            names, values = lines[index + 1].split(), lines[index + 2].split()
            if "filetype" in names and len(values) == len(names):
                executable |= values[names.index("filetype")] == str(MH_EXECUTE)
        elif len(fields) == 2 and fields[0] == "cmd":
            command = fields[1]
        elif len(fields) == 2 and fields[0] == "name" and command in DYLIB_COMMANDS:
            dylibs[_strip_offset(fields[1])] = None
        elif len(fields) == 2 and fields[0] == "path" and command == "LC_RPATH":
            rpaths[_strip_offset(fields[1])] = None
    return LoadCommands(executable, tuple(dylibs), tuple(rpaths))


def macho_files(bundle: Path) -> list[Path]:
    """Return the bundle's Mach-O files, skipping symlinks such as a framework's Versions/Current."""
    found = []
    for root, _dirs, files in os.walk(bundle):
        for name in files:
            path = Path(root) / name
            if path.is_symlink() or not path.is_file():
                continue
            with path.open("rb") as stream:
                if stream.read(4) in MACHO_MAGICS:
                    found.append(path)
    return sorted(found)


def _run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    try:
        return run_captured(cmd, timeout=TOOL_TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired:
        return subprocess.CompletedProcess(cmd, 1, "", f"timed out after {TOOL_TIMEOUT_SECONDS}s")


def read_load_commands(path: Path, otool: str = "otool") -> LoadCommands:
    result = _run([otool, "-l", "-arch", "all", str(path)])
    if result.returncode != 0:
        raise RuntimeError(f"otool -l failed for {path}: {result.stderr.strip()}")
    return parse_load_commands(result.stdout)


def _is_system(path: str) -> bool:
    return path.startswith(SYSTEM_PREFIXES)


class Resolver:
    """Resolve the install names a bundle's files load the way dyld does, within the bundle."""

    def __init__(self, bundle: Path, executable: Path, commands: dict[Path, LoadCommands]) -> None:
        self.bundle = bundle.resolve()
        self.executable = executable
        self.commands = commands

    def _expand(self, path: str, loader: Path) -> Path | None:
        """Expand @loader_path and @executable_path in an install name or rpath of *loader*."""
        info = self.commands.get(loader)
        host = loader if info is not None and info.executable else self.executable
        for token, directory in (
            ("@loader_path", loader.parent),
            ("@executable_path", host.parent),
        ):
            if path == token or path.startswith(f"{token}/"):
                return Path(os.path.normpath(f"{directory}{path[len(token) :]}"))
        return None

    def _in_bundle(self, candidate: Path) -> bool:
        return candidate.exists() and candidate.resolve().is_relative_to(self.bundle)

    def problem(self, name: str, loader: Path) -> str | None:
        """Return why *loader* can't load install name *name* from the bundle or macOS, else None."""
        if _is_system(name):
            return None
        if name.startswith("@rpath/"):
            rest = name.removeprefix("@rpath/")
            for owner in dict.fromkeys((loader, self.executable)):
                info = self.commands.get(owner)
                for rpath in info.rpaths if info is not None else ():
                    directory = self._expand(rpath, owner)
                    if directory is not None and self._in_bundle(directory / rest):
                        return None
            return "not found inside the bundle through any LC_RPATH"
        if name.startswith("@"):
            candidate = self._expand(name, loader)
            if candidate is not None and self._in_bundle(candidate):
                return None
            return "not found inside the bundle"
        if name.startswith("/"):
            return "absolute path outside the bundle and macOS"
        return "relative path, resolved against the working directory"


def audit_load_commands(bundle: Path, otool: str = "otool") -> tuple[int, list[str]]:
    """Return the number of Mach-O files in *bundle* and a line for each library one can't load."""
    files = macho_files(bundle)
    executable = bundle / "Contents" / "MacOS" / bundle.stem
    commands: dict[Path, LoadCommands] = {}
    problems: list[str] = []
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        results = pool.map(functools.partial(_read_or_error, otool), files)
        for path, result in zip(files, results, strict=True):
            if isinstance(result, str):
                problems.append(f"{path.relative_to(bundle)}: {result}")
            else:
                commands[path] = result
    resolver = Resolver(bundle, executable, commands)
    for path, info in commands.items():
        relative = path.relative_to(bundle)
        searched = path == executable or any(name.startswith("@rpath/") for name in info.dylibs)
        for rpath in info.rpaths if searched else ():
            if rpath.startswith("/") and not _is_system(rpath):
                problems.append(f"{relative}: LC_RPATH {rpath}: absolute path outside the bundle")
        for name in info.dylibs:
            reason = resolver.problem(name, path)
            if reason is not None:
                problems.append(f"{relative} -> {name}: {reason}")
    return len(files), problems


def _read_or_error(otool: str, path: Path) -> LoadCommands | str:
    try:
        return read_load_commands(path, otool)
    except (OSError, RuntimeError) as error:
        return str(error)


def check_signature(bundle: Path) -> tuple[str, str]:
    """Verify a Developer ID signature; return the Signature summary and codesign's error, if any.

    Ad-hoc signed and unsigned bundles have no identity to verify, so they are only reported.
    """
    if shutil.which("codesign") is None:
        return "not checked (no codesign)", ""
    display = _run(["codesign", "--display", "--verbose=2", str(bundle)])
    details = f"{display.stdout}{display.stderr}".strip()
    if display.returncode != 0 and "not signed at all" in details:
        return "unsigned, not verified", ""
    if display.returncode != 0:
        return "failed", details
    if "Signature=adhoc" in details:
        return "ad-hoc, not verified", ""
    verify = _run(["codesign", "--verify", "--deep", "--strict", "--verbose=2", str(bundle)])
    if verify.returncode != 0:
        return "failed", f"{verify.stdout}{verify.stderr}".strip()
    return "verified", ""


def audit_bundle(bundle: Path, otool: str = "otool") -> bool:
    """Audit *bundle*'s signature and load commands; annotate and summarize; return whether it passed."""
    signature, signature_error = check_signature(bundle)
    if signature_error:
        gh_error(f"codesign rejected {bundle.name}: {signature_error}", title="Bundle audit")
    elif signature == "verified":
        print(f"codesign --verify --deep --strict passed for {bundle.name}")
    else:
        gh_notice(f"{bundle.name} signature {signature}: skipped codesign --verify --deep --strict")

    files, problems = audit_load_commands(bundle, otool)
    for problem in problems:
        print(problem)
    if problems:
        gh_error(
            f"{len(problems)} libraries in {bundle.name} don't load from the bundle or macOS: "
            f"{problems[0]}",
            title="Bundle audit",
        )
    else:
        print(f"Every library {files} Mach-O files load resolves inside {bundle.name} or to macOS")

    facts = [
        ("Result", "failed" if problems or signature == "failed" else "passed"),
        ("Bundle", code_span(bundle.name)),
        ("Signature", signature),
        ("Mach-O files", files),
        ("Unresolved libraries", len(problems)),
    ]
    shown = problems[:SUMMARY_PROBLEMS]
    if len(problems) > len(shown):
        shown.append(more_items(len(problems) - len(shown)))
    collapsed = [("codesign", code_block(signature_error))] if signature_error else []
    write_step_summary(
        summary_section(
            "Bundle audit",
            facts,
            body=code_block("\n".join(shown)) if shown else "",
            collapsed=collapsed,
        )
    )
    return not problems and signature != "failed"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundle", type=Path, help="The .app bundle")
    parser.add_argument("--otool", default="otool", help="otool or a compatible llvm-otool")
    args = parser.parse_args(argv)
    if not (args.bundle / "Contents" / "MacOS").is_dir():
        gh_error(f"Not an app bundle: {args.bundle}")
        return 1
    return 0 if audit_bundle(args.bundle, args.otool) else 1


if __name__ == "__main__":
    raise SystemExit(main())
