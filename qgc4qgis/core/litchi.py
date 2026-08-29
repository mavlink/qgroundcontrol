"""Litchi CSV exporter for QGC4QGIS route models."""

import csv
import io
from pathlib import Path
from typing import Any

from qgc4qgis.core.i18n import tr
from qgc4qgis.core.missionitems import DistanceMode
from qgc4qgis.core.route import Route

# Pitch bounds for Litchi CSV gimbal pitch angle [-90..20]
MIN_GIMBAL_PITCH = -90.0
MAX_GIMBAL_PITCH = 20.0

# Altitude bounds for Litchi CSV [-200..500]
MIN_ALTITUDE = -200.0
MAX_ALTITUDE = 500.0

# Waypoint count limit for Litchi CSV
MAX_LITCHI_WAYPOINTS = 99


def clamp_gimbal_pitch(pitch: float | int | None, default: float = -90.0) -> float:
    """Clamp gimbal pitch angle to Litchi valid range [-90.0, 20.0]."""
    val = float(pitch if pitch is not None else default)
    return max(MIN_GIMBAL_PITCH, min(MAX_GIMBAL_PITCH, val))


def get_litchi_headers(max_actions: int = 0) -> list[str]:
    """Get D8 Litchi CSV header column names in exact order.

    :param max_actions: Maximum number of action pairs (actiontypeN, actionparamN).
    :return: List of column header names.
    """
    headers = [
        "latitude",
        "longitude",
        "altitude(m)",
        "heading(deg)",
        "curvesize(m)",
        "rotationdir",
        "gimbalmode",
        "gimbalpitchangle",
    ]
    for i in range(1, max_actions + 1):
        headers.extend([f"actiontype{i}", f"actionparam{i}"])
    headers.extend(
        [
            "altitudemode",
            "speed(m/s)",
            "poi_latitude",
            "poi_longitude",
            "poi_altitude(m)",
            "poi_altitudemode",
            "photo_timeinterval",
            "photo_distinterval",
        ]
    )
    return headers


def validate_litchi_route(
    route: Route,
    default_gimbal_pitch: float = -90.0,
) -> list[str]:
    """Validate a Route model against Litchi limits and return a list of warnings.

    :param route: Route dataclass instance to validate.
    :param default_gimbal_pitch: Default gimbal pitch angle in degrees.
    :return: List of warning strings.
    """
    warnings: list[str] = list(route.avisos) if route.avisos else []
    waypoints = route.waypoints if route.waypoints else []

    if len(waypoints) > MAX_LITCHI_WAYPOINTS:
        warnings.append(
            tr(
                "Number of waypoints ({n_waypoints}) exceeds the Litchi limit ({MAX_LITCHI_WAYPOINTS})."
            ).format(n_waypoints=len(waypoints), MAX_LITCHI_WAYPOINTS=MAX_LITCHI_WAYPOINTS)
        )

    for i, wp in enumerate(waypoints):
        pitch = wp.gimbal_pitch if wp.gimbal_pitch is not None else default_gimbal_pitch
        if pitch < MIN_GIMBAL_PITCH or pitch > MAX_GIMBAL_PITCH:
            warnings.append(
                tr(
                    "Gimbal pitch ({pitch:.1f}°) at waypoint {wp_index} outside the range "
                    "[{MIN_GIMBAL_PITCH:.1f}, {MAX_GIMBAL_PITCH:.1f}]."
                ).format(
                    pitch=pitch,
                    wp_index=i + 1,
                    MIN_GIMBAL_PITCH=MIN_GIMBAL_PITCH,
                    MAX_GIMBAL_PITCH=MAX_GIMBAL_PITCH,
                )
            )

        if wp.altura < MIN_ALTITUDE or wp.altura > MAX_ALTITUDE:
            warnings.append(
                tr(
                    "Altitude ({wp.altura:.1f}m) at waypoint {wp_index} outside the range "
                    "[{MIN_ALTITUDE:.1f}, {MAX_ALTITUDE:.1f}]."
                ).format(
                    wp=wp, wp_index=i + 1, MIN_ALTITUDE=MIN_ALTITUDE, MAX_ALTITUDE=MAX_ALTITUDE
                )
            )

    modo = getattr(route, "modo_disparo", getattr(route, "trigger_mode", ""))
    if modo != "POR_FOTO":
        has_photo_action = False
        for wp in waypoints:
            actions = wp.acoes if wp.acoes else []
            for act in actions:
                act_type = act.get("actiontype", act.get("type", act.get("action_type", -1)))
                try:
                    if int(act_type) == 1:
                        has_photo_action = True
                        break
                except (ValueError, TypeError):
                    pass
            if has_photo_action:
                break
        if not has_photo_action:
            warnings.append(
                tr(
                    'Trigger mode "{modo}": the CSV does not carry a "Take Photo" action per waypoint '
                    "— waypoints only sit at the ends of transects and the capture depends on the interval "
                    "(photo_distinterval/photo_timeinterval). For a photo action at every photo center, "
                    'use trigger mode "By photo".'
                ).format(modo=modo)
            )

    return warnings


