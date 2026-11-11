#!/usr/bin/env python3
"""Generate the EGM96 geoid grid compiled into QGroundControl and the reference values of its test.

The source is GeographicLib's ``egm96-5`` geoid dataset: the height of the EGM96 geoid above the
WGS84 ellipsoid on a 5 arc-minute grid, which GeographicLib computed from NGA's public-domain EGM96
model and distributes as a PGM image.

    https://geographiclib.sourceforge.io/C++/doc/geoid.html
    https://sourceforge.net/projects/geographiclib/files/geoids-distrib/egm96-5.tar.bz2

Neither the build nor CI runs this script. Run it after changing it:

    python3 src/Utilities/Geo/generate_egm96_grid.py --downloads <directory>

It downloads the dataset into <directory> unless it is already there, checks it against the SHA-256
that SourceForge publishes for the file, rewrites ``EGM96Grid.inc`` beside this script and
``test/Utilities/Geo/EGM96Reference.inc``, and prints how far the grid's bilinear interpolation is
from every point of the dataset. The committed grid has 2 cells per degree, the default.
``--cells-per-degree`` selects another resolution, for which the tolerances below and in GeoidTest
have to change, and ``--statistics-only`` prints the error of a resolution without writing.

The reference values are points of the dataset itself, where ``GeographicLib::Geoid`` returns the
stored height without interpolating.
"""

from __future__ import annotations

import argparse
import array
import bz2
import hashlib
import io
import math
import random
import sys
import tarfile
import urllib.request
from pathlib import Path

DATASET_URL = (
    "https://downloads.sourceforge.net/project/geographiclib/geoids-distrib/egm96-5.tar.bz2"
)
DATASET_SHA256 = "c46224f8f723dc915d97179f4e1580a98d6c742fe2b82cd8fef0ecaaad13e614"
DATASET_MEMBER = "geoids/egm96-5.pgm"

# The dataset: 12 points per degree, rows from 90N southwards, columns from 0E eastwards.
POINTS_PER_DEGREE = 12
DATASET_ROWS = 180 * POINTS_PER_DEGREE + 1
DATASET_COLUMNS = 360 * POINTS_PER_DEGREE

REPOSITORY = Path(__file__).resolve().parents[3]
GRID_FILE = Path(__file__).resolve().with_name("EGM96Grid.inc")
REFERENCE_FILE = REPOSITORY / "test" / "Utilities" / "Geo" / "EGM96Reference.inc"

# What GeoidTest asserts of the committed grid; generation fails when the reference values would
# not pass it. The grid is 3.93 m off at most, and at the sampled points 0.70 m and 0.126 m RMS.
NODE_TOLERANCE_METERS = 0.006
MAXIMUM_ERROR_METERS = 4.0
SAMPLE_MAXIMUM_ERROR_METERS = 1.0
SAMPLE_RMS_METERS = 0.15

SAMPLE_SIZE = 300
SAMPLE_SEED = 1996
STEEPEST_POINTS = 24
WORST_POINTS = 12
# The steepest and the worst points are searched for in blocks of this many dataset points, and
# those kept are at least this many degrees apart.
BLOCK_POINTS = 5 * POINTS_PER_DEGREE
SEPARATION_DEGREES = 2.0

# Points of the grid: the poles, both sides of the date line and of the Greenwich meridian, the
# equator, some in each quadrant, and half degrees. A 1 degree grid is 13.7 m off at the one on
# Hawaii and 10 m at the one in the Himalaya.
NODES = [
    (90, 0),
    (90, -120),
    (-90, 0),
    (-90, 75),
    (89, 45),
    (-89, -135),
    (0, 180),
    (0, -180),
    (45, 180),
    (-60, -180),
    (10, 179),
    (10, -179),
    (0, 0),
    (51, 0),
    (-30, 0),
    (0, -1),
    (0, 1),
    (47, 8),
    (35, -120),
    (-34, 151),
    (-23, -46),
    (28, 84),
    (64, -22),
    (-77, 166),
    (19.5, -155.5),
    (30.5, 79.5),
    (89.5, 0.5),
    (-0.5, 179.5),
    (-45.5, -70.5),
]

