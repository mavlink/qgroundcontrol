"""Publish the canonical devcontainer independently of application builds."""

import json
import os
import re
import shlex
import shutil
import subprocess
from fnmatch import fnmatchcase
from pathlib import Path

import pytest
import yaml

ROOT = Path(__file__).resolve().parents[3]
WORKFLOW = yaml.safe_load((ROOT / ".github/workflows/devcontainer.yml").read_text())
JOB = WORKFLOW["jobs"]["image"]
PUBLICATION_SCRIPT = next(step["run"] for step in JOB["steps"] if step.get("id") == "publication")


def test_image_tracks_master_pushes_and_published_stable_releases():
    triggers = WORKFLOW.get("on", WORKFLOW.get(True))
    assert triggers["push"]["branches"] == ["master"]
    assert triggers["push"]["paths"] == triggers["pull_request"]["paths"]
    assert triggers["release"] == {"types": ["published"]}
    assert "workflow_dispatch" in triggers
    assert ".github/build-config.json" in triggers["pull_request"]["paths"]
    assert "deploy/docker/install_analysis.py" in triggers["pull_request"]["paths"]
    assert JOB["if"] == (
        "github.event_name != 'release' || "
        "(!github.event.release.draft && !github.event.release.prerelease)"
    )
    login = next(
        step for step in JOB["steps"] if step.get("uses", "").startswith("docker/login-action@")
    )
    assert login["if"] == "steps.publication.outputs.publish == 'true'"
    assert login["with"]["registry"] == "ghcr.io"
    assert not any("DOCKERHUB" in str(step) for step in JOB["steps"])


def test_latest_builds_ignore_unrelated_commits_and_reject_superseded_input_changes():
    assert WORKFLOW["concurrency"]["group"] == (
        "${{ github.workflow }}-${{ github.event_name == 'push' && 'latest' || "
        "github.event.release.tag_name || inputs.release_tag || github.ref }}"
    )
    assert WORKFLOW["concurrency"]["cancel-in-progress"] == (
        "${{ github.event_name == 'pull_request' || github.event_name == 'push' }}"
    )
    assert (
        "actions/workflows/devcontainer.yml/runs?branch=master&event=push&per_page=1"
        in PUBLICATION_SCRIPT
    )
    assert "--jq '.workflow_runs[0].head_sha'" in PUBLICATION_SCRIPT
    assert "/commits/master" not in PUBLICATION_SCRIPT


def _copied_inputs(dockerfile, target):
    stages, dependencies, aliases = {}, {}, {}
    current = None
    for line in re.sub(r"\\\n[ \t]*", " ", dockerfile).splitlines():
        parts = line.strip().split(maxsplit=1)
        if len(parts) != 2:
            continue
        instruction, arguments = parts
        if instruction.upper() == "FROM":
            tokens = shlex.split(arguments)
            while tokens[0].startswith("--"):
                tokens.pop(0)
            current = (
                tokens[-1] if len(tokens) >= 3 and tokens[-2].upper() == "AS" else str(len(stages))
            )
            aliases[str(len(stages))] = current
            aliases[current] = current
            stages[current] = set()
            dependencies[current] = {tokens[0]}
        elif instruction.upper() in {"COPY", "ADD"}:
            assert current is not None
            source_stage = None
            while arguments.startswith("--"):
                option, arguments = arguments.split(maxsplit=1)
                if option.startswith("--from="):
                    source_stage = shlex.split(option.split("=", 1)[1])[0]
            if source_stage is not None:
                assert "$" not in source_stage, "Resolve dynamic COPY stages in this coverage test"
                dependencies[current].add(source_stage)
                continue
            tokens = json.loads(arguments) if arguments.startswith("[") else shlex.split(arguments)
            assert len(tokens) >= 2
            stages[current].update(tokens[:-1])

    assert target in stages
    pending, visited, inputs = [target], set(), set()
    while pending:
        stage = aliases.get(pending.pop())
        if stage is None or stage in visited:
            continue
        visited.add(stage)
        inputs.update(stages[stage])
        pending.extend(dependencies[stage])
    return inputs


def test_path_filters_exactly_cover_the_target_copy_dependency_closure():
    build = next(step for step in JOB["steps"] if step.get("id") == "image")["with"]
    dockerfile = ROOT / build["file"]
    inputs = _copied_inputs(dockerfile.read_text(), build["target"])
    expected = {
        ".github/workflows/devcontainer.yml",
        ".devcontainer/devcontainer.json",
        ".dockerignore",
        dockerfile.relative_to(ROOT).as_posix(),
    }
    for source in inputs:
        path = ROOT / source
        assert path.exists(), f"Resolve COPY source {source!r} in the path coverage test"
        expected.add(source.rstrip("/") + "/**" if path.is_dir() else source)
    specific_ignore = dockerfile.with_name(dockerfile.name + ".dockerignore")
    if specific_ignore.exists():
        expected.add(specific_ignore.relative_to(ROOT).as_posix())
    triggers = WORKFLOW.get("on", WORKFLOW.get(True))
    assert set(triggers["push"]["paths"]) == expected
    assert set(triggers["pull_request"]["paths"]) == expected
    assert triggers["release"] == {"types": ["published"]}


