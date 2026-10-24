"""Tests for tools/_bootstrap.py."""

from __future__ import annotations

import ast
import shlex
import subprocess
import sys
import textwrap
from typing import TYPE_CHECKING

import pytest

from ._helpers import REPO_ROOT, TOOLS_DIR

if TYPE_CHECKING:
    from pathlib import Path


def _spawn_bootstrap(env: dict[str, str], body: str) -> subprocess.CompletedProcess:
    """Run a snippet in a subprocess with tools/ on sys.path so _bootstrap imports."""
    code = textwrap.dedent(body)
    full_env = {**env, "PYTHONDONTWRITEBYTECODE": "1"}
    return subprocess.run(
        [sys.executable, "-c", code],
        cwd=REPO_ROOT,
        env=full_env,
        capture_output=True,
        text=True,
        timeout=15,
        check=False,
    )


class TestDebugConfigure:
    """`_configure_debug()` runs at import time: GHA annotations, plus debug hooks when enabled."""

    BOOTSTRAP_IMPORT = "import sys; sys.path.insert(0, 'tools'); import _bootstrap"

    def test_disabled_by_default_leaves_faulthandler_off(self):
        result = _spawn_bootstrap(
            env={"PATH": ""},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            import faulthandler
            print('enabled' if faulthandler.is_enabled() else 'disabled')
            """,
        )
        assert result.returncode == 0, result.stderr
        assert result.stdout.strip() == "disabled"

    @pytest.mark.parametrize("value", ["1", "true", "TRUE", "yes", "on"])
    def test_truthy_value_enables_faulthandler(self, value):
        result = _spawn_bootstrap(
            env={"PATH": "", "QGC_CI_DEBUG": value},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            import faulthandler
            print('enabled' if faulthandler.is_enabled() else 'disabled')
            """,
        )
        assert result.returncode == 0, result.stderr
        assert result.stdout.strip() == "enabled"

    @pytest.mark.parametrize("value", ["", "0", "false", "no", "off", "random"])
    def test_non_truthy_value_keeps_faulthandler_off(self, value):
        result = _spawn_bootstrap(
            env={"PATH": "", "QGC_CI_DEBUG": value},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            import faulthandler
            print('enabled' if faulthandler.is_enabled() else 'disabled')
            """,
        )
        assert result.returncode == 0, result.stderr
        assert result.stdout.strip() == "disabled"

    @pytest.mark.parametrize(("value", "expected"), [("1", "enabled"), ("0", "disabled")])
    def test_runner_debug_logging_enables_faulthandler(self, value, expected):
        result = _spawn_bootstrap(
            env={"PATH": "", "RUNNER_DEBUG": value},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            import faulthandler
            print('enabled' if faulthandler.is_enabled() else 'disabled')
            """,
        )
        assert result.returncode == 0, result.stderr
        assert result.stdout.strip() == expected

    def test_gha_excepthook_emits_error_annotation(self):
        result = _spawn_bootstrap(
            env={"PATH": "", "QGC_CI_DEBUG": "1", "GITHUB_ACTIONS": "true"},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            raise RuntimeError('boom')
            """,
        )
        assert result.returncode != 0
        assert "::error::RuntimeError: boom" in result.stderr
        assert "RuntimeError: boom" in result.stderr  # plus the normal traceback

    def test_non_gha_skips_error_annotation(self):
        result = _spawn_bootstrap(
            env={"PATH": "", "QGC_CI_DEBUG": "1"},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            raise RuntimeError('boom')
            """,
        )
        assert result.returncode != 0
        assert "::error::" not in result.stderr
        assert "RuntimeError: boom" in result.stderr

    def test_idempotent_when_imported_twice(self):
        # Second import should not double-wrap the excepthook; only one ::error:: line.
        result = _spawn_bootstrap(
            env={"PATH": "", "QGC_CI_DEBUG": "1", "GITHUB_ACTIONS": "true"},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            import importlib, _bootstrap
            importlib.reload(_bootstrap)
            raise RuntimeError('boom')
            """,
        )
        assert result.returncode != 0
        assert result.stderr.count("::error::RuntimeError: boom") == 1

    def test_gha_error_annotation_escapes_workflow_command_data(self):
        result = _spawn_bootstrap(
            env={"PATH": "", "QGC_CI_DEBUG": "1", "GITHUB_ACTIONS": "true"},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            raise RuntimeError('100% done\\r\\n::warning::spoofed')
            """,
        )
        annotations = [line for line in result.stderr.splitlines() if line.startswith("::error::")]
        assert annotations == ["::error::RuntimeError: 100%25 done%0D%0A::warning::spoofed"]

    def test_unraisable_annotation_escapes_workflow_command_data(self):
        result = _spawn_bootstrap(
            env={"PATH": "", "QGC_CI_DEBUG": "1", "GITHUB_ACTIONS": "true"},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            class Noisy:
                def __del__(self):
                    raise ValueError('a%b\\nc')
            Noisy()
            """,
        )
        assert result.returncode == 0, result.stderr
        assert "::error::unraisable: ValueError: a%25b%0Ac" in result.stderr.splitlines()

    def test_gha_annotates_uncaught_exceptions_without_debug_logging(self):
        result = _spawn_bootstrap(
            env={"PATH": "", "GITHUB_ACTIONS": "true"},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            import faulthandler
            print('enabled' if faulthandler.is_enabled() else 'disabled')
            class Noisy:
                def __del__(self):
                    raise ValueError('ignored')
            Noisy()
            raise RuntimeError('boom')
            """,
        )
        assert result.stdout.strip() == "disabled"
        assert "::error::RuntimeError: boom" in result.stderr.splitlines()
        assert "::error::unraisable" not in result.stderr

    def test_failed_command_annotation_quotes_the_end_of_its_stderr(self):
        script = "import sys; [print(n, file=sys.stderr) for n in range(15)]; exit(3)"
        result = _spawn_bootstrap(
            env={"PATH": "", "GITHUB_ACTIONS": "true"},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            import subprocess
            subprocess.run([sys.executable, '-c', {script!r}], capture_output=True, check=True)
            """,
        )
        (annotation,) = [line for line in result.stderr.splitlines() if line.startswith("::error")]
        assert annotation.split("%0A") == [
            "::error::CalledProcessError: Command failed with exit status 3: "
            + shlex.join([sys.executable, "-c", script]),
            "stderr (last 10 lines):",
            *(f"  {n}" for n in range(5, 15)),
        ]

    def test_failed_command_annotation_falls_back_to_stdout_and_hides_credentials(self):
        result = _spawn_bootstrap(
            env={"PATH": "", "GITHUB_ACTIONS": "true"},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            import subprocess
            raise subprocess.CalledProcessError(
                128,
                ['git', 'clone', 'https://x-access-token:ghs_secret@github.com/o/r.git'],
                output=b"fatal: Authorization: Bearer ghs_secret rejected\\n",
                stderr=b"",
            )
            """,
        )
        (annotation,) = [line for line in result.stderr.splitlines() if line.startswith("::error")]
        assert annotation.split("%0A") == [
            "::error::CalledProcessError: Command failed with exit status 128: "
            "git clone https://***@github.com/o/r.git",
            "stdout (last 1 lines):",
            "  fatal: Authorization: Bearer *** rejected",
        ]

    @pytest.mark.parametrize(("gha", "exit_code"), [("true", 0), ("", 1)])
    def test_gha_stdout_escapes_what_a_windows_code_page_cannot_encode(self, gha, exit_code):
        result = _spawn_bootstrap(
            env={"PATH": "", "GITHUB_ACTIONS": gha, "PYTHONIOENCODING": "cp1252"},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            print('\\u2713 linked')
            """,
        )
        assert result.returncode == exit_code
        assert result.stdout == ("\\u2713 linked\n" if gha else "")

    def test_cancellation_is_not_annotated(self):
        result = _spawn_bootstrap(
            env={"PATH": "", "GITHUB_ACTIONS": "true"},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            raise KeyboardInterrupt
            """,
        )
        assert result.returncode != 0
        assert "::error" not in result.stderr
        assert "KeyboardInterrupt" in result.stderr


