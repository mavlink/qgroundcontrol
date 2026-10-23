"""Tests for tools/common/paths.py."""

from __future__ import annotations

import shutil
import subprocess
import sys
from typing import TYPE_CHECKING

from common.file_traversal import find_repo_root
from common.paths import REPO_ROOT, TOOLS_DIR

if TYPE_CHECKING:
    from pathlib import Path


def test_tools_dir_is_the_absolute_tools_directory() -> None:
    assert TOOLS_DIR.is_absolute()
    assert TOOLS_DIR.name == "tools"
    assert (TOOLS_DIR / "common" / "paths.py").is_file()


def test_repo_root_is_the_checkout_containing_tools() -> None:
    assert TOOLS_DIR.parent == REPO_ROOT
    assert find_repo_root(TOOLS_DIR) == REPO_ROOT
    assert (REPO_ROOT / ".github" / "build-config.json").is_file()


def test_locations_follow_a_relocated_tools_copy(tmp_path: Path) -> None:
    # Docker copies tools/common without the rest of the checkout; -I -S hides site-packages.
    common = tmp_path / "tools" / "common"
    common.mkdir(parents=True)
    for name in ("__init__.py", "paths.py"):
        shutil.copyfile(TOOLS_DIR / "common" / name, common / name)
    probe = (
        "import sys; sys.path.insert(0, sys.argv[1]); from common import paths; "
        "print(paths.TOOLS_DIR); print(paths.REPO_ROOT)"
    )
    result = subprocess.run(
        [sys.executable, "-I", "-S", "-c", probe, str(common.parent)],
        capture_output=True,
        text=True,
        timeout=60,
        check=True,
    )
    resolved = tmp_path.resolve()
    assert result.stdout.splitlines() == [str(resolved / "tools"), str(resolved)]
