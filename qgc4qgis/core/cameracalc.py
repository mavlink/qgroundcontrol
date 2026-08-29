"""Camera geometry and footprint calculations ported from QGroundControl CameraCalc."""

from dataclasses import dataclass
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from qgc4qgis.core.cameras import CameraSpec


def is_valid_camera_spec(spec: "CameraSpec | None") -> bool:
    """Check if camera specification has valid positive dimensions."""
    if spec is None:
        return False
    return (
        spec.sensorWidth > 0
        and spec.sensorHeight > 0
        and spec.imageWidth > 0
        and spec.imageHeight > 0
        and spec.focalLength > 0
    )


def gsd_from_altitude(altitude: float, spec: "CameraSpec | None") -> float:
    """Calculate Ground Sample Distance (GSD in cm/px) from altitude (in meters).

    Formula: (altitude_m * 100.0 * sensorWidth_mm) / (imageWidth_px * focalLength_mm)
    Input guard: returns 0.0 if altitude <= 0 or spec is invalid.
    """
    if altitude <= 0 or not is_valid_camera_spec(spec):
        return 0.0
    assert spec is not None
    return (altitude * 100.0 * spec.sensorWidth) / (spec.imageWidth * spec.focalLength)


def altitude_from_gsd(gsd: float, spec: "CameraSpec | None") -> float:
    """Calculate altitude (in meters) from Ground Sample Distance (GSD in cm/px).

    Formula: (imageWidth_px * gsd_cm_px * focalLength_mm) / (sensorWidth_mm * 100.0)
    Input guard: returns 0.0 if gsd <= 0 or spec is invalid.
    """
    if gsd <= 0 or not is_valid_camera_spec(spec):
        return 0.0
    assert spec is not None
    return (spec.imageWidth * gsd * spec.focalLength) / (spec.sensorWidth * 100.0)


def image_footprints(spec: "CameraSpec | None", gsd: float) -> tuple[float, float]:
    """Calculate ground image footprint sizes (side, frontal) in meters.

    If spec.landscape is True:
        side = (imageWidth * gsd) / 100.0
        frontal = (imageHeight * gsd) / 100.0
    If spec.landscape is False (portrait):
        side = (imageHeight * gsd) / 100.0
        frontal = (imageWidth * gsd) / 100.0

    Input guard: returns (0.0, 0.0) if gsd <= 0 or spec is invalid.
    """
    if gsd <= 0 or not is_valid_camera_spec(spec):
        return (0.0, 0.0)
    assert spec is not None

    if spec.landscape:
        side = (spec.imageWidth * gsd) / 100.0
        frontal = (spec.imageHeight * gsd) / 100.0
    else:
        side = (spec.imageHeight * gsd) / 100.0
        frontal = (spec.imageWidth * gsd) / 100.0

    return (side, frontal)


def adjusted_footprints(
    footprint_side: float,
    footprint_frontal: float,
    side_overlap: float,
    frontal_overlap: float,
) -> tuple[float, float]:
    """Calculate footprints adjusted down for overlap (in meters).

    adjusted_side = footprint_side * (100.0 - side_overlap) / 100.0
    adjusted_frontal = footprint_frontal * (100.0 - frontal_overlap) / 100.0

    Input guard: returns (0.0, 0.0) if any footprint is <= 0.
    """
    if footprint_side <= 0 or footprint_frontal <= 0:
        return (0.0, 0.0)

    adjusted_side = footprint_side * ((100.0 - side_overlap) / 100.0)
    adjusted_frontal = footprint_frontal * ((100.0 - frontal_overlap) / 100.0)
    return (adjusted_side, adjusted_frontal)


def trigger_distance(adjusted_footprint_frontal: float) -> float:
    """Return trigger distance (in meters) equal to adjusted frontal footprint."""
    if adjusted_footprint_frontal <= 0:
        return 0.0
    return adjusted_footprint_frontal


def time_between_shots(trigger_dist: float, flight_speed: float) -> float:
    """Calculate time between camera triggers (in seconds).

    Formula: trigger_distance / flight_speed
    Input guard: returns 0.0 if trigger_dist <= 0 or flight_speed <= 0.
    """
    if trigger_dist <= 0 or flight_speed <= 0:
        return 0.0
    return trigger_dist / flight_speed


@dataclass
class CameraCalc:
    """Camera calculation state manager matching QGC CameraCalc logic."""

    spec: "CameraSpec | None" = None
    value_set_is_distance: bool = True
    distance_to_surface: float = 0.0  # altitude in meters
    image_density: float = 0.0  # GSD in cm/px
    frontal_overlap: float = 70.0  # %
    side_overlap: float = 70.0  # %

    # Computed state
    image_footprint_side: float = 0.0
    image_footprint_frontal: float = 0.0
    adjusted_footprint_side: float = 0.0
    adjusted_footprint_frontal: float = 0.0

    def recalculate(self) -> bool:
        """Recalculate camera metrics based on current state.

        Input guard: if spec is invalid or input distance/GSD <= 0, abort recalculation.
        Returns True if calculation succeeded, False if input guard aborted.
        """
        if not is_valid_camera_spec(self.spec):
            return False

        if self.value_set_is_distance:
            if self.distance_to_surface <= 0:
                return False
            self.image_density = gsd_from_altitude(self.distance_to_surface, self.spec)
        else:
            if self.image_density <= 0:
                return False
            self.distance_to_surface = altitude_from_gsd(self.image_density, self.spec)

        if self.image_density <= 0 or self.distance_to_surface <= 0:
            return False

        self.image_footprint_side, self.image_footprint_frontal = image_footprints(
            self.spec, self.image_density
        )
        self.adjusted_footprint_side, self.adjusted_footprint_frontal = adjusted_footprints(
            self.image_footprint_side,
            self.image_footprint_frontal,
            self.side_overlap,
            self.frontal_overlap,
        )
        return True

    @property
    def trigger_distance(self) -> float:
        """Return distance between image captures (meters)."""
        return trigger_distance(self.adjusted_footprint_frontal)

    def time_between_shots(self, flight_speed: float) -> float:
        """Return time interval between image captures (seconds) at given flight speed."""
        return time_between_shots(self.trigger_distance, flight_speed)
