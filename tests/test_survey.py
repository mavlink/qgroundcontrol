"""Tests for survey transect parallel line generation algorithms in qgc4qgis.core.survey."""

import math

import pytest

from qgc4qgis.core.survey import (
    EntryLocation,
    adjust_line_direction,
    adjust_transects_to_entry_location,
    apply_boustrophedon,
    clamp_grid_angle_90,
    clip_lines_with_polygon,
    extend_turnaround,
    generate_parallel_lines,
    generate_survey_transects,
    intersect_lines_with_polygon,
    line_angle,
    optimize_transects_for_shortest_distance,
    reverse_internal_transect_points,
    reverse_transect_order,
    rotate_point,
    segment_intersection,
)


def test_clamp_grid_angle_90():
    """Verify angle clamping to [-90, 90] range matching QGC _clampGridAngle90."""
    assert clamp_grid_angle_90(0.0) == 0.0
    assert clamp_grid_angle_90(45.0) == 45.0
    assert clamp_grid_angle_90(90.0) == 90.0
    assert clamp_grid_angle_90(-90.0) == -90.0
    assert clamp_grid_angle_90(100.0) == -80.0
    assert clamp_grid_angle_90(-100.0) == 80.0
    assert clamp_grid_angle_90(180.0) == 0.0


def test_rotate_point():
    """Verify point rotation around origin by -angle_deg degrees."""
    origin = (0.0, 0.0)
    point = (10.0, 0.0)

    # Angle 90deg -> rotation by -90deg rad -> (0.0, -10.0)
    rx, ry = rotate_point(point, origin, 90.0)
    assert pytest.approx(rx, abs=1e-6) == 0.0
    assert pytest.approx(ry, abs=1e-6) == -10.0

    # Angle 0deg -> unchanged
    rx0, ry0 = rotate_point(point, origin, 0.0)
    assert pytest.approx(rx0, abs=1e-6) == 10.0
    assert pytest.approx(ry0, abs=1e-6) == 0.0


def test_generate_parallel_lines_degenerate_polygon():
    """Verify empty list returned for degenerate polygons (< 3 points or 0 area)."""
    assert generate_parallel_lines([], 0.0, 20.0) == []
    assert generate_parallel_lines([(0.0, 0.0), (10.0, 0.0)], 0.0, 20.0) == []

    # Single point repeated (maxWidth <= 0)
    assert generate_parallel_lines([(5.0, 5.0)] * 3, 0.0, 20.0) == []


def test_generate_parallel_lines_basic():
    """Verify basic parallel line generation across a 100m x 100m square polygon."""
    poly = [(0.0, 0.0), (100.0, 0.0), (100.0, 100.0), (0.0, 100.0)]
    spacing = 20.0
    angle = 0.0

    lines = generate_parallel_lines(poly, grid_angle=angle, grid_spacing=spacing)

    # Bbox = 100x100, diagonal = sqrt(20000) ~= 141.421356
    # maxWidth = diagonal * 1.5 ~= 212.132034
    # halfWidth ~= 106.066017
    # center = (50, 50)
    # transectX goes from -56.066017 to 156.066017 with step 20.0
    # Expected number of steps = ceil(212.132034 / 20.0) = 11
    assert len(lines) == 11

    # Check first line segment coordinates
    p1, p2 = lines[0]
    # For angle 0, rotate_point is identity.
    # top_pt.x == bottom_pt.x == transectX
    assert pytest.approx(p1[0], abs=1e-5) == p2[0]
    assert pytest.approx(p1[1], abs=1e-5) == 50.0 - 106.066017
    assert pytest.approx(p2[1], abs=1e-5) == 50.0 + 106.066017


def test_generate_parallel_lines_refly():
    """Verify refly=True adds 90 degrees to grid angle."""
    poly = [(0.0, 0.0), (100.0, 0.0), (100.0, 100.0), (0.0, 100.0)]
    spacing = 20.0

    lines_refly = generate_parallel_lines(poly, grid_angle=0.0, grid_spacing=spacing, refly=True)
    lines_angle90 = generate_parallel_lines(
        poly, grid_angle=90.0, grid_spacing=spacing, refly=False
    )

    assert len(lines_refly) == len(lines_angle90)
    for (r1, r2), (a1, a2) in zip(lines_refly, lines_angle90, strict=True):
        assert pytest.approx(r1[0], abs=1e-6) == a1[0]
        assert pytest.approx(r1[1], abs=1e-6) == a1[1]
        assert pytest.approx(r2[0], abs=1e-6) == a2[0]
        assert pytest.approx(r2[1], abs=1e-6) == a2[1]


