"""Job completion markers, bounded job scheduling and the selection of jobs to validate (stdlib only)."""

from __future__ import annotations

import itertools
import random
from concurrent.futures import FIRST_COMPLETED, wait
from typing import TYPE_CHECKING, TypeVar

from .tilemath import JOB_ZOOM, TileKey

if TYPE_CHECKING:
    from collections.abc import Callable, Iterable, Iterator, Mapping, Sequence
    from concurrent.futures import Future
    from pathlib import Path

OCEAN_STATUS = "ocean"
# Tasks queued per pool worker: enough to keep every worker busy while results are collected.
IN_FLIGHT_PER_WORKER = 4

T = TypeVar("T")
R = TypeVar("R")


def done_marker(out: Path, job: TileKey) -> Path:
    return out / "_done" / f"{job.z}_{job.x}_{job.y}"


def mark_done(out: Path, job: TileKey, status: str) -> None:
    marker = done_marker(out, job)
    marker.parent.mkdir(parents=True, exist_ok=True)
    marker.write_text(status + "\n", encoding="utf-8")


def done_markers(out: Path) -> list[Path]:
    return list((out / "_done").glob("*_*_*"))


def check_passed_marker(out: Path) -> Path:
    """Written by a whole-build check that found no problems; production uploads require it."""
    return out / "_check" / "passed"


def completed_jobs(out: Path) -> dict[TileKey, str]:
    """Completed jobs and the status each was marked with."""
    return {
        TileKey(*(int(part) for part in marker.name.split("_"))): marker.read_text(
            encoding="utf-8"
        ).strip()
        for marker in done_markers(out)
    }


def bounded_completion(
    submit: Callable[[T], Future[R]], items: Iterable[T], limit: int
) -> Iterator[tuple[T, Future[R]]]:
    """Yield ``(item, future)`` as futures complete, keeping at most ``limit`` in flight.

    Submitting every job upfront would hold a future and its arguments for each of the
    ~262k jobs of a global run before the first one finishes.
    """
    if limit < 1:
        raise ValueError("Limit must be positive")
    pending = iter(items)
    running = {submit(item): item for item in itertools.islice(pending, limit)}
    while running:
        done, _ = wait(running, return_when=FIRST_COMPLETED)
        for future in done:
            yield running.pop(future), future
        for item in itertools.islice(pending, len(done)):
            running[submit(item)] = item


def parse_job(text: str) -> TileKey:
    """Parse a ``Z/X/Y`` job key; it must be a valid tile at JOB_ZOOM."""
    try:
        z, x, y = (int(part) for part in text.split("/"))
    except ValueError:
        raise ValueError(f"Job must be Z/X/Y, got {text!r}") from None
    if z != JOB_ZOOM or not (0 <= x < 1 << z and 0 <= y < 1 << z):
        raise ValueError(f"Job {text!r} is not a z{JOB_ZOOM} tile")
    return TileKey(z, x, y)


def select_jobs(
    completed: Mapping[TileKey, str],
    *,
    explicit: Sequence[TileKey] = (),
    sample: int | None = None,
    seed: int = 0,
) -> list[TileKey]:
    """Explicit jobs, a seeded random sample of land jobs, or every completed job.

    Sampling skips ocean jobs: they are one shared sea tile with nothing to compare.
    """
    if explicit:
        missing = sorted(set(explicit) - completed.keys())
        if missing:
            raise ValueError(f"Not completed: {', '.join(str(job) for job in missing)}")
        return sorted(set(explicit))
    if sample is not None:
        if sample < 1:
            raise ValueError("Sample size must be positive")
        land = sorted(job for job, status in completed.items() if status != OCEAN_STATUS)
        if sample >= len(land):
            return land
        return sorted(random.Random(seed).sample(land, sample))
    return sorted(completed)
