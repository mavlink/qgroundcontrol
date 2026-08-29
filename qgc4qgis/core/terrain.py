"""Terrain elevation sampling from raster layers for QGC4QGIS.

Provides terrain sampling for flight plan points using QGIS elevation rasters,
including CRS reprojection, out-of-bounds handling, and NoData filtering.
"""

import math
from collections.abc import Sequence
from dataclasses import dataclass
from typing import Any

from qgis.core import (
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsPointXY,
    QgsProject,
    QgsRasterLayer,
)

from qgc4qgis.core.geo import (
    geodesic_azimuth,
    geodesic_destination,
    geodesic_distance,
)

DEFAULT_TERRAIN_ADJUST_TOLERANCE: float = 10.0


@dataclass
class TerrainPoint:
    """Representation of a flight path point with terrain elevation info.

    :param lat: Latitude in degrees.
    :param lon: Longitude in degrees.
    :param altitude: Altitude in meters.
    :param is_interstitial: True if point was generated between segment endpoints.
    """

    lat: float
    lon: float
    altitude: float
    is_interstitial: bool = False


def sample_terrain(
    raster_layer: QgsRasterLayer | None,
    points: Sequence[tuple[float, float] | tuple[float, float, float] | QgsPointXY],
    points_crs: QgsCoordinateReferenceSystem | str = "EPSG:4326",
    band: int = 1,
    nodata_value: float | None = None,
    transform_context: Any | None = None,
) -> list[float | None]:
    """Sample elevation values from a raster layer for a list of flight points.

    Points are automatically reprojected from `points_crs` to the CRS of `raster_layer`.
    Out-of-bounds points and points falling on NoData pixels return `nodata_value`.

    :param raster_layer: QgsRasterLayer representing elevation/DEM.
    :param points: Sequence of points as (lat, lon), (lat, lon, alt) or QgsPointXY.
    :param points_crs: CRS of input points (default: WGS84 EPSG:4326).
    :param band: Raster band number to sample (1-indexed, default: 1).
    :param nodata_value: Value returned when sampling fails or hits NoData (default: None).
    :param transform_context: Optional QgsCoordinateTransformContext.
    :return: List of sampled elevation values (floats or nodata_value).
    """
    if raster_layer is None or not raster_layer.isValid():
        return [nodata_value] * len(points)

    if not points:
        return []

    if isinstance(points_crs, str):
        src_crs = QgsCoordinateReferenceSystem(points_crs)
    else:
        src_crs = points_crs

    raster_crs = raster_layer.crs()

    if transform_context is None:
        transform_context = QgsProject.instance().transformContext()

    need_transform = src_crs != raster_crs
    if need_transform:
        transform = QgsCoordinateTransform(src_crs, raster_crs, transform_context)
    else:
        transform = None

    provider = raster_layer.dataProvider()
    extent = raster_layer.extent()

    if provider is None:
        return [nodata_value] * len(points)

    source_nodata = provider.sourceNoDataValue(band)
    has_source_nodata = source_nodata is not None and not math.isnan(source_nodata)

    results: list[float | None] = []

    for pt in points:
        if isinstance(pt, QgsPointXY):
            src_pt = pt
        elif isinstance(pt, (tuple, list)):
            if src_crs.isGeographic():
                src_pt = QgsPointXY(float(pt[1]), float(pt[0]))
            else:
                src_pt = QgsPointXY(float(pt[0]), float(pt[1]))
        else:
            src_pt = QgsPointXY(float(pt.x()), float(pt.y()))

        if transform is not None:
            try:
                raster_pt = transform.transform(src_pt)
            except Exception:
                results.append(nodata_value)
                continue
        else:
            raster_pt = src_pt

        if not extent.contains(raster_pt):
            results.append(nodata_value)
            continue

        val, ok = provider.sample(raster_pt, band)

        if not ok or math.isnan(val):
            results.append(nodata_value)
            continue

        if has_source_nodata and math.isclose(val, source_nodata, rel_tol=1e-5, abs_tol=1e-5):
            results.append(nodata_value)
            continue

        results.append(float(val))

    return results


def sample_terrain_point(
    raster_layer: QgsRasterLayer | None,
    point: tuple[float, float] | tuple[float, float, float] | QgsPointXY,
    points_crs: QgsCoordinateReferenceSystem | str = "EPSG:4326",
    band: int = 1,
    nodata_value: float | None = None,
    transform_context: Any | None = None,
) -> float | None:
    """Sample elevation value for a single flight point."""
    res = sample_terrain(
        raster_layer=raster_layer,
        points=[point],
        points_crs=points_crs,
        band=band,
        nodata_value=nodata_value,
        transform_context=transform_context,
    )
    return res[0] if res else nodata_value


def _extract_lat_lon_alt(
    pt: tuple[float, float] | tuple[float, float, float] | TerrainPoint,
    default_alt: float = 0.0,
) -> tuple[float, float, float, bool]:
    """Helper to extract (lat, lon, alt, is_interstitial) from flexible point input."""
    if isinstance(pt, TerrainPoint):
        return (pt.lat, pt.lon, pt.altitude, pt.is_interstitial)
    if len(pt) >= 3:
        return (float(pt[0]), float(pt[1]), float(pt[2]), False)
    return (float(pt[0]), float(pt[1]), default_alt, False)