def test_copy_closure_follows_inheritance_and_copy_from_but_not_unrelated_stages():
    assert _copied_inputs(
        """
        FROM ubuntu AS copied
        COPY ["copied.py", "/tools/"]
        FROM ubuntu AS unrelated
        COPY android.py /tools/
        FROM ubuntu AS base
        COPY tools/ /tools/
        FROM base AS devcontainer
        COPY --from=copied /tools/ /tools/
        COPY --from=registry/image:version /binary /usr/bin/
        COPY install.py \\
            config.json /setup/
        """,
        "devcontainer",
    ) == {"copied.py", "tools/", "install.py", "config.json"}


@pytest.mark.parametrize(
    "path",
    [
        "docs/en/qgc-dev-guide/getting_started/container.md",
        "tools/tests/test_install_analysis.py",
        ".github/scripts/tests/test_devcontainer_workflow.py",
        "src/Vehicle/Vehicle.cc",
        "tools/analyze.py",
        "tools/setup/setup_vscode.py",
        "deploy/docker/install_sysroot_aarch64.py",
        "deploy/docker/run_docker.py",
    ],
)
def test_non_image_changes_do_not_trigger_latest(path):
    triggers = WORKFLOW.get("on", WORKFLOW.get(True))
    assert not any(fnmatchcase(path, pattern) for pattern in triggers["push"]["paths"])


def test_published_identity_supports_version_and_digest_pins():
    metadata = next(step for step in JOB["steps"] if step.get("id") == "meta")
    assert metadata["with"]["context"] == "git"
    assert metadata["with"]["images"] == "ghcr.io/mavlink/qgroundcontrol-dev"
    assert metadata["with"]["flavor"] == "latest=false"
    assert metadata["with"]["tags"].splitlines() == [
        "type=raw,value=${{ steps.publication.outputs.tag }}",
    ]
    summary = JOB["steps"][-1]
    assert summary["env"]["IMAGE_DIGEST"] == "${{ steps.image.outputs.digest }}"
    assert "ghcr.io/mavlink/qgroundcontrol-dev@%s" in summary["run"]


def test_release_source_is_selected_before_checkout():
    checkout = next(
        step for step in JOB["steps"] if step.get("uses", "").startswith("actions/checkout@")
    )
    assert checkout["with"]["ref"] == "${{ steps.publication.outputs.source_ref }}"
    resolver = next(step for step in JOB["steps"] if step.get("id") == "publication")
    assert JOB["steps"].index(resolver) < JOB["steps"].index(checkout)
    assert "--json isDraft,isPrerelease,tagName" in PUBLICATION_SCRIPT
    assert "select(.isDraft == false and .isPrerelease == false) | .tagName" in PUBLICATION_SCRIPT


def test_token_created_releases_dispatch_the_same_image_pipeline():
    release = yaml.safe_load((ROOT / ".github/workflows/release.yml").read_text())
    job = release["jobs"]["semantic-release"]
    assert job["permissions"]["actions"] == "write"
    dispatch = next(
        step
        for step in job["steps"]
        if step.get("name") == "Dispatch development image publication"
    )
    assert dispatch["if"] == "steps.release.outputs.new_release == 'true'"
    assert dispatch["env"]["RELEASE_TAG"] == "v${{ steps.release.outputs.version }}"
    assert dispatch["run"] == (
        'gh workflow run devcontainer.yml --ref master -f "release_tag=$RELEASE_TAG"'
    )


def test_publication_builds_only_tooling_and_preserves_cache_policy():
    build = next(step for step in JOB["steps"] if step.get("id") == "image")
    inputs = build["with"]
    assert inputs["target"] == "devcontainer"
    assert inputs["platforms"] == "linux/amd64"
    assert inputs["push"] == "${{ steps.publication.outputs.publish == 'true' }}"
    assert inputs["load"] is True
    assert inputs["cache-from"] == "type=gha,version=2,scope=qgc-docker-devcontainer-devcontainer"
    assert inputs["cache-to"] == (
        "${{ github.event_name != 'pull_request' && "
        "'type=gha,version=2,scope=qgc-docker-devcontainer-devcontainer,mode=max' || '' }}"
    )
    assert not any(step.get("uses") == "./.github/actions/docker" for step in JOB["steps"])
    assert "cmake --build" not in "\n".join(step.get("run", "") for step in JOB["steps"])
    assert "install_analysis.py" not in yaml.safe_dump(JOB)


