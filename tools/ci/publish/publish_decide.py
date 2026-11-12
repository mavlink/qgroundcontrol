#!/usr/bin/env python3
"""Decide whether a ref's packages are published now, and to which targets.

``decide`` answers for the platform run that just completed (the ``workflow_run`` event), or for
a ref named by hand (``--ref``), which stands for that ref's newest complete build, or for the
commit of it that ``--commit`` names:

1. Only a ``push`` run of a platform workflow of this repository counts, on ``master``, a
   ``Stable*`` branch, or a ``vX.Y.Z`` tag. The name alone is not trusted: a branch must contain
   the commit, and a tag must exist, be that commit, and be on a ``Stable*`` branch.
2. A commit is published whole: every platform workflow's most recent push run for it must have
   completed (a re-run counts by its latest attempt) with a conclusion its ref is published
   from (``PUBLISHABLE_CONCLUSIONS``). For a release tag that is ``success`` alone. A branch
   also takes ``failure``, so that a job which builds no package does not hold the packages
   back: ``publish_set.py`` then finds out, after download, whether every package was built by
   a job that passed. A run that was cancelled or never ran counts as not completed. The run
   that completes the set publishes. The runs are looked up by commit, so the pushes that
   followed it do not hide them.
3. A commit gives way to a newer one that passed on every platform and that its branch still
   contains, which is sure to publish itself, and never replaces a newer one that a target
   already holds: the daily release records its commit in its notes, S3 in ``published.json``
   beside the files. A commit a target already holds is published again only on request by
   hand, which is how a platform rebuilt for the same commit gets published, and how the
   record of a run that was re-run since is brought up to date.
4. ``latest/`` is ordered by release version instead: a release older than the one it holds is
   published to its own ``builds/<tag>/`` only, unless a request by hand sets ``--force-latest``.
   The release it holds is replaced when its tag names another commit by now. A ``latest/``
   without a public record is not taken for empty: its job starts, and ``aws_upload.py``
   refuses there unless the bucket has a record or the request forces it.
5. A tag's packages also go to the tag's GitHub Release. Its job always starts: it attaches
   them to a draft, which cannot be seen from here, and only checks a published release,
   which keeps its files unless a request by hand sets ``--force-release``. A release that
   GitHub reports as immutable keeps them even then.

``confirm`` repeats steps 3 to 5 for one target from inside the job that writes it, where runs
take turns. S3 is read without credentials here, and ``aws_upload.py`` reads the record again
before it uploads: from the same public URL, or with credentials when it is told to. A record
that another repository wrote is reported and not compared with: a dry run in a fork reads
upstream's bucket. Whether the packages carry publish markers is ``publish_set.py``'s part,
after download.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from dataclasses import dataclass, field
from itertools import zip_longest
from pathlib import Path
from typing import TYPE_CHECKING, Any
from urllib.parse import quote

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import (
    compare_commits,
    env_default,
    env_flag,
    gh_api_json,
    gh_api_json_or_none,
    gh_error,
    require_repository,
    resolve_ref_commit,
    write_github_output,
    write_step_summary,
)
from common.io import read_json
from common.markdown import code_span, summary_section
from qgc_tools.release_tag_check import is_release_tag, release_version

from ci.publish.aws_upload import (
    LATEST_PREFIX,
    PublishedStateError,
    Record,
    builds_prefix,
    public_record,
)
from ci.publish.daily_release import DAILY_REF, published_commit
from ci.publish.publish_set import PUBLISHABLE_CONCLUSIONS, tag_problem

if TYPE_CHECKING:
    from collections.abc import Callable, Collection, Iterator, Mapping, Sequence

# The platform workflows whose packages are published, by display name. publish.yml starts on
# exactly these, and each attests its packages in its own jobs.
REQUIRED_WORKFLOWS = {
    "Linux": ".github/workflows/linux.yml",
    "Windows": ".github/workflows/windows.yml",
    "MacOS": ".github/workflows/macos.yml",
    "Android": ".github/workflows/android.yml",
}
TARGETS = ("daily", "s3", "latest", "release")
# Runs are listed a page at a time. GitHub answers a filtered listing with at most its newest
# RUN_LIMIT runs.
PAGE_SIZE = 100
RUN_LIMIT = 1000
# The workflows' push branches (master, Stable*), limited to names that are safe in a path.
_BRANCH = re.compile(r"master|Stable[A-Za-z0-9._-]*")
_COMMIT = re.compile(r"[0-9a-f]{40}")

Run = dict[str, Any]


class RewrittenHistoryError(RuntimeError):
    """A target holds a commit that the commit to publish does not descend from."""


@dataclass(frozen=True)
class Ref:
    kind: str
    name: str

    @property
    def full(self) -> str:
        return f"refs/{'heads' if self.kind == 'branch' else 'tags'}/{self.name}"


@dataclass(frozen=True)
class Build:
    """A commit's most recent push run of each platform workflow on one ref."""

    commit: str
    runs: Mapping[str, Run]

    @property
    def pushed_at(self) -> str:
        return max((str(run.get("created_at") or "") for run in self.runs.values()), default="")

    def waiting_for(self, admitted: Collection[str] = ("success",)) -> list[str]:
        """The workflows whose run has not concluded as one of *admitted*, each with its state."""
        states = ((name, run_state(self.runs.get(name))) for name in REQUIRED_WORKFLOWS)
        return [f"{name} ({state})" for name, state in states if state not in admitted]

    @property
    def passed(self) -> bool:
        """Whether every platform's run passed, which leaves no doubt that it built its packages."""
        return not self.waiting_for()