def generate_interstitial_points(
    points: Sequence[tuple[float, float] | tuple[float, float, float] | TerrainPoint],
    raster_layer: QgsRasterLayer | None = None,
    target_altitude: float = 50.0,
    step_distance: float = 10.0,
    points_crs: QgsCoordinateReferenceSystem | str = "EPSG:4326",
    nodata_value: float | None = None,
    transform_context: Any | None = None,
) -> list[TerrainPoint]:
    """Generate interstitial terrain points between segment endpoints.

    Samples elevation along each segment at steps of `step_distance` meters.
    If `raster_layer` is provided and valid, elevation is sampled and added to `target_altitude`.
    Otherwise, linear altitude interpolation is used between segment endpoints.

    :param points: Sequence of segment endpoints as (lat, lon), (lat, lon, alt) or TerrainPoint.
    :param raster_layer: Optional QgsRasterLayer representing elevation/DEM.
    :param target_altitude: Altitude offset above terrain in meters (default: 50.0).
    :param step_distance: Spacing between interstitial points in meters (default: 10.0).
    :param points_crs: CRS of input points (default: EPSG:4326).
    :param nodata_value: Fallback value for NoData/OOB (default: None).
    :param transform_context: Optional QgsCoordinateTransformContext.
    :return: List of TerrainPoint objects with interstitial points inserted.
    """
    if not points:
        return []

    parsed_points = [_extract_lat_lon_alt(pt, target_altitude) for pt in points]
    if len(parsed_points) == 1:
        lat, lon, alt, is_inter = parsed_points[0]
        if raster_layer is not None and raster_layer.isValid():
            elev = sample_terrain_point(
                raster_layer,
                (lat, lon),
                points_crs=points_crs,
                nodata_value=nodata_value,
                transform_context=transform_context,
            )
            if elev is not None:
                alt = float(elev) + target_altitude
        return [TerrainPoint(lat, lon, alt, is_inter)]

    result: list[TerrainPoint] = []

    for i in range(len(parsed_points) - 1):
        start_lat, start_lon, start_alt, _ = parsed_points[i]
        end_lat, end_lon, end_alt, _ = parsed_points[i + 1]

        # Sample terrain elevation at segment start if raster is provided
        if raster_layer is not None and raster_layer.isValid():
            elev_start = sample_terrain_point(
                raster_layer,
                (start_lat, start_lon),
                points_crs=points_crs,
                nodata_value=nodata_value,
                transform_context=transform_context,
            )
            if elev_start is not None:
                start_alt = float(elev_start) + target_altitude

        if i == 0:
            result.append(TerrainPoint(start_lat, start_lon, start_alt, is_interstitial=False))

        dist = geodesic_distance(start_lat, start_lon, end_lat, end_lon)
        az = geodesic_azimuth(start_lat, start_lon, end_lat, end_lon)

        if dist > step_distance and step_distance > 0.0:
            n_steps = math.ceil(dist / step_distance)
            for k in range(1, n_steps):
                frac = k / n_steps
                sample_dist = dist * frac
                ilat, ilon = geodesic_destination(start_lat, start_lon, az, sample_dist)

                ialt: float | None = None
                if raster_layer is not None and raster_layer.isValid():
                    ielev = sample_terrain_point(
                        raster_layer,
                        (ilat, ilon),
                        points_crs=points_crs,
                        nodata_value=nodata_value,
                        transform_context=transform_context,
                    )
                    if ielev is not None:
                        ialt = float(ielev) + target_altitude

                if ialt is None:
                    ialt = start_alt + (end_alt - start_alt) * frac

                result.append(TerrainPoint(ilat, ilon, ialt, is_interstitial=True))

        # Sample terrain elevation at segment end if raster is provided
        if raster_layer is not None and raster_layer.isValid():
            elev_end = sample_terrain_point(
                raster_layer,
                (end_lat, end_lon),
                points_crs=points_crs,
                nodata_value=nodata_value,
                transform_context=transform_context,
            )
            if elev_end is not None:
                end_alt = float(elev_end) + target_altitude

        result.append(TerrainPoint(end_lat, end_lon, end_alt, is_interstitial=False))

    return result


def adjust_for_tolerance(
    points: Sequence[TerrainPoint],
    tolerance: float = DEFAULT_TERRAIN_ADJUST_TOLERANCE,
) -> list[TerrainPoint]:
    """Decimate interstitial terrain points based on altitude tolerance.

    Ported from QGC TransectStyleComplexItem::_adjustForTolerance.

    Always retains non-interstitial points (segment endpoints).
    An interstitial point is retained only if the absolute altitude difference from
    the last retained point exceeds `tolerance` (default: 10.0 m).

    :param points: Sequence of TerrainPoint objects.
    :param tolerance: Altitude difference threshold in meters (default: 10.0).
    :return: Decimated list of TerrainPoint objects.
    """
    if not points:
        return []

    adjusted: list[TerrainPoint] = [points[0]]
    last_point = points[0]

    for pt in points[1:]:
        if not pt.is_interstitial or abs(last_point.altitude - pt.altitude) > tolerance:
            adjusted.append(pt)
            last_point = pt

    return adjusted


