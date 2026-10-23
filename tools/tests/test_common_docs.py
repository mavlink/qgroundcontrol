"""Keep the tools/ documentation indexes complete and free of stale entries."""

from __future__ import annotations

import re

from ._helpers import REPO_ROOT, TOOLS_DIR

# A recipe header: name, optional parameters, then a colon that does not start `:=`.
_RECIPE_HEADER = re.compile(r"^([A-Za-z][\w-]*)\b[^:\n]*:(?!=)", re.MULTILINE)
_TREE_ENTRY = re.compile(r"^[├└]── (.+?)(?:\s+#.*)?$", re.MULTILINE)
_NESTED_TREE_ENTRY = re.compile(r"^│   [├└]── (\S+)")


def _readme_section(readme: str, heading: str) -> str:
    start = readme.index(f"\n## {heading}\n")
    end = readme.find("\n## ", start + 1)
    return readme[start : end if end != -1 else None]


def _documented_tree() -> str:
    section = _readme_section(
        (TOOLS_DIR / "README.md").read_text(encoding="utf-8"), "Directory Structure"
    )
    return section.split("```text\n", 1)[1].split("```", 1)[0]


def _documented_tree_entries() -> set[str]:
    tree = _documented_tree()
    return {name.strip() for entry in _TREE_ENTRY.findall(tree) for name in entry.split(",")}


def _documented_children(directory: str) -> set[str]:
    lines = _documented_tree().splitlines()
    header = re.compile(rf"^[├└]── {re.escape(directory)}\s")
    start = next(index for index, line in enumerate(lines) if header.match(line))
    children: set[str] = set()
    for line in lines[start + 1 :]:
        match = _NESTED_TREE_ENTRY.match(line)
        if not match:
            break
        children.add(match.group(1))
    return children


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


def test_tools_readme_documents_every_just_recipe() -> None:
    recipes = set(_RECIPE_HEADER.findall((REPO_ROOT / "justfile").read_text(encoding="utf-8")))
    public = {name for name in recipes if not name.startswith("_")} - {"default"}
    readme = (TOOLS_DIR / "README.md").read_text(encoding="utf-8")
    tabulated = set(
        re.findall(
            r"^\| `just ([\w-]+)",
            _readme_section(readme, "Just Command Reference"),
            flags=re.MULTILINE,
        )
    )
    mentioned = set(re.findall(r"(?:^|`)just ([a-z][\w-]*)", readme, flags=re.MULTILINE))
    assert tabulated == public
    assert mentioned <= recipes


def test_tools_readme_tree_lists_top_level_scripts_and_directories() -> None:
    documented = _documented_tree_entries()
    scripts = {path.name for path in TOOLS_DIR.glob("*.py")}
    directories = {
        f"{path.name}/"
        for path in TOOLS_DIR.iterdir()
        if path.is_dir() and not path.name.startswith((".", "__"))
    }
    assert {name for name in documented if name.endswith(".py")} == scripts
    assert {name for name in documented if name.endswith("/")} == directories


def test_tools_readme_tree_lists_every_config_file() -> None:
    actual = {path.name for path in (TOOLS_DIR / "configs").iterdir() if path.is_file()}
    assert _documented_children("configs/") == actual


def test_tools_readme_script_paths_exist() -> None:
    readme = (TOOLS_DIR / "README.md").read_text(encoding="utf-8")
    referenced = set(re.findall(r"\btools/[\w./-]+\.py\b", readme))
    assert {path for path in referenced if not (REPO_ROOT / path).is_file()} == set()
