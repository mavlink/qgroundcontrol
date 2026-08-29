"""Survey transect generation algorithms ported from QGroundControl SurveyComplexItem."""

import math
from collections.abc import Sequence


class EntryLocation:
    """Survey entry location options matching QGC GridEntryLocation."""

    TOP_LEFT = 0
    TOP_RIGHT = 1
    BOTTOM_LEFT = 2
    BOTTOM_RIGHT = 3


MAX_TRANSECT_COUNT = 1000


def clamp_grid_angle_90(angle: float) -> float:
    """Clamp grid angle to range [-90.0, 90.0] degrees.

    Prevents transects from being rotated into a reversed order.
    """
    grid_angle = float(angle)
    if grid_angle > 90.0:
        grid_angle -= 180.0
    elif grid_angle < -90.0:
        grid_angle += 180.0
    return grid_angle


def rotate_point(
    point: tuple[float, float], origin: tuple[float, float], angle_deg: float
) -> tuple[float, float]:
    """Rotate a 2D point around origin by -angle_deg degrees.

    :param point: Coordinate (x, y) to rotate.
    :param origin: Center of rotation (ox, oy).
    :param angle_deg: Angle in degrees. Rotation angle used is -angle_deg.
    :return: Rotated coordinate (rx, ry).
    """
    radians = math.radians(-angle_deg)
    cos_a = math.cos(radians)
    sin_a = math.sin(radians)
    px, py = point
    ox, oy = origin
    dx = px - ox
    dy = py - oy
    rx = (dx * cos_a) - (dy * sin_a) + ox
    ry = (dx * sin_a) + (dy * cos_a) + oy
    return (float(rx), float(ry))


def generate_parallel_lines(
    polygon_points: Sequence[tuple[float, float]],
    grid_angle: float,
    grid_spacing: float,
    refly: bool = False,
    max_transect_count: int = MAX_TRANSECT_COUNT,
) -> list[tuple[tuple[float, float], tuple[float, float]]]:
    """Generate parallel sweep lines across a 2D planar polygon bounding box.

    Ported from QGroundControl SurveyComplexItem logic:
    - Clamps grid_angle to [-90, 90] degrees using clamp_grid_angle_90.
    - Adds 90 degrees if refly is True.
    - Computes bounding box, center, and diagonal of polygon_points.
    - Sets maxWidth = diagonal * 1.5.
    - Enforces max_transect_count limit by raising grid_spacing to diagonal / max_transect_count if needed.
    - Sweeps along X from bounding_center.x - maxWidth / 2 to bounding_center.x + maxWidth / 2.
    - Rotates each line by -grid_angle around bounding_center.

    :param polygon_points: List/sequence of 2D (x, y) vertices in meters.
    :param grid_angle: Grid angle in degrees.
    :param grid_spacing: Spacing between transects in meters.
    :param refly: If True, adds 90 degrees to grid angle for cross-grid survey.
    :param max_transect_count: Maximum allowed transects (default 1000).
    :return: List of line segments [((x1, y1), (x2, y2)), ...].
    """
    if len(polygon_points) < 3:
        return []

    angle = clamp_grid_angle_90(grid_angle)
    if refly:
        angle += 90.0

    min_x = min(pt[0] for pt in polygon_points)
    max_x = max(pt[0] for pt in polygon_points)
    min_y = min(pt[1] for pt in polygon_points)
    max_y = max(pt[1] for pt in polygon_points)

    width = max_x - min_x
    height = max_y - min_y
    bounding_center = ((min_x + max_x) / 2.0, (min_y + max_y) / 2.0)

    diagonal = math.hypot(width, height)
    max_width = diagonal * 1.5

    if max_width <= 0.0:
        return []

    lines: list[tuple[tuple[float, float], tuple[float, float]]] = []

    if grid_spacing <= 0.0:
        half_w = max_width / 2.0
        top_pt = (bounding_center[0], bounding_center[1] - half_w)
        bottom_pt = (bounding_center[0], bounding_center[1] + half_w)
        p1 = rotate_point(top_pt, bounding_center, angle)
        p2 = rotate_point(bottom_pt, bounding_center, angle)
        lines.append((p1, p2))
    else:
        min_spacing = diagonal / max_transect_count
        if grid_spacing < min_spacing:
            grid_spacing = min_spacing

        half_width = max_width / 2.0
        transect_x = bounding_center[0] - half_width
        transect_x_max = transect_x + max_width
        while transect_x < transect_x_max:
            top_pt = (transect_x, bounding_center[1] - half_width)
            bottom_pt = (transect_x, bounding_center[1] + half_width)
            p1 = rotate_point(top_pt, bounding_center, angle)
            p2 = rotate_point(bottom_pt, bounding_center, angle)
            lines.append((p1, p2))
            transect_x += grid_spacing

    return lines


