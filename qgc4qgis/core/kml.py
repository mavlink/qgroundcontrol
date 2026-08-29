"""Litchi KML exporter for QGC4QGIS route models.

Targeted for classic Litchi Mission Hub (flylitchi.com/hub) where the importer is togeojson.
"""

from pathlib import Path

from qgc4qgis.core import xmlwrite as ET
from qgc4qgis.core.i18n import tr
from qgc4qgis.core.route import Route

# Altitude bounds for Litchi KML [-200..500]
MIN_KML_ALTITUDE = -200.0
MAX_KML_ALTITUDE = 500.0

# Waypoint count ceiling for Mission Hub
MAX_HUB_WAYPOINTS = 10000

KML_NS = "http://www.opengis.net/kml/2.2"
ET.register_namespace("", KML_NS)


def validate_litchi_kml_route(route: Route) -> list[str]:
    """Validate a Route model for Litchi KML export and return warning messages.

    :param route: Route dataclass instance to validate.
    :return: List of warning strings.
    """
    warnings: list[str] = list(route.avisos) if route.avisos else []
    warnings.append(
        tr("KML does not carry heading, gimbal, speed, curve, or POI: ")
        + tr(
            "Litchi applies its own defaults. Adjust in Mission Hub after importing, "
            "or use the .csv, which carries everything."
        )
    )

    waypoints = route.waypoints if route.waypoints else []
    n = len(waypoints)

    if n > MAX_HUB_WAYPOINTS:
        warnings.append(
            tr(
                "Number of waypoints ({n}) exceeds the Mission Hub ceiling ({MAX_HUB_WAYPOINTS}); "
            ).format(n=n, MAX_HUB_WAYPOINTS=MAX_HUB_WAYPOINTS)
            + tr("the excess is silently discarded on import.")
        )

    for i, wp in enumerate(waypoints, start=1):
        a = float(wp.altura)
        if a < MIN_KML_ALTITUDE or a > MAX_KML_ALTITUDE:
            warnings.append(
                tr(
                    "Height of {a:.2f} m at waypoint {i} outside the range "
                    "[{MIN_KML_ALTITUDE}, {MAX_KML_ALTITUDE}]: "
                ).format(
                    a=a, i=i, MIN_KML_ALTITUDE=MIN_KML_ALTITUDE, MAX_KML_ALTITUDE=MAX_KML_ALTITUDE
                )
                + tr("Mission Hub truncates it to the limit.")
            )
        if f"{a:.2f}" in ("0.00", "-0.00"):
            warnings.append(
                tr("Height of waypoint {i} is zero in the KML: ").format(i=i)
                + tr("Mission Hub replaces zero height with 30 m without warning.")
            )

    return warnings


def route_to_litchi_kml(route: Route, name: str = "QGC4QGIS Mission") -> str:
    """Serialize a Route model into a Litchi-compatible KML string.

    :param route: Route dataclass instance to serialize.
    :param name: Name of the Document and Placemark in KML (default "QGC4QGIS Mission").
    :return: Formatted KML XML string.
    :raises ValueError: If the route contains no waypoints.
    """
    waypoints = route.waypoints if route.waypoints else []
    if not waypoints:
        raise ValueError(tr("Route has no waypoints: nothing to export to KML."))

    kml = ET.Element(f"{{{KML_NS}}}kml")
    doc = ET.SubElement(kml, f"{{{KML_NS}}}Document")

    doc_name = ET.SubElement(doc, f"{{{KML_NS}}}name")
    doc_name.text = name

    placemark = ET.SubElement(doc, f"{{{KML_NS}}}Placemark")
    pm_name = ET.SubElement(placemark, f"{{{KML_NS}}}name")
    pm_name.text = name

    linestring = ET.SubElement(placemark, f"{{{KML_NS}}}LineString")
    coords_elem = ET.SubElement(linestring, f"{{{KML_NS}}}coordinates")

    coords_str = " ".join(
        f"{float(wp.lon):.8f},{float(wp.lat):.8f},{float(wp.altura):.2f}" for wp in waypoints
    )
    coords_elem.text = coords_str

    xml_bytes = ET.tostring(kml, encoding="utf-8", xml_declaration=True)
    return xml_bytes.decode("utf-8")


def save_litchi_kml(
    filepath: str | Path,
    route: Route,
    name: str = "QGC4QGIS Mission",
) -> list[str]:
    """Serialize a Route model to KML, write to file in UTF-8, and return warning messages.

    :param filepath: Output path for the KML file.
    :param route: Route dataclass instance to export.
    :param name: Name of the Document and Placemark in KML (default "QGC4QGIS Mission").
    :return: List of warning messages from validate_litchi_kml_route.
    """
    kml_str = route_to_litchi_kml(route, name=name)
    path = Path(filepath)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(kml_str, encoding="utf-8")

    return validate_litchi_kml_route(route)
