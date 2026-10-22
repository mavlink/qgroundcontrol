#!/usr/bin/env python3
"""Plan the cache action: save policy, cache scope, keys, and job environment.

`.github/actions/cache` runs this once before restoring anything. It passes its
inputs as INPUT_* variables, plus PR_NUMBER and the hashFiles() digests that key
the caches (COMPILER_CACHE_HASH, MOC_CACHE_HASH, GST_SDK_FILES_HASH). Keys and decisions
go to GITHUB_OUTPUT; the compiler, moc and CPM cache settings and the save keys
read by `save-build-cache` go to GITHUB_ENV.

QGC_COMPILER_CACHE=sccache (the RunsOn sccache pilot) selects sccache when runs-on/action
has exported its S3 bucket: the ccache archive is neither restored nor saved
(QGC_CCACHE_KEY stays empty) and sccache reads and writes S3 objects directly.
"""

from __future__ import annotations

import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import append_github_env, gh_warning, write_github_output

from ci.cache.build_cache_artifact import artifact_name
from ci.cache.cpm_helper import (
    compute_cpm_fingerprint,
    compute_gstreamer_fingerprint,
    prepare_cpm_cache,
)

if TYPE_CHECKING:
    from collections.abc import Mapping

# Events whose compiler/moc snapshots build_cache_artifact.py can match to a trusted producer.
ARTIFACT_EVENTS = frozenset({"pull_request", "push", "schedule", "workflow_dispatch"})
PULL_REQUEST_EVENTS = frozenset({"pull_request", "pull_request_target"})
CCACHE_CONF = Path("tools/configs/ccache.conf")
DEFAULT_CCACHE_MAX_SIZE = "2G"


@dataclass(frozen=True)
class CacheInputs:
    """The cache action's inputs and the workflow context that selects its caches."""

    host: str
    target: str
    build_type: str
    variant: str = ""
    key_suffix: str = ""
    cpm_modules: str = ""
    save_cache: str = "auto"
    pr_save_min_misses: str = ""
    event: str = ""
    ref_name: str = ""
    pr_number: str = ""
    backend: str = "github"
    run_id: str = ""
    run_attempt: str = ""
    ccache_hash: str = ""
    moccache_hash: str = ""
    gst_files_hash: str = ""
    compiler_cache: str = ""
    sccache_bucket: str = ""
    runner_temp: str = ""

    @classmethod
    def from_env(cls, env: Mapping[str, str]) -> CacheInputs:
        return cls(
            host=env.get("INPUT_HOST", ""),
            target=env.get("INPUT_TARGET", ""),
            build_type=env.get("INPUT_BUILD_TYPE", ""),
            variant=env.get("INPUT_VARIANT", ""),
            key_suffix=env.get("INPUT_KEY_SUFFIX", ""),
            cpm_modules=env.get("INPUT_CPM_MODULES", ""),
            save_cache=env.get("INPUT_SAVE_CACHE", "auto"),
            pr_save_min_misses=env.get("INPUT_PR_SAVE_MIN_MISSES", ""),
            event=env.get("GITHUB_EVENT_NAME", ""),
            ref_name=env.get("GITHUB_REF_NAME", ""),
            pr_number=env.get("PR_NUMBER", ""),
            backend=env.get("QGC_ACTIONS_CACHE_BACKEND", "github"),
            run_id=env.get("GITHUB_RUN_ID", ""),
            run_attempt=env.get("GITHUB_RUN_ATTEMPT", ""),
            ccache_hash=env.get("COMPILER_CACHE_HASH", ""),
            moccache_hash=env.get("MOC_CACHE_HASH", ""),
            gst_files_hash=env.get("GST_SDK_FILES_HASH", ""),
            compiler_cache=env.get("QGC_COMPILER_CACHE", ""),
            sccache_bucket=env.get("SCCACHE_BUCKET", ""),
            runner_temp=env.get("RUNNER_TEMP", ""),
        )


@dataclass(frozen=True)
class CachePlan:
    outputs: dict[str, str]
    env: dict[str, str]


def save_policy(requested: str, event: str) -> str:
    """Resolve `auto`: pull_request_target caches would land in the base branch's scope."""
    if requested != "auto":
        return requested
    return "false" if event == "pull_request_target" else "true"


def scoped_save_policy(requested: str, save: str, event: str, backend: str) -> str:
    """Pull requests on GitHub storage restore the default branch's compiler, moc and CPM caches.

    Saving their own copies cost upload time and, for CPM, evicted shared entries
    from GitHub's size-limited cache. S3 backends keep saving per-PR copies.
    """
    if requested == "auto" and save == "true" and event == "pull_request" and backend != "s3":
        return "false"
    return save


def _same(value: str, expected: str) -> bool:
    # These decisions replaced action `if:` conditions, which compare strings case-insensitively.
    return value.lower() == expected.lower()


def _restore_keys(*prefixes: str) -> str:
    return "\n".join(prefix for prefix in prefixes if prefix)