def segment_intersection(
    line1: tuple[tuple[float, float], tuple[float, float]],
    line2: tuple[tuple[float, float], tuple[float, float]],
) -> tuple[float, float] | None:
    """Find intersection point of two 2D line segments (bounded intersection).

    :param line1: First line segment ((x1, y1), (x2, y2)).
    :param line2: Second line segment ((x3, y3), (x4, y4)).
    :return: (x, y) coordinate tuple if segments intersect, else None.
    """
    (x1, y1), (x2, y2) = line1
    (x3, y3), (x4, y4) = line2

    rx, ry = x2 - x1, y2 - y1
    sx, sy = x4 - x3, y4 - y3

    denom = rx * sy - ry * sx
    if abs(denom) < 1e-12:
        return None

    qp_x, qp_y = x3 - x1, y3 - y1
    t = (qp_x * sy - qp_y * sx) / denom
    u = (qp_x * ry - qp_y * rx) / denom

    if -1e-9 <= t <= 1.0 + 1e-9 and -1e-9 <= u <= 1.0 + 1e-9:
        t_clamped = max(0.0, min(1.0, t))
        ix = x1 + t_clamped * rx
        iy = y1 + t_clamped * ry
        return (float(ix), float(iy))

    return None


def intersect_lines_with_polygon(
    line_list: Sequence[tuple[tuple[float, float], tuple[float, float]]],
    polygon_points: Sequence[tuple[float, float]],
) -> list[tuple[tuple[float, float], tuple[float, float]]]:
    """Intersect line segments with polygon edges and reduce to furthest point pairs.

    Ported from QGroundControl SurveyComplexItem::_intersectLinesWithPolygon:
    - Intersects line segments with all polygon edges.
    - If multiple intersection points exist along a line, selects the pair furthest apart.

    :param line_list: Sequence of line segments [((x1, y1), (x2, y2)), ...].
    :param polygon_points: Sequence of 2D (x, y) polygon vertices in meters.
    :return: List of intersected line segments.
    """
    if len(polygon_points) < 3:
        return []

    poly = list(polygon_points)
    if poly[0] != poly[-1]:
        poly.append(poly[0])

    result_lines: list[tuple[tuple[float, float], tuple[float, float]]] = []

    for line in line_list:
        intersections: list[tuple[float, float]] = []

        for j in range(len(poly) - 1):
            edge = (poly[j], poly[j + 1])
            ipt = segment_intersection(line, edge)
            if ipt is not None and not any(
                math.hypot(pt[0] - ipt[0], pt[1] - ipt[1]) < 1e-6 for pt in intersections
            ):
                intersections.append(ipt)

        if len(intersections) > 1:
            first_point: tuple[float, float] | None = None
            second_point: tuple[float, float] | None = None
            current_max_distance = 0.0

            for pt1 in intersections:
                for pt2 in intersections:
                    dist = math.hypot(pt1[0] - pt2[0], pt1[1] - pt2[1])
                    if dist > current_max_distance:
                        first_point = pt1
                        second_point = pt2
                        current_max_distance = dist

            if first_point is not None and second_point is not None and current_max_distance > 0.0:
                result_lines.append((first_point, second_point))

    return result_lines


