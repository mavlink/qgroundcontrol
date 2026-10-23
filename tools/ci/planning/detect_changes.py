"""Decide which platforms the current GitHub event builds, for plan_platform.py.

Pull requests build a platform only when its inputs changed (or the diff is
unknown). Tag refs build only release tags reachable from a Stable* branch;
every other event (push, merge_group, workflow_dispatch) builds everything.
"""

from __future__ import annotations

import os
import re
from dataclasses import dataclass
from typing import TYPE_CHECKING

from common.gh_actions import gh_warning
from common.git import pr_diff_refs, run_git
from qgc_tools.release_tag_check import check_release_tag

if TYPE_CHECKING:
    from collections.abc import Sequence

PLATFORMS = (
    "linux",
    "windows",
    "macos",
    "ios",
    "android",
    "docker-linux",
    "docker-android",
)

# Patterns that trigger a build for ANY platform
_COMMON_PATTERNS: list[str] = [
    r"^src/",
    r"^CMakeLists\.txt$",
    r"^CMakePresets\.json$",
    r"^cmake/",
    r"^libs/",
    r"^resources/",
    r"^translations/",
    r"^.*\.qrc$",
    r"^test/",
    r"^\.github/actions/",
    r"^tools/ci/",
    r"^\.github/build-config\.json$",
    r"^\.github/workflows/_detect-changes\.yml$",
    # tools/configs otherwise holds lint, docs, and release settings that never reach a build.
    r"^tools/(?!setup/|tests/|configs/(?!ccache\.conf$))",
    r"^tools/setup/(install_qt\.py|qt_installer/|install_python\.py|read_config\.py|install_dependencies/)",
]

# Per-platform additional patterns
_PLATFORM_PATTERNS: dict[str, list[str]] = {
    "android": [r"^android/"],
    # linux.yml's custom-plugin job builds the custom-example overlay.
    "linux": [r"^custom-example/"],
    "docker-linux": [
        # Single multi-stage Dockerfile builds every variant; any change rebuilds.
        r"^deploy/docker/Dockerfile$",
        r"^deploy/docker/entrypoint\.(sh|py)$",
        r"^deploy/docker/install_sysroot_aarch64\.py$",
        r"^deploy/docker/run-docker\.sh$",
        r"^deploy/docker/run_docker\.py$",
        r"^deploy/docker/lib/",
        r"^deploy/linux/",
    ],
    "docker-android": [
        r"^deploy/docker/Dockerfile$",
        r"^deploy/docker/entrypoint\.(sh|py)$",
        r"^deploy/docker/run_docker\.py$",
        r"^deploy/docker/run-docker\.sh$",
        r"^deploy/docker/lib/",
        r"^android/",
        r"^deploy/android/",
    ],
}

_SETUP_PATTERNS: dict[str, list[str]] = {
    "linux": [r"^tools/setup/.*debian", r"^tools/setup/install_dependencies/"],
    "windows": [r"^tools/setup/.*windows"],
    "macos": [r"^tools/setup/.*macos"],
    "android": [r"^tools/setup/"],
    "ios": [r"^tools/setup/.*ios", r"^tools/setup/.*macos"],
    "docker-linux": [r"^tools/setup/"],
    "docker-android": [r"^tools/setup/"],
}


def workflow_name_for_platform(platform: str) -> str:
    """Map a platform to its workflow YAML filename (without extension)."""
    if platform.startswith("docker-"):
        return "docker"
    return platform


def build_patterns(platform: str) -> list[re.Pattern[str]]:
    """Build the list of compiled regex patterns for a platform."""
    wf = workflow_name_for_platform(platform)
    raw: list[str] = list(_COMMON_PATTERNS)
    raw.append(rf"^deploy/{re.escape(platform)}/")
    raw.append(rf"^\.github/workflows/{re.escape(wf)}\.yml$")
    raw.extend(_PLATFORM_PATTERNS.get(platform, []))
    raw.extend(_SETUP_PATTERNS.get(platform, []))
    if platform.startswith("docker-"):
        raw.append(
            r"^deploy/docker/(_variants\.py|run_docker\.py|variants\.json|docker_helper\.py)$"
        )
    return [re.compile(p) for p in raw]


def has_relevant_changes(files: Sequence[str], platform: str) -> bool:
    """Check if any changed file matches the platform's patterns."""
    patterns = build_patterns(platform)
    for f in files:
        if not f:
            continue
        for p in patterns:
            if p.search(f):
                return True
    return False


# ---------------------------------------------------------------------------
# Git helpers — these call out to git as subprocesses
# ---------------------------------------------------------------------------


def get_changed_files() -> list[str] | None:
    """Return the pull request's changed files, or None when they cannot be computed.

    The merge checkout is compared with its first parent (``common.git.pr_diff_refs``), so a PR
    that is behind its base branch does not count the base branch's newer changes. Renames are
    listed as both paths: with rename detection a file moved out of a watched directory would
    only report its new, possibly unwatched, path.
    """
    refs = pr_diff_refs(os.environ.get("PR_BASE_SHA"), os.environ.get("PR_HEAD_SHA"))
    if refs is None:
        return None
    r = run_git("diff", "--no-renames", "--name-only", f"{refs[0]}...{refs[1]}")
    if r.returncode != 0:
        return None
    return r.stdout.strip().splitlines()


@dataclass(frozen=True)
class Detection:
    builds: dict[str, bool]
    # The pull-request diff; None for other events or when it can't be computed.
    changed_files: list[str] | None = None


def detect_builds(platforms: Sequence[str]) -> Detection:
    """Decide which *platforms* the current event (``EVENT_NAME``) builds.

    Raises GitQueryError when git cannot tell whether a tag is on a Stable* branch.
    """
    if os.environ.get("EVENT_NAME") == "pull_request":
        changed = get_changed_files()
        if changed is None:
            gh_warning("Unable to compute changed files reliably; forcing build for safety.")
            return Detection(dict.fromkeys(platforms, True))
        print("Changed files:")
        for path in changed:
            print(f"  {path}")
        return Detection({p: has_relevant_changes(changed, p) for p in platforms}, changed)

    # Release tags are cut on Stable* branches only; any other tag must not reach the release
    # pipeline (stable builds, S3 latest upload, store and registry pushes).
    if os.environ.get("GITHUB_REF_TYPE") == "tag":
        release_tag = check_release_tag(os.environ.get("GITHUB_REF_NAME", "HEAD"), "skipping build")
        return Detection(dict.fromkeys(platforms, release_tag))
    return Detection(dict.fromkeys(platforms, True))