def determine_cache_scope(event_name: str, ref_name: str, pr_number: str = "") -> str:
    """Return the normalized cache scope used by CI."""
    scope = "shared"
    if event_name == "pull_request":
        scope = f"pr-{pr_number or 'unknown'}"
    elif ref_name == "master" and event_name in {"push", "schedule", "workflow_dispatch"}:
        scope = "shared"
    elif event_name == "workflow_dispatch":
        scope = f"manual-{ref_name}"
    elif event_name == "push":
        if ref_name != "master":
            scope = f"branch-{ref_name}"
    else:
        scope = f"{event_name}-{ref_name}"
    return scope.replace("/", "-")


def read_max_size(config: Path) -> str:
    """Return ``max_size`` from ccache.conf, or the 2G default when it is unreadable or unset."""
    try:
        text = config.read_text(encoding="utf-8")
    except OSError as error:
        print(f"Warning: cannot read {config}, using default max_size: {error}", file=sys.stderr)
        return DEFAULT_CCACHE_MAX_SIZE
    for line in text.splitlines():
        key, separator, value = line.partition("=")
        if separator and key.strip() == "max_size" and value.strip():
            return value.strip()
    return DEFAULT_CCACHE_MAX_SIZE


def ci_cache_budget(host: str, build_type: str, variant: str, backend: str, s3_size: str) -> str:
    """Keep large object sets resident without enlarging every GitHub archive."""
    if backend == "s3":
        return s3_size
    if host.startswith("windows") or variant in {"coverage", "sanitizers", "asan", "ubsan", "tsan"}:
        return "3G"
    if build_type == "Debug" or host in {"mac", "macos"}:
        return "2G"
    return "1G"


def ccache_environment(workspace: Path, max_size: str) -> dict[str, str]:
    """Return the standard ccache environment variables for a CI workspace."""
    workspace_path = Path(str(workspace).replace("\\", "/"))
    return {
        "CCACHE_DIR": (workspace_path / ".ccache").as_posix(),
        "CCACHE_BASEDIR": workspace_path.as_posix(),
        "CCACHE_CONFIGPATH": (workspace_path / CCACHE_CONF).as_posix(),
        "CCACHE_MAXSIZE": max_size,
        "CCACHE_NOFILECLONE": "1",
    }


def compiler_cache_tool(requested: str, sccache_bucket: str) -> str:
    """Return the compiler cache for this job: ccache unless the sccache pilot can reach S3.

    An ephemeral runner's local sccache cache starts empty, so without runs-on/action's
    bucket the pilot would lose the ccache archive and gain nothing.
    """
    if not _same(requested.strip(), "sccache"):
        return "ccache"
    if not sccache_bucket:
        gh_warning(
            "QGC_COMPILER_CACHE=sccache needs the RunsOn S3 bucket that runs-on/action exports "
            "with `sccache: s3`; using ccache"
        )
        return "ccache"
    return "sccache"


def sccache_environment(workspace: Path, temp: Path, *, writable: bool) -> dict[str, str]:
    """Return the sccache server settings; runs-on/action already exported the S3 location.

    Every branch reads and writes the same S3 prefix, so pull requests only read it.
    """
    return {
        "SCCACHE_BASEDIRS": workspace.absolute().as_posix(),
        # The default 10-minute idle exit would restart the server with zeroed statistics.
        "SCCACHE_IDLE_TIMEOUT": "0",
        "SCCACHE_ERROR_LOG": (temp / "sccache.log").absolute().as_posix(),
        "SCCACHE_LOG": "warn",
        "SCCACHE_S3_RW_MODE": "READ_WRITE" if writable else "READ_ONLY",
    }


