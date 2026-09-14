#!/usr/bin/env python3
"""Retain compiler caches outside GitHub's shared dependency-cache eviction pool."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import tarfile
import tempfile
from pathlib import Path, PurePosixPath
from typing import Any

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import append_github_env, gh, gh_warning, write_github_output
from common.io import extract_tar_data

CACHE_ROOTS = (".ccache", ".cache/moccache")


def artifact_name(compiler_key: str, moc_key: str) -> str:
    """Keys include platform, configuration, compiler identity, and branch scope."""
    digest = hashlib.sha256(f"{compiler_key}\n{moc_key}".encode()).hexdigest()
    return f"compiler-cache-v1-{digest}"


def compatible_run(current: dict[str, Any], candidate: dict[str, Any]) -> bool:
    """An artifact name alone is insufficient to establish a trusted producer."""
    if candidate.get("id") == current.get("id"):
        return False
    for field in ("workflow_id", "head_branch"):
        if not current.get(field) or candidate.get(field) != current[field]:
            return False
    repository_id = current.get("head_repository", {}).get("id")
    if not repository_id or candidate.get("head_repository", {}).get("id") != repository_id:
        return False
    allowed = {"push", "schedule", "workflow_dispatch"}
    if current.get("event") == "pull_request":
        allowed = {"pull_request"}
    elif current.get("event") not in allowed:
        return False
    return candidate.get("event") in allowed


def trusted_baseline(
    current: dict[str, Any], candidate: dict[str, Any], repository: dict[str, Any]
) -> bool:
    return (
        candidate.get("id") != current.get("id")
        and candidate.get("workflow_id") == current.get("workflow_id")
        and bool(current.get("workflow_id"))
        and candidate.get("event") in {"push", "schedule", "workflow_dispatch"}
        and candidate.get("head_branch") == repository["default_branch"]
        and candidate.get("head_repository", {}).get("id") == repository["id"]
    )


def find_artifact(
    repo: str, run_id: str, name: str, shared_name: str = ""
) -> dict[str, Any] | None:
    current = json.loads(gh("api", f"repos/{repo}/actions/runs/{run_id}").stdout)
    for candidate_name in dict.fromkeys(filter(None, (name, shared_name))):
        repository = None
        if candidate_name != name:
            repository = json.loads(gh("api", f"repos/{repo}").stdout)
        artifact = _find_named_artifact(repo, current, candidate_name, repository)
        if artifact is not None:
            return artifact
    return None


def _find_named_artifact(
    repo: str, current: dict[str, Any], name: str, repository: dict[str, Any] | None
) -> dict[str, Any] | None:
    response = gh(
        "api",
        "--method",
        "GET",
        f"repos/{repo}/actions/artifacts",
        "-f",
        f"name={name}",
        "-f",
        "per_page=30",
    )
    artifacts = json.loads(response.stdout)["artifacts"]
    for artifact in sorted(artifacts, key=lambda item: item["id"], reverse=True):
        if artifact.get("expired") or artifact.get("name") != name:
            continue
        producer = artifact.get("workflow_run", {}).get("id")
        if not isinstance(producer, int) or producer <= 0 or producer == current["id"]:
            continue
        candidate = json.loads(gh("api", f"repos/{repo}/actions/runs/{producer}").stdout)
        # The snapshot is published immediately after successful compilation;
        # later tests or packaging may still be running or may have failed.
        if (
            trusted_baseline(current, candidate, repository)
            if repository
            else compatible_run(current, candidate)
        ):
            return artifact
    return None


def pack_cache(workspace: Path, destination: Path) -> Path | None:
    destination.mkdir(parents=True, exist_ok=True)
    archive = destination / "cache.tar"
    count = 0
    with tarfile.open(archive, "w") as tar:
        for root in CACHE_ROOTS:
            source = workspace / root
            if source.is_symlink() or not source.resolve().is_relative_to(workspace.resolve()):
                raise ValueError(f"Cache root is a symlink: {source}")
            for path in source.rglob("*"):
                if path.is_file() and not path.is_symlink():
                    tar.add(path, arcname=path.relative_to(workspace).as_posix(), recursive=False)
                    count += 1
    if not count:
        archive.unlink()
        return None
    return archive


def unpack_cache(archive: Path, workspace: Path) -> None:
    """Validate every member before replacing either cache directory."""
    with tarfile.open(archive) as tar:
        for member in tar.getmembers():
            path = PurePosixPath(member.name)
            if (
                path.is_absolute()
                or ".." in path.parts
                or "\\" in member.name
                or ":" in member.name
                or not member.isfile()
                or not any(
                    path.is_relative_to(root) and path != PurePosixPath(root)
                    for root in CACHE_ROOTS
                )
            ):
                raise ValueError(f"Unexpected compiler cache member: {member.name}")
    with tempfile.TemporaryDirectory(dir=workspace) as temporary:
        extracted = Path(temporary)
        extract_tar_data(archive, extracted)
        for root in CACHE_ROOTS:
            source = extracted / root
            destination = workspace / root
            if destination.is_symlink() or destination.parent.is_symlink():
                raise ValueError(f"Cache destination is a symlink: {destination}")
            if source.is_dir():
                if destination.exists():
                    shutil.rmtree(destination)
                destination.parent.mkdir(parents=True, exist_ok=True)
                source.rename(destination)


def restore(repo: str, run_id: str, name: str, workspace: Path, shared_name: str = "") -> bool:
    artifact = find_artifact(repo, run_id, name, shared_name)
    if artifact is None:
        print("No compatible compiler cache artifact; trying the dependency cache fallback.")
        return False
    with tempfile.TemporaryDirectory() as temporary:
        directory = Path(temporary)
        gh(
            "run",
            "download",
            str(artifact["workflow_run"]["id"]),
            "--repo",
            repo,
            "--name",
            artifact["name"],
            "--dir",
            str(directory),
        )
        unpack_cache(directory / "cache.tar", workspace)
    print(f"Restored compiler/moc snapshot from run {artifact['workflow_run']['id']}")
    return True


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("configure", "restore", "pack"))
    args = parser.parse_args(argv)
    try:
        if args.command == "configure":
            enabled = os.environ.get(
                "QGC_ACTIONS_CACHE_BACKEND", "github"
            ) != "s3" and os.environ.get("GITHUB_EVENT_NAME") in {
                "pull_request",
                "push",
                "schedule",
                "workflow_dispatch",
            }
            append_github_env(
                {
                    "QGC_BUILD_CACHE_ARTIFACT": str(enabled).lower(),
                    "QGC_BUILD_CACHE_ARTIFACT_NAME": artifact_name(
                        os.environ["COMPILER_KEY"], os.environ["MOC_KEY"]
                    ),
                    "QGC_BUILD_CACHE_SHARED_ARTIFACT": artifact_name(
                        os.environ["COMPILER_SHARED_KEY"], os.environ["MOC_SHARED_KEY"]
                    ),
                }
            )
        elif args.command == "pack":
            directory = Path(
                tempfile.mkdtemp(prefix="compiler-cache-", dir=os.environ.get("RUNNER_TEMP"))
            )
            archive = pack_cache(Path(os.environ["GITHUB_WORKSPACE"]), directory)
            if archive:
                write_github_output({"path": str(archive)})
        else:
            repo = os.environ["GITHUB_REPOSITORY"]
            run_id = os.environ["GITHUB_RUN_ID"]
            if not re.fullmatch(r"[\w.-]+/[\w.-]+", repo) or not run_id.isdecimal():
                raise ValueError("Invalid repository or workflow run ID")
            found = restore(
                repo,
                run_id,
                os.environ["QGC_BUILD_CACHE_ARTIFACT_NAME"],
                Path(os.environ["GITHUB_WORKSPACE"]),
                os.environ.get("QGC_BUILD_CACHE_SHARED_ARTIFACT", ""),
            )
            write_github_output({"restored": str(found).lower()})
    except (
        OSError,
        ValueError,
        KeyError,
        tarfile.TarError,
        subprocess.CalledProcessError,
    ) as error:
        gh_warning(f"Compiler cache artifact {args.command} unavailable: {error}")
        write_github_output({"restored": "false"})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