Point = tuple[float, float]


class Dataset:
    """The geoid heights of the PGM image, in millimeters."""

    def __init__(self, pgm: bytes) -> None:
        header: list[str] = []
        position = 0
        offset = scale = ""
        while len(header) < 3:
            end = pgm.index(b"\n", position)
            line = pgm[position:end].decode("ascii")
            position = end + 1
            if line.startswith("# Offset "):
                offset = line.split()[2]
            elif line.startswith("# Scale "):
                scale = line.split()[2]
            elif not line.startswith("#"):
                header.append(line)
        if header != ["P5", f"{DATASET_COLUMNS} {DATASET_ROWS}", "65535"]:
            raise ValueError(f"unexpected PGM header {header}")
        if (offset, scale) != ("-108", "0.003"):
            raise ValueError(f"unexpected PGM offset {offset} or scale {scale}")
        pixels = array.array("H")
        pixels.frombytes(pgm[position:])
        if sys.byteorder == "little":
            pixels.byteswap()
        if len(pixels) != DATASET_ROWS * DATASET_COLUMNS:
            raise ValueError(f"unexpected PGM size {len(pixels)}")
        # Height in meters is offset + scale * pixel, so in millimeters it is an integer.
        self.millimeters = array.array("i", (3 * pixel - 108000 for pixel in pixels))

    def row(self, row: int) -> array.array:
        return self.millimeters[row * DATASET_COLUMNS : (row + 1) * DATASET_COLUMNS]

    def meters(self, point: Point) -> float:
        row, column = dataset_index(point)
        return self.millimeters[row * DATASET_COLUMNS + column] * 0.001


def dataset_index(point: Point) -> tuple[int, int]:
    return (
        round((90.0 - point[0]) * POINTS_PER_DEGREE),
        round(point[1] * POINTS_PER_DEGREE) % DATASET_COLUMNS,
    )


def dataset_point(row: int, column: int) -> Point:
    """Latitude, and longitude in [-180, 180), of a dataset point."""
    if column >= DATASET_COLUMNS // 2:
        column -= DATASET_COLUMNS
    return (90.0 - row / POINTS_PER_DEGREE, column / POINTS_PER_DEGREE)


def describe(point: Point) -> str:
    latitude, longitude = point
    return (
        f"{abs(latitude):.4g}{'N' if latitude >= 0 else 'S'}"
        f" {abs(longitude):.5g}{'E' if longitude >= 0 else 'W'}"
    )


def load_dataset(downloads: Path) -> Dataset:
    archive = downloads / DATASET_URL.rsplit("/", 1)[1]
    if not archive.exists():
        downloads.mkdir(parents=True, exist_ok=True)
        print(f"Downloading {DATASET_URL}")
        request = urllib.request.Request(DATASET_URL, headers={"User-Agent": "curl/8"})
        with urllib.request.urlopen(request, timeout=300) as response:
            archive.write_bytes(response.read())
    data = archive.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != DATASET_SHA256:
        raise ValueError(f"{archive} has SHA-256 {digest}, expected {DATASET_SHA256}")
    with tarfile.open(fileobj=io.BytesIO(bz2.decompress(data))) as tar:
        member = tar.extractfile(DATASET_MEMBER)
        if member is None:
            raise ValueError(f"{archive} has no {DATASET_MEMBER}")
        return Dataset(member.read())


def centimeters(millimeters: int) -> int:
    """Round to centimeters, halves away from zero."""
    rounded = (abs(millimeters) + 5) // 10
    return rounded if millimeters >= 0 else -rounded


