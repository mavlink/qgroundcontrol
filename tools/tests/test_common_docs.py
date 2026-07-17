"""Keep the shared-tooling module index complete and free of stale entries."""

from __future__ import annotations

import re

from ._helpers import TOOLS_DIR


def test_common_readme_indexes_every_shared_module() -> None:
    common_dir = TOOLS_DIR / "common"
    actual = {
        path.name
        for path in common_dir.iterdir()
        if path.is_file() and path.name not in {"README.md", "__init__.py"}
    }
    readme = (common_dir / "README.md").read_text(encoding="utf-8")
    documented = set(re.findall(r"^\| `([^`]+)` \|", readme, flags=re.MULTILINE))
    assert documented == actual