def test_generate_parallel_lines_max_transects_cap():
    """Verify max transect cap (1000) raises grid_spacing when spacing is too small."""
    poly = [(0.0, 0.0), (100.0, 0.0), (100.0, 100.0), (0.0, 100.0)]
    tiny_spacing = 0.001  # Extremely small spacing that would create > 200,000 lines

    lines = generate_parallel_lines(poly, grid_angle=0.0, grid_spacing=tiny_spacing)

    # Grid spacing should be raised to diagonal / 1000 ~= 0.14142m
    # maxWidth = diagonal * 1.5 -> total width sweep = 1.5 * diagonal
    # Number of lines = ceil(1.5 * diagonal / (diagonal / 1000)) = 1500 lines across expanded maxWidth
    assert len(lines) <= 1501


def test_generate_parallel_lines_invalid_grid_spacing():
    """Verify fallback to single center line when grid_spacing <= 0."""
    poly = [(0.0, 0.0), (100.0, 0.0), (100.0, 100.0), (0.0, 100.0)]
    lines = generate_parallel_lines(poly, grid_angle=0.0, grid_spacing=0.0)

    assert len(lines) == 1
    p1, p2 = lines[0]
    assert pytest.approx(p1[0], abs=1e-5) == 50.0
    assert pytest.approx(p2[0], abs=1e-5) == 50.0


def test_segment_intersection():
    """Verify 2D line segment bounded intersection."""
    l1 = ((0.0, 50.0), (100.0, 50.0))
    l2 = ((50.0, 0.0), (50.0, 100.0))
    ipt = segment_intersection(l1, l2)
    assert ipt is not None
    assert pytest.approx(ipt[0], abs=1e-6) == 50.0
    assert pytest.approx(ipt[1], abs=1e-6) == 50.0

    # Non-intersecting parallel segments
    l3 = ((0.0, 0.0), (100.0, 0.0))
    assert segment_intersection(l1, l3) is None

    # Disjoint segments
    l4 = ((200.0, 0.0), (200.0, 100.0))
    assert segment_intersection(l1, l4) is None


def test_intersect_lines_with_polygon_reduction():
    """Verify line x polygon edge intersection reduces multiple points to furthest pair."""
    poly = [(0.0, 0.0), (100.0, 0.0), (100.0, 100.0), (0.0, 100.0)]
    # Sweep line crossing from outside (x=50, y=-50) to (x=50, y=150)
    lines = [((50.0, -50.0), (50.0, 150.0))]

    clipped = intersect_lines_with_polygon(lines, poly)
    assert len(clipped) == 1
    p1, p2 = clipped[0]
    # Points should be (50, 0) and (50, 100)
    assert pytest.approx(min(p1[1], p2[1]), abs=1e-6) == 0.0
    assert pytest.approx(max(p1[1], p2[1]), abs=1e-6) == 100.0


def test_adjust_line_direction():
    """Verify uniformization of line directions with 1 degree tolerance."""
    # First line points upwards: angle ~90deg
    l1 = ((0.0, 0.0), (0.0, 100.0))
    assert pytest.approx(line_angle(l1), abs=1e-6) == 90.0

    # Second line points downwards: angle ~270deg (differing by 180deg > 1deg)
    l2 = ((10.0, 100.0), (10.0, 0.0))
    assert pytest.approx(line_angle(l2), abs=1e-6) == 270.0

    adjusted = adjust_line_direction([l1, l2])
    assert len(adjusted) == 2
    # Second line should be swapped to point upwards
    assert adjusted[1] == ((10.0, 0.0), (10.0, 100.0))
    assert pytest.approx(line_angle(adjusted[1]), abs=1e-6) == 90.0


