"""Check new or changed Qt TS messages against a Git baseline.

Run from tools/: python -m translations.check_placeholders --base-ref HEAD
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING

from _bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.git import run_git  # noqa: E402
from common.proc import run_bytes  # noqa: E402
from common.xml import XMLParseError, xml_parse  # noqa: E402

if TYPE_CHECKING:
    from xml.etree.ElementTree import Element

_MARKER = re.compile(r"%L?(?:\d+|n)|%\s*(?:[Ll]\s*)?(?:\d+|[nN]\b)")
_VALID_MARKER = re.compile(r"%L?(?:[1-9][0-9]?|n)")
_TRANSLATION_TYPES = {"", "unfinished", "vanished", "obsolete"}


@dataclass(frozen=True)
class MessageKey:
    context: str
    source: str
    comment: str
    identifier: str


@dataclass(frozen=True)
class Message:
    language: str
    numerus: bool
    translation_type: str
    forms: tuple[tuple[str, ...], ...]


def text_content(element: Element | None) -> str:
    if element is None:
        return ""
    parts = [element.text or ""]
    for child in element:
        if child.tag != "byte" or len(child) or child.text:
            raise ValueError(f"unexpected <{child.tag}> inside <{element.tag}>")
        value = child.get("value", "")
        number = int(value[1:], 16) if value.startswith("x") else int(value, 10)
        if not 0 <= number <= 0xFFFF:
            raise ValueError(f"invalid TS byte value: {value!r}")
        parts.extend((chr(number) if number else "", child.tail or ""))
    return "".join(parts)


def translation_variants(element: Element) -> tuple[str, ...]:
    if element.get("variants") == "yes":
        if (element.text or "").strip() or any(
            child.tag != "lengthvariant" or (child.tail or "").strip() for child in element
        ):
            raise ValueError("invalid translation length variants")
        return tuple(text_content(child) for child in element)
    return (text_content(element),)


def parse_catalog(path: Path) -> dict[MessageKey, Message]:
    root = xml_parse(path).getroot()
    if root is None or root.tag != "TS":
        raise ValueError("expected a <TS> catalog")
    for child in root:
        if child.tag not in {"context", "message", "dependencies"} and not child.tag.startswith(
            "extra-"
        ):
            raise ValueError(f"unexpected <{child.tag}> inside <TS>")
    messages: dict[MessageKey, Message] = {}
    for context in [root, *root.findall("context")]:
        if context is root:
            context_name = ""
        else:
            if len(context.findall("name")) != 1:
                raise ValueError("each TS context must have one <name>")
            context_name = text_content(context.find("name"))
        for element in context.findall("message"):
            identifier = element.get("id", "")
            if len(element.findall("source")) != 1 and not (
                identifier and not element.findall("source")
            ):
                raise ValueError(f"context {context_name!r}: expected one <source>")
            if len(element.findall("translation")) > 1 or len(element.findall("comment")) > 1:
                raise ValueError(f"context {context_name!r}: duplicate translation/comment")
            key = MessageKey(
                context_name,
                text_content(element.find("source")),
                text_content(element.find("comment")),
                identifier,
            )
            if key in messages:
                raise ValueError(f"duplicate message identity: {key!r}")
            numerus = element.get("numerus", "no")
            if numerus not in {"yes", "no"}:
                raise ValueError(f"invalid numerus attribute: {numerus!r}")
            translation = element.find("translation")
            translation_type = (
                translation.get("type", "") if translation is not None else "unfinished"
            )
            if translation_type not in _TRANSLATION_TYPES:
                raise ValueError(f"invalid translation type: {translation_type!r}")
            forms: tuple[tuple[str, ...], ...] = ()
            if translation is not None:
                if numerus == "yes":
                    if (translation.text or "").strip() or any(
                        child.tag != "numerusform" or (child.tail or "").strip()
                        for child in translation
                    ):
                        raise ValueError("numerus translations must contain <numerusform> forms")
                    forms = tuple(translation_variants(child) for child in translation)
                else:
                    forms = (translation_variants(translation),)
            messages[key] = Message(
                root.get("language", ""), numerus == "yes", translation_type, forms
            )
    return messages


def placeholders(text: str) -> tuple[set[str], set[str]]:
    candidates = set(_MARKER.findall(text))
    valid = {marker for marker in candidates if _VALID_MARKER.fullmatch(marker)}
    return valid, candidates - valid


def message_errors(key: MessageKey, message: Message) -> list[str]:
    if message.translation_type in {"vanished", "obsolete"}:
        return []
    # lrelease drops unfinished messages whose first form is empty.
    if (
        message.translation_type == "unfinished"
        and not (key.identifier and not key.context)
        and (not message.forms or not "\u009c".join(message.forms[0]))
    ):
        return []
    expected, source_literals = placeholders(key.source)
    errors = []
    for form_number, variants in enumerate(message.forms, 1):
        for variant_number, text in enumerate(variants, 1):
            if not text:
                continue
            actual, malformed = placeholders(text)
            details = []
            for label, markers in (
                ("missing", expected - actual),
                ("extra", actual - expected),
                ("malformed", malformed - source_literals),
            ):
                if markers:
                    details.append(f"{label}: {', '.join(repr(m) for m in sorted(markers))}")
            if details:
                location = f"plural form {form_number}" if message.numerus else "translation"
                if len(variants) > 1:
                    location += f", length variant {variant_number}"
                errors.append(f"{location}: {'; '.join(details)}")
    return errors


def baseline_files(repo: Path, base_ref: str) -> tuple[str, dict[str, tuple[str, str]]]:
    if base_ref == "EMPTY":
        return base_ref, {}
    base = run_git(
        "rev-parse", "--verify", "--end-of-options", f"{base_ref}^{{tree}}", cwd=repo, check=True
    ).stdout.strip()
    tree = run_git(
        "ls-tree", "-r", "-z", "--full-tree", base, "--", "translations", cwd=repo, check=True
    ).stdout
    files = {}
    for record in tree.split("\0"):
        if record:
            metadata, name = record.split("\t", 1)
            mode, kind, object_id = metadata.split()
            if name.endswith(".ts"):
                if kind != "blob":
                    raise ValueError(f"baseline catalog is not a blob: {name!r}")
                files[name] = (mode, object_id)
    return base, files


def selected_files(repo: Path, base: str, requested: list[str]) -> list[str]:
    if requested:
        return sorted(set(requested))
    if base == "EMPTY":
        tracked = run_git("ls-files", "-z", "--cached", "--", "translations", cwd=repo, check=True)
    else:
        tracked = run_git(
            "diff",
            "--no-ext-diff",
            "--no-textconv",
            "--no-renames",
            "--name-only",
            "-z",
            base,
            "--",
            "translations",
            cwd=repo,
            check=True,
        )
    untracked = run_git(
        "ls-files",
        "-z",
        "--others",
        "--exclude-standard",
        "--",
        "translations",
        cwd=repo,
        check=True,
    )
    return sorted(
        {name for name in (tracked.stdout + untracked.stdout).split("\0") if name.endswith(".ts")}
    )


def validate(repo: Path, base_ref: str, requested: list[str]) -> tuple[list[str], int, int]:
    base, baseline = baseline_files(repo, base_ref)
    errors: list[str] = []
    checked_messages = 0
    checked_files = 0
    with tempfile.TemporaryDirectory(prefix="qgc-translations-") as temporary:
        baseline_path = Path(temporary) / "baseline.ts"
        for name in selected_files(repo, base, requested):
            relative = Path(name)
            if (
                relative.is_absolute()
                or ".." in relative.parts
                or not relative.parts
                or relative.parts[0] != "translations"
                or relative.suffix != ".ts"
            ):
                raise ValueError(f"expected a repository-relative translations/*.ts path: {name!r}")
            path = repo / relative
            if path.is_symlink() or not path.resolve().is_relative_to(repo):
                raise ValueError(f"catalog must be a regular repository file: {name!r}")
            if not path.exists():
                if name in baseline:
                    continue
                raise FileNotFoundError(f"catalog not found: {name!r}")
            previous = {}
            if name in baseline:
                mode, object_id = baseline[name]
                if mode not in {"100644", "100755"}:
                    raise ValueError(f"baseline catalog is not a regular file: {name!r}")
                content = run_bytes(
                    ["git", "cat-file", "blob", object_id], cwd=repo, check=True
                ).stdout
                baseline_path.write_bytes(content)
                try:
                    previous = parse_catalog(baseline_path)
                except (XMLParseError, ValueError) as error:
                    raise ValueError(f"baseline {base_ref}:{name}: {error}") from error
            try:
                current = parse_catalog(path)
            except (XMLParseError, ValueError) as error:
                raise ValueError(f"{name}: {error}") from error
            checked_files += 1
            for key, message in current.items():
                if previous.get(key) == message:
                    continue
                checked_messages += 1
                for error in message_errors(key, message):
                    errors.append(
                        f"{name}: context={key.context!r}, source={key.source!r}, "
                        f"comment={key.comment!r}, id={key.identifier!r}: {error}"
                    )
    return errors, checked_files, checked_messages


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--base-ref",
        default=os.environ.get("PRE_COMMIT_FROM_REF", "HEAD"),
        help="Git baseline (default: PRE_COMMIT_FROM_REF or HEAD); EMPTY checks all messages",
    )
    parser.add_argument("files", nargs="*", help="Repository-relative catalog paths")
    args = parser.parse_args(argv)
    try:
        repo = Path(run_git("rev-parse", "--show-toplevel", check=True).stdout.strip()).resolve()
        errors, files, messages = validate(repo, args.base_ref, args.files)
    except subprocess.CalledProcessError as error:
        stderr = error.stderr
        if isinstance(stderr, bytes):
            stderr = stderr.decode("utf-8", errors="replace")
        print(f"Translation validation failed: {error}: {(stderr or '').strip()}", file=sys.stderr)
        return 1
    except (OSError, UnicodeError, XMLParseError, ValueError) as error:
        print(f"Translation validation failed: {error}", file=sys.stderr)
        return 1
    for error in errors:
        print(error, file=sys.stderr)
    print(
        f"Checked {messages} new/changed message(s) in {files} catalog(s) against {args.base_ref}."
    )
    return int(bool(errors))


if __name__ == "__main__":
    sys.exit(main())