@dataclass(frozen=True)
class Decision:
    publish: bool
    reason: str
    ref: Ref | None = None
    commit: str = ""
    runs: tuple[Run, ...] = ()
    # Every target of the ref with why it takes the commit or not, and those that take it.
    targets: Mapping[str, str] = field(default_factory=dict)
    selected: tuple[str, ...] = ()


def publishing_ref(name: str) -> Ref | None:
    """Return the branch or release tag that *name* can be, by its form alone."""
    if _BRANCH.fullmatch(name):
        return Ref("branch", name)
    if is_release_tag(name):
        return Ref("tag", name)
    return None


def targets_for(ref: Ref) -> tuple[str, ...]:
    if ref.kind == "tag":
        return ("s3", "latest", "release")
    return ("daily", "s3") if ref.full == DAILY_REF else ("s3",)


def trigger_problem(run: Mapping[str, Any], repo: str) -> str | None:
    """Return why a completed workflow run publishes nothing, or None when it may."""
    workflow = str(run.get("name") or "")
    if REQUIRED_WORKFLOWS.get(workflow) != run.get("path"):
        return f"{code_span(run.get('path'))} is not a platform workflow that publishes"
    if run.get("event") != "push":
        return f"the {workflow} run was started by `{run.get('event')}`, not by a push"
    if run.get("conclusion") not in PUBLISHABLE_CONCLUSIONS["branch"]:
        return f"the {workflow} run concluded `{run.get('conclusion')}`"
    head = (run.get("head_repository") or {}).get("full_name")
    if head != repo:
        return f"the {workflow} run built {head}, not this repository"
    return None


def run_state(run: Mapping[str, Any] | None) -> str:
    """Return ``success``, or what a run is short of it: its conclusion or its status."""
    if run is None:
        return "no run"
    if run.get("status") != "completed":
        return str(run.get("status") or "unknown")
    return str(run.get("conclusion") or "unknown")


def _attempt(run: Mapping[str, Any]) -> int:
    return int(run.get("run_attempt") or 0)


def _newer(run: Mapping[str, Any], other: Mapping[str, Any]) -> bool:
    def age(item: Mapping[str, Any]) -> tuple[str, int]:
        return str(item.get("created_at") or ""), int(item.get("id") or 0)

    return age(run) > age(other)


def builds_of(runs: Mapping[str, Sequence[Run]], ref: Ref, repo: str) -> list[Build]:
    """Group a ref's push runs, listed per workflow, by commit: the newest push first.

    A workflow's run counts only as a push of this repository to the ref by that workflow's own
    file, and only the most recent one per commit (a re-pushed tag builds twice).
    """
    commits: dict[str, dict[str, Run]] = {}
    for workflow, path in REQUIRED_WORKFLOWS.items():
        for run in runs.get(workflow, ()):
            if (
                run.get("path") != path
                or run.get("event") != "push"
                or run.get("head_branch") != ref.name
                or (run.get("head_repository") or {}).get("full_name") != repo
            ):
                continue
            latest = commits.setdefault(str(run.get("head_sha")), {})
            if workflow not in latest or _newer(run, latest[workflow]):
                latest[workflow] = run
    builds = [Build(commit, latest) for commit, latest in commits.items()]
    return sorted(builds, key=lambda build: build.pushed_at, reverse=True)