def test_clip_lines_with_polygon_small_polygon_fallback():
    """Verify fallback for small polygon (< 2 transects intersecting)."""
    # 10m x 10m small polygon
    poly = [(0.0, 0.0), (10.0, 0.0), (10.0, 10.0), (0.0, 10.0)]
    # Grid spacing = 100m, so sweep lines miss the 10m polygon
    parallel_lines = generate_parallel_lines(poly, grid_angle=0.0, grid_spacing=100.0)

    # clip_lines_with_polygon should trigger small polygon fallback and create 1 centered transect
    clipped = clip_lines_with_polygon(parallel_lines, poly)
    assert len(clipped) == 1
    p1, p2 = clipped[0]
    # Transect should pass through center (x=5.0) spanning y from 0 to 10
    assert pytest.approx(p1[0], abs=1e-5) == 5.0
    assert pytest.approx(p2[0], abs=1e-5) == 5.0
    assert pytest.approx(min(p1[1], p2[1]), abs=1e-5) == 0.0
    assert pytest.approx(max(p1[1], p2[1]), abs=1e-5) == 10.0


def test_reversal_helpers():
    """Verify reverse_transect_order and reverse_internal_transect_points."""
    transects = [[(0.0, 0.0), (0.0, 10.0)], [(5.0, 0.0), (5.0, 10.0)]]

    # Order reversal
    rev_order = reverse_transect_order(transects)
    assert rev_order[0] == [(5.0, 0.0), (5.0, 10.0)]
    assert rev_order[1] == [(0.0, 0.0), (0.0, 10.0)]

    # Internal points reversal
    rev_pts = reverse_internal_transect_points(transects)
    assert rev_pts[0] == [(0.0, 10.0), (0.0, 0.0)]
    assert rev_pts[1] == [(5.0, 10.0), (5.0, 0.0)]


def test_adjust_transects_to_entry_location():
    """Verify adjustment of transects for all 4 entryLocation values."""
    transects = [[(0.0, 0.0), (0.0, 10.0)], [(5.0, 0.0), (5.0, 10.0)]]

    # 0: TOP_LEFT -> No reversal
    tl = adjust_transects_to_entry_location(transects, EntryLocation.TOP_LEFT)
    assert tl == transects

    # 1: TOP_RIGHT -> Reverse transect order only
    tr = adjust_transects_to_entry_location(transects, EntryLocation.TOP_RIGHT)
    assert tr[0] == [(5.0, 0.0), (5.0, 10.0)]
    assert tr[1] == [(0.0, 0.0), (0.0, 10.0)]

    # 2: BOTTOM_LEFT -> Reverse internal points only
    bl = adjust_transects_to_entry_location(transects, EntryLocation.BOTTOM_LEFT)
    assert bl[0] == [(0.0, 10.0), (0.0, 0.0)]
    assert bl[1] == [(5.0, 10.0), (5.0, 0.0)]

    # 3: BOTTOM_RIGHT -> Reverse both internal points and transect order
    br = adjust_transects_to_entry_location(transects, EntryLocation.BOTTOM_RIGHT)
    assert br[0] == [(5.0, 10.0), (5.0, 0.0)]
    assert br[1] == [(0.0, 10.0), (0.0, 0.0)]


def test_optimize_transects_for_shortest_distance_d5():
    """Verify optimization evaluating all 4 candidate configurations (D5 rule)."""
    # 3 vertical transects
    l0 = [(0.0, 0.0), (0.0, 10.0)]
    l1 = [(5.0, 0.0), (5.0, 10.0)]
    l2 = [(10.0, 0.0), (10.0, 10.0)]
    transects = [l0, l1, l2]

    # Candidate 0 closest: reference (0, 0)
    opt0 = optimize_transects_for_shortest_distance(transects, (0.0, 0.0))
    assert opt0[0][0] == (0.0, 0.0)

    # Candidate 1 closest: reference (0, 10) -> reversed internal points
    opt1 = optimize_transects_for_shortest_distance(transects, (0.0, 10.0))
    assert opt1[0][0] == (0.0, 10.0)

    # Candidate 2 closest: reference (10, 0) -> reversed transect order
    opt2 = optimize_transects_for_shortest_distance(transects, (10.0, 0.0))
    assert opt2[0][0] == (10.0, 0.0)

    # Candidate 3 closest: reference (10, 10) -> reversed transects order & reversed internal points (D5)
    opt3 = optimize_transects_for_shortest_distance(transects, (10.0, 10.0))
    assert opt3[0][0] == (10.0, 10.0)


