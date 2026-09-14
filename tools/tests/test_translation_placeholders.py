"""Exercise message-level translation checks and their lightweight CI wiring."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path
from xml.sax.saxutils import escape

import pytest
import yaml
from common.git import run_git
from common.xml import XMLParseError

from translations import check_placeholders as checker

ROOT = Path(__file__).resolve().parents[2]
SOURCE = "Switches to '%1' when you click Stop."
CATALOG = "translations/with spaces.ts"


def message(
    source: str = SOURCE,
    translation: str | None = "%1",
    *,
    status: str = "",
    forms: list[str] | None = None,
    comment: str = "",
    identifier: str = "",
    metadata: str = "",
) -> str:
    numerus = ' numerus="yes"' if forms is not None else ""
    identity = f' id="{identifier}"' if identifier else ""
    body = f"<source>{escape(source)}</source><comment>{escape(comment)}</comment>"
    if translation is not None:
        translated = (
            "".join(f"<numerusform>{escape(form)}</numerusform>" for form in forms)
            if forms is not None
            else escape(translation)
        )
        body += f'<translation type="{status}">{translated}</translation>'
    return f"<message{numerus}{identity}>{metadata}{body}</message>"


def catalog(*messages: str, context: str = "PIDTuning", language: str = "ko_KR") -> str:
    return (
        '<?xml version="1.0" encoding="utf-8"?>\n<!DOCTYPE TS>\n'
        f'<TS version="2.1" language="{language}"><context><name>{context}</name>'
        f"{''.join(messages)}</context></TS>"
    )


def write_catalog(repo: Path, content: str, name: str = CATALOG) -> Path:
    path = repo / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    return path


def git(repo: Path, *args: str) -> str:
    return run_git(*args, cwd=repo, check=True).stdout.strip()


def commit_fixture(repo: Path) -> str:
    git(repo, "add", ".")
    git(repo, "-c", "core.hooksPath=/dev/null", "commit", "-qm", "Fixture")
    return git(repo, "rev-parse", "HEAD")


@pytest.fixture
def repo(tmp_path: Path) -> Path:
    git(tmp_path, "init", "-q")
    git(tmp_path, "config", "user.name", "Test")
    git(tmp_path, "config", "user.email", "test@example.invalid")
    git(tmp_path, "config", "commit.gpgsign", "false")
    write_catalog(tmp_path, catalog(message("Legacy %2", "historical defect"), message()))
    commit_fixture(tmp_path)
    return tmp_path


def check_xml(tmp_path: Path, content: str) -> list[str]:
    parsed = checker.parse_catalog(write_catalog(tmp_path, content))
    return [error for key, value in parsed.items() for error in checker.message_errors(key, value)]


@pytest.mark.parametrize(
    ("source", "translation"),
    [
        ("%1 %2", "%2 then %1"),
        ("%1 %1", "%1"),
        ("%1", "%1 and again %1"),
        ("%1 %9 %10 %99", "%99 %10 %9 %1"),
        ("%L1 %L99 %n %Ln", "%Ln %n %L99 %L1"),
        ("100% ready", "ready: 100%"),
        ("100% charged", "100% nominal"),
        ("%1 at 50%", "%1: 50% normal"),
        ("%%1", "%%1"),
        ("100% 1 Hz", "100% 1 Hz"),
        ("Range %1\nTo %2", "To %2\nFrom %1"),
        ("<b>%1</b> & %2", "%2 & <b>%1</b>"),
    ],
)
def test_valid_markers(tmp_path: Path, source: str, translation: str) -> None:
    assert check_xml(tmp_path, catalog(message(source, translation))) == []


@pytest.mark.parametrize(
    ("source", "translation", "diagnostic"),
    [
        (SOURCE, "% 1", "missing: '%1'; malformed: '% 1'"),
        (SOURCE, "Altitude mode", "missing: '%1'"),
        ("%1", "%2", "missing: '%1'; extra: '%2'"),
        ("no arguments", "%1", "extra: '%1'"),
        ("%1", "%1 %2", "extra: '%2'"),
        ("%L1", "%1", "missing: '%L1'; extra: '%1'"),
        ("%1", "%L1", "missing: '%1'; extra: '%L1'"),
        ("%Ln", "%n", "missing: '%Ln'; extra: '%n'"),
        ("%1", "   ", "missing: '%1'"),
        ("%1", "%0", "malformed: '%0'"),
        ("%1", "%01", "malformed: '%01'"),
        ("%10", "%100", "malformed: '%100'"),
        ("%L1", "%L 1", "malformed: '%L 1'"),
        ("%L1", "%l1", "malformed: '%l1'"),
        ("%1", "%1 % 2", "malformed: '% 2'"),
        ("%n", "%N", "malformed: '%N'"),
        ("%n", "% n", "malformed: '% n'"),
        ("%1", "%\u00a01", "malformed: '%\\xa01'"),
        ("%1", "%\uff11", "malformed: '%\uff11'"),
    ],
)
def test_mismatches(tmp_path: Path, source: str, translation: str, diagnostic: str) -> None:
    errors = check_xml(tmp_path, catalog(message(source, translation)))
    assert len(errors) == 1
    assert diagnostic in errors[0]


@pytest.mark.parametrize("status", ["", "unfinished"])
@pytest.mark.parametrize("translation", ["", None])
def test_empty_translation_fallback(tmp_path: Path, status: str, translation: str | None) -> None:
    assert check_xml(tmp_path, catalog(message(translation=translation, status=status))) == []


@pytest.mark.parametrize("status", ["obsolete", "vanished"])
def test_ignored_entries(tmp_path: Path, status: str) -> None:
    assert check_xml(tmp_path, catalog(message(translation="bad", status=status))) == []


def test_unfinished_nonempty_translation_ships(tmp_path: Path) -> None:
    assert check_xml(tmp_path, catalog(message(translation="bad", status="unfinished")))


def test_each_plural_form_and_localized_numerus(tmp_path: Path) -> None:
    valid = message("%Ln %L1", forms=["%L1: %Ln", "%Ln: %L1", "%L1 %Ln %Ln"])
    assert check_xml(tmp_path, catalog(valid)) == []
    invalid = message("%n %1", forms=["%1 %n", "%n", "%1 %n %2"])
    errors = check_xml(tmp_path, catalog(invalid))
    assert errors == ["plural form 2: missing: '%1'", "plural form 3: extra: '%2'"]


def test_unfinished_empty_first_plural_omits_whole_message(tmp_path: Path) -> None:
    assert check_xml(tmp_path, catalog(message("%n", status="unfinished", forms=["", "bad"]))) == []


@pytest.mark.parametrize("status", ["", "unfinished"])
def test_empty_later_plural_falls_back(tmp_path: Path, status: str) -> None:
    assert check_xml(tmp_path, catalog(message("%n", status=status, forms=["%n", ""]))) == []


def test_finished_empty_first_plural_does_not_hide_later_form(tmp_path: Path) -> None:
    assert check_xml(tmp_path, catalog(message("%n", forms=["", "bad"]))) == [
        "plural form 2: missing: '%n'"
    ]


def test_id_based_unfinished_retains_nonempty_forms(tmp_path: Path) -> None:
    content = catalog(
        message("%n", status="unfinished", forms=["", "bad"], identifier="count"), context=""
    )
    assert check_xml(tmp_path, content) == ["plural form 2: missing: '%n'"]


def test_top_level_id_based_message(tmp_path: Path) -> None:
    content = f"<TS>{message('%1', 'bad', identifier='mode')}</TS>"
    assert check_xml(tmp_path, content) == ["translation: missing: '%1'"]


def test_multiline_cdata_entities_and_byte_elements(tmp_path: Path) -> None:
    content = catalog(
        "<message><source><![CDATA[<b>%1</b>\n& %2]]></source>"
        '<translation>&lt;b&gt;%<byte value="x31"/>&lt;/b&gt;\n&amp; %2</translation></message>'
    )
    assert check_xml(tmp_path, content) == []


@pytest.mark.parametrize("plural", [False, True])
def test_length_variants(tmp_path: Path, plural: bool) -> None:
    variants = "<lengthvariant>%1 long</lengthvariant><lengthvariant>bad</lengthvariant>"
    if plural:
        translated = (
            f'<translation><numerusform variants="yes">{variants}</numerusform></translation>'
        )
    else:
        translated = f'<translation variants="yes">{variants}</translation>'
    content = catalog(
        f'<message numerus="{"yes" if plural else "no"}"><source>%1</source>{translated}</message>'
    )
    errors = check_xml(tmp_path, content)
    assert len(errors) == 1
    assert "length variant 2: missing: '%1'" in errors[0]


@pytest.mark.parametrize(
    "content",
    [
        "<TS>",
        "<root/>",
        "<TS><not-a-context/></TS>",
        "<TS><context><message/></context></TS>",
        catalog("<message><translation>%1</translation></message>"),
        catalog(message() + message()),
        catalog(message(status="unknown")),
        catalog(message().replace("<message>", '<message numerus="maybe">')),
        catalog(message().replace("%1</translation>", "%1<b/></translation>")),
        catalog(message().replace("%1</translation>", '<byte value="invalid"/></translation>')),
        catalog(message().replace("%1</translation>", '<byte value="x10000"/></translation>')),
        catalog(message().replace("%1</translation>", '<byte value="49">bad</byte></translation>')),
        catalog(message().replace("<message>", '<message numerus="yes">')),
        catalog(message().replace("</message>", "<translation/></message>")),
        '<!DOCTYPE TS [<!ENTITY bad SYSTEM "file:///etc/passwd">]><TS/>',
    ],
)
def test_invalid_xml_and_structure_fail(tmp_path: Path, content: str) -> None:
    with pytest.raises((ValueError, XMLParseError)):
        checker.parse_catalog(write_catalog(tmp_path, content))


def test_unchanged_historical_debt_is_not_rechecked(repo: Path) -> None:
    assert checker.validate(repo, "HEAD", [CATALOG]) == ([], 1, 0)
    write_catalog(
        repo, catalog(message("Legacy %2", "historical defect"), message(translation="Now %1"))
    )
    assert checker.validate(repo, "HEAD", []) == ([], 1, 1)


def test_metadata_and_message_order_do_not_recheck_debt(repo: Path) -> None:
    metadata = (
        '<location filename="new.qml" line="123"/><translatorcomment>note</translatorcomment>'
    )
    content = catalog(message(), message("Legacy %2", "historical defect", metadata=metadata))
    write_catalog(repo, content.replace("</message>", "</message>\n"))
    assert checker.validate(repo, "HEAD", []) == ([], 1, 0)


@pytest.mark.parametrize(
    "replacement",
    [
        message(translation="Stop mode"),
        message("Changed source %1 %3"),
        message("Legacy %2", "updated but still broken"),
        message("Legacy %2", "historical defect", status="unfinished"),
        message("Legacy %2", "historical defect", comment="new meaning"),
        message("Legacy %2", "historical defect", identifier="new-id"),
        message("Legacy %2", forms=["historical defect"]),
    ],
)
def test_changed_identity_or_value_is_checked(repo: Path, replacement: str) -> None:
    write_catalog(repo, catalog(replacement))
    errors, _, checked = checker.validate(repo, "HEAD", [])
    assert checked == 1
    assert errors
    assert f"{CATALOG}: context='PIDTuning'" in errors[0]
    assert "source=" in errors[0]


def test_context_and_language_changes_are_checked(repo: Path) -> None:
    for attributes in ({"context": "Other"}, {"language": "zh_CN"}):
        write_catalog(repo, catalog(message("Legacy %2", "historical defect"), **attributes))
        assert checker.validate(repo, "HEAD", [CATALOG])[0]


@pytest.mark.parametrize("old_status", ["unfinished", "vanished", "obsolete"])
def test_activating_or_finishing_bad_translation_fails(repo: Path, old_status: str) -> None:
    write_catalog(repo, catalog(message(translation="bad", status=old_status)))
    base = commit_fixture(repo)
    write_catalog(repo, catalog(message(translation="bad")))
    assert checker.validate(repo, base, [CATALOG])[0]


def test_multiline_and_plural_changes_are_whole_messages(repo: Path) -> None:
    write_catalog(repo, catalog(message("%n\n%1", forms=["%1\n%n", "%n\n%1"])))
    base = commit_fixture(repo)
    write_catalog(repo, catalog(message("%n\n%1", forms=["%1\n%n", "%n\nbad"])))
    errors, _, checked = checker.validate(repo, base, [])
    assert checked == 1
    assert len(errors) == 1
    assert "plural form 2: missing: '%1'" in errors[0]


@pytest.mark.parametrize("staged", [False, True])
def test_new_catalog_is_checked(repo: Path, staged: bool) -> None:
    name = "translations/new language.ts"
    write_catalog(repo, catalog(message(translation="bad")), name)
    if staged:
        git(repo, "add", name)
    errors, files, messages = checker.validate(repo, "HEAD", [])
    assert (files, messages) == (1, 1)
    assert errors
    assert errors[0].startswith(name)


def test_deleted_messages_and_catalogs_are_skipped(repo: Path) -> None:
    path = write_catalog(repo, catalog(message()))
    assert checker.validate(repo, "HEAD", []) == ([], 1, 0)
    path.unlink()
    assert checker.validate(repo, "HEAD", []) == ([], 0, 0)
    assert checker.validate(repo, "HEAD", [CATALOG]) == ([], 0, 0)


def test_renamed_catalog_is_new(repo: Path) -> None:
    (repo / CATALOG).rename(repo / "translations/renamed.ts")
    errors, files, messages = checker.validate(repo, "HEAD", [])
    assert (files, messages) == (1, 2)
    assert errors


def test_explicit_baseline_detects_committed_changes(repo: Path) -> None:
    base = git(repo, "rev-parse", "HEAD")
    write_catalog(repo, catalog(message(translation="bad")))
    commit_fixture(repo)
    assert checker.validate(repo, "HEAD", []) == ([], 0, 0)
    assert checker.validate(repo, base, [])[0]


def test_base_is_exact_not_merge_base(repo: Path) -> None:
    ancestor = git(repo, "rev-parse", "HEAD")
    write_catalog(repo, catalog(message(translation="bad")))
    base = commit_fixture(repo)
    tree = git(repo, "rev-parse", "HEAD^{tree}")
    sibling = run_git(
        "commit-tree", tree, "-p", ancestor, "-m", "Sibling fixture", cwd=repo, check=True
    ).stdout.strip()
    git(repo, "update-ref", "HEAD", sibling)
    assert checker.validate(repo, base, [CATALOG]) == ([], 1, 0)
    assert checker.validate(repo, ancestor, [CATALOG])[0]


def test_empty_baseline_checks_all_messages(repo: Path) -> None:
    errors, files, messages = checker.validate(repo, "EMPTY", [])
    assert (files, messages) == (1, 2)
    assert errors


def test_unborn_head_requires_explicit_empty_baseline(tmp_path: Path) -> None:
    git(tmp_path, "init", "-q")
    write_catalog(tmp_path, catalog(message()))
    with pytest.raises(subprocess.CalledProcessError):
        checker.validate(tmp_path, "HEAD", [])
    assert checker.validate(tmp_path, "EMPTY", []) == ([], 1, 1)


def test_missing_ref_fails_even_with_no_changes(repo: Path) -> None:
    with pytest.raises(subprocess.CalledProcessError):
        checker.validate(repo, "missing-base", [])


def test_shallow_history_cannot_silently_skip(repo: Path, tmp_path: Path) -> None:
    base = git(repo, "rev-parse", "HEAD")
    write_catalog(repo, catalog(message(translation="bad")))
    commit_fixture(repo)
    shallow = tmp_path / "shallow"
    git(tmp_path, "clone", "-q", "--depth=1", repo.as_uri(), str(shallow))
    with pytest.raises(subprocess.CalledProcessError):
        checker.validate(shallow, base, [])


@pytest.mark.parametrize("baseline", [False, True])
def test_xml_errors_identify_current_or_baseline_catalog(repo: Path, baseline: bool) -> None:
    write_catalog(repo, "<TS>")
    if baseline:
        commit_fixture(repo)
        write_catalog(repo, catalog(message()))
    with pytest.raises(ValueError, match=f"{'baseline HEAD:' if baseline else ''}{CATALOG}"):
        checker.validate(repo, "HEAD", [])


def test_non_utf8_xml_baseline_uses_declared_encoding(repo: Path) -> None:
    content = catalog(message(translation="caf\u00e9 %1"))
    path = repo / CATALOG
    path.write_bytes(content.replace('encoding="utf-8"', 'encoding="iso-8859-1"').encode("latin-1"))
    base = commit_fixture(repo)
    write_catalog(repo, content)
    assert checker.validate(repo, base, [CATALOG]) == ([], 1, 0)


@pytest.mark.parametrize("name", ["translations/missing.ts", "../outside.ts", "tools/code.ts"])
def test_invalid_requested_paths_fail(repo: Path, name: str) -> None:
    with pytest.raises((ValueError, FileNotFoundError)):
        checker.validate(repo, "HEAD", [name])


def test_symlink_catalog_is_rejected(repo: Path) -> None:
    link = repo / "translations/link.ts"
    link.symlink_to(repo / CATALOG)
    with pytest.raises(ValueError, match="regular repository file"):
        checker.validate(repo, "HEAD", [])


def test_git_failures_surface_in_cli(repo: Path, monkeypatch: pytest.MonkeyPatch, capsys) -> None:
    def fail(*args, **kwargs):
        raise subprocess.CalledProcessError(128, ["git", *args], stderr="injected Git failure")

    monkeypatch.chdir(repo)
    monkeypatch.setattr(checker, "run_git", fail)
    assert checker.main([]) == 1
    assert "injected Git failure" in capsys.readouterr().err


@pytest.mark.parametrize("command", ["rev-parse", "ls-tree", "diff", "ls-files"])
def test_git_discovery_failures_are_not_empty_selections(
    repo: Path, monkeypatch: pytest.MonkeyPatch, command: str
) -> None:
    def fail(*args, **kwargs):
        if args[0] == command:
            raise subprocess.CalledProcessError(128, ["git", *args], stderr="discovery failed")
        return run_git(*args, **kwargs)

    monkeypatch.setattr(checker, "run_git", fail)
    with pytest.raises(subprocess.CalledProcessError):
        checker.validate(repo, "HEAD", [])


def test_baseline_blob_read_failure_is_not_a_new_file(repo: Path, monkeypatch) -> None:
    def fail(*args, **kwargs):
        raise subprocess.CalledProcessError(128, args[0], stderr=b"missing object")

    monkeypatch.setattr(checker, "run_bytes", fail)
    with pytest.raises(subprocess.CalledProcessError):
        checker.validate(repo, "HEAD", [CATALOG])


def test_unreadable_catalog_is_not_skipped(
    repo: Path, monkeypatch: pytest.MonkeyPatch, capsys
) -> None:
    def fail(*args, **kwargs):
        raise PermissionError("unreadable catalog")

    monkeypatch.chdir(repo)
    monkeypatch.setattr(checker, "parse_catalog", fail)
    assert checker.main([CATALOG]) == 1
    assert "unreadable catalog" in capsys.readouterr().err


def test_cli_base_environment_and_override(repo: Path, monkeypatch, capsys) -> None:
    monkeypatch.chdir(repo)
    monkeypatch.setenv("PRE_COMMIT_FROM_REF", "missing-base")
    assert checker.main([]) == 1
    assert "missing-base" in capsys.readouterr().err
    assert checker.main(["--base-ref", "HEAD", CATALOG]) == 0
    assert (
        "Checked 0 new/changed message(s) in 1 catalog(s) against HEAD." in capsys.readouterr().out
    )


def test_cli_outside_git_fails(tmp_path: Path, monkeypatch, capsys) -> None:
    monkeypatch.chdir(tmp_path)
    assert checker.main([]) == 1
    assert "not a git repository" in capsys.readouterr().err


def test_module_entrypoint_and_default_head(repo: Path) -> None:
    env = {key: value for key, value in os.environ.items() if not key.startswith("PRE_COMMIT_")}
    env["PYTHONPATH"] = str(ROOT / "tools")
    write_catalog(repo, catalog(message(translation="bad")))
    result = subprocess.run(
        [sys.executable, "-m", "translations.check_placeholders"],
        cwd=repo,
        env=env,
        text=True,
        capture_output=True,
        check=False,
    )
    assert result.returncode == 1
    assert "missing: '%1'" in result.stderr


def test_workflow_runs_for_translation_only_prs_with_real_base() -> None:
    workflow = yaml.safe_load((ROOT / ".github/workflows/translation-validation.yml").read_text())
    events = workflow["on"] if "on" in workflow else workflow[True]
    assert set(events) == {"pull_request", "push"}
    for event in events.values():
        assert "translations/**/*.ts" in event["paths"]
        assert "paths-ignore" not in event
    assert workflow["permissions"] == {"contents": "read"}
    job = workflow["jobs"]["validate"]
    assert "if" not in job
    steps = job["steps"]
    checkout = next(step for step in steps if step.get("uses", "").startswith("actions/checkout@"))
    assert checkout["with"]["fetch-depth"] == 0
    assert checkout["with"]["persist-credentials"] is False
    assert "ref" not in checkout["with"]
    setup = next(step for step in steps if step.get("uses") == "./.github/actions/setup-python")
    assert setup["with"]["groups"] == "scripts,test"
    validation = steps[-1]
    assert validation["working-directory"] == "tools"
    assert validation["env"]["BASE_REF"] == (
        "${{ github.event.pull_request.base.sha || github.event.before }}"
    )
    assert '--base-ref "$BASE_REF"' in validation["run"]
    assert "BASE_REF=EMPTY" in validation["run"]
    assert 'git fetch --no-tags --depth=1 origin "$BASE_REF"' in validation["run"]
    assert "HEAD^" not in validation["run"]
    assert not any("qt-install" in step.get("uses", "") for step in steps)


def test_pre_commit_hook_uses_catalog_filter_and_existing_runner() -> None:
    config = yaml.safe_load((ROOT / ".pre-commit-config.yaml").read_text())
    hook = next(
        hook
        for repo in config["repos"]
        for hook in repo["hooks"]
        if hook["id"] == "qt-translation-placeholders"
    )
    assert hook["entry"] == (
        "uv run --frozen --directory tools --group scripts python -m translations.check_placeholders"
    )
    assert hook["files"] == r"^translations/.*\.ts$"
    assert hook["language"] == "system"
    assert hook.get("pass_filenames", True)