def plan_caches(inputs: CacheInputs, workspace: Path) -> CachePlan:
    """Resolve every key and decision, creating the compiler and CPM cache directories."""
    host, target, build_type = inputs.host, inputs.target, inputs.build_type
    scope = determine_cache_scope(inputs.event, inputs.ref_name, inputs.pr_number)
    save = save_policy(inputs.save_cache, inputs.event)
    scoped_save = scoped_save_policy(inputs.save_cache, save, inputs.event, inputs.backend)
    release_save = _same(save, "true") and _same(build_type, "Release")
    generation = f"{inputs.run_id}-{inputs.run_attempt}"

    variant_suffix = f"-{inputs.variant}" if inputs.variant else ""
    key_suffix = f"-{inputs.key_suffix}" if inputs.key_suffix else ""
    ccache_base = f"ccache-{host}-{target}-{build_type}{variant_suffix}"
    ccache_leg = f"{ccache_base}{key_suffix}"
    ccache_key = f"{ccache_leg}-{scope}-{inputs.ccache_hash}"
    moccache_base = f"moccache-{host}-{target}-{scope}"
    moccache_leg = f"{build_type}{variant_suffix}{key_suffix}"

    outputs = {
        "ccache_key": f"{ccache_key}-{generation}",
        "ccache_restore_keys": _restore_keys(
            f"{ccache_key}-",
            f"{ccache_leg}-{scope}-",
            f"{ccache_base}-{scope}-",
            f"{ccache_leg}-shared-",
            f"{ccache_base}-shared-",
            f"ccache-{host}-{target}-{build_type}-shared-" if inputs.variant else "",
        ),
        "moccache_key": f"{moccache_base}-{inputs.moccache_hash}-{moccache_leg}-{generation}",
        "moccache_restore_keys": _restore_keys(
            f"{moccache_base}-{inputs.moccache_hash}-",
            f"{moccache_base}-",
            f"moccache-{host}-{target}-shared-",
        ),
        "gst_mode": "",
        "cpm_mode": "",
        "cpm_shared_probe": "false",
    }

    tool = compiler_cache_tool(inputs.compiler_cache, inputs.sccache_bucket)
    if tool == "sccache":
        # sccache reads and writes S3 objects itself: no ccache archive is restored or saved.
        outputs.update(ccache_key="", ccache_restore_keys="")
        environment = sccache_environment(
            workspace,
            Path(inputs.runner_temp or workspace),
            writable=_same(save, "true") and inputs.event not in PULL_REQUEST_EVENTS,
        )
    else:
        environment = ccache_environment(
            workspace,
            ci_cache_budget(
                host,
                build_type,
                inputs.variant,
                inputs.backend,
                read_max_size(workspace / CCACHE_CONF),
            ),
        )
        Path(environment["CCACHE_DIR"]).mkdir(parents=True, exist_ok=True)
    environment["QGC_COMPILER_CACHE_TOOL"] = tool

    if inputs.cpm_modules:
        root = workspace.resolve()
        environment.update(prepare_cpm_cache(inputs.cpm_modules, workspace))
        fingerprint = compute_cpm_fingerprint(root)
        cpm_base = f"cpm-sources-v2-{host}-{target}{key_suffix}"
        outputs["cpm_mode"] = (
            "save" if _same(scoped_save, "true") and _same(build_type, "Release") else "restore"
        )
        # PRs and branches restore an identical default-branch snapshot instead of saving a copy.
        outputs["cpm_shared_probe"] = str(release_save and not _same(scope, "shared")).lower()
        outputs["cpm_key"] = f"{cpm_base}-{scope}-{fingerprint}"
        outputs["cpm_shared_key"] = f"{cpm_base}-shared-{fingerprint}"
        outputs["cpm_restore_keys"] = _restore_keys(
            f"{cpm_base}-{scope}-",
            f"{cpm_base}-shared-",
            f"cpm-sources-v2-{host}-{target}-{scope}-",
            f"cpm-sources-v2-{host}-{target}-shared-",
        )
        # Desktop Linux and macOS builds use the system GStreamer.
        if target.lower().startswith("win") or _same(target, "android") or _same(target, "ios"):
            outputs["gst_mode"] = "save" if release_save else "restore"
            outputs["gst_key"] = (
                f"gst-sdk-v1-{host}-{target}-{compute_gstreamer_fingerprint(root)}"
                f"-{inputs.gst_files_hash}"
            )

    # Snapshot names leave out the config hashes in the dependency-cache keys: artifacts are
    # found by exact name, and ccache and moccache entries are content-addressed.
    environment.update(
        {
            "QGC_BUILD_CACHE_ARTIFACT": str(
                tool == "ccache" and inputs.backend != "s3" and inputs.event in ARTIFACT_EVENTS
            ).lower(),
            "QGC_BUILD_CACHE_ARTIFACT_NAME": artifact_name(
                f"{ccache_leg}-{scope}", f"{moccache_base}-{moccache_leg}"
            ),
            "QGC_BUILD_CACHE_SHARED_ARTIFACT": artifact_name(
                f"{ccache_leg}-shared", f"moccache-{host}-{target}-shared-{moccache_leg}"
            ),
            "MOCCACHE_STATS": "1",
            "QGC_SAVE_BUILD_CACHE": scoped_save,
            "QGC_CCACHE_KEY": outputs["ccache_key"],
            "QGC_MOCCACHE_KEY": outputs["moccache_key"],
            "QGC_PR_CACHE_SAVE_MIN_MISSES": inputs.pr_save_min_misses
            if re.fullmatch(r"[0-9]+", inputs.pr_save_min_misses)
            else "0",
        }
    )
    for name, value in environment.items():
        if "\n" in value or "\r" in value:
            raise ValueError(f"{name} must be a single line: {value!r}")
    return CachePlan(outputs, environment)


def main() -> int:
    plan = plan_caches(
        CacheInputs.from_env(os.environ), Path(os.environ.get("GITHUB_WORKSPACE", "."))
    )
    for name, value in {**plan.outputs, **plan.env}.items():
        print(f"{name}={value}".replace("\n", " | "))
    write_github_output(plan.outputs)
    append_github_env(plan.env)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