def test_apply_boustrophedon():
    """Verify lawnmower pattern alternates direction on odd-indexed transects."""
    transects = [
        [(0.0, 0.0), (0.0, 10.0)],
        [(5.0, 0.0), (5.0, 10.0)],
        [(10.0, 0.0), (10.0, 10.0)],
    ]
    boustro = apply_boustrophedon(transects)
    assert boustro[0] == [(0.0, 0.0), (0.0, 10.0)]
    assert boustro[1] == [(5.0, 10.0), (5.0, 0.0)]
    assert boustro[2] == [(10.0, 0.0), (10.0, 10.0)]


def test_extend_turnaround():
    """Verify extension of entry and exit points by turnaround_distance."""
    transects = [[(0.0, 0.0), (0.0, 10.0)]]
    extended = extend_turnaround(transects, turnaround_distance=5.0)

    assert len(extended[0]) == 4
    # Entry turnaround: 5m before (0, 0) along (0, 1) vector -> (0, -5)
    assert pytest.approx(extended[0][0][0], abs=1e-5) == 0.0
    assert pytest.approx(extended[0][0][1], abs=1e-5) == -5.0
    # Original entry & exit
    assert extended[0][1] == (0.0, 0.0)
    assert extended[0][2] == (0.0, 10.0)
    # Exit turnaround: 5m past (0, 10) along (0, 1) vector -> (0, 15)
    assert pytest.approx(extended[0][3][0], abs=1e-5) == 0.0
    assert pytest.approx(extended[0][3][1], abs=1e-5) == 15.0


def test_generate_survey_transects_pipeline():
    """Verify full generate_survey_transects pipeline including refly90Degrees."""
    poly = [(0.0, 0.0), (100.0, 0.0), (100.0, 100.0), (0.0, 100.0)]
    spacing = 50.0

    # Single pass with turnaround and boustrophedon
    t_single = generate_survey_transects(
        poly,
        grid_angle=0.0,
        grid_spacing=spacing,
        entry_location=EntryLocation.TOP_LEFT,
        turnaround_distance=10.0,
        refly_90_degrees=False,
    )
    assert len(t_single) > 0
    # Each transect should have 4 points due to turnaround extension
    for transect in t_single:
        assert len(transect) == 4

    # Refly pass: should generate double the transects (perpendicular grid added)
    t_refly = generate_survey_transects(
        poly,
        grid_angle=0.0,
        grid_spacing=spacing,
        entry_location=EntryLocation.TOP_LEFT,
        turnaround_distance=10.0,
        refly_90_degrees=True,
    )
    assert len(t_refly) == 2 * len(t_single)


def test_survey_200m_square_transect_count():
    """Verify transect count over a 200m x 200m square for known grid spacings."""
    poly_200 = [(0.0, 0.0), (200.0, 0.0), (200.0, 200.0), (0.0, 200.0)]

    # 50m spacing -> 4 transects
    t_50 = generate_survey_transects(
        poly_200, grid_angle=0.0, grid_spacing=50.0, entry_location=EntryLocation.TOP_LEFT
    )
    assert len(t_50) == 4

    # 40m spacing -> 5 transects
    t_40 = generate_survey_transects(
        poly_200, grid_angle=0.0, grid_spacing=40.0, entry_location=EntryLocation.TOP_LEFT
    )
    assert len(t_40) == 5

    # 100m spacing -> 2 transects
    t_100 = generate_survey_transects(
        poly_200, grid_angle=0.0, grid_spacing=100.0, entry_location=EntryLocation.TOP_LEFT
    )
    assert len(t_100) == 2


def test_survey_200m_square_angle_zero_north_south():
    """Verify grid_angle=0 produces North-South transects (constant X, Y spanning 0 to 200m)."""
    poly_200 = [(0.0, 0.0), (200.0, 0.0), (200.0, 200.0), (0.0, 200.0)]
    transects = generate_survey_transects(
        poly_200, grid_angle=0.0, grid_spacing=50.0, entry_location=EntryLocation.TOP_LEFT
    )

    for transect in transects:
        p1, p2 = transect[0], transect[1]
        assert pytest.approx(p1[0], abs=1e-5) == p2[0]
        min_y = min(p1[1], p2[1])
        max_y = max(p1[1], p2[1])
        assert pytest.approx(min_y, abs=1e-5) == 0.0
        assert pytest.approx(max_y, abs=1e-5) == 200.0