def test_editor_and_publication_build_the_same_default_container():
    editor = json.loads((ROOT / ".devcontainer/devcontainer.json").read_text())["build"]
    publication = next(step for step in JOB["steps"] if step.get("id") == "image")["with"]
    assert editor["target"] == publication["target"] == "devcontainer"
    assert (ROOT / ".devcontainer" / editor["dockerfile"]).resolve() == (
        ROOT / publication["file"]
    ).resolve()
    assert (ROOT / ".devcontainer" / editor["context"]).resolve() == (
        ROOT / publication["context"]
    ).resolve()
    assert editor["options"] == [f"--platform={publication['platforms']}"]


def test_no_workflow_job_consumes_the_development_image():
    for path in (ROOT / ".github/workflows").glob("*.yml"):
        workflow = yaml.safe_load(path.read_text())
        for job in workflow.get("jobs", {}).values():
            container = str(job.get("container", ""))
            assert "devcontainer" not in container
            assert "qgroundcontrol-dev" not in container
    application = (ROOT / ".github/workflows/docker.yml").read_text()
    assert "devcontainer" not in application
    assert "qgroundcontrol-dev" not in application


SHA = "a" * 40
VALIDATION = f"publish=false\ntag=validation\nsource_ref={SHA}\n"
LATEST = f"publish=true\ntag=latest\nsource_ref={SHA}\n"
STABLE = "publish=true\ntag=v5.1.4\nsource_ref=refs/tags/v5.1.4\n"
MOCK_GH = """
gh() {
  if [[ "${MOCK_API_FAILURE:-false}" == true ]]; then return 1; fi
  case "$1" in
    api) printf '%s\\n' "$MOCK_LATEST_INPUT_SHA" ;;
    release) printf '%s\\n' "$MOCK_STABLE_TAG" ;;
    *) return 2 ;;
  esac
}
"""


@pytest.mark.skipif(os.name == "nt" or not shutil.which("bash"), reason="Publication uses Bash")
@pytest.mark.parametrize(
    ("overrides", "expected"),
    [
        ({"EVENT_NAME": "push"}, LATEST),
        ({"EVENT_NAME": "push", "MOCK_LATEST_INPUT_SHA": "b" * 40}, VALIDATION),
        ({"EVENT_NAME": "push", "MOCK_LATEST_INPUT_SHA": ""}, None),
        ({"EVENT_NAME": "push", "GITHUB_REF": "refs/heads/feature"}, VALIDATION),
        ({"EVENT_NAME": "push", "GITHUB_REPOSITORY": "fork/qgroundcontrol"}, VALIDATION),
        ({"EVENT_NAME": "pull_request", "RELEASE_TAG": "v5.1.4"}, VALIDATION),
        ({"EVENT_NAME": "release", "RELEASE_TAG": "v5.1.4"}, STABLE),
        ({"EVENT_NAME": "workflow_dispatch", "RELEASE_TAG": "v5.1.4"}, STABLE),
        ({"EVENT_NAME": "workflow_dispatch"}, VALIDATION),
        ({"EVENT_NAME": "release", "RELEASE_TAG": "v5.1.4", "MOCK_STABLE_TAG": ""}, None),
        ({"EVENT_NAME": "release", "RELEASE_TAG": "latest"}, None),
        ({"EVENT_NAME": "release", "RELEASE_TAG": "devcontainer-v1.0.0"}, None),
        ({"EVENT_NAME": "release", "RELEASE_TAG": "v5.1.4", "MOCK_API_FAILURE": "true"}, None),
    ],
)
def test_publication_identity_never_substitutes_master_for_release(tmp_path, overrides, expected):
    output = tmp_path / "output"
    result = subprocess.run(
        ["bash", "-e", "-o", "pipefail", "-c", MOCK_GH + PUBLICATION_SCRIPT],
        cwd=tmp_path,
        env={
            **os.environ,
            "GITHUB_SHA": SHA,
            "GITHUB_REPOSITORY": "mavlink/qgroundcontrol",
            "GITHUB_REF": "refs/heads/master",
            "RELEASE_TAG": "",
            "GITHUB_OUTPUT": str(output),
            "MOCK_LATEST_INPUT_SHA": SHA,
            "MOCK_STABLE_TAG": "v5.1.4",
            **overrides,
        },
        capture_output=True,
        text=True,
    )
    if expected is None:
        assert result.returncode != 0
        assert not output.exists()
    else:
        assert result.returncode == 0, result.stderr
        assert output.read_text() == expected
