#!/usr/bin/env python3
"""Report a package's contents against master's artifact and check it for GPL-licensed GStreamer.

Lists the package's files (NSIS installer with 7-Zip, DMG mounted with hdiutil, AppImage with
--appimage-extract, APK, App Bundle and IPA as zip archives) and compares them with the package in
the same-named artifact of the newest successful master run of this workflow: package and contents
size with their change, files added and removed, and the largest size changes, in the job summary.
A size change beyond --size-threshold percent is a warning; a missing baseline only leaves the
comparison out. Files named after a GPL-denylisted library or plugin (build-config.json's
gstreamer_plugin_policy.gpl_denylist), and mobile binaries
containing a GPL encoder's markers, fail the step; with --gpl-findings warn they are warnings, for a
package type with a known issue (KNOWN_GPL_ISSUES).
"""

from __future__ import annotations

import argparse
import contextlib
import json
import os
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import zipfile
from dataclasses import dataclass
from pathlib import Path
from typing import IO, TYPE_CHECKING
from urllib.parse import urlencode

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.format import format_bytes, format_count
from common.gh_actions import (
    download_artifact_zip,
    env_default,
    gh_api_json,
    gh_error,
    gh_warning,
    list_run_artifacts,
    write_step_summary,
)
from common.markdown import code_span, escape_text, md_table, more_items, summary_section
from common.paths import REPO_ROOT
from common.proc import run_captured

if TYPE_CHECKING:
    from collections.abc import Iterable, Iterator

BUILD_CONFIG = REPO_ROOT / ".github" / "build-config.json"
ZIP_SUFFIXES = (".aab", ".apk", ".ipa")
PACKAGE_SUFFIXES = (".appimage", ".dmg", ".exe", *ZIP_SUFFIXES)
NON_PR_EVENTS = frozenset({"push", "schedule", "workflow_dispatch"})
BASELINE_RUNS = 10
TOOL_TIMEOUT_SECONDS = 600
DEFAULT_THRESHOLD_PERCENT = 10.0
DEFAULT_TOP = 10
MAX_GPL_ANNOTATIONS = 10
GPL_FINDINGS_MODES = ("fail", "warn")
# Why a package type ships GPL code; --gpl-findings warn reports its findings as this known issue.
KNOWN_GPL_ISSUES = {
    ".appimage": (
        "Ubuntu's `libavcodec`, which the bundled gst-libav plugin (`libgstlibav.so`) loads, links "
        "the GPL x264 and x265 encoders, so the AppImage's dependency deployment bundles `libx264` "
        "and `libx265`"
    ),
}
# Static mobile builds link GStreamer plugins into a few binaries, so the denylisted names never
# show as files there: look for the GPL encoders' entry points and the banners they write into
# every stream (stripped binaries keep the banners). QGC itself names x264enc/x265enc, so element
# names cannot be markers.
GPL_BINARY_MARKERS = (
    b"x264_encoder_open",
    b"videolan.org/x264.html",
    b"x265_encoder_open",
    b"Multicoreware",
)
BINARY_MAGIC = frozenset(
    {
        b"\x7fELF",
        b"\xca\xfe\xba\xbe",
        b"\xce\xfa\xed\xfe",
        b"\xcf\xfa\xed\xfe",
        b"\xfe\xed\xfa\xce",
        b"\xfe\xed\xfa\xcf",
    }
)
SCAN_CHUNK_BYTES = 8 * 1024 * 1024


@dataclass(frozen=True)
class Listing:
    """A package's own size and the uncompressed size of each file in it, by POSIX path."""

    size: int
    files: dict[str, int]

    @property
    def contents_size(self) -> int:
        return sum(self.files.values())


@dataclass(frozen=True)
class Baseline:
    """The master package a report compares with, and the run that built it."""

    listing: Listing
    run_url: str
    run_number: int
    head_sha: str


@dataclass(frozen=True)
class Comparison:
    added: dict[str, int]
    removed: dict[str, int]
    changed: dict[str, tuple[int, int]]


