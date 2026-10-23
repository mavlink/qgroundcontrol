"""Compare an aqt-installed Qt tree (reference) with the QGC installer's (candidate).

Accepted differences: the installer's own records exist only in the candidate; text may differ
by the install root (pkg-config files, qtenv2.bat, host-tool scripts); and pkg-config ``prefix=``
lines may differ, because aqt leaves them unpatched on linux_arm64. Names, types, modes, symlink
targets and all other content must match.
"""

from __future__ import annotations

import os
import re
import stat
from dataclasses import dataclass, field
from pathlib import Path

from common.io import sha256_file
from common.markdown import code_span, md_table, more_items, summary_section

INSTALLER_RECORDS = frozenset({".qgc-modules", ".qgc-qt-manifest.json"})
_PKGCONFIG = re.compile(r"lib/pkgconfig/[^/]+\.pc")
_PKGCONFIG_PREFIX = re.compile(rb"^prefix=.*\n?", re.MULTILINE)
_LISTED = 50

Entry = tuple[str, int, str]  # kind, permission bits, symlink target or content SHA-256


def scan(root: Path) -> dict[str, Entry]:
    entries: dict[str, Entry] = {}
    for dirpath, dirnames, filenames in os.walk(root):
        for name in (*dirnames, *filenames):
            path = Path(dirpath, name)
            info = path.lstat()
            rel = path.relative_to(root).as_posix()
            if stat.S_ISLNK(info.st_mode):
                entries[rel] = ("symlink", 0, os.readlink(path))
            elif stat.S_ISDIR(info.st_mode):
                entries[rel] = ("dir", stat.S_IMODE(info.st_mode), "")
            else:
                entries[rel] = ("file", stat.S_IMODE(info.st_mode), sha256_file(path))
    return entries


@dataclass
class Comparison:
    entries: int
    unexpected: dict[str, list[str]] = field(default_factory=dict)
    known: dict[str, list[str]] = field(default_factory=dict)

    @property
    def ok(self) -> bool:
        return not any(self.unexpected.values())


def _differs_beyond_install_root(
    rel: str, reference: Path, candidate: Path, roots: list[tuple[bytes, bytes]]
) -> bool:
    expected, actual = reference.read_bytes(), candidate.read_bytes()
    for old, new in roots:
        expected = expected.replace(old, new)
    if _PKGCONFIG.fullmatch(rel):
        expected, actual = (_PKGCONFIG_PREFIX.sub(b"", data) for data in (expected, actual))
    return expected != actual


def compare_trees(reference_outdir: Path, candidate_outdir: Path, arch_root: str) -> Comparison:
    """Compare ``<outdir>/<arch_root>`` of both installs; paths inside files may differ by outdir."""
    reference, candidate = reference_outdir / arch_root, candidate_outdir / arch_root
    ref, cand = scan(reference), scan(candidate)
    roots = [
        (str(reference_outdir).encode(), str(candidate_outdir).encode()),
        (reference_outdir.as_posix().encode(), candidate_outdir.as_posix().encode()),
    ]
    names = ("only in aqt", "only in qgc", "type", "mode", "symlink", "content")
    unexpected: dict[str, list[str]] = {name: [] for name in names}
    known: dict[str, list[str]] = {"installer records": [], "install root in text": []}
    unexpected["only in aqt"] = sorted(ref.keys() - cand.keys())
    for rel in sorted(cand.keys() - ref.keys()):
        (
            known["installer records"] if rel in INSTALLER_RECORDS else unexpected["only in qgc"]
        ).append(rel)
    for rel in sorted(ref.keys() & cand.keys()):
        (ref_kind, ref_mode, ref_value), (kind, mode, value) = ref[rel], cand[rel]
        if ref_kind != kind:
            unexpected["type"].append(f"{rel}: {ref_kind} vs {kind}")
            continue
        if ref_mode != mode:
            unexpected["mode"].append(f"{rel}: {ref_mode:o} vs {mode:o}")
        if ref_value == value:
            continue
        if kind == "symlink":
            unexpected["symlink"].append(f"{rel}: {ref_value} vs {value}")
        elif _differs_beyond_install_root(rel, reference / rel, candidate / rel, roots):
            unexpected["content"].append(rel)
        else:
            known["install root in text"].append(rel)
    return Comparison(len(ref), unexpected, known)


def render_markdown(title: str, comparison: Comparison) -> str:
    """Return the comparison as a job-summary section."""
    groups = [("unexpected", comparison.unexpected), ("known", comparison.known)]
    counts = md_table(
        ["Difference", "Kind", "Count"],
        [[name, kind, len(items)] for kind, group in groups for name, items in group.items()],
    )
    collapsed = []
    for kind, group in groups:
        for name, items in group.items():
            listed = [f"- {code_span(item)}" for item in items[:_LISTED]]
            if len(items) > _LISTED:
                listed.append(f"- {more_items(len(items) - _LISTED)}")
            collapsed.append((f"{kind}: {name}", "\n".join(listed)))
    return summary_section(
        f"Qt installer parity: {title}",
        [
            ("Result", "identical apart from known differences" if comparison.ok else "FAIL"),
            ("Entries in the aqt tree", comparison.entries),
        ],
        body=counts,
        collapsed=collapsed,
    )