def _tools_sources() -> list[Path]:
    """Non-test Python files under tools/."""
    excluded = {"tests", ".venv", "__pycache__"}
    return [
        path
        for path in sorted(TOOLS_DIR.rglob("*.py"))
        if not excluded & set(path.relative_to(TOOLS_DIR).parts)
    ]


def test_tools_sources_compile() -> None:
    """Every tools/ source compiles on the running interpreter (CI tests the oldest and newest).

    Unlike ast.parse, compiling also rejects symbol-table errors such as a module-level `return`.
    """
    errors = []
    for path in _tools_sources():
        try:
            compile(path.read_bytes(), str(path), "exec", dont_inherit=True)
        except SyntaxError as error:
            errors.append(f"{path.relative_to(TOOLS_DIR)}:{error.lineno}: {error.msg}")
    assert not errors, "\n".join(errors)


def _is_sys_path_call(node: ast.AST) -> bool:
    return (
        isinstance(node, ast.Call)
        and isinstance(node.func, ast.Attribute)
        and ast.unparse(node.func.value) == "sys.path"
    )


def _bootstrap_problems(path: Path, tools_dir: Path = TOOLS_DIR) -> list[str]:
    rel = path.relative_to(tools_dir.parent).as_posix()
    tree = ast.parse(path.read_text(encoding="utf-8"))
    depth = len(path.relative_to(tools_dir).parts) - 1
    canonical = f"sys.path.insert(0, str(Path(__file__).resolve().parents[{depth}]))"
    is_entry = any(
        isinstance(node, ast.If) and "__name__" in ast.unparse(node.test) for node in tree.body
    )
    inserts = [
        node for node in tree.body if isinstance(node, ast.Expr) and _is_sys_path_call(node.value)
    ]
    problems = []
    if sum(map(_is_sys_path_call, ast.walk(tree))) != len(inserts):
        problems.append(f"{rel}: change sys.path only with one bare top-level `{canonical}`")
    for node in inserts:
        if not is_entry:
            problems.append(f"{rel}: library modules must not change sys.path")
        elif depth == 0:
            problems.append(f"{rel}: top-level scripts already run with tools/ on sys.path")
        elif ast.unparse(node) != canonical:
            problems.append(f"{rel}: use `{canonical}`")
    for node in ast.walk(tree):
        if isinstance(node, ast.ImportFrom) and node.module == "_bootstrap":
            problems.append(f"{rel}: use `import _bootstrap as _bootstrap`")
        if not isinstance(node, ast.Import):
            continue
        for alias in node.names:
            if alias.name != "_bootstrap":
                continue
            if alias.asname != "_bootstrap":
                problems.append(f"{rel}: use `import _bootstrap as _bootstrap`")
            if depth and not any(insert.lineno < node.lineno for insert in inserts):
                problems.append(f"{rel}: put `{canonical}` before `import _bootstrap`")
    return problems