def route_to_litchi_rows(
    route: Route,
    default_speed: float = 0.0,
    default_gimbal_pitch: float = -90.0,
    photo_timeinterval: float | None = None,
    photo_distinterval: float | None = None,
    max_actions: int | None = None,
) -> list[dict[str, Any]]:
    """Convert a Route model into a list of row dictionaries matching D8 Litchi CSV layout.

    :param route: Route dataclass instance containing waypoints and trigger parameters.
    :param default_speed: Default flight speed in m/s if waypoint speed is None.
    :param default_gimbal_pitch: Default gimbal pitch in degrees if waypoint pitch is None.
    :param photo_timeinterval: Explicit photo time interval in seconds (mutually exclusive with photo_distinterval).
    :param photo_distinterval: Explicit photo distance interval in meters (mutually exclusive with photo_timeinterval).
    :param max_actions: Explicit maximum action pairs; if None, derived from route waypoints.
    :return: List of dictionaries representing Litchi CSV rows in exact D8 header order.
    """
    waypoints = route.waypoints if route.waypoints else []

    # Calculate max actions across all waypoints
    if max_actions is None:
        computed_max = 0
        for wp in waypoints:
            if wp.acoes:
                computed_max = max(computed_max, len(wp.acoes))
        num_action_pairs = computed_max
    else:
        num_action_pairs = max(0, int(max_actions))

    # Resolve mutually exclusive photo_timeinterval / photo_distinterval (D8: unused -> -1)
    time_interval = -1.0
    dist_interval = -1.0

    if photo_distinterval is not None and photo_distinterval > 0:
        dist_interval = float(photo_distinterval)
    elif photo_timeinterval is not None and photo_timeinterval > 0:
        time_interval = float(photo_timeinterval)
    else:
        trigger_mode = getattr(route, "modo_disparo", getattr(route, "trigger_mode", ""))
        trigger_dist = float(
            getattr(route, "distancia_disparo", getattr(route, "trigger_distance", 0.0))
        )
        if trigger_mode == "POR_DISTANCIA" and trigger_dist > 0:
            dist_interval = trigger_dist
        elif trigger_mode == "POR_TEMPO" and trigger_dist > 0:
            time_interval = trigger_dist

    # Map distance mode to Litchi altitudemode (0: Relative to Takeoff/AGL, 1: MSL/Above Ground)
    route_dist_mode = getattr(
        route, "modo_altitude", getattr(route, "distance_mode", DistanceMode.RELATIVE)
    )
    if route_dist_mode in (DistanceMode.RELATIVE, 0, "0", "AGL", "RELATIVE", "relative", "agl"):
        alt_mode = 0
    else:
        alt_mode = 1

    rows: list[dict[str, Any]] = []

    for wp in waypoints:
        row: dict[str, Any] = {
            "latitude": float(wp.lat),
            "longitude": float(wp.lon),
            "altitude(m)": float(wp.altura),
            "heading(deg)": float(wp.heading) if wp.heading is not None else 0.0,
            "curvesize(m)": 0,
            "rotationdir": 0,
            "gimbalmode": 2,
            "gimbalpitchangle": clamp_gimbal_pitch(wp.gimbal_pitch, default=default_gimbal_pitch),
        }

        # Action block
        actions = wp.acoes if wp.acoes else []
        for i in range(1, num_action_pairs + 1):
            if i <= len(actions):
                act = actions[i - 1]
                act_type = act.get("actiontype", act.get("type", act.get("action_type", -1)))
                act_param = act.get("actionparam", act.get("param", act.get("action_param", 0)))
                row[f"actiontype{i}"] = int(act_type) if act_type is not None else -1
                row[f"actionparam{i}"] = float(act_param) if act_param is not None else 0.0
            else:
                # Padding for short lines
                row[f"actiontype{i}"] = -1
                row[f"actionparam{i}"] = 0

        # Trailing columns
        speed_val = wp.velocidade if wp.velocidade is not None else default_speed
        row.update(
            {
                "altitudemode": alt_mode,
                "speed(m/s)": float(speed_val),
                "poi_latitude": 0.0,
                "poi_longitude": 0.0,
                "poi_altitude(m)": 0.0,
                "poi_altitudemode": 0,
                "photo_timeinterval": time_interval,
                "photo_distinterval": dist_interval,
            }
        )

        rows.append(row)

    return rows