def coarse_grid(dataset: Dataset, cells_per_degree: int) -> list[list[int]]:
    """Centimeter heights: rows from 90S northwards, columns from 180W eastwards to 180E."""
    step = POINTS_PER_DEGREE // cells_per_degree
    half = DATASET_COLUMNS // 2
    grid = []
    for row in range(DATASET_ROWS - 1, -1, -step):
        values = dataset.row(row)
        columns = range(half, half + DATASET_COLUMNS + 1, step)
        grid.append([centimeters(values[column % DATASET_COLUMNS]) for column in columns])
    return grid


def interpolate(grid: list[list[int]], point: Point) -> float:
    """The compiled grid's bilinear interpolation in meters, as QGCGeo::geoidSeparation does it."""
    cells_per_degree = (len(grid) - 1) // 180
    north = (point[0] + 90.0) * cells_per_degree
    east = (point[1] + 180.0) % 360.0 * cells_per_degree
    row = min(int(north), len(grid) - 2)
    column = min(int(east), len(grid[0]) - 2)
    lower, upper = grid[row], grid[row + 1]
    fraction = east - column
    south = lower[column] + (lower[column + 1] - lower[column]) * fraction
    north_value = upper[column] + (upper[column + 1] - upper[column]) * fraction
    return (south + (north_value - south) * (north - row)) * 0.01


class Statistics:
    """The grid's interpolation error at every dataset point."""

    def __init__(self) -> None:
        self.count = 0
        self.squares = 0.0
        self.weighted_squares = 0.0
        self.weights = 0.0
        self.over = dict.fromkeys((1.0, 2.0, 3.0, 5.0), 0)
        # The largest error of each block, with the dataset row and column that has it.
        self.block_worst: dict[tuple[int, int], tuple[float, int, int]] = {}

    def worst(self) -> tuple[float, Point]:
        error, row, column = max(self.block_worst.values(), default=(0.0, 0, 0))
        return error, dataset_point(row, column)

    def summary(self) -> list[str]:
        rms = math.sqrt(self.squares / self.count)
        weighted = math.sqrt(self.weighted_squares / self.weights)
        over = ", ".join(
            f"{limit:g} m on {100.0 * count / self.count:.2g}%"
            for limit, count in self.over.items()
        )
        error, point = self.worst()
        return [
            f"{rms:.2f} m RMS ({weighted:.2f} m weighted by area), by more than {over},",
            f"and by {error:.2f} m at most, at {describe(point)}",
        ]