def line_angle(line: tuple[tuple[float, float], tuple[float, float]]) -> float:
    """Calculate angle of line segment from p1 to p2 in degrees [0.0, 360.0)."""
    p1, p2 = line
    dx = p2[0] - p1[0]
    dy = p2[1] - p1[1]
    angle = math.degrees(math.atan2(dy, dx))
    if angle < 0:
        angle += 360.0
    return angle


def adjust_line_direction(
    line_list: Sequence[tuple[tuple[float, float], tuple[float, float]]],
) -> list[tuple[tuple[float, float], tuple[float, float]]]:
    """Adjust line segments so all transects run in the same direction (P1 -> P2).

    Ported from QGroundControl SurveyComplexItem::_adjustLineDirection:
    - Uses angle of first line as reference direction.
    - If a line's angle differs from first angle by > 1.0 degree, swaps P1 and P2.

    :param line_list: Sequence of line segments.
    :return: List of line segments with uniform direction.
    """
    if not line_list:
        return []

    result_lines: list[tuple[tuple[float, float], tuple[float, float]]] = []
    first_angle = line_angle(line_list[0])

    for line in line_list:
        ang = line_angle(line)
        if abs(ang - first_angle) > 1.0:
            result_lines.append((line[1], line[0]))
        else:
            result_lines.append(line)

    return result_lines


def clip_lines_with_polygon(
    line_list: Sequence[tuple[tuple[float, float], tuple[float, float]]],
    polygon_points: Sequence[tuple[float, float]],
) -> list[tuple[tuple[float, float], tuple[float, float]]]:
    """Clip parallel sweep lines to polygon boundaries with fallback for small polygons.

    Ported from QGroundControl SurveyComplexItem polygon clipping logic:
    - Intersects line_list with polygon edges via intersect_lines_with_polygon.
    - If < 2 transects intersect the polygon and line_list is non-empty, creates a single
      centered transect passing through the polygon bounding center and intersects it.
    - Adjusts line direction of all intersected transects with 1° tolerance using adjust_line_direction.

    :param line_list: List of parallel sweep line segments.
    :param polygon_points: Sequence of 2D (x, y) polygon vertices in meters.
    :return: List of clipped transects with uniform direction.
    """
    if len(polygon_points) < 3:
        return []

    intersect_lines = intersect_lines_with_polygon(line_list, polygon_points)

    if len(intersect_lines) < 2 and line_list:
        min_x = min(pt[0] for pt in polygon_points)
        max_x = max(pt[0] for pt in polygon_points)
        min_y = min(pt[1] for pt in polygon_points)
        max_y = max(pt[1] for pt in polygon_points)
        bounding_center = ((min_x + max_x) / 2.0, (min_y + max_y) / 2.0)

        first_line = line_list[0]
        line_center = (
            (first_line[0][0] + first_line[1][0]) / 2.0,
            (first_line[0][1] + first_line[1][1]) / 2.0,
        )
        center_offset = (
            bounding_center[0] - line_center[0],
            bounding_center[1] - line_center[1],
        )
        centered_first_line = (
            (
                first_line[0][0] + center_offset[0],
                first_line[0][1] + center_offset[1],
            ),
            (
                first_line[1][0] + center_offset[0],
                first_line[1][1] + center_offset[1],
            ),
        )
        intersect_lines = intersect_lines_with_polygon([centered_first_line], polygon_points)

    return adjust_line_direction(intersect_lines)


def reverse_transect_order(
    transects: Sequence[Sequence[tuple[float, float]]],
) -> list[list[tuple[float, float]]]:
    """Reverse the order of transects (first transect becomes last)."""
    return [list(t) for t in reversed(transects)]


def reverse_internal_transect_points(
    transects: Sequence[Sequence[tuple[float, float]]],
) -> list[list[tuple[float, float]]]:
    """Reverse the points within each transect (first point becomes last)."""
    return [list(reversed(t)) for t in transects]