def route_to_litchi_csv(
    route: Route,
    default_speed: float = 0.0,
    default_gimbal_pitch: float = -90.0,
    photo_timeinterval: float | None = None,
    photo_distinterval: float | None = None,
    max_actions: int | None = None,
) -> str:
    """Serialize a Route model into a D8 Litchi CSV formatted string."""
    rows = route_to_litchi_rows(
        route=route,
        default_speed=default_speed,
        default_gimbal_pitch=default_gimbal_pitch,
        photo_timeinterval=photo_timeinterval,
        photo_distinterval=photo_distinterval,
        max_actions=max_actions,
    )
    if not rows:
        headers = get_litchi_headers(0)
        output = io.StringIO()
        writer = csv.DictWriter(
            output, fieldnames=headers, lineterminator="\n", quoting=csv.QUOTE_MINIMAL
        )
        writer.writeheader()
        return output.getvalue()

    headers = list(rows[0].keys())
    output = io.StringIO()
    writer = csv.DictWriter(
        output, fieldnames=headers, lineterminator="\n", quoting=csv.QUOTE_MINIMAL
    )
    writer.writeheader()
    writer.writerows(rows)
    return output.getvalue()


def save_litchi_csv(
    filepath: str | Path,
    route: Route,
    default_speed: float = 0.0,
    default_gimbal_pitch: float = -90.0,
    photo_timeinterval: float | None = None,
    photo_distinterval: float | None = None,
    max_actions: int | None = None,
) -> list[str]:
    """Serialize a Route model, write it to a Litchi CSV file, and return warning messages.

    :param filepath: Output path for the Litchi CSV file.
    :param route: Route dataclass instance to export.
    :param default_speed: Default flight speed in m/s.
    :param default_gimbal_pitch: Default gimbal pitch angle in degrees.
    :param photo_timeinterval: Optional explicit photo time interval in seconds.
    :param photo_distinterval: Optional explicit photo distance interval in meters.
    :param max_actions: Optional explicit maximum action pairs.
    :return: List of warning messages.
    """
    csv_str = route_to_litchi_csv(
        route=route,
        default_speed=default_speed,
        default_gimbal_pitch=default_gimbal_pitch,
        photo_timeinterval=photo_timeinterval,
        photo_distinterval=photo_distinterval,
        max_actions=max_actions,
    )
    path = Path(filepath)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as f:
        f.write(csv_str)

    return validate_litchi_route(route, default_gimbal_pitch=default_gimbal_pitch)