def gpl_denylist(config: Path = BUILD_CONFIG) -> list[str]:
    """Return build-config.json's gstreamer_plugin_policy.gpl_denylist, which the CMake install check uses too."""
    policy = json.loads(config.read_text(encoding="utf-8")).get("gstreamer_plugin_policy", {})
    names = policy.get("gpl_denylist") or []
    if not names:
        raise RuntimeError(f"gstreamer_plugin_policy.gpl_denylist not found in {config}")
    return list(names)


def gpl_name_pattern(names: Iterable[str]) -> re.Pattern[str]:
    """Match a file name the way PluginPolicy.cmake's qgc_gst_gpl_files() does."""
    alternatives = "|".join(re.escape(name) for name in names)
    return re.compile(rf"^(lib)?(gst)?({alternatives})([-_.]|$)")


def _run_tool(cmd: list[str], *, cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    result = run_captured(cmd, cwd=cwd, timeout=TOOL_TIMEOUT_SECONDS)
    if result.returncode != 0:
        detail = (result.stderr or result.stdout).strip().splitlines()[-1:] or ["no output"]
        raise RuntimeError(
            f"{Path(cmd[0]).name} failed with exit code {result.returncode}: {detail[0]}"
        )
    return result


def _walk(root: Path) -> dict[str, int]:
    """Return the regular files under *root*; symbolic links are left out."""
    files = {}
    for directory, _, names in os.walk(root):
        for name in names:
            path = Path(directory) / name
            info = path.lstat()
            if stat.S_ISREG(info.st_mode):
                files[path.relative_to(root).as_posix()] = info.st_size
    return files


def _seven_zip() -> str:
    for name in ("7z", "7zz"):
        if found := shutil.which(name):
            return found
    bundled = Path(os.environ.get("PROGRAMFILES", r"C:\Program Files")) / "7-Zip" / "7z.exe"
    if bundled.is_file():
        return str(bundled)
    raise RuntimeError("7-Zip (7z) is not installed")


def parse_7z_listing(output: str) -> dict[str, int]:
    """Return the files of a ``7z l -slt`` listing, whose entries follow a ``----------`` line."""
    _, separator, entries = output.partition("\n----------\n")
    if not separator:
        raise RuntimeError("7z listing has no file entries")
    files = {}
    for block in re.split(r"\n\s*\n", entries):
        fields = {
            key.strip(): value.strip()
            for key, _, value in (line.partition("=") for line in block.splitlines())
        }
        path = fields.get("Path")
        if not path or fields.get("Folder") == "+" or fields.get("Attributes", "").startswith("D"):
            continue
        files[path.replace("\\", "/")] = int(fields.get("Size") or 0)
    return files


def _list_nsis(package: Path) -> dict[str, int]:
    return parse_7z_listing(_run_tool([_seven_zip(), "l", "-slt", str(package)]).stdout)


@contextlib.contextmanager
def _attached_dmg(package: Path, mount_point: Path) -> Iterator[Path]:
    mount_point.mkdir(parents=True)
    _run_tool(
        [
            "hdiutil",
            "attach",
            "-nobrowse",
            "-readonly",
            "-mountpoint",
            str(mount_point),
            str(package),
        ]
    )
    try:
        yield mount_point
    finally:
        detach = run_captured(["hdiutil", "detach", str(mount_point)], timeout=TOOL_TIMEOUT_SECONDS)
        if detach.returncode != 0:
            forced = ["hdiutil", "detach", "-force", str(mount_point)]
            if run_captured(forced, timeout=TOOL_TIMEOUT_SECONDS).returncode != 0:
                gh_warning(f"Failed to detach {mount_point}: {detach.stderr.strip()}")


def _list_dmg(package: Path, work_dir: Path) -> dict[str, int]:
    with _attached_dmg(package, work_dir / "mount") as mount_point:
        return _walk(mount_point)


def _list_appimage(package: Path, work_dir: Path) -> dict[str, int]:
    work_dir.mkdir(parents=True, exist_ok=True)
    package.chmod(package.stat().st_mode | stat.S_IXUSR)
    _run_tool([str(package.resolve()), "--appimage-extract"], cwd=work_dir)
    root = work_dir / "squashfs-root"
    if not root.is_dir():
        raise RuntimeError(f"{package.name} --appimage-extract wrote no squashfs-root")
    files = _walk(root)
    shutil.rmtree(root, ignore_errors=True)
    return files


def _list_zip(package: Path) -> dict[str, int]:
    with zipfile.ZipFile(package) as archive:
        return {info.filename: info.file_size for info in archive.infolist() if not info.is_dir()}


def list_package(package: Path, work_dir: Path) -> Listing:
    """List *package*'s files with its type's tool; *work_dir* holds mounts and extractions."""
    suffix = package.suffix.lower()
    if suffix == ".exe":
        files = _list_nsis(package)
    elif suffix == ".dmg":
        files = _list_dmg(package, work_dir)
    elif suffix == ".appimage":
        files = _list_appimage(package, work_dir)
    elif suffix in ZIP_SUFFIXES:
        files = _list_zip(package)
    else:
        raise RuntimeError(f"Unsupported package type: {package.name}")
    return Listing(size=package.stat().st_size, files=files)


def _find_markers(stream: IO[bytes], data: bytes) -> list[bytes]:
    """Return the GPL markers in *data* followed by the rest of *stream*, read in chunks."""
    overlap = max(len(marker) for marker in GPL_BINARY_MARKERS) - 1
    found: list[bytes] = []
    while data:
        found += [m for m in GPL_BINARY_MARKERS if m not in found and m in data]
        chunk = stream.read(SCAN_CHUNK_BYTES)
        data = data[-overlap:] + chunk if chunk else b""
    return found


def scan_binaries(package: Path) -> list[tuple[str, str]]:
    """Return (member, marker) for each GPL marker in a zip package's ELF and Mach-O members."""
    hits = []
    with zipfile.ZipFile(package) as archive:
        for info in archive.infolist():
            if info.is_dir() or info.file_size < 4:
                continue
            with archive.open(info) as member:
                head = member.read(4)
                if head in BINARY_MAGIC:
                    hits += [
                        (info.filename, marker.decode()) for marker in _find_markers(member, head)
                    ]
    return hits


def gpl_findings(package: Path, listing: Listing, names: Iterable[str]) -> list[tuple[str, str]]:
    """Return (path, reason) for each GPL-licensed file or binary marker in the package."""
    pattern = gpl_name_pattern(names)
    findings = [
        (path, f"named after GPL `{match.group(3)}`")
        for path in sorted(listing.files)
        if (match := pattern.match(path.rsplit("/", 1)[-1]))
    ]
    if package.suffix.lower() in ZIP_SUFFIXES:
        findings += [(path, f"contains `{marker}`") for path, marker in scan_binaries(package)]
    return findings


def compare(current: Listing, baseline: Listing) -> Comparison:
    files, old = current.files, baseline.files
    return Comparison(
        added={path: size for path, size in files.items() if path not in old},
        removed={path: size for path, size in old.items() if path not in files},
        changed={
            path: (old[path], size)
            for path, size in files.items()
            if path in old and old[path] != size
        },
    )


def workflow_file(workflow_ref: str) -> str:
    """Return the file name in a GITHUB_WORKFLOW_REF (owner/repo/.github/workflows/x.yml@ref)."""
    return workflow_ref.split("@", 1)[0].rsplit("/", 1)[-1]


def find_baseline_artifact(
    repo: str, workflow: str, branch: str, artifact_name: str
) -> tuple[dict, dict] | None:
    """Return the newest successful *branch* push run of *workflow* with a live *artifact_name*."""
    query = urlencode({"branch": branch, "status": "success", "per_page": BASELINE_RUNS})
    data = gh_api_json(f"repos/{repo}/actions/workflows/{workflow}/runs?{query}")
    for run in data.get("workflow_runs") or []:
        # A pull request from a fork's own master branch also reports head_branch "master".
        if (
            run.get("event") not in NON_PR_EVENTS
            or run.get("head_branch") != branch
            or (run.get("head_repository") or {}).get("full_name") != repo
        ):
            continue
        for artifact in list_run_artifacts(repo, int(run["id"])):
            if artifact.get("name") == artifact_name and not artifact.get("expired"):
                return run, artifact
    return None


def _extract_package(archive: Path, package_name: str, destination: Path) -> Path:
    with zipfile.ZipFile(archive) as artifact:
        if package_name not in artifact.namelist():
            raise RuntimeError(f"the artifact has no {package_name}")
        destination.mkdir(parents=True, exist_ok=True)
        target = destination / package_name
        with artifact.open(package_name) as source, target.open("wb") as handle:
            shutil.copyfileobj(source, handle, SCAN_CHUNK_BYTES)
    return target


def fetch_baseline(
    repo: str, workflow: str, branch: str, artifact_name: str, package_name: str, work_dir: Path
) -> Baseline | str:
    """Return the baseline listing, or why there is none."""
    try:
        found = find_baseline_artifact(repo, workflow, branch, artifact_name)
        if found is None:
            return f"no successful {branch} run of {workflow} has a live {artifact_name} artifact"
        run, artifact = found
        archive = work_dir / "baseline.zip"
        download_artifact_zip(repo, artifact["id"], archive)
        package = _extract_package(archive, package_name, work_dir / "package")
        archive.unlink()
        listing = list_package(package, work_dir / "contents")
    except (
        subprocess.SubprocessError,
        RuntimeError,
        ValueError,
        OSError,
        zipfile.BadZipFile,
    ) as exc:
        return f"cannot read {branch}'s {artifact_name}: {exc}"
    return Baseline(
        listing=listing,
        run_url=str(run.get("html_url") or ""),
        run_number=int(run.get("run_number") or run["id"]),
        head_sha=str(run.get("head_sha") or ""),
    )


def _count(number: int, noun: str) -> str:
    return f"{format_count(number)} {noun}{'' if number == 1 else 's'}"


def _percent(new: int, old: int) -> float | None:
    return None if old <= 0 else 100.0 * (new - old) / old


def _file_size(size: int, *, signed: bool = False) -> str:
    """Return a size in B, KB, or MB (1024-based, like format_bytes), optionally signed."""
    sign = ("+" if size > 0 else "-") if signed else ""
    size = abs(size) if signed else size
    if size < 1024:
        return f"{sign}{size} B"
    if size < 1024 * 1024:
        return f"{sign}{size / 1024:.1f} KB"
    return f"{sign}{format_bytes(size)}"


def _change(new: int, old: int) -> str:
    if new == old:
        return "unchanged"
    percent = _percent(new, old)
    delta = _file_size(new - old, signed=True)
    return delta if percent is None else f"{delta} ({percent:+.1f}%)"


def _file_rows(files: dict[str, int], limit: int) -> list[tuple[str, str]]:
    ordered = sorted(files.items(), key=lambda item: (-item[1], item[0]))
    return [(code_span(path), _file_size(size)) for path, size in ordered[:limit]]


def _files_table(files: dict[str, int], limit: int) -> str:
    return md_table(
        ["Path", "Size"], _file_rows(files, limit), align=["left", "right"], markdown=True
    )


def _top_files(title: str, files: dict[str, int], top: int) -> str:
    if not files:
        return ""
    trailer = f"\n\n{more_items(len(files) - top)}" if len(files) > top else ""
    return f"**{title}** ({format_count(len(files))})\n\n{_files_table(files, top)}{trailer}"


def _largest_changes(changed: dict[str, tuple[int, int]], top: int, branch: str) -> str:
    if not changed:
        return ""
    ordered = sorted(changed.items(), key=lambda item: (-abs(item[1][1] - item[1][0]), item[0]))
    rows = [
        (code_span(path), _file_size(old), _file_size(new), _change(new, old))
        for path, (old, new) in ordered[:top]
    ]
    table = md_table(
        ["Path", branch, "This build", "Change"],
        rows,
        align=["left", "right", "right", "right"],
        markdown=True,
    )
    return f"**Largest size changes** ({_count(len(changed), 'file')} changed)\n\n{table}"


def _sizes_table(current: Listing, reference: Listing | None, branch: str) -> str:
    rows: list[list[str]] = [
        ["Package", format_bytes(current.size)],
        [
            "Contents",
            f"{format_bytes(current.contents_size)} in {_count(len(current.files), 'file')}",
        ],
    ]
    headers = ["Size", "This build"]
    if reference is not None:
        headers += [branch, "Change"]
        rows[0] += [format_bytes(reference.size), _change(current.size, reference.size)]
        rows[1] += [
            f"{format_bytes(reference.contents_size)} in {_count(len(reference.files), 'file')}",
            _change(current.contents_size, reference.contents_size),
        ]
    return md_table(headers, rows, align=["left", *["right"] * (len(headers) - 1)])


def render_report(
    package_name: str,
    current: Listing,
    baseline: Baseline | str,
    findings: list[tuple[str, str]],
    *,
    branch: str,
    top: int,
    gpl_findings: str = "fail",
) -> str:
    """Return the job summary's Artifact report section."""
    reference = baseline.listing if isinstance(baseline, Baseline) else None
    if isinstance(baseline, Baseline):
        source = f"{branch} run #{baseline.run_number}"
        baseline_fact = f"[{source}]({baseline.run_url})" if baseline.run_url else source
        if baseline.head_sha:
            baseline_fact += f" at {code_span(baseline.head_sha[:9])}"
    else:
        baseline_fact = f"none: {escape_text(baseline)}"
    warn_only = gpl_findings == "warn"
    if not findings:
        result = "passed"
    elif warn_only:
        result = f"passed with {_count(len(findings), 'GPL finding')} (known issue, warnings only)"
    else:
        result = f"failed: {_count(len(findings), 'GPL finding')}"
    facts: list[tuple[str, object]] = [
        ("Result", result),
        ("Package", code_span(package_name)),
        ("Baseline", baseline_fact),
    ]
    sections = [_sizes_table(current, reference, branch)]
    collapsed = []
    if findings:
        rows = [(code_span(path), reason) for path, reason in findings]
        heading = "**GPL-licensed files** (QGC's GStreamer notice covers LGPL only)"
        if warn_only:
            heading = (
                "**GPL-licensed files: known issue, reported as warnings** "
                "(QGC's GStreamer notice covers LGPL only)"
            )
            if note := KNOWN_GPL_ISSUES.get(Path(package_name).suffix.lower()):
                heading += f". {note}."
        sections.insert(0, f"{heading}\n\n" + md_table(["Path", "Finding"], rows, markdown=True))
    if reference is not None:
        diff = compare(current, reference)
        facts.append(
            (
                "Files",
                f"{format_count(len(diff.added))} added, "
                f"{format_count(len(diff.removed))} removed, "
                f"{format_count(len(diff.changed))} changed",
            )
        )
        sections += [
            _top_files("Added files", diff.added, top),
            _top_files("Removed files", diff.removed, top),
            _largest_changes(diff.changed, top, branch),
        ]
        for label, files in (("added", diff.added), ("removed", diff.removed)):
            if len(files) > top:
                collapsed.append(
                    (f"All {len(files)} {label} files", _files_table(files, len(files)))
                )
    return summary_section(
        "Artifact report",
        facts,
        body="\n\n".join(section for section in sections if section),
        collapsed=collapsed,
    )


def warn_size_changes(
    package_name: str, current: Listing, baseline: Listing, threshold: float, branch: str
) -> None:
    """Warn when the package or its contents grew or shrank by more than *threshold* percent."""
    for label, new, old in (
        ("package", current.size, baseline.size),
        ("contents", current.contents_size, baseline.contents_size),
    ):
        percent = _percent(new, old)
        if percent is not None and abs(percent) > threshold:
            gh_warning(
                f"{package_name} {label} size changed {percent:+.1f}% vs {branch} "
                f"({format_bytes(old)} -> {format_bytes(new)}), "
                f"beyond the ±{threshold:g}% threshold",
                title="Artifact size change",
            )


def report_gpl_findings(package_name: str, findings: list[tuple[str, str]], mode: str) -> None:
    """Annotate the GPL findings as errors, or as warnings naming the known issue in warn mode."""
    annotate = gh_warning if mode == "warn" else gh_error
    note = KNOWN_GPL_ISSUES.get(Path(package_name).suffix.lower(), "") if mode == "warn" else ""
    title = "GPL check (known issue)" if mode == "warn" else "GPL check"
    for path, reason in findings[:MAX_GPL_ANNOTATIONS]:
        message = f"{package_name} ships GPL-licensed code: {path} ({reason})"
        if mode == "warn":
            message += f". Known issue: {note}" if note else ", reported as a warning"
        annotate(message.replace("`", ""), title=title)
    if len(findings) > MAX_GPL_ANNOTATIONS:
        annotate(
            f"{more_items(len(findings) - MAX_GPL_ANNOTATIONS)} GPL findings in {package_name}",
            title=title,
        )


def _gpl_findings_mode(value: str) -> str:
    if value not in GPL_FINDINGS_MODES:
        raise argparse.ArgumentTypeError(f"must be one of {', '.join(GPL_FINDINGS_MODES)}: {value}")
    return value


def _positive_float(value: str) -> float:
    number = float(value)
    if number <= 0:
        raise argparse.ArgumentTypeError(f"must be positive: {value}")
    return number


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", default=env_default("INPUT_PACKAGE"), help="Package to report")
    parser.add_argument(
        "--artifact-name",
        default=env_default("INPUT_ARTIFACT_NAME"),
        help="GitHub artifact the package is uploaded as; the baseline is master's of that name",
    )
    parser.add_argument(
        "--size-threshold",
        type=_positive_float,
        default=env_default("INPUT_SIZE_THRESHOLD", str(DEFAULT_THRESHOLD_PERCENT)),
        help="Size change in percent above which the report warns (default: %(default)s)",
    )
    parser.add_argument(
        "--gpl-findings",
        type=_gpl_findings_mode,
        default=env_default("INPUT_GPL_FINDINGS", "fail"),
        help="fail on GPL findings, or warn for a known issue (default: %(default)s)",
    )
    parser.add_argument("--top", type=int, default=DEFAULT_TOP, help="Files listed per table")
    parser.add_argument("--repo", default=env_default("GITHUB_REPOSITORY"), help="owner/repo")
    parser.add_argument(
        "--workflow",
        default=workflow_file(env_default("GITHUB_WORKFLOW_REF")),
        help="Workflow file whose runs hold the baseline (default: this run's)",
    )
    parser.add_argument("--branch", default="master", help="Baseline branch (default: %(default)s)")
    args = parser.parse_args(argv)
    if not args.package or not args.artifact_name:
        parser.error(
            "--package and --artifact-name (or INPUT_PACKAGE, INPUT_ARTIFACT_NAME) are required"
        )
    package = Path(args.package)
    if not package.is_file() or not package.name.lower().endswith(PACKAGE_SUFFIXES):
        gh_error(f"Package not found or unsupported: {package}", title="Artifact report")
        return 1

    with tempfile.TemporaryDirectory(
        prefix="artifact-report-", dir=os.environ.get("RUNNER_TEMP"), ignore_cleanup_errors=True
    ) as temp:
        work_dir = Path(temp)
        try:
            current = list_package(package, work_dir / "current")
        except (subprocess.SubprocessError, RuntimeError, OSError, zipfile.BadZipFile) as exc:
            gh_error(
                f"Cannot list {package.name}, so its GPL check did not run: {exc}",
                title="Artifact report",
            )
            return 1
        print(
            f"{package.name}: {_count(len(current.files), 'file')}, "
            f"{format_bytes(current.contents_size)}"
        )
        findings = gpl_findings(package, current, gpl_denylist())

        if args.repo and args.workflow:
            baseline = fetch_baseline(
                args.repo,
                args.workflow,
                args.branch,
                args.artifact_name,
                package.name,
                work_dir / "baseline",
            )
        else:
            baseline = (
                "the repository or workflow is unknown (GITHUB_REPOSITORY, GITHUB_WORKFLOW_REF)"
            )

    if isinstance(baseline, Baseline):
        print(f"Baseline: {args.branch} run #{baseline.run_number} {baseline.run_url}")
        warn_size_changes(package.name, current, baseline.listing, args.size_threshold, args.branch)
    else:
        print(f"No baseline: {baseline}")
    write_step_summary(
        render_report(
            package.name,
            current,
            baseline,
            findings,
            branch=args.branch,
            top=args.top,
            gpl_findings=args.gpl_findings,
        )
    )
    report_gpl_findings(package.name, findings, args.gpl_findings)
    return 1 if findings and args.gpl_findings == "fail" else 0


if __name__ == "__main__":
    raise SystemExit(main())