def test_scripts_use_the_canonical_bootstrap() -> None:
    problems = [problem for path in _tools_sources() for problem in _bootstrap_problems(path)]
    assert not problems, "\n".join(problems)


# moccache runs once per moc invocation, so it skips the debug hooks to start faster.
TOP_LEVEL_WITHOUT_BOOTSTRAP = {"moccache_launcher.py"}


def test_top_level_entry_scripts_import_bootstrap() -> None:
    missing = set()
    for path in TOOLS_DIR.glob("*.py"):
        tree = ast.parse(path.read_text(encoding="utf-8"))
        is_entry = any(
            isinstance(node, ast.If) and "__name__" in ast.unparse(node.test) for node in tree.body
        )
        imports_bootstrap = any(
            isinstance(node, ast.Import) and any(alias.name == "_bootstrap" for alias in node.names)
            for node in tree.body
        )
        if is_entry and not imports_bootstrap:
            missing.add(path.name)
    assert missing == TOP_LEVEL_WITHOUT_BOOTSTRAP


def _signature(function: ast.FunctionDef) -> tuple[str, str]:
    return ast.unparse(function.args), ast.unparse(function.returns) if function.returns else ""


CI_MAIN = ast.parse("def main(argv: list[str] | None = None) -> int: ...").body[0]
# cache_plan.main() reads only the environment; drop this entry when it takes argv.
CI_MAIN_WITHOUT_ARGV: set[str] = set()


def test_ci_entry_scripts_take_argv_and_return_an_exit_code() -> None:
    """Tests call `main([...])` directly instead of patching `sys.argv`."""
    assert isinstance(CI_MAIN, ast.FunctionDef)
    ci_dir = TOOLS_DIR / "ci"
    nonconforming = set()
    for path in ci_dir.rglob("*.py"):
        tree = ast.parse(path.read_text(encoding="utf-8"))
        is_entry = any(
            isinstance(node, ast.If) and "__name__" in ast.unparse(node.test) for node in tree.body
        )
        mains = [
            node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "main"
        ]
        if is_entry and [_signature(main) for main in mains] != [_signature(CI_MAIN)]:
            nonconforming.add(path.relative_to(ci_dir).as_posix())
    assert nonconforming == CI_MAIN_WITHOUT_ARGV


@pytest.mark.parametrize(
    ("relative_path", "source", "expected"),
    [
        (
            "ci/pkg/guarded.py",
            "import sys\nfrom pathlib import Path\n_d = str(Path(__file__).resolve().parents[2])\n"
            "if _d not in sys.path:\n    sys.path.insert(0, _d)\nimport _bootstrap as _bootstrap\n"
            "if __name__ == '__main__':\n    pass\n",
            "change sys.path only with one bare top-level",
        ),
        (
            "ci/pkg/wrong_depth.py",
            "import sys\nfrom pathlib import Path\n"
            "sys.path.insert(0, str(Path(__file__).resolve().parents[1]))\n"
            "import _bootstrap as _bootstrap\nif __name__ == '__main__':\n    pass\n",
            "use `sys.path.insert(0, str(Path(__file__).resolve().parents[2]))`",
        ),
        ("pkg/lib.py", "import sys\nsys.path.insert(0, 'x')\n", "library modules"),
        ("top.py", "from _bootstrap import ensure_tools_dir\n", "use `import _bootstrap as"),
        ("pkg/no_insert.py", "import _bootstrap as _bootstrap\n", "before `import _bootstrap`"),
    ],
)
def test_bootstrap_problems_flag_noncanonical_shapes(tmp_path, relative_path, source, expected):
    tools = tmp_path / "tools"
    script = tools / relative_path
    script.parent.mkdir(parents=True)
    script.write_text(source, encoding="utf-8")
    assert any(expected in problem for problem in _bootstrap_problems(script, tools))
