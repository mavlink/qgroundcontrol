#!/usr/bin/env python3
"""Prepare a build artifact for attestation and upload.

``prepare`` resolves the artifact (default: $RUNNER_TEMP/build/<artifact name>), verifies or
writes its SHA-256 sidecar, writes the CPM dependency SBOM and build-identity manifest beside it,
emits the paths and attestation subject the later steps consume, and adds an Artifact section
(file, size, SHA-256) to the job summary. With ``--publish`` the manifest carries the publish
marker: the name the Publish Builds workflow publishes the package under, the workflow that
built it, and every package name that workflow marks in this checkout (``PUBLISHED_BUILDS``).
The publisher holds a commit's set complete when it has exactly the packages its builds list.
The manifest also names the job (``--job-id``) and attempt that built the package, by which
the publisher refuses a package from a job that failed or that a re-run has built again. A
package marked without a usable job id is still built: ``prepare`` warns, records no id, and
leaves the refusal to the publisher, whose failure is then its own and not the build's. The
action attests the manifest with the package, so the publisher can take it for the build's word.

``--companion`` names a file that goes beside the package on a GitHub Release and nowhere else:
the ``.zsync`` of an AppImage that embeds update information. The marker lists it with its
SHA-256, the action attests and uploads it with the package, and the publisher then requires
it, so that a release never ships an AppImage whose updater finds no update file.

``checksum`` verifies or writes only the SHA-256 sidecar of a package uploaded outside
attest-and-upload; CPack must emit the ``.deb`` and ``.rpm`` sidecars, so theirs must exist.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.format import format_bytes, format_count
from common.gh_actions import (
    gh_error,
    gh_warning,
    parse_bool,
    write_github_output,
    write_step_summary,
)
from common.io import ensure_sha256_sidecar, sha256_file, write_json
from common.markdown import code_span, summary_section
from qgc_tools.build_artifacts import (
    BUILD_METADATA_SCHEMA_VERSION,
    BUILD_METADATA_SUFFIX,
    companion_names,
    is_distributable_artifact,
    is_public_name,
    workflow_packages,
)

from ci.security.generate_cpm_sbom import generate_sbom

_JOB_ID = re.compile(r"[1-9][0-9]*")


def resolve_artifact(override: str, default: str) -> str:
    path = override or default
    p = Path(path)
    if not p.exists():
        gh_error(f"attest-and-upload: artifact not found at '{path}'")
        parent = p.parent
        if parent.is_dir():
            for entry in sorted(parent.iterdir()):
                print(f"  {entry.name}")
        else:
            print("(parent dir missing)")
        sys.exit(1)
    return path


def write_dependency_sbom(build_dir: Path, output: Path) -> None:
    sbom = generate_sbom(build_dir)
    count = len(sbom["components"])
    if not count:
        gh_error("attest-and-upload: CPM dependency SBOM contains no components")
        sys.exit(1)
    write_json(output, sbom)
    print(f"SBOM written to {output} ({count} components)")


def write_metadata(
    source: Path,
    build_dir: Path,
    *,
    publish: bool = False,
    job_id: str = "",
    companion: Path | None = None,
) -> Path:
    """Record producer identity alongside the package, without runner-local secrets.

    *publish* adds the publish marker: the name the package is published as, its workflow,
    and every package that workflow marks, by which the publisher tells a complete set. The
    marker also lists the *companion* file, which the publisher then requires.
    *job_id* is the id the jobs API gives the building job, unlike ``job``, its workflow key;
    empty when the job could not tell.
    """
    digest = sha256_file(source)
    config = {}
    cache = build_dir / "CMakeCache.txt"
    allowed = {
        "CMAKE_BUILD_TYPE",
        "CMAKE_CXX_COMPILER_ID",
        "CMAKE_SYSTEM_PROCESSOR",
        "QGC_BUILD_TESTING",
        "QGC_MACOS_UNIVERSAL_BUILD",
    }
    if cache.is_file():
        for line in cache.read_text(encoding="utf-8").splitlines():
            key = line.partition(":")[0]
            if key in allowed and "=" in line:
                config[key] = line.partition("=")[2]
    path = source.with_name(source.name + BUILD_METADATA_SUFFIX)
    workflow = os.environ.get("GITHUB_WORKFLOW", "")
    metadata = {
        "schema_version": BUILD_METADATA_SCHEMA_VERSION,
        "repository": os.environ.get("GITHUB_REPOSITORY", ""),
        "commit": os.environ.get("GITHUB_SHA", ""),
        "ref": os.environ.get("GITHUB_REF", ""),
        "event": os.environ.get("GITHUB_EVENT_NAME", ""),
        "workflow": workflow,
        "job": os.environ.get("GITHUB_JOB", ""),
        "job_id": job_id,
        "run_id": os.environ.get("GITHUB_RUN_ID", ""),
        "run_attempt": os.environ.get("GITHUB_RUN_ATTEMPT", ""),
        "runner_os": os.environ.get("RUNNER_OS", ""),
        "runner_arch": os.environ.get("RUNNER_ARCH", ""),
        "configuration": config,
        "artifact": {"name": source.name, "size": source.stat().st_size, "sha256": digest},
    }
    if publish:
        metadata["publish"] = {
            "name": source.name,
            "workflow": workflow,
            "packages": workflow_packages(workflow),
        }
        if companion is not None:
            metadata["publish"]["companions"] = [
                {
                    "name": companion.name,
                    "size": companion.stat().st_size,
                    "sha256": sha256_file(companion),
                }
            ]
    write_json(path, metadata, sort_keys=True)
    return path


def companion_problem(source: Path, companion: Path) -> str | None:
    """Return why *companion* cannot go beside the package *source* on a release, or None.

    The publisher reads it from the root of the artifact, where only a file beside the package
    ends up, and takes no other name than the package's own with ``.zsync``.
    """
    if companion.name not in companion_names(source.name):
        return (
            f"'{companion.name}' is not a file that is published beside '{source.name}': only "
            "an AppImage has one, its <file>.zsync"
        )
    if companion.parent.resolve() != source.parent.resolve():
        return f"'{companion}' is not beside '{source}'"
    if not companion.is_file() or companion.stat().st_size == 0:
        return f"'{companion}' is missing or empty"
    return None


def publish_problem(
    source: Path, artifact_name: str, package_name: str, workflow: str
) -> str | None:
    """Return why the Publish Builds workflow could not publish this package, or None.

    It looks for packages in distributable artifacts only, publishes each under its file name,
    and requires a workflow's packages to be the ones their markers list. These are mistakes in
    the workflow, the same on every runner, so they fail the build.
    """
    if source.name != artifact_name:
        return f"the file is named '{source.name}', not artifact-name '{artifact_name}'"
    if not is_public_name(source.name):
        return f"'{source.name}' is not a publishable QGroundControl package name"
    if not is_distributable_artifact(package_name):
        return f"package-name '{package_name}' is not a distributable artifact name"
    if source.name not in workflow_packages(workflow):
        return (
            f"PUBLISHED_BUILDS does not list '{source.name}' as a package of the "
            f"'{workflow}' workflow"
        )
    return None


def cmd_prepare(args: argparse.Namespace) -> None:
    subject_name = args.subject_name or args.package_name
    build_dir = args.runner_temp / "build"
    artifact = resolve_artifact(args.override, str(build_dir / args.artifact_name))
    source = Path(artifact)
    workflow = os.environ.get("GITHUB_WORKFLOW", "")
    if args.publish and (
        problem := publish_problem(source, args.artifact_name, args.package_name, workflow)
    ):
        gh_error(f"attest-and-upload: cannot mark the package for publishing: {problem}")
        sys.exit(1)
    companion = Path(args.companion) if args.companion else None
    if companion and (problem := companion_problem(source, companion)):
        gh_error(f"attest-and-upload: companion-path: {problem}")
        sys.exit(1)
    job_id = args.job_id if _JOB_ID.fullmatch(args.job_id) else ""
    if args.publish and not job_id:
        # The runner decides whether the job learns its id, so this is not the build's failure.
        gh_warning(
            f"attest-and-upload: {source.name} is marked for publishing, but job-id is "
            f"'{args.job_id}', not the number that `job.check_run_id` gives. The build is not "
            "affected. Publish Builds will refuse the packages of this run, because it accepts "
            "a marked package only from a job that it can look up.",
            title="Package cannot be published from this run",
        )
    try:
        checksum = ensure_sha256_sidecar(source)
    except (OSError, ValueError) as exc:
        gh_error(f"attest-and-upload: {exc}")
        sys.exit(1)

    dependency_sbom = source.parent / f"{subject_name}.dependencies.cdx.json"
    write_dependency_sbom(build_dir, dependency_sbom)
    metadata = write_metadata(
        source, build_dir, publish=args.publish, job_id=job_id, companion=companion
    )
    sbom_path = args.runner_temp / f"{subject_name}.sbom.spdx.json"
    print(f"Artifact: {artifact}\nSBOM path: {sbom_path}\nMarked for publishing: {args.publish}")

    # What the provenance attestation covers: the publisher verifies each of these files.
    subjects = [artifact, str(metadata), *([str(companion)] if companion else [])]
    write_github_output(
        {
            "path": artifact,
            "checksum": str(checksum),
            "companion": str(companion or ""),
            "dependency-sbom": str(dependency_sbom),
            "metadata": str(metadata),
            "provenance-subjects": "\n".join(subjects),
            "sbom-path": str(sbom_path),
            "subject-name": subject_name,
        }
    )
    size = source.stat().st_size
    write_step_summary(
        summary_section(
            "Artifact",
            [
                ("File", code_span(source.name)),
                ("Size", f"{format_bytes(size)} ({format_count(size)} bytes)"),
                ("SHA-256", code_span(checksum.read_text(encoding="utf-8").split()[0])),
                ("Companion file", code_span(companion.name) if companion else None),
                ("Attestation subject", code_span(subject_name)),
            ],
        )
    )


def cmd_checksum(args: argparse.Namespace) -> None:
    source = Path(args.source_path)
    checksum = source.with_name(f"{source.name}.sha256")
    try:
        if source.suffix in {".deb", ".rpm"} and not checksum.is_file():
            raise FileNotFoundError(f"Checksum sidecar not found: {checksum}")
        ensure_sha256_sidecar(source, checksum)
    except (OSError, ValueError) as exc:
        gh_error(f"checksum: {exc}")
        sys.exit(1)

    print(f"Verified {checksum}")
    write_github_output({"checksum": str(checksum)})


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    p_prepare = sub.add_parser(
        "prepare", help="Resolve the artifact and write its checksum, dependency SBOM and metadata"
    )
    p_prepare.add_argument(
        "--artifact-name", required=True, help="Artifact file name in <runner temp>/build"
    )
    p_prepare.add_argument("--override", default="", help="Artifact path overriding the default")
    p_prepare.add_argument("--package-name", required=True, help="GitHub artifact name")
    p_prepare.add_argument(
        "--subject-name", default="", help="Attestation subject name (default: --package-name)"
    )
    p_prepare.add_argument(
        "--publish",
        type=parse_bool,
        default=False,
        metavar="BOOL",
        help="Mark the package for the Publish Builds workflow to publish under its file name",
    )
    p_prepare.add_argument(
        "--job-id",
        default="",
        help="Id of the job that builds the package (job.check_run_id); a package marked "
        "without it is built, with a warning, and not published",
    )
    p_prepare.add_argument(
        "--companion",
        default="",
        help="A file beside the package that is published with it on a GitHub Release only "
        "(an AppImage's <file>.zsync): attested, and listed in the publish marker",
    )
    runner_temp = os.environ.get("RUNNER_TEMP")
    p_prepare.add_argument(
        "--runner-temp",
        type=Path,
        default=runner_temp,
        required=not runner_temp,
        help="Holds the build/ directory to inventory and the SPDX SBOM (default: $RUNNER_TEMP)",
    )
    p_prepare.set_defaults(func=cmd_prepare)

    p_checksum = sub.add_parser(
        "checksum",
        help="Verify the artifact's <name>.sha256 sidecar, writing it when absent (not for .deb/.rpm)",
    )
    p_checksum.add_argument("--source-path", required=True, help="Artifact to checksum")
    p_checksum.set_defaults(func=cmd_checksum)

    args = parser.parse_args(argv)
    args.func(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
