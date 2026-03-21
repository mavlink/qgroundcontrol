from __future__ import annotations

from pathlib import Path

from precommit_results import main


def test_precommit_results_writes_json_and_copies_output(tmp_path):
    output_file = tmp_path / "pre-commit-output.txt"
    output_file.write_text("hook output\n", encoding="utf-8")
    out_dir = tmp_path / "artifact"

    result = main(
        [
            "--output-dir",
            str(out_dir),
            "--exit-code",
            "1",
            "--passed",
            "10",
            "--failed",
            "2",
            "--skipped",
            "1",
            "--run-url",
            "https://example.test/run/1",
            "--output-file",
            str(output_file),
        ]
    )

    assert result == 0
    assert (out_dir / "pre-commit-results.json").exists()
    assert (out_dir / "pre-commit-output.txt").read_text(encoding="utf-8") == "hook output\n"
