#!/usr/bin/env python3
"""Draft release notes for a ``vX.Y.Z`` tag with git-cliff (``tools/configs/cliff.toml``).

The notes cover the commits since the previous ``vX.Y.Z`` tag reachable from the tag (all history
for a first release). A tag that does not exist yet drafts the unreleased commits since the last
release reachable from ``HEAD``.
``--draft`` creates a draft GitHub Release with the notes unless a release for the tag already
exists; an existing release is never modified. Release assets are uploaded by hand.

Usage:
    python3 tools/ci/publish/release_notes.py v5.1.5
    python3 tools/ci/publish/release_notes.py "$RELEASE_TAG" --output notes.md --draft  # CI
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import gh, gh_error, gh_notice, write_step_summary
from common.git import run_git
from common.markdown import code_span, summary_section
from common.paths import TOOLS_DIR
from common.proc import run_captured
from qgc_tools.release_tag_check import is_release_tag

CLIFF_CONFIG = TOOLS_DIR / "configs" / "cliff.toml"


def _version(tag: str) -> tuple[int, ...]:
    return tuple(int(part) for part in tag[1:].split("."))


def tag_exists(tag: str) -> bool:
    return (
        run_git("rev-parse", "--verify", "--quiet", f"refs/tags/{tag}^{{commit}}").returncode == 0
    )


def previous_release_tag(tag: str, end: str) -> str | None:
    """Return the highest release tag below *tag* that is reachable from *end*."""
    result = run_git("tag", "--merged", end, "--list", "v*")
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or f"git tag exited {result.returncode}")
    candidates = [
        name
        for name in result.stdout.split()
        if is_release_tag(name) and _version(name) < _version(tag)
    ]
    return max(candidates, key=_version, default=None)


def find_git_cliff() -> str | None:
    """Prefer the git-cliff installed beside this interpreter (tools/.venv), then PATH."""
    return shutil.which("git-cliff", path=str(Path(sys.executable).parent)) or shutil.which(
        "git-cliff"
    )


def cliff_command(git_cliff: str, tag: str, previous: str | None, *, released: bool) -> list[str]:
    end = tag if released else "HEAD"
    # Only this tag ends a release, so the range renders as one section even when it contains
    # other tags; an unreleased range has no matching tag and takes --tag's name.
    command = [git_cliff, "--config", str(CLIFF_CONFIG), "--tag-pattern", f"^{re.escape(tag)}$"]
    if not released:
        command += ["--unreleased", "--tag", tag]
    if previous:
        command.append(f"{previous}..{end}")
    elif released:
        # git-cliff takes only A..B ranges; --latest covers the history up to the only release.
        command.append("--latest")
    return command


def generate_notes(tag: str, *, released: bool) -> str:
    git_cliff = find_git_cliff()
    if not git_cliff:
        raise RuntimeError(
            "git-cliff is not installed; run `python3 tools/setup/install_python.py release`"
        )
    previous = previous_release_tag(tag, tag if released else "HEAD")
    result = run_captured(cliff_command(git_cliff, tag, previous, released=released))
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or f"git-cliff exited {result.returncode}")
    notes = result.stdout.strip()
    # git-cliff renders a bare heading, or nothing, when every commit was skipped.
    if not any(line.startswith("- ") for line in notes.splitlines()):
        notes = f"## Changes in {tag}\n\nNo notable changes."
    return f"{notes}\n"


def draft_release(tag: str, notes_file: Path) -> int:
    """Create a draft release for *tag* unless one exists; never modify an existing release."""
    existing = gh("release", "view", tag, "--json", "isDraft,url", check=False)
    if existing.returncode == 0:
        release = json.loads(existing.stdout)
        state = "draft" if release.get("isDraft") else "published"
        gh_notice(f"A {state} {tag} release already exists; left unchanged: {release.get('url')}")
        return 0
    if "release not found" not in existing.stderr:
        gh_error(f"Cannot check for an existing {tag} release: {existing.stderr.strip()}")
        return 1
    created = gh(
        "release",
        "create",
        tag,
        "--draft",
        "--verify-tag",
        "--title",
        tag,
        "--notes-file",
        str(notes_file),
        check=False,
    )
    if created.returncode != 0:
        gh_error(f"Cannot create the draft {tag} release: {created.stderr.strip()}")
        return 1
    print(f"Created draft release {tag}: {created.stdout.strip()}")
    return 0


def notes_summary(tag: str, notes: str, *, released: bool) -> str:
    """Return the job summary's Release notes section; note headings sit below its title."""
    demoted = re.sub(r"^(#{1,4}) ", r"\1## ", notes, flags=re.MULTILINE)
    return summary_section(
        "Release notes",
        [
            ("Tag", code_span(tag)),
            (
                "Tag pushed",
                "yes" if released else "no: previews the commits since the last release",
            ),
        ],
        body=demoted,
    )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("tag", help="Release tag (vX.Y.Z); need not exist yet without --draft")
    parser.add_argument("--output", type=Path, help="Also write the notes to this file")
    parser.add_argument(
        "--draft",
        action="store_true",
        help="Create a draft GitHub Release from --output unless one exists (needs gh auth)",
    )
    args = parser.parse_args(argv)
    if args.draft and args.output is None:
        parser.error("--draft requires --output")
    if not is_release_tag(args.tag):
        gh_error(f"{args.tag} is not a vX.Y.Z release tag")
        return 1
    released = tag_exists(args.tag)
    if args.draft and not released:
        gh_error(f"Tag {args.tag} does not exist; push it before drafting its release")
        return 1
    try:
        notes = generate_notes(args.tag, released=released)
    except RuntimeError as exc:
        gh_error(f"Cannot generate release notes for {args.tag}: {exc}")
        return 1

    sys.stdout.write(notes)
    write_step_summary(notes_summary(args.tag, notes, released=released))
    output: Path | None = args.output
    if output is None:
        return 0
    output.write_text(notes, encoding="utf-8")
    return draft_release(args.tag, output) if args.draft else 0


if __name__ == "__main__":
    raise SystemExit(main())
