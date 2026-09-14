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
        (vscode_dir / template_name).write_text(template_name, encoding="utf-8")

    existing_settings = vscode_dir / "settings.json"
    existing_settings.write_text("local settings", encoding="utf-8")

    created = setup_vscode.install_vscode_templates(vscode_dir)

    assert existing_settings.read_text(encoding="utf-8") == "local settings"
    assert {path.name for path in created} == {"tasks.json", "launch.json"}
    assert (vscode_dir / "tasks.json").read_text(encoding="utf-8") == "tasks.default.json"
    assert (vscode_dir / "launch.json").read_text(encoding="utf-8") == "launch.default.json"

    (vscode_dir / "launch.json").unlink()
    assert setup_vscode.install_vscode_templates(vscode_dir, {"launch"}) == []
    assert not (vscode_dir / "launch.json").exists()


def test_container_interpreter_only_updates_new_settings(tmp_path):
    import json

    from setup.setup_vscode import TEMPLATES, install_vscode_templates

    for template, _destination in TEMPLATES:
        (tmp_path / template).write_text('{"cmake.useCMakePresets": "always"}')
    install_vscode_templates(tmp_path, python_interpreter="/opt/qgc-venv/bin/python")
    settings = json.loads((tmp_path / "settings.json").read_text())
    assert settings["cmake.useCMakePresets"] == "always"
    assert settings["python.defaultInterpreterPath"] == "/opt/qgc-venv/bin/python"
    install_vscode_templates(tmp_path, python_interpreter="different")
    assert json.loads((tmp_path / "settings.json").read_text()) == settings