def test_survey_200m_square_entry_location_variants():
    """Verify the 4 entryLocation variants alter only transect order and point direction."""
    poly_200 = [(0.0, 0.0), (200.0, 0.0), (200.0, 200.0), (0.0, 200.0)]
    spacing = 50.0

    t_tl = generate_survey_transects(
        poly_200, grid_angle=0.0, grid_spacing=spacing, entry_location=EntryLocation.TOP_LEFT
    )
    t_tr = generate_survey_transects(
        poly_200, grid_angle=0.0, grid_spacing=spacing, entry_location=EntryLocation.TOP_RIGHT
    )
    t_bl = generate_survey_transects(
        poly_200, grid_angle=0.0, grid_spacing=spacing, entry_location=EntryLocation.BOTTOM_LEFT
    )
    t_br = generate_survey_transects(
        poly_200, grid_angle=0.0, grid_spacing=spacing, entry_location=EntryLocation.BOTTOM_RIGHT
    )

    assert len(t_tl) == len(t_tr) == len(t_bl) == len(t_br) == 4

    # TOP_LEFT starts at min X (left), entry at Y=0
    assert t_tl[0][0][0] < t_tl[-1][0][0]
    assert pytest.approx(t_tl[0][0][1], abs=1e-5) == 0.0

    # TOP_RIGHT reverses transect order (starts at max X / right), entry at Y=0
    assert t_tr[0][0][0] > t_tr[-1][0][0]
    assert pytest.approx(t_tr[0][0][1], abs=1e-5) == 0.0
    x_tl = [t[0][0] for t in t_tl]
    x_tr = [t[0][0] for t in t_tr]
    assert [pytest.approx(x, abs=1e-5) for x in x_tr] == list(
        reversed([pytest.approx(x, abs=1e-5) for x in x_tl])
    )

    # BOTTOM_LEFT starts at min X (left), entry at Y=200 (reversed internal points)
    assert t_bl[0][0][0] < t_bl[-1][0][0]
    assert pytest.approx(t_bl[0][0][1], abs=1e-5) == 200.0

    # BOTTOM_RIGHT starts at max X (right), entry at Y=200 (reversed order & internal points)
    assert t_br[0][0][0] > t_br[-1][0][0]
    assert pytest.approx(t_br[0][0][1], abs=1e-5) == 200.0

    # Geometrically, all 4 variants sweep the exact same X positions
    set_x_tl = sorted(round(t[0][0], 4) for t in t_tl)
    set_x_tr = sorted(round(t[0][0], 4) for t in t_tr)
    set_x_bl = sorted(round(t[0][0], 4) for t in t_bl)
    set_x_br = sorted(round(t[0][0], 4) for t in t_br)
    assert set_x_tl == set_x_tr == set_x_bl == set_x_br


def test_survey_200m_square_turnaround_extension():
    """Verify turnaround extends each transect end by turnaround_distance along transect vector."""
    poly_200 = [(0.0, 0.0), (200.0, 0.0), (200.0, 200.0), (0.0, 200.0)]
    turnaround = 15.0

    transects = generate_survey_transects(
        poly_200,
        grid_angle=0.0,
        grid_spacing=50.0,
        entry_location=EntryLocation.TOP_LEFT,
        turnaround_distance=turnaround,
    )

    for transect in transects:
        assert len(transect) == 4
        ta_entry, entry, exit_pt, ta_exit = transect

        assert pytest.approx(ta_entry[0], abs=1e-5) == entry[0]
        assert pytest.approx(ta_exit[0], abs=1e-5) == exit_pt[0]

        dist_entry = math.hypot(ta_entry[0] - entry[0], ta_entry[1] - entry[1])
        assert pytest.approx(dist_entry, abs=1e-5) == turnaround

        dist_exit = math.hypot(ta_exit[0] - exit_pt[0], ta_exit[1] - exit_pt[1])
        assert pytest.approx(dist_exit, abs=1e-5) == turnaround

    t0 = transects[0]
    assert pytest.approx(t0[0][1], abs=1e-5) == -15.0
    assert pytest.approx(t0[1][1], abs=1e-5) == 0.0
    assert pytest.approx(t0[2][1], abs=1e-5) == 200.0
    assert pytest.approx(t0[3][1], abs=1e-5) == 215.0