def adjust_transects_to_entry_location(
    transects: Sequence[Sequence[tuple[float, float]]],
    entry_location: int = EntryLocation.TOP_LEFT,
) -> list[list[tuple[float, float]]]:
    """Adjust transect order and internal orientation according to entry location.

    Entry location mapping:
    - 0 (TOP_LEFT): Default orientation.
    - 1 (TOP_RIGHT): Reverse transect order.
    - 2 (BOTTOM_LEFT): Reverse points within each transect.
    - 3 (BOTTOM_RIGHT): Reverse points within each transect AND reverse transect order.

    :param transects: Sequence of transect point sequences.
    :param entry_location: Entry location index (0..3).
    :return: Adjusted list of transects.
    """
    if not transects:
        return []

    res = [list(t) for t in transects]
    reverse_points = entry_location in (EntryLocation.BOTTOM_LEFT, EntryLocation.BOTTOM_RIGHT)
    reverse_transects = entry_location in (EntryLocation.TOP_RIGHT, EntryLocation.BOTTOM_RIGHT)

    if reverse_points:
        res = reverse_internal_transect_points(res)
    if reverse_transects:
        res = reverse_transect_order(res)

    return res


def optimize_transects_for_shortest_distance(
    transects: Sequence[Sequence[tuple[float, float]]],
    distance_coord: tuple[float, float],
) -> list[list[tuple[float, float]]]:
    """Reorder transects and internal points so the starting point of the first transect
    is closest to distance_coord.

    Evaluates all 4 candidate configurations (D5):
    - Candidate 0: Original order, original internal points
    - Candidate 1: Original order, reversed internal points
    - Candidate 2: Reversed order, original internal points
    - Candidate 3: Reversed order, reversed internal points

    :param transects: Sequence of transect point sequences.
    :param distance_coord: 2D reference point (x, y).
    :return: Reordered list of transects.
    """
    if not transects:
        return []

    first_transect = transects[0]
    last_transect = transects[-1]

    def dist(p1: tuple[float, float], p2: tuple[float, float]) -> float:
        return math.hypot(p1[0] - p2[0], p1[1] - p2[1])

    d0 = dist(first_transect[0], distance_coord)
    d1 = dist(first_transect[-1], distance_coord)
    d2 = dist(last_transect[0], distance_coord)
    d3 = dist(last_transect[-1], distance_coord)

    candidate_distances = [d0, d1, d2, d3]
    shortest_index = 0
    shortest_distance = candidate_distances[0]
    for i in range(1, 4):  # Check all 4 candidates (0, 1, 2, 3) (D5)
        if candidate_distances[i] < shortest_distance:
            shortest_index = i
            shortest_distance = candidate_distances[i]

    res = [list(t) for t in transects]
    if shortest_index > 1:
        res = reverse_transect_order(res)
    if shortest_index & 1:
        res = reverse_internal_transect_points(res)

    return res


def apply_boustrophedon(
    transects: Sequence[Sequence[tuple[float, float]]],
) -> list[list[tuple[float, float]]]:
    """Apply boustrophedon (lawnmower) pattern by reversing alternate transects (index 1, 3, 5...).

    :param transects: Sequence of transect point sequences.
    :return: List of transects with alternating direction.
    """
    res: list[list[tuple[float, float]]] = []
    for i, transect in enumerate(transects):
        pts = list(transect)
        if i % 2 == 1:
            pts = list(reversed(pts))
        res.append(pts)
    return res


