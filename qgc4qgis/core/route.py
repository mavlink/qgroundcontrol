"""Route generation and waypoint data structures for QGC4QGIS."""

import math
from collections.abc import Sequence
from dataclasses import dataclass, field
from typing import Any

from qgc4qgis.core.geo import AEQDProjection, geodesic_azimuth
from qgc4qgis.core.i18n import tr
from qgc4qgis.core.missionitems import DistanceMode


@dataclass
class PhotoCenter:
    """Sampled photo center along a survey transect."""

    lat: float
    lon: float
    azimuth_deg: float
    photo_in_transect_id: int
    px: float = 0.0
    py: float = 0.0
    ux: float = 0.0
    uy: float = 0.0
    proj: Any = None

    def footprint_corners(self, fp_frontal: float, fp_side: float) -> list[tuple[float, float]]:
        """Calculate footprint corner geodetic coordinates (lat, lon) for this photo center."""
        if self.proj is None:
            return []
        h = fp_frontal / 2.0
        w = fp_side / 2.0
        c1 = (self.px + h * self.ux + w * self.uy, self.py + h * self.uy - w * self.ux)
        c2 = (self.px - h * self.ux + w * self.uy, self.py - h * self.uy - w * self.ux)
        c3 = (self.px - h * self.ux - w * self.uy, self.py - h * self.uy + w * self.ux)
        c4 = (self.px + h * self.ux - w * self.uy, self.py + h * self.uy + w * self.ux)
        return [self.proj.inverse(cx, cy) for cx, cy in (c1, c2, c3, c4)]


def sample_photo_centers(
    transect: Sequence[tuple[float, ...]],
    trigger_distance: float,
) -> list[PhotoCenter]:
    """Sample photo centers along a single transect based on trigger distance.

    :param transect: Sequence of geodetic point tuples (lat, lon) or (lat, lon, alt).
    :param trigger_distance: Distance between photo triggers in meters.
    :return: List of PhotoCenter instances.
    """
    if len(transect) < 2:
        return []

    ref_lat, ref_lon = float(transect[0][0]), float(transect[0][1])
    proj = AEQDProjection(ref_lat, ref_lon)
    planar_transect = [proj.forward(float(pt[0]), float(pt[1])) for pt in transect]

    photo_in_transect_id = 1
    centers: list[PhotoCenter] = []

    for k in range(len(planar_transect) - 1):
        p1 = planar_transect[k]
        p2 = planar_transect[k + 1]
        dx = p2[0] - p1[0]
        dy = p2[1] - p1[1]
        seg_len = math.hypot(dx, dy)
        if seg_len < 1e-6:
            continue

        ux = dx / seg_len
        uy = dy / seg_len
        azimuth_deg = math.degrees(math.atan2(ux, uy)) % 360.0

        d = 0.0
        while d <= seg_len + 1e-6:
            px = p1[0] + d * ux
            py = p1[1] + d * uy
            lat_p, lon_p = proj.inverse(px, py)

            centers.append(
                PhotoCenter(
                    lat=lat_p,
                    lon=lon_p,
                    azimuth_deg=azimuth_deg,
                    photo_in_transect_id=photo_in_transect_id,
                    px=px,
                    py=py,
                    ux=ux,
                    uy=uy,
                    proj=proj,
                )
            )

            photo_in_transect_id += 1

            if trigger_distance <= 0.0:
                break
            d += trigger_distance

    return centers


@dataclass
class RouteWaypoint:
    """Waypoint in a flight route."""

    lat: float
    lon: float
    altura: float = 0.0
    velocidade: float | None = None
    heading: float | None = None
    gimbal_pitch: float | None = None
    acoes: list[dict[str, Any]] = field(default_factory=list)

    @property
    def alt(self) -> float:
        """Alias for altura."""
        return self.altura

    @property
    def actions(self) -> list[dict[str, Any]]:
        """Alias for acoes."""
        return self.acoes


@dataclass
class Route:
    """Flight route containing a sequence of waypoints and survey parameters."""

    waypoints: list[RouteWaypoint] = field(default_factory=list)
    modo_disparo: str = "POR_DISTANCIA"
    distancia_disparo: float = 0.0
    modo_altitude: int = DistanceMode.RELATIVE
    avisos: list[str] = field(default_factory=list)

    @property
    def trigger_mode(self) -> str:
        """Alias for modo_disparo."""
        return self.modo_disparo

    @property
    def trigger_distance(self) -> float:
        """Alias for distancia_disparo."""
        return self.distancia_disparo

    @property
    def distance_mode(self) -> int:
        """Alias for modo_altitude."""
        return self.modo_altitude

    @property
    def warnings(self) -> list[str]:
        """Alias for avisos."""
        return self.avisos


