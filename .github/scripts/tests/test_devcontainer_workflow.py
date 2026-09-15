"""Publication contracts for the existing, renamed development container."""

import json
import shutil
import subprocess
import urllib.error
from email.message import Message
from pathlib import Path
from unittest.mock import Mock

import pytest
import qgc_dev
import yaml

ROOT = Path(__file__).resolve().parents[3]
WORKFLOW = yaml.safe_load((ROOT / ".github/workflows/qgc-dev.yml").read_text())
TRIGGERS = WORKFLOW.get("on", WORKFLOW.get(True))
SHA = "1" * 40
DIGEST = "sha256:" + "2" * 64


def test_triggers_cover_only_image_input_closure():
    assert TRIGGERS["push"]["branches"] == ["master"]
    assert set(TRIGGERS["push"]["paths"]) == set(qgc_dev.IMAGE_INPUTS)
    assert TRIGGERS["pull_request"]["paths"] == TRIGGERS["push"]["paths"]
    assert "workflow_dispatch" in TRIGGERS
    assert TRIGGERS["release"]["types"] == ["published"]
    assert (
        WORKFLOW["concurrency"]["cancel-in-progress"]
        == "${{ github.event_name == 'pull_request' }}"
    )


@pytest.mark.parametrize(
    ("path", "expected"),
    [
        ("src/Vehicle/Vehicle.cc", False),
        ("docs/en/guide.md", False),
        ("tools/tests/test_install_qt.py", False),
        ("tools/setup/setup_vscode.py", False),
        ("tools/setup/install_qt.py", True),
        ("tools/setup/install_dependencies/_debian.py", True),
        ("tools/common/net.py", True),
        ("tools/qgc_tools/python_env.py", True),
        ("tools/uv.lock", True),
        ("deploy/docker/docker-bake.hcl", True),
        (".github/scripts/ccache_helper.py", True),
        (".devcontainer/devcontainer.json", True),
    ],
)
def test_meaningful_changes(path, expected):
    assert qgc_dev.meaningful([path]) is expected


def test_read_only_validation_and_digest_promotion_are_separate():
    jobs = WORKFLOW["jobs"]
    assert WORKFLOW["permissions"] == {"contents": "read"}
    assert "permissions" not in jobs["validate"]
    assert jobs["build-digests"]["if"] == "needs.plan.outputs.publish == 'true'"
    assert jobs["build-digests"]["needs"] == ["plan", "validate"]
    assert jobs["publish"]["needs"] == ["plan", "build-digests"]
    for name in ("validate", "build-digests"):
        assert jobs[name]["strategy"]["matrix"] == "${{ fromJSON(needs.plan.outputs.matrix) }}"
    assert not any("QEMU" in str(job) for job in jobs.values())
    action = yaml.safe_load((ROOT / ".github/actions/qgc-dev/action.yml").read_text())
    steps = action["runs"]["steps"]
    uses = [step.get("uses", "") for step in steps]
    assert uses.index("runs-on/action@v2") < uses.index("docker/setup-buildx-action@v4")
    bake = next(step for step in steps if step.get("id") == "bake")
    assert bake["with"]["targets"] == "qgc-dev"
    assert "version=2,scope=qgc-dev-${{ inputs.scope }}" in bake["with"]["set"]
    assert "github.event_name != 'pull_request'" in bake["with"]["set"]
    assert "push-by-digest=true" in bake["with"]["set"]


def test_release_hook_and_application_consumers_are_unchanged():
    release = yaml.safe_load((ROOT / ".github/workflows/release.yml").read_text())
    hook = release["jobs"]["publish-qgc-dev"]
    assert hook["needs"] == "semantic-release"
    assert hook["uses"] == "./.github/workflows/qgc-dev.yml"
    assert hook["with"]["release_tag"] == "v${{ needs.semantic-release.outputs.version }}"
    for name in ("docker.yml", "analysis.yml", "linux.yml"):
        assert "qgc-dev" not in (ROOT / ".github/workflows" / name).read_text()
    dev = json.loads((ROOT / ".devcontainer/devcontainer.json").read_text())
    assert dev["name"] == dev["build"]["target"] == "qgc-dev"