def target_verdict(
    ref: Ref,
    published: str | None,
    commit: str,
    relation: Callable[[str, str], str | None],
    *,
    republish: bool = False,
) -> tuple[bool, str]:
    """Return whether *commit* replaces the commit a target holds, and why.

    A branch's target only moves forward. *relation* is how its second commit relates to its
    first (``compare_commits``). A tag names one commit, so its builds directory takes whatever
    it names; ``latest/`` is ``release_verdict``'s. *republish* publishes a commit again to a
    target that already holds it.
    """
    if published is None:
        return True, "nothing is published yet"
    if published == commit:
        if republish:
            return True, "holds this commit, which is published again"
        return False, "already holds this commit"
    if ref.kind == "tag":
        return True, f"replaces {published}"
    found = relation(published, commit)
    if found == "ahead":
        return True, f"moves forward from {published}"
    if found in ("behind", "identical"):
        return False, f"holds the newer commit {published}"
    raise RewrittenHistoryError(
        f"{ref.full} was rewritten: the published commit {published} is not an ancestor of "
        f"{commit} ({found or 'unknown to the repository'}). Remove the record of the "
        "published commit to publish from the rewritten branch."
    )


def release_verdict(
    tag: str, commit: str, held: Record | None, *, republish: bool = False, force: bool = False
) -> tuple[bool, str]:
    """Return whether release *tag* at *commit* replaces what ``latest/`` holds, and why.

    ``latest/`` is ordered by version, not by commit: two tags on one commit are two releases,
    and a patch release on an older Stable line is older than a newer line's release. It goes
    back to an older release only when *force* asks for that. The release it holds gives way
    to the same release at another commit: its tag was moved and built again.

    Without a public record (*held* is None) the answer is to start the job. ``latest/`` held a
    release before it had a record, and a missing record reads like a private one from here, so
    skipping the target and taking it for empty would both be wrong. ``aws_upload.py promote``
    reads the record again and refuses without one, unless forced.
    """
    version = release_version(tag)
    if version is None:
        raise ValueError(f"{tag!r} is not a release tag")
    if held is None:
        if force:
            return True, "has no public record of what it holds, and is replaced as requested"
        return True, (
            "has no public record of what it holds: its job reads the record again, and "
            "without one stops until the record is seeded or the release is published by hand "
            "with force_latest"
        )
    held_version = release_version(held.release or "")
    if held_version is None:
        if force:
            return True, f"replaces {held.commit}, whose release is not recorded"
        raise PublishedStateError(
            f"latest/ holds {held.commit}, but its record does not name the release, so {tag} "
            "cannot be compared with it. Publish by hand with force_latest to replace it."
        )
    if held_version == version:
        if held.commit != commit:
            return True, f"replaces {held.commit} of the same release"
        if republish:
            return True, "holds this release, which is published again"
        return False, "already holds this release"
    if held_version < version:
        return True, f"moves forward from {held.release}"
    if force:
        return True, f"goes back from {held.release}, as requested"
    return False, f"holds the newer release {held.release}"


def assets_verdict(
    published: bool, *, force: bool = False, immutable: bool = False
) -> tuple[bool, str]:
    """Return that the job of a tag's GitHub Release starts, and what it is to do there.

    *published* is whether the tag has a published release, whose files do not change under
    its users unless *force* asks for that: the job then only checks that the release has
    these files, and fails when it does not, so that a release published before its packages
    arrived does not pass for complete. Without a published release the job attaches them: a
    draft is hidden from a token that only reads, so ``release_assets.py`` looks for it there.
    GitHub accepts no change to the files of an *immutable* release, whatever *force* asks.
    """
    if not published:
        return True, "has no published release: the packages go to the tag's draft"
    if immutable:
        return True, (
            "is published and immutable: its job checks that it has these files, and nothing "
            "can change them, force_release included"
        )
    if force:
        return True, "is already published, and its files are replaced as requested"
    return True, (
        "is already published: its job checks that it has these files, and changes none "
        "without force_release"
    )