def route_from_transects(
    transects: Sequence[Sequence[tuple[float, ...]]],
    altitude: float = 50.0,
    trigger_distance: float = 0.0,
    trigger_mode: str = "POR_DISTANCIA",
    distance_mode: int = DistanceMode.RELATIVE,
    flight_speed: float | None = None,
    gimbal_pitch: float | None = None,
    *,
    altura: float | None = None,
    distancia_disparo: float | None = None,
    modo_disparo: str | None = None,
    modo_altitude: int | None = None,
    velocidade: float | None = None,
    avisos: Sequence[str] | None = None,
    max_waypoints: int = 500,
) -> Route:
    """Generate a Route from survey transects in POR_DISTANCIA or POR_FOTO mode.

    In POR_DISTANCIA mode, creates waypoints at transect endpoints.
    In POR_FOTO mode, samples waypoints at photo centers along each transect.
    Calculates heading as the geodesic forward azimuth along segments.

    :param transects: Sequence of transects, each containing geodetic point tuples (lat, lon) or (lat, lon, alt).
    :param altitude: Default altitude in meters if points are 2D (lat, lon).
    :param trigger_distance: Distance between photo triggers in meters.
    :param trigger_mode: Trigger mode string ("POR_DISTANCIA" or "POR_FOTO").
    :param distance_mode: Distance mode enum value (default DistanceMode.RELATIVE = 1).
    :param flight_speed: Flight speed in m/s (optional).
    :param gimbal_pitch: Gimbal pitch angle in degrees (optional).
    :param altura: Alias for altitude.
    :param distancia_disparo: Alias for trigger_distance.
    :param modo_disparo: Alias for trigger_mode.
    :param modo_altitude: Alias for distance_mode.
    :param velocidade: Alias for flight_speed.
    :param avisos: Initial warnings list.
    :param max_waypoints: Maximum allowed waypoints before appending a warning (default 500).
    :return: Route dataclass instance with generated waypoints.
    """
    alt_val = float(altura if altura is not None else altitude)
    trig_dist_val = float(distancia_disparo if distancia_disparo is not None else trigger_distance)
    trig_mode_val = str(modo_disparo if modo_disparo is not None else trigger_mode)
    dist_mode_val = int(modo_altitude if modo_altitude is not None else distance_mode)
    speed_val = (
        float(velocidade if velocidade is not None else flight_speed)
        if (velocidade is not None or flight_speed is not None)
        else None
    )
    gimbal_val = float(gimbal_pitch) if gimbal_pitch is not None else None
    warnings_list = list(avisos) if avisos is not None else []

    waypoints: list[RouteWaypoint] = []

    if trig_mode_val == "POR_FOTO" and trig_dist_val > 0.0:
        for transect in transects:
            centers = sample_photo_centers(transect, trigger_distance=trig_dist_val)
            for center in centers:
                wp = RouteWaypoint(
                    lat=center.lat,
                    lon=center.lon,
                    altura=alt_val,
                    velocidade=speed_val,
                    heading=center.azimuth_deg,
                    gimbal_pitch=gimbal_val,
                    acoes=[{"actiontype": 1, "actionparam": 0}],
                )
                waypoints.append(wp)
    else:
        for transect in transects:
            transect_wps: list[RouteWaypoint] = []
            for pt in transect:
                alt = float(pt[2]) if len(pt) >= 3 else alt_val
                transect_wps.append(
                    RouteWaypoint(
                        lat=float(pt[0]),
                        lon=float(pt[1]),
                        altura=alt,
                        velocidade=speed_val,
                        heading=0.0,
                        gimbal_pitch=gimbal_val,
                        acoes=[],
                    )
                )

            prev_heading: float | None = None
            for i, wp in enumerate(transect_wps):
                if i < len(transect_wps) - 1:
                    nxt = transect[i + 1]
                    hdg = geodesic_azimuth(wp.lat, wp.lon, float(nxt[0]), float(nxt[1]))
                elif prev_heading is not None:
                    hdg = prev_heading
                else:
                    hdg = 0.0
                wp.heading = hdg
                prev_heading = hdg

            waypoints.extend(transect_wps)

    if max_waypoints > 0 and len(waypoints) > max_waypoints:
        warnings_list.append(
            tr("Number of waypoints ({n_waypoints}) exceeds the limit ({max_waypoints}).").format(
                n_waypoints=len(waypoints), max_waypoints=max_waypoints
            )
        )

    return Route(
        waypoints=waypoints,
        modo_disparo=trig_mode_val,
        distancia_disparo=trig_dist_val,
        modo_altitude=dist_mode_val,
        avisos=warnings_list,
    )


def rebase_route_to_takeoff(route: Route, takeoff_elevation: float) -> Route:
    """Rebase route waypoints altitude relative to takeoff elevation.

    If route.modo_altitude is already DistanceMode.RELATIVE, returns route unchanged (idempotent).
    Otherwise returns a new Route instance with updated waypoint altitudes and distance mode.
    """
    if route.modo_altitude == DistanceMode.RELATIVE:
        return route

    new_waypoints: list[RouteWaypoint] = []
    for wp in route.waypoints:
        new_wp = RouteWaypoint(
            lat=wp.lat,
            lon=wp.lon,
            altura=wp.altura - takeoff_elevation,
            velocidade=wp.velocidade,
            heading=wp.heading,
            gimbal_pitch=wp.gimbal_pitch,
            acoes=[dict(a) for a in wp.acoes],
        )
        wait_time = getattr(wp, "wait_time", None)
        if wait_time is not None:
            new_wp.wait_time = wait_time
        new_waypoints.append(new_wp)

    min_alt = min((wp.altura for wp in new_waypoints), default=0.0)
    max_alt = max((wp.altura for wp in new_waypoints), default=0.0)

    warnings = list(route.avisos)
    warnings.append(
        tr(
            "Terrain mode converted to height relative to the takeoff point "
            "(elevation {takeoff_elevation:.1f} m); heights from {min_alt:.1f} m to {max_alt:.1f} m."
        ).format(takeoff_elevation=takeoff_elevation, min_alt=min_alt, max_alt=max_alt)
    )

    le_zero_count = sum(1 for wp in new_waypoints if wp.altura <= 0)
    if le_zero_count > 0:
        warnings.append(
            tr(
                "Relative height ≤ 0 in {le_zero_count} waypoint(s): the route goes below the takeoff point."
            ).format(le_zero_count=le_zero_count)
        )

    return Route(
        waypoints=new_waypoints,
        modo_disparo=route.modo_disparo,
        distancia_disparo=route.distancia_disparo,
        modo_altitude=DistanceMode.RELATIVE,
        avisos=warnings,
    )