def extend_turnaround(
    transects: Sequence[Sequence[tuple[float, float]]],
    turnaround_distance: float,
) -> list[list[tuple[float, float]]]:
    """Extend transect ends by turnaround_distance meters along the transect direction.

    Adds turnaround coordinates before entry and after exit points of each transect.

    :param transects: Sequence of transect point sequences.
    :param turnaround_distance: Extension distance in meters.
    :return: List of extended transects.
    """
    if turnaround_distance <= 0.0:
        return [list(t) for t in transects]

    res: list[list[tuple[float, float]]] = []
    for transect in transects:
        pts = list(transect)
        if len(pts) < 2:
            res.append(pts)
            continue

        p1 = pts[0]
        p2 = pts[1]
        dx1 = p2[0] - p1[0]
        dy1 = p2[1] - p1[1]
        len1 = math.hypot(dx1, dy1)

        p_last = pts[-1]
        p_prev = pts[-2]
        dx2 = p_last[0] - p_prev[0]
        dy2 = p_last[1] - p_prev[1]
        len2 = math.hypot(dx2, dy2)

        extended_pts = list(pts)

        if len1 > 1e-12:
            u1_x = dx1 / len1
            u1_y = dy1 / len1
            ta_entry = (
                p1[0] - u1_x * turnaround_distance,
                p1[1] - u1_y * turnaround_distance,
            )
            extended_pts.insert(0, ta_entry)

        if len2 > 1e-12:
            u2_x = dx2 / len2
            u2_y = dy2 / len2
            ta_exit = (
                p_last[0] + u2_x * turnaround_distance,
                p_last[1] + u2_y * turnaround_distance,
            )
            extended_pts.append(ta_exit)

        res.append(extended_pts)

    return res


def generate_survey_transects(
    polygon_points: Sequence[tuple[float, float]],
    grid_angle: float,
    grid_spacing: float,
    entry_location: int = EntryLocation.TOP_LEFT,
    turnaround_distance: float = 0.0,
    refly_90_degrees: bool = False,
    max_transect_count: int = MAX_TRANSECT_COUNT,
) -> list[list[tuple[float, float]]]:
    """Generate ordered survey transects with entry location, boustrophedon pattern,
    turnaround extensions, optional refly cross-grid, and shortest-distance candidate optimization.

    :param polygon_points: List/sequence of 2D (x, y) vertices in meters.
    :param grid_angle: Grid angle in degrees.
    :param grid_spacing: Spacing between transects in meters.
    :param entry_location: Entry location index (0: TOP_LEFT, 1: TOP_RIGHT, 2: BOTTOM_LEFT, 3: BOTTOM_RIGHT).
    :param turnaround_distance: Distance in meters to extend transect ends for turnaround.
    :param refly_90_degrees: If True, generates a second perpendicular grid (+90°) and optimizes entry distance.
    :param max_transect_count: Maximum allowed transects (default 1000).
    :return: List of ordered transects, where each transect is a list of 2D (x, y) point tuples.
    """
    if len(polygon_points) < 3:
        return []

    lines1 = generate_parallel_lines(
        polygon_points,
        grid_angle=grid_angle,
        grid_spacing=grid_spacing,
        refly=False,
        max_transect_count=max_transect_count,
    )
    clipped1 = clip_lines_with_polygon(lines1, polygon_points)
    transects1: list[list[tuple[float, float]]] = [[p1, p2] for p1, p2 in clipped1]

    if not transects1:
        return []

    transects1 = adjust_transects_to_entry_location(transects1, entry_location)
    transects1 = apply_boustrophedon(transects1)
    transects1 = extend_turnaround(transects1, turnaround_distance)

    if refly_90_degrees:
        lines2 = generate_parallel_lines(
            polygon_points,
            grid_angle=grid_angle,
            grid_spacing=grid_spacing,
            refly=True,
            max_transect_count=max_transect_count,
        )
        clipped2 = clip_lines_with_polygon(lines2, polygon_points)
        transects2: list[list[tuple[float, float]]] = [[p1, p2] for p1, p2 in clipped2]

        if transects2:
            transects2 = adjust_transects_to_entry_location(transects2, entry_location)
            ref_coord = transects1[-1][-1]
            transects2 = optimize_transects_for_shortest_distance(transects2, ref_coord)
            transects2 = apply_boustrophedon(transects2)
            transects2 = extend_turnaround(transects2, turnaround_distance)
            return transects1 + transects2

    return transects1