def adjust_for_max_rates(
    points: Sequence[TerrainPoint],
    max_climb_rate: float = 0.0,
    max_descent_rate: float = 0.0,
    flight_speed: float = 0.0,
) -> list[TerrainPoint]:
    """Adjust point altitudes to enforce max climb and descent rate limits with 0.1 m/s slack.

    Ported from QGC TransectStyleComplexItem::_adjustForMaxRates.

    :param points: Sequence of TerrainPoint objects.
    :param max_climb_rate: Maximum allowed climb rate in m/s (0.0 or negative disables).
    :param max_descent_rate: Maximum allowed descent rate in m/s (0.0 or negative disables, positive value).
    :param flight_speed: Vehicle horizontal flight speed in m/s.
    :return: Adjusted list of TerrainPoint objects.
    """
    if not points:
        return []

    if math.isnan(flight_speed) or flight_speed <= 0.0:
        return [TerrainPoint(pt.lat, pt.lon, pt.altitude, pt.is_interstitial) for pt in points]

    if max_climb_rate <= 0.0 and max_descent_rate <= 0.0:
        return [TerrainPoint(pt.lat, pt.lon, pt.altitude, pt.is_interstitial) for pt in points]

    res_points = [
        TerrainPoint(pt.lat, pt.lon, float(pt.altitude), pt.is_interstitial) for pt in points
    ]
    n = len(res_points)
    if n < 2:
        return res_points

    # Climb rate pass
    if max_climb_rate > 0.0:
        while True:
            climb_adjusted = False
            for i in range(n - 1):
                from_pt = res_points[i]
                to_pt = res_points[i + 1]

                alt_diff = to_pt.altitude - from_pt.altitude
                dist = geodesic_distance(from_pt.lat, from_pt.lon, to_pt.lat, to_pt.lon)
                if dist <= 0.0:
                    continue

                seconds = dist / flight_speed
                climb_rate = alt_diff / seconds

                if climb_rate > 0.0 and (climb_rate - max_climb_rate) > 0.1:
                    max_alt_delta = max_climb_rate * seconds
                    from_pt.altitude = to_pt.altitude - max_alt_delta
                    climb_adjusted = True

            if not climb_adjusted:
                break

    # Descent rate pass
    if max_descent_rate > 0.0:
        max_descent_neg = -max_descent_rate
        while True:
            descent_adjusted = False
            for i in range(n - 1):
                from_pt = res_points[i]
                to_pt = res_points[i + 1]

                alt_diff = to_pt.altitude - from_pt.altitude
                dist = geodesic_distance(from_pt.lat, from_pt.lon, to_pt.lat, to_pt.lon)
                if dist <= 0.0:
                    continue

                seconds = dist / flight_speed
                descent_rate = alt_diff / seconds

                if descent_rate < 0.0 and (descent_rate - max_descent_neg) < -0.1:
                    max_alt_delta = max_descent_neg * seconds
                    to_pt.altitude = from_pt.altitude + max_alt_delta
                    descent_adjusted = True

            if not descent_adjusted:
                break

    return res_points


def adjust_terrain_flight_path(
    points: Sequence[tuple[float, float] | tuple[float, float, float] | TerrainPoint],
    raster_layer: QgsRasterLayer | None = None,
    target_altitude: float = 50.0,
    step_distance: float = 10.0,
    tolerance: float = DEFAULT_TERRAIN_ADJUST_TOLERANCE,
    max_climb_rate: float = 0.0,
    max_descent_rate: float = 0.0,
    flight_speed: float = 0.0,
    points_crs: QgsCoordinateReferenceSystem | str = "EPSG:4326",
    nodata_value: float | None = None,
    transform_context: Any | None = None,
) -> list[TerrainPoint]:
    """Perform full terrain flight path adjustment.

    1. Generates interstitial points between segment endpoints (sampling elevation).
    2. Decimates interstitial points within `tolerance` altitude change.
    3. Adjusts altitudes for climb and descent rate limits with 0.1 m/s slack.

    :return: Fully adjusted list of TerrainPoint objects.
    """
    interstitial_points = generate_interstitial_points(
        points=points,
        raster_layer=raster_layer,
        target_altitude=target_altitude,
        step_distance=step_distance,
        points_crs=points_crs,
        nodata_value=nodata_value,
        transform_context=transform_context,
    )

    decimated = adjust_for_tolerance(interstitial_points, tolerance=tolerance)

    adjusted = adjust_for_max_rates(
        points=decimated,
        max_climb_rate=max_climb_rate,
        max_descent_rate=max_descent_rate,
        flight_speed=flight_speed,
    )

    return adjusted