def measure(dataset: Dataset, grid: list[list[int]]) -> Statistics:
    step = (DATASET_ROWS - 1) // (len(grid) - 1)
    half = DATASET_COLUMNS // 2
    statistics = Statistics()
    limits = sorted(statistics.over)
    for row in range(DATASET_ROWS):
        values = dataset.row(row)
        north = DATASET_ROWS - 1 - row
        i = min(north // step, len(grid) - 2)
        fraction = (north - i * step) / step
        line = [low + (up - low) * fraction for low, up in zip(grid[i], grid[i + 1], strict=True)]
        squares = 0.0
        for column in range(DATASET_COLUMNS):
            east = (column + half) % DATASET_COLUMNS
            j = east // step
            interpolated = line[j] + (line[j + 1] - line[j]) * ((east - j * step) / step)
            error = abs(interpolated * 0.01 - values[column] * 0.001)
            squares += error * error
            if error > 0.5:
                for limit in limits:
                    if error > limit:
                        statistics.over[limit] += 1
                key = (row // BLOCK_POINTS, column // BLOCK_POINTS)
                if error > statistics.block_worst.get(key, (0.0, 0, 0))[0]:
                    statistics.block_worst[key] = (error, row, column)
        weight = math.cos(math.radians(90.0 - row / POINTS_PER_DEGREE))
        statistics.count += DATASET_COLUMNS
        statistics.squares += squares
        statistics.weighted_squares += squares * weight
        statistics.weights += DATASET_COLUMNS * weight
    return statistics


def spread(blocks: dict[tuple[int, int], tuple[float, int, int]], count: int) -> list[Point]:
    """The points of the largest block values that are apart from each other."""
    chosen: list[Point] = []
    for _, row, column in sorted(blocks.values(), reverse=True):
        point = dataset_point(row, column)
        if all(
            abs(point[0] - other[0]) >= SEPARATION_DEGREES
            or abs(point[1] - other[1]) >= SEPARATION_DEGREES
            for other in chosen
        ):
            chosen.append(point)
            if len(chosen) == count:
                break
    return chosen


def steepest_blocks(dataset: Dataset) -> dict[tuple[int, int], tuple[float, int, int]]:
    """The steepest slope of each block; the meridians converge too much beyond 85 degrees."""
    steepest: dict[tuple[int, int], tuple[float, int, int]] = {}
    for row in range(BLOCK_POINTS, DATASET_ROWS - BLOCK_POINTS):
        above, values, below = dataset.row(row - 1), dataset.row(row), dataset.row(row + 1)
        narrowing = 1.0 / math.cos(math.radians(90.0 - row / POINTS_PER_DEGREE))
        for start in range(0, DATASET_COLUMNS, BLOCK_POINTS):
            best = (0.0, 0, 0)
            for column in range(start, start + BLOCK_POINTS):
                northward = above[column] - below[column]
                eastward = (values[(column + 1) % DATASET_COLUMNS] - values[column - 1]) * narrowing
                slope = northward * northward + eastward * eastward
                if slope > best[0]:
                    best = (slope, row, column)
            key = (row // BLOCK_POINTS, start)
            if best > steepest.get(key, (0.0, 0, 0)):
                steepest[key] = best
    return steepest


def reference_points(dataset: Dataset, statistics: Statistics) -> dict[str, list[Point]]:
    generator = random.Random(SAMPLE_SEED)
    sample: list[Point] = []
    while len(sample) < SAMPLE_SIZE:
        # Uniform over the sphere, at dataset points.
        latitude = math.degrees(math.asin(generator.uniform(-1.0, 1.0)))
        point = dataset_point(
            round((90.0 - latitude) * POINTS_PER_DEGREE), generator.randrange(DATASET_COLUMNS)
        )
        if point not in sample:
            sample.append(point)

    steep = spread(steepest_blocks(dataset), STEEPEST_POINTS)
    steep += [point for point in spread(statistics.block_worst, WORST_POINTS) if point not in steep]
    nodes: list[Point] = [(float(latitude), float(longitude)) for latitude, longitude in NODES]
    return {"Node": nodes, "Sample": sample, "Steep": steep}


def check_reference(
    dataset: Dataset, grid: list[list[int]], points: dict[str, list[Point]]
) -> None:
    def errors(kind: str) -> list[float]:
        return [abs(interpolate(grid, point) - dataset.meters(point)) for point in points[kind]]

    node, sample, steep = errors("Node"), errors("Sample"), errors("Steep")
    sample_rms = math.sqrt(sum(error * error for error in sample) / len(sample))
    print(f"Reference: {len(node)} grid points within {max(node):.4f} m")
    print(f"Reference: {len(sample)} sampled within {max(sample):.2f} m, {sample_rms:.3f} m RMS")
    print(f"Reference: {len(steep)} steep within {max(steep):.2f} m")
    if (
        max(node) > NODE_TOLERANCE_METERS
        or max(sample) > SAMPLE_MAXIMUM_ERROR_METERS
        or max(steep) > MAXIMUM_ERROR_METERS
        or sample_rms > SAMPLE_RMS_METERS
    ):
        raise ValueError("the reference values exceed the tolerances GeoidTest asserts")


def write_grid(grid: list[list[int]], statistics: Statistics) -> None:
    cells_per_degree = (len(grid) - 1) // 180
    summary = statistics.summary()
    lines = [
        "// EGM96 geoid height above the WGS84 ellipsoid in centimeters, on a"
        f" {1 / cells_per_degree:g} degree grid.",
        "// Rows run from 90S northwards and columns from 180W eastwards; the last column repeats"
        " the first.",
        f"// Generated by {Path(__file__).name} from GeographicLib's egm96-5 dataset",
        f"// (SHA-256 {DATASET_SHA256}). Do not edit.",
        f"// Bilinear interpolation differs from the dataset's {statistics.count} points by",
        f"// {summary[0]}",
        f"// {summary[1]}.",
        f"constexpr int EGM96_GRID_CELLS_PER_DEGREE = {cells_per_degree};",
        f"constexpr std::int16_t EGM96_GRID_CENTIMETERS[{len(grid)}][{len(grid[0])}] = {{",
    ]
    for index, values in enumerate(grid):
        lines.append(f"    // Latitude {index / cells_per_degree - 90:g}")
        line = "    {"
        for value in values:
            text = f"{value},"
            if len(line) + len(text) > 118:
                lines.append(line)
                line = "     "
            line += text
        lines.append(line.rstrip(",") + "},")
    lines.append("};")
    GRID_FILE.write_text("\n".join(lines) + "\n", encoding="ascii")
    print(f"Wrote {GRID_FILE}")


def write_reference(dataset: Dataset, points: dict[str, list[Point]]) -> None:
    count = sum(len(kind) for kind in points.values())
    lines = [
        f"// {count} EGM96 geoid heights above the WGS84 ellipsoid: kind, latitude and longitude in"
        " degrees, height in meters.",
        f"// Generated by src/Utilities/Geo/{Path(__file__).name} from GeographicLib's egm96-5"
        " dataset",
        f"// (SHA-256 {DATASET_SHA256}). Do not edit.",
        "// Node: points of the compiled grid. Sample: uniform over the sphere. Steep: where the"
        " geoid is steepest",
        "// and where the compiled grid is furthest from it.",
    ]
    for kind, kind_points in points.items():
        lines += [
            f"EGM96_REFERENCE({kind}, {point[0]:.6f}, {point[1]:.6f}, {dataset.meters(point):.3f})"
            for point in kind_points
        ]
    REFERENCE_FILE.write_text("\n".join(lines) + "\n", encoding="ascii")
    print(f"Wrote {REFERENCE_FILE}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate QGroundControl's EGM96 geoid grid and its test reference values."
    )
    parser.add_argument(
        "--downloads", type=Path, required=True, help="directory that keeps the downloaded dataset"
    )
    parser.add_argument(
        "--cells-per-degree",
        type=int,
        default=2,
        choices=(1, 2, 3, 4, 6, 12),
        help="grid resolution (default: %(default)s)",
    )
    parser.add_argument(
        "--statistics-only", action="store_true", help="print the grid's error and write nothing"
    )
    arguments = parser.parse_args()

    dataset = load_dataset(arguments.downloads)
    lowest, highest = min(dataset.millimeters), max(dataset.millimeters)
    print(f"Dataset: {lowest / 1000:.3f} m to {highest / 1000:.3f} m")
    grid = coarse_grid(dataset, arguments.cells_per_degree)
    statistics = measure(dataset, grid)
    print(f"Grid: {len(grid)} x {len(grid[0])} values, {2 * len(grid) * len(grid[0])} bytes")
    print("Error:", " ".join(statistics.summary()))
    for point in spread(statistics.block_worst, WORST_POINTS):
        error = abs(interpolate(grid, point) - dataset.meters(point))
        print(f"  {error:6.2f} m at {describe(point)}")
    if arguments.statistics_only:
        return 0

    points = reference_points(dataset, statistics)
    check_reference(dataset, grid, points)
    write_grid(grid, statistics)
    write_reference(dataset, points)
    return 0


if __name__ == "__main__":
    sys.exit(main())
