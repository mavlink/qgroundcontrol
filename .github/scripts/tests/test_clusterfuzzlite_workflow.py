"""Keep PR fuzzing independent of the repository-wide artifact history."""

import yaml
from _helpers import REPO_ROOT


def test_pr_fuzzing_uses_seed_corpus_and_preserves_diagnostics() -> None:
    workflow = yaml.safe_load(
        (REPO_ROOT / ".github/workflows/clusterfuzzlite.yml").read_text(encoding="utf-8")
    )
    job = workflow["jobs"]["fuzz"]
    steps = job["steps"]
    run = next(
        step for step in steps if "clusterfuzzlite/actions/run_fuzzers@" in step.get("uses", "")
    )
    assert run["env"]["NO_CLUSTERFUZZ_DEPLOYMENT"] == "true"
    assert run["with"]["fuzz-seconds"] == 300
    assert run["with"]["sanitizer"] == "address"
    assert run["with"]["mode"] == "code-change"
    assert run["with"]["output-sarif"] is True
    assert run["with"].get("dry-run", False) is False
    assert run.get("continue-on-error", False) is False
    assert 5 < run["timeout-minutes"] < job["timeout-minutes"]

    upload = next(
        step for step in steps if step.get("uses", "").startswith("actions/upload-artifact@")
    )
    assert upload["if"] == "${{ !cancelled() }}"
    assert set(upload["with"]["path"].splitlines()) == {
        "out/artifacts",
        "cifuzz-sarif/results.sarif",
    }