def _run_pages(
    repo: str, workflow_path: str, ref_name: str, commit: str = ""
) -> Iterator[list[Run]]:
    """Yield a workflow's push runs on a branch or tag name, newest first, a page at a time.

    With *commit*, only that commit's runs.
    """
    query = f"branch={quote(ref_name, safe='')}&event=push&per_page={PAGE_SIZE}"
    if commit:
        query += f"&head_sha={commit}"
    path = f"repos/{repo}/actions/workflows/{Path(workflow_path).name}/runs?{query}"
    for page in range(1, RUN_LIMIT // PAGE_SIZE + 1):
        runs = list(gh_api_json(f"{path}&page={page}").get("workflow_runs") or [])
        yield runs
        if len(runs) < PAGE_SIZE:
            return


def push_runs(repo: str, workflow_path: str, ref_name: str, commit: str) -> list[Run]:
    """Return a workflow's push runs for *commit* on a branch or tag name."""
    return [run for page in _run_pages(repo, workflow_path, ref_name, commit) for run in page]


def _listings(repo: str, ref: Ref) -> Iterator[dict[str, list[Run]]]:
    """Yield the ref's push runs by workflow, newest first, a further page of each at a time."""
    listed: dict[str, list[Run]] = {name: [] for name in REQUIRED_WORKFLOWS}
    pages = (_run_pages(repo, path, ref.name) for path in REQUIRED_WORKFLOWS.values())
    for step in zip_longest(*pages, fillvalue=()):
        for name, page in zip(REQUIRED_WORKFLOWS, step, strict=True):
            listed[name] += page
        yield listed


def newest_finished(repo: str, ref: Ref, on_branch: Callable[[str], bool]) -> str:
    """Return the newest commit of a branch whose builds have all finished, or "" without one.

    Finished is what the branch is published from: each platform's run completed, and none
    was stopped. Only a commit the branch still contains counts (*on_branch*): a reset branch
    leaves finished builds behind that can never be published.
    """
    for listed in _listings(repo, ref):
        for build in builds_of(listed, ref, repo):
            if not build.waiting_for(PUBLISHABLE_CONCLUSIONS[ref.kind]) and on_branch(build.commit):
                return build.commit
    return ""


def newer_passed(
    repo: str,
    ref: Ref,
    build: Build,
    relation: Callable[[str, str], str | None],
    on_branch: Callable[[str], bool],
) -> str | None:
    """Return a commit of the branch that every platform passed after *build*'s, or None.

    Such a commit is sure to be published by its own run. A newer one whose run failed
    somewhere may lack a package: it stands in the way of *build* only once a target holds it.
    """
    listed: Mapping[str, Sequence[Run]] = {}
    for listed in _listings(repo, ref):
        # Far enough once every workflow's listing goes back to this build's push.
        oldest = [str(runs[-1].get("created_at") or "") if runs else "" for runs in listed.values()]
        if all(created and created <= build.pushed_at for created in oldest):
            break
    for newer in builds_of(listed, ref, repo):
        if (
            newer.passed
            and newer.commit != build.commit
            and newer.pushed_at >= build.pushed_at
            and relation(build.commit, newer.commit) == "ahead"
            and on_branch(newer.commit)
        ):
            return newer.commit
    return None


def on_stable_branch(repo: str, commit: str) -> bool:
    """Whether a ``Stable*`` branch contains *commit*, as release_tag_check requires of a tag."""
    for ref in gh_api_json(f"repos/{repo}/git/matching-refs/heads/Stable"):
        tip = str((ref.get("object") or {}).get("sha") or "")
        if tip == commit or compare_commits(repo, commit, tip) == "ahead":
            return True
    return False


def verdict(
    repo: str,
    ref: Ref,
    target: str,
    commit: str,
    relation: Callable[[str, str], str | None],
    *,
    republish: bool = False,
    force: Collection[str] = (),
) -> tuple[bool, str]:
    """Return whether *target* takes *commit* of *ref* in place of what it holds, and why.

    *force* names the targets that a request by hand replaces whatever they hold.
    """
    if target == "release":
        published = gh_api_json_or_none(f"repos/{repo}/releases/tags/{ref.name}")
        return assets_verdict(
            published is not None,
            force="release" in force,
            immutable=bool(published) and published.get("immutable") is True,
        )
    if target == "daily":
        return target_verdict(ref, published_commit(repo), commit, relation, republish=republish)
    record = public_record(LATEST_PREFIX if target == "latest" else builds_prefix(ref.name))
    if record and record.repository not in ("", repo):
        # A fork's dry run reads upstream's bucket, whose commits it need not have.
        return True, (
            f"holds a build of {record.repository}, not of this repository: its commits are "
            "not compared, and the job that uploads refuses to replace it"
        )
    if target == "latest":
        return release_verdict(
            ref.name, commit, record, republish=republish, force="latest" in force
        )
    held = record.commit if record else None
    return target_verdict(ref, held, commit, relation, republish=republish)


def decide(
    repo: str,
    *,
    run: Mapping[str, Any] | None = None,
    ref_name: str = "",
    commit: str = "",
    enabled: Collection[str] = TARGETS,
    force: Collection[str] = (),
) -> Decision:
    """Decide for a completed platform *run*, or for *ref_name* and optionally one *commit* of it.

    *enabled* names the targets this repository publishes to. Without a *run* the request was
    made by hand: it publishes again what the targets already hold, and replaces what the
    targets named in *force* hold: for ``latest``, a newer release or one it has no record of;
    for ``release``, the files of a published GitHub Release.
    """
    if run is not None:
        if problem := trigger_problem(run, repo):
            return Decision(False, problem)
        ref_name, commit = str(run.get("head_branch") or ""), str(run.get("head_sha") or "")
    ref = publishing_ref(ref_name)
    if ref is None:
        return Decision(
            False,
            f"{code_span(ref_name)} is not master, a Stable* branch or a vX.Y.Z release tag",
        )
    if run is None and commit and not _COMMIT.fullmatch(commit):
        return Decision(
            False,
            f"{code_span(commit)} is not a full commit SHA: the commit input takes all 40 "
            "digits, in lower case, of a commit of the ref",
            ref,
        )
    admitted = PUBLISHABLE_CONCLUSIONS[ref.kind]
    if run is not None and run.get("conclusion") not in admitted:
        return Decision(
            False,
            f"the {run.get('name')} run concluded `{run.get('conclusion')}`, and a release tag "
            "is published only when every workflow passed",
            ref,
        )
    targets = [target for target in targets_for(ref) if target in enabled]
    if not targets:
        return Decision(False, f"this repository publishes nothing for {ref.full}", ref)
    tip = resolve_ref_commit(repo, ref.full.removeprefix("refs/"))
    if tip is None:
        return Decision(False, f"{ref.full} does not exist", ref)

    relations: dict[tuple[str, str], str | None] = {}

    def relation(base: str, head: str) -> str | None:
        if (base, head) not in relations:
            relations[base, head] = compare_commits(repo, base, head)
        return relations[base, head]

    def on_branch(candidate: str) -> bool:
        return candidate == tip or relation(candidate, tip) == "ahead"

    if ref.kind == "tag":
        if commit and commit != tip:
            return Decision(
                False, f"{commit} is not what tag {ref.name} names ({tip}): not a tag build", ref
            )
        if not on_stable_branch(repo, tip):
            return Decision(False, f"tag {ref.name} is not on a Stable* branch", ref, tip)
        commit = tip
    elif not commit:
        commit = newest_finished(repo, ref, on_branch)
        if not commit:
            return Decision(
                False, f"no commit on {ref.name} has finished building on every platform", ref
            )

    listed = {
        name: push_runs(repo, path, ref.name, commit) for name, path in REQUIRED_WORKFLOWS.items()
    }
    if run is not None:
        # The event is newer than the listing can be, unless the run was re-run since.
        same = [item for item in listed[run["name"]] if item.get("id") == run.get("id")]
        if all(_attempt(item) <= _attempt(run) for item in same):
            others = [item for item in listed[run["name"]] if item not in same]
            listed[run["name"]] = [dict(run), *others]
    builds = {build.commit: build for build in builds_of(listed, ref, repo)}
    build = builds.get(commit, Build(commit, {}))
    if waiting := build.waiting_for(admitted):
        return Decision(False, f"waiting for {', '.join(waiting)}", ref, commit)
    if ref.kind == "branch":
        if not on_branch(commit):
            return Decision(
                False, f"{commit} is not on branch {ref.name}, whose tip is {tip}", ref, commit
            )
        if newer := newer_passed(repo, ref, build, relation, on_branch):
            return Decision(
                False,
                f"{newer} is a newer commit of {ref.name} that passed on every platform, "
                "which its own run publishes",
                ref,
                commit,
            )

    verdicts = {
        target: verdict(
            repo,
            ref,
            target,
            commit,
            relation,
            republish=run is None,
            force=force if run is None else (),
        )
        for target in targets
    }
    selected = tuple(target for target, (publish, _) in verdicts.items() if publish)
    built = "every platform built this commit"
    if failed := build.waiting_for():
        built = (
            f"every platform's run of this commit has completed, but not all passed "
            f"({', '.join(failed)}): it is published if a job that passed built every package"
        )
    return Decision(
        bool(selected),
        built if selected else "every target already holds this commit or a newer one",
        ref,
        commit,
        tuple(build.runs[name] for name in REQUIRED_WORKFLOWS),
        {target: why for target, (_, why) in verdicts.items()},
        selected,
    )


def _runs_output(decision: Decision) -> str:
    return json.dumps(
        [
            {
                "workflow": run["name"],
                "path": run["path"],
                "id": run["id"],
                "attempt": _attempt(run),
                "conclusion": run["conclusion"],
            }
            for run in decision.runs
        ]
    )


def _summarize(decision: Decision, trigger: str, *, dry_run: bool) -> None:
    result = "✅ publish" if decision.publish else "⏭️ nothing to publish"
    builds = (
        f"[{run['name']}]({run['html_url']})"
        + ("" if run["conclusion"] == "success" else f" (`{run['conclusion']}`)")
        for run in decision.runs
    )
    facts: list[tuple[str, object]] = [
        ("Trigger", trigger),
        ("Decision", f"{result}: {decision.reason}"),
        ("Ref", code_span(decision.ref.full) if decision.ref else None),
        ("Commit", code_span(decision.commit) if decision.commit else None),
        *((f"Target `{target}`", why) for target, why in decision.targets.items()),
        ("Builds", ", ".join(builds)),
        ("Mode", "🧪 dry run: nothing is written" if dry_run else None),
    ]
    print("\n".join(f"{label}: {value}" for label, value in facts if value))
    write_step_summary(summary_section("Publish decision", facts))


def _forced(args: argparse.Namespace) -> tuple[str, ...]:
    flags = (("latest", args.force_latest), ("release", args.force_release))
    return tuple(target for target, forced in flags if forced)


def cmd_decide(args: argparse.Namespace) -> int:
    event_path = os.environ.get("GITHUB_EVENT_PATH", "")
    event = read_json(Path(event_path)) if event_path and not args.ref else {}
    run = event.get("workflow_run") if isinstance(event, dict) else None
    if not run and not args.ref:
        gh_error("Nothing to decide: no workflow_run event and no --ref")
        return 2
    dry_run = args.dry_run
    commit = args.commit.strip()
    enabled = [
        target
        for target, variable in (
            ("daily", "PUBLISH_DAILY_RELEASE"),
            ("s3", "PUBLISH_S3"),
            ("latest", "PUBLISH_S3"),
            ("release", "PUBLISH_RELEASE_ASSETS"),
        )
        if dry_run or env_flag(variable)
    ]
    if run:
        trigger = f"{run.get('name')} run {run.get('id')} completed"
    else:
        trigger = f"request for {code_span(args.ref)}" + (
            f" at {code_span(commit)}" if commit else ""
        )
    try:
        decision = decide(
            args.repo,
            run=run or None,
            ref_name=args.ref,
            commit=commit,
            enabled=enabled,
            force=_forced(args),
        )
    except (RewrittenHistoryError, PublishedStateError) as exc:
        gh_error(str(exc), title="Cannot tell what is published")
        return 1
    _summarize(decision, trigger, dry_run=dry_run)
    write_github_output(
        {
            "publish": str(decision.publish).lower(),
            "ref": decision.ref.full if decision.ref else "",
            "commit": decision.commit,
            "runs": _runs_output(decision),
            **{target: str(target in decision.selected).lower() for target in TARGETS},
            "dry-run": str(dry_run).lower(),
        }
    )
    return 0


def cmd_confirm(args: argparse.Namespace) -> int:
    ref = publishing_ref(args.ref.rpartition("/")[2])
    if ref is None or ref.full != args.ref:
        gh_error(f"{args.ref!r} is not a ref that is published")
        return 1
    if args.target not in targets_for(ref) or not _COMMIT.fullmatch(args.commit):
        gh_error(f"{args.target} does not publish {args.commit!r} of {args.ref}")
        return 1
    try:
        # The decision resolved the tag, but this job waited its turn since.
        moved = tag_problem(args.repo, ref.name, args.commit) if ref.kind == "tag" else None
        if moved:
            proceed, why = False, moved
        else:
            proceed, why = verdict(
                args.repo,
                ref,
                args.target,
                args.commit,
                lambda base, head: compare_commits(args.repo, base, head),
                republish=args.republish,
                force=_forced(args),
            )
    except (RewrittenHistoryError, PublishedStateError) as exc:
        gh_error(str(exc), title="Cannot tell what is published")
        return 1
    print(f"{args.target}: {'publish' if proceed else 'skip'}: {why}")
    if not proceed:
        reason = why if moved else f"the target {why}"
        write_step_summary(
            summary_section(f"Target `{args.target}`", [("Result", f"⏭️ not published: {reason}")])
        )
    write_github_output({"proceed": str(proceed).lower()})
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    p_decide = sub.add_parser("decide", help="Decide what the event or --ref publishes")
    p_decide.add_argument(
        "--ref",
        default=env_default("PUBLISH_REF"),
        help="Branch or release tag name to publish instead of the event's (default: $PUBLISH_REF)",
    )
    p_decide.add_argument(
        "--commit",
        default=env_default("PUBLISH_COMMIT"),
        help="Decide for this commit of --ref, not its newest complete one "
        "(default: $PUBLISH_COMMIT)",
    )
    p_decide.add_argument(
        "--dry-run",
        action="store_true",
        default=env_flag("PUBLISH_DRY_RUN"),
        help="Plan every target, whether or not this repository publishes to it "
        "(default: $PUBLISH_DRY_RUN)",
    )
    p_decide.set_defaults(func=cmd_decide)

    p_confirm = sub.add_parser("confirm", help="Check again that one target takes the commit")
    p_confirm.add_argument("--target", required=True, choices=TARGETS)
    p_confirm.add_argument(
        "--republish",
        action="store_true",
        default=env_flag("PUBLISH_REPUBLISH"),
        help="Publish again a commit the target already holds, as a request by hand does "
        "(default: $PUBLISH_REPUBLISH)",
    )
    for option, variable in (("--ref", "PUBLISH_REF"), ("--commit", "PUBLISH_COMMIT")):
        value = env_default(variable)
        p_confirm.add_argument(
            option, default=value, required=not value, help=f"(default: ${variable})"
        )
    p_confirm.set_defaults(func=cmd_confirm)
    for command in (p_decide, p_confirm):
        command.add_argument(
            "--force-latest",
            action="store_true",
            default=env_flag("PUBLISH_FORCE_LATEST"),
            help="On a request by hand, replace whatever latest/ holds: a newer release, or one "
            "it has no record of (default: $PUBLISH_FORCE_LATEST)",
        )
        command.add_argument(
            "--force-release",
            action="store_true",
            default=env_flag("PUBLISH_FORCE_RELEASE"),
            help="On a request by hand, replace the files of a tag's GitHub Release that is "
            "already published (default: $PUBLISH_FORCE_RELEASE)",
        )
        command.add_argument(
            "--repo",
            default="",
            help="Repository in owner/repo format (default: $GITHUB_REPOSITORY)",
        )

    args = parser.parse_args(argv)
    args.repo = args.repo or require_repository()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
