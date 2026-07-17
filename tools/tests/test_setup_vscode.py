"""Tests for installing the tracked VS Code templates."""

from __future__ import annotations

from typing import TYPE_CHECKING

from ._helpers import load_script_module

if TYPE_CHECKING:
    from pathlib import Path

setup_vscode = load_script_module("setup/setup_vscode.py", "setup_vscode")


def test_install_vscode_templates_copies_missing_files_without_overwriting(tmp_path: Path) -> None:
    vscode_dir = tmp_path / ".vscode"
    vscode_dir.mkdir()
    for template_name, _destination_name in setup_vscode.TEMPLATES:
        (vscode_dir / template_name).write_text(template_name)

    existing_settings = vscode_dir / "settings.json"
    existing_settings.write_text("local settings")

    created = setup_vscode.install_vscode_templates(vscode_dir)

    assert existing_settings.read_text() == "local settings"
    assert {path.name for path in created} == {"tasks.json", "launch.json"}
    assert (vscode_dir / "tasks.json").read_text() == "tasks.default.json"
    assert (vscode_dir / "launch.json").read_text() == "launch.default.json"

    (vscode_dir / "launch.json").unlink()
    assert setup_vscode.install_vscode_templates(vscode_dir, {"launch"}) == []
    assert not (vscode_dir / "launch.json").exists()