@pytest.mark.skipif(not shutil.which("docker"), reason="Docker Buildx not installed")
def test_actual_bake_definition():
    result = subprocess.run(
        ["docker", "buildx", "bake", "-f", qgc_dev.BAKE, "--print"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=True,
    )
    target = json.loads(result.stdout)["target"]
    assert list(target) == ["qgc-dev"]
    assert target["qgc-dev"]["target"] == "qgc-dev"
    assert set(target["qgc-dev"]["platforms"]) == set(qgc_dev.PLATFORMS)
    assert target["qgc-dev"]["tags"] == ["qgc-dev:local"]


@pytest.mark.parametrize("tag", ["analysis-image-v1.0.0", "latest", "v1.0.0-rc1", "v1.0"])
def test_release_rejects_nonstable_tags(tag):
    with pytest.raises(ValueError, match="exact stable"):
        qgc_dev.release_source(tag)


@pytest.mark.parametrize(
    "invalid",
    [{"draft": True}, {"prerelease": True}, {"published_at": None}, {"tag_name": "v1.2.4"}],
)
def test_release_must_be_actually_published(monkeypatch, invalid):
    release = {
        "tag_name": "v1.2.3",
        "draft": False,
        "prerelease": False,
        "published_at": "now",
        **invalid,
    }
    monkeypatch.setenv("GITHUB_REPOSITORY", "mavlink/qgroundcontrol")
    monkeypatch.setattr(qgc_dev, "command", lambda *args: json.dumps(release))
    with pytest.raises(ValueError, match="actual published"):
        qgc_dev.release_source("v1.2.3")


@pytest.mark.parametrize(
    ("event", "ref", "release", "repo", "tag"),
    [
        ("workflow_dispatch", "refs/heads/master", "", "mavlink/qgroundcontrol", ""),
        ("pull_request", "refs/pull/1/merge", "", "mavlink/qgroundcontrol", ""),
        ("push", "refs/heads/master", "", "mavlink/qgroundcontrol", "latest"),
        ("push", "refs/heads/master", "", "fork/qgroundcontrol", ""),
        ("release", "refs/tags/v1.2.3", "v1.2.3", "mavlink/qgroundcontrol", "v1.2.3"),
        ("workflow_dispatch", "refs/heads/master", "v1.2.3", "mavlink/qgroundcontrol", "v1.2.3"),
    ],
)
def test_plan_manual_builds_without_a_diff_and_stable_never_writes_latest(
    monkeypatch, tmp_path, event, ref, release, repo, tag
):
    monkeypatch.chdir(tmp_path)
    bake = tmp_path / qgc_dev.BAKE
    bake.parent.mkdir(parents=True)
    bake.touch()
    for key, value in {
        "GITHUB_EVENT_NAME": event,
        "GITHUB_REF": ref,
        "RELEASE_TAG": release,
        "GITHUB_REPOSITORY": repo,
        "GITHUB_OUTPUT": str(tmp_path / "output"),
    }.items():
        monkeypatch.setenv(key, value)
    monkeypatch.setattr(qgc_dev, "command", lambda *args: SHA)
    monkeypatch.setattr(qgc_dev, "release_source", lambda tag: SHA)
    monkeypatch.setattr(qgc_dev.subprocess, "run", Mock())
    monkeypatch.setattr(qgc_dev, "bake_matrix", lambda: {"include": []})
    qgc_dev.plan()
    outputs = dict(line.split("=", 1) for line in (tmp_path / "output").read_text().splitlines())
    assert outputs["tag"] == tag
    assert outputs["publish"] == str(bool(tag)).lower()
    assert outputs["source"] == SHA


def records(path, source=SHA):
    for platform in qgc_dev.PLATFORMS:
        directory = path / platform.replace("/", "-")
        directory.mkdir(parents=True)
        (directory / "qgc-dev-digest.json").write_text(
            json.dumps(
                {
                    "platform": platform,
                    "source": source,
                    "digest": DIGEST,
                }
            )
        )


def test_collect_requires_both_exact_source_digests(tmp_path):
    records(tmp_path)
    assert set(qgc_dev.collect_digests(tmp_path, set(qgc_dev.PLATFORMS), SHA)) == set(
        qgc_dev.PLATFORMS
    )
    with pytest.raises(ValueError, match="source/digest"):
        qgc_dev.collect_digests(tmp_path, set(qgc_dev.PLATFORMS), "wrong")
    (tmp_path / "linux-arm64/qgc-dev-digest.json").unlink()
    with pytest.raises(ValueError, match="Both"):
        qgc_dev.collect_digests(tmp_path, set(qgc_dev.PLATFORMS), SHA)


def test_index_rejects_incomplete_or_duplicate_architecture():
    registry = object.__new__(qgc_dev.Registry)
    with pytest.raises(ValueError, match="exactly both"):
        registry.verify_index(
            {
                "mediaType": "application/vnd.oci.image.index.v1+json",
                "manifests": [{"platform": {"os": "linux", "architecture": "amd64"}}] * 2,
            },
            set(qgc_dev.PLATFORMS),
            SHA,
        )


@pytest.mark.parametrize(("changed", "promote"), [("src/main.cc", True), ("tools/uv.lock", False)])
def test_source_only_push_does_not_suppress_pending_image(monkeypatch, tmp_path, changed, promote):
    setup_publish(monkeypatch, tmp_path, "latest")
    monkeypatch.setattr(qgc_dev, "command", lambda *args: changed)
    registry = Mock()
    monkeypatch.setattr(qgc_dev, "Registry", lambda: registry)
    registry.read.return_value = ({}, DIGEST)
    run = Mock()
    monkeypatch.setattr(qgc_dev.subprocess, "run", run)
    qgc_dev.publish()
    promotions = [
        c for c in run.call_args_list if c.args[0][:3] == ["docker", "buildx", "imagetools"]
    ]
    assert bool(promotions) is promote


def setup_publish(monkeypatch, tmp_path, tag):
    monkeypatch.chdir(tmp_path)
    records(tmp_path / "digests")
    for key, value in {
        "GITHUB_REPOSITORY": "mavlink/qgroundcontrol",
        "IMAGE_TAG": tag,
        "SOURCE_REVISION": SHA,
        "IMAGE_MATRIX": json.dumps({"include": [{"platform": p} for p in qgc_dev.PLATFORMS]}),
        "GITHUB_STEP_SUMMARY": str(tmp_path / "summary"),
    }.items():
        monkeypatch.setenv(key, value)


@pytest.mark.parametrize(
    "existing", ["valid", "wrong-source", "unauthorized", "absent", "moved-tag"]
)
def test_stable_retry_never_retargets_existing_or_failed_lookup(monkeypatch, tmp_path, existing):
    setup_publish(monkeypatch, tmp_path, "v1.2.3")
    monkeypatch.setattr(
        qgc_dev, "release_source", lambda tag: "wrong" if existing == "moved-tag" else SHA
    )
    registry = Mock()
    registry.read.return_value = ({}, DIGEST)
    if existing in {"unauthorized", "absent"}:
        registry.read.side_effect = [
            urllib.error.HTTPError(
                "url", 401 if existing == "unauthorized" else 404, "error", Message(), None
            ),
            ({}, DIGEST),
        ]
    if existing == "wrong-source":
        registry.verify_index.side_effect = ValueError("source mismatch")
    monkeypatch.setattr(qgc_dev, "Registry", lambda: registry)
    run = Mock()
    monkeypatch.setattr(qgc_dev.subprocess, "run", run)
    if existing in {"valid", "absent"}:
        qgc_dev.publish()
    else:
        with pytest.raises((ValueError, urllib.error.HTTPError)):
            qgc_dev.publish()
    assert run.called is (existing == "absent")
    if run.called:
        assert f"{qgc_dev.IMAGE}:v1.2.3" in run.call_args.args[0]
        assert not any(":latest" in arg for arg in run.call_args.args[0])
