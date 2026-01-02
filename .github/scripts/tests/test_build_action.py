"""Tests for build_action.py."""

from __future__ import annotations

import sys
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from build_action import ActionBuilder, parse_args


class TestActionBuilder:
    """Tests for ActionBuilder class."""

    def test_repo_url(self):
        """Test repository URL generation."""
        builder = ActionBuilder("actions/checkout")
        assert builder.repo_url == "https://github.com/actions/checkout.git"

    def test_repo_name(self):
        """Test repository name extraction."""
        builder = ActionBuilder("actions/checkout")
        assert builder.repo_name == "checkout"

        builder = ActionBuilder("owner/my-action")
        assert builder.repo_name == "my-action"

    def test_default_ref(self):
        """Test default ref is HEAD."""
        builder = ActionBuilder("actions/checkout")
        assert builder.ref == "HEAD"

    def test_custom_ref(self):
        """Test custom ref."""
        builder = ActionBuilder("actions/checkout", ref="v4.2.0")
        assert builder.ref == "v4.2.0"

    def test_check_prerequisites_all_present(self):
        """Test prerequisites check when all tools present."""
        builder = ActionBuilder("actions/checkout")

        with patch("shutil.which") as mock_which:
            mock_which.return_value = "/usr/bin/tool"
            assert builder.check_prerequisites() is True

    def test_check_prerequisites_missing_git(self):
        """Test prerequisites check when git is missing."""
        builder = ActionBuilder("actions/checkout")

        def which_side_effect(cmd):
            return None if cmd == "git" else "/usr/bin/tool"

        with patch("shutil.which", side_effect=which_side_effect):
            assert builder.check_prerequisites() is False

    def test_check_prerequisites_missing_node(self):
        """Test prerequisites check when node is missing."""
        builder = ActionBuilder("actions/checkout")

        def which_side_effect(cmd):
            return None if cmd == "node" else "/usr/bin/tool"

        with patch("shutil.which", side_effect=which_side_effect):
            assert builder.check_prerequisites() is False

    def test_check_prerequisites_missing_npm(self):
        """Test prerequisites check when npm is missing."""
        builder = ActionBuilder("actions/checkout")

        def which_side_effect(cmd):
            return None if cmd == "npm" else "/usr/bin/tool"

        with patch("shutil.which", side_effect=which_side_effect):
            assert builder.check_prerequisites() is False


class TestVerifyBuild:
    """Tests for build verification."""

    def test_verify_build_with_dist(self, tmp_path: Path):
        """Test verification with dist directory."""
        builder = ActionBuilder("test/repo")

        # Create action.yml and dist/
        (tmp_path / "action.yml").write_text("name: Test\n")
        dist = tmp_path / "dist"
        dist.mkdir()
        (dist / "index.js").write_text("// built\n")

        assert builder.verify_build(tmp_path) is True

    def test_verify_build_with_lib(self, tmp_path: Path):
        """Test verification with lib directory."""
        builder = ActionBuilder("test/repo")

        # Create action.yml and lib/
        (tmp_path / "action.yml").write_text("name: Test\n")
        lib = tmp_path / "lib"
        lib.mkdir()
        (lib / "main.js").write_text("// built\n")

        assert builder.verify_build(tmp_path) is True

    def test_verify_build_no_action_yml(self, tmp_path: Path):
        """Test verification fails without action.yml."""
        builder = ActionBuilder("test/repo")

        assert builder.verify_build(tmp_path) is False

    def test_verify_build_action_yaml(self, tmp_path: Path):
        """Test verification accepts action.yaml."""
        builder = ActionBuilder("test/repo")

        (tmp_path / "action.yaml").write_text("name: Test\n")
        dist = tmp_path / "dist"
        dist.mkdir()
        (dist / "index.js").write_text("// built\n")

        assert builder.verify_build(tmp_path) is True


class TestParseArgs:
    """Tests for argument parsing."""

    def test_parse_repo_only(self):
        """Test parsing with just repo."""
        args = parse_args(["actions/checkout"])
        assert args.repo == "actions/checkout"
        assert args.ref == "HEAD"
        assert args.output_dir is None

    def test_parse_with_ref(self):
        """Test parsing with ref."""
        args = parse_args(["actions/checkout", "--ref", "v4.2.0"])
        assert args.repo == "actions/checkout"
        assert args.ref == "v4.2.0"

    def test_parse_with_output(self):
        """Test parsing with output directory."""
        args = parse_args(["actions/checkout", "--output", "./my-action"])
        assert args.repo == "actions/checkout"
        assert args.output_dir == Path("./my-action")

    def test_parse_short_output(self):
        """Test parsing with short output flag."""
        args = parse_args(["actions/checkout", "-o", "./my-action"])
        assert args.output_dir == Path("./my-action")

    def test_parse_all_options(self):
        """Test parsing with all options."""
        args = parse_args([
            "owner/repo",
            "--ref", "feature-branch",
            "--output", "/tmp/action",
            "--node-version", "20",
        ])
        assert args.repo == "owner/repo"
        assert args.ref == "feature-branch"
        assert args.output_dir == Path("/tmp/action")
        assert args.node_version == "20"


class TestGitHubOutput:
    """Tests for GitHub Actions output."""

    def test_output_github_actions(self, tmp_path: Path):
        """Test GitHub Actions output writing."""
        from build_action import output_github_actions

        output_file = tmp_path / "github_output"

        with patch.dict("os.environ", {"GITHUB_OUTPUT": str(output_file)}):
            output_github_actions(Path("/path/to/action"))

        content = output_file.read_text()
        assert "action_path=/path/to/action" in content

    def test_output_github_actions_no_env(self, tmp_path: Path):
        """Test GitHub Actions output without env var."""
        from build_action import output_github_actions

        with patch.dict("os.environ", {}, clear=True):
            # Should not raise
            output_github_actions(Path("/path/to/action"))
