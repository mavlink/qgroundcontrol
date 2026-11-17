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

# Paths that no platform build runs or reads. They are checked first, so a path they do not name
# keeps triggering whatever the build patterns below say: when unsure, leave a path out.
# test_detect_changes.py fails if a script that a platform workflow runs or imports is listed.
_NOT_BUILD_INPUTS: list[str] = [
    r"\.md$",
    # Developer, lint, documentation and maintenance entry points.
    r"^tools/(analyze|build_profile|check_deps|ci_script_index|clean|configure|pre_commit"
    r"|terrain_tiles|update_px4_metadata|validate_cmake_presets|validate_configs)\.py$",
    r"^tools/(analyzers|coding-style|simulation|terrain)/",
    # CMake's memcheck setup reads the Valgrind suppressions.
    r"^tools/debuggers/(?!valgrind\.supp$)",
    r"^tools/translations/check_placeholders\.py$",
    # Scripts of other workflows: analysis, cache cleanup, docs, Pages, publishing, releases,
    # pull request reports, runner images and VM builds.
    r"^tools/ci/cache/gh_cache_cleanup\.py$",
    r"^tools/ci/gstreamer/",
    r"^tools/ci/planning/analysis_matrix\.py$",
    r"^tools/ci/publish/(aws_upload|daily_release|deploy_docs|pages_\w+|publish_\w+"
    r"|release_assets|release_branch_check|release_notes)\.py$",
    r"^tools/ci/reporting/(analysis_review|collect_build_status|download_artifacts"
    r"|generate_build_results_comment|gh_pr_size_label|report_context|workflow_runs)\.py$",
    r"^tools/ci/runner/(analysis_tools|apt_debs_artifact|install_multipass|runner_image"
    r"|validate_runner_configs)\.py$",
]
_NOT_BUILD_INPUT_RES = [re.compile(pattern) for pattern in _NOT_BUILD_INPUTS]

# The locked Python environment, which every job installs the same way. A change to these files
# alone (Renovate's lock updates) builds Linux only, as the canary for the other platforms.
PYTHON_ENVIRONMENT_FILES = ("tools/pyproject.toml", "tools/uv.lock")

# Application sources, resources and tests: every native platform builds them. The Docker builder
# images do not depend on them, so Docker builds only for the CMake files among them.
_SOURCE_PATTERNS: list[str] = [
    r"^src/",
    r"^resources/",
    r"^translations/",
    r"^.*\.qrc$",
    r"^test/",
]
_SOURCE_CMAKE_PATTERN = r"^(src|resources|translations|test)/(.*/)?(CMakeLists\.txt|[^/]+\.cmake)$"

# Patterns that trigger a build for ANY platform
_COMMON_PATTERNS: list[str] = [
    r"^CMakeLists\.txt$",
    r"^CMakePresets\.json$",
    r"^cmake/",
    r"^libs/",
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
        r"^deploy/docker/install_android_sdk\.sh$",
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
    raw = [_SOURCE_CMAKE_PATTERN] if platform.startswith("docker-") else list(_SOURCE_PATTERNS)
    raw.extend(_COMMON_PATTERNS)
    raw.append(rf"^deploy/{re.escape(platform)}/")
    raw.append(rf"^\.github/workflows/{re.escape(wf)}\.yml$")
    raw.extend(_PLATFORM_PATTERNS.get(platform, []))
    raw.extend(_SETUP_PATTERNS.get(platform, []))
    if platform.startswith("docker-"):
        raw.append(
            r"^deploy/docker/(_variants\.py|run_docker\.py|variants\.json|docker_helper\.py)$"
        )
    return [re.compile(p) for p in raw]


def is_build_input(path: str) -> bool:
    """Whether *path* can affect a build: False only for paths that _NOT_BUILD_INPUTS names."""
    return bool(path) and not any(pattern.search(path) for pattern in _NOT_BUILD_INPUT_RES)


def has_relevant_changes(files: Sequence[str], platform: str) -> bool:
    """Check if any changed file is a build input that matches the platform's patterns."""
    patterns = build_patterns(platform)
    for f in files:
        if not is_build_input(f):
            continue
        if f in PYTHON_ENVIRONMENT_FILES and platform != "linux":
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
