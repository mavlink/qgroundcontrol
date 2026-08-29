"""Tests for CameraCalc calculations, catalog verification, and input guards."""

import pytest

from qgc4qgis.core.cameracalc import (
    CameraCalc,
    adjusted_footprints,
    altitude_from_gsd,
    gsd_from_altitude,
    image_footprints,
    time_between_shots,
    trigger_distance,
)
from qgc4qgis.core.cameras import CameraSpec, load_cameras


@pytest.fixture
def sample_camera() -> CameraSpec:
    """Fixture providing a standard test camera spec (landscape orientation)."""
    return CameraSpec(
        canonicalName="Test Camera",
        brand="TestBrand",
        model="TestModel",
        sensorWidth=13.2,
        sensorHeight=8.8,
        imageWidth=5472,
        imageHeight=3648,
        focalLength=8.8,
        landscape=True,
    )


def test_catalog_camera_hand_calculated_values():
    """Verify calculations against hand-checked values for catalog camera Sony a6000 16mm.

    Sony a6000 16mm spec:
        sensorWidth = 23.5 mm, sensorHeight = 15.6 mm
        imageWidth = 6000 px, imageHeight = 4000 px
        focalLength = 16.0 mm, landscape = True

    Hand calculations at altitude = 120.0 m, 70% overlap (side and frontal):
        GSD = (120 * 100 * 23.5) / (6000 * 16) = 282000 / 96000 = 2.9375 cm/px
        footprint_side = (6000 * 2.9375) / 100 = 176.25 m
        footprint_frontal = (4000 * 2.9375) / 100 = 117.5 m
        adjusted_footprint_side = 176.25 * (1 - 0.70) = 52.875 m
        adjusted_footprint_frontal = 117.5 * (1 - 0.70) = 35.25 m
        trigger_distance = 35.25 m
        time_between_shots (at 10 m/s) = 35.25 / 10.0 = 3.525 s
    """
    cameras = load_cameras()
    sony_a6000 = next((c for c in cameras if c.canonicalName == "Sony a6000 16mm"), None)
    assert sony_a6000 is not None

    calc = CameraCalc(
        spec=sony_a6000,
        value_set_is_distance=True,
        distance_to_surface=120.0,
        side_overlap=70.0,
        frontal_overlap=70.0,
    )

    success = calc.recalculate()
    assert success is True

    assert pytest.approx(calc.image_density, abs=1e-6) == 2.9375
    assert pytest.approx(calc.image_footprint_side, abs=1e-6) == 176.25
    assert pytest.approx(calc.image_footprint_frontal, abs=1e-6) == 117.5
    assert pytest.approx(calc.adjusted_footprint_side, abs=1e-6) == 52.875
    assert pytest.approx(calc.adjusted_footprint_frontal, abs=1e-6) == 35.25
    assert pytest.approx(calc.trigger_distance, abs=1e-6) == 35.25
    assert pytest.approx(calc.time_between_shots(10.0), abs=1e-6) == 3.525


def test_roundtrip_mode_a_and_mode_b(sample_camera: CameraSpec):
    """Test roundtrip conversion: Mode A (distance -> GSD) <-> Mode B (GSD -> distance)."""
    calc = CameraCalc(
        spec=sample_camera,
        value_set_is_distance=True,
        distance_to_surface=100.0,
        frontal_overlap=70.0,
        side_overlap=70.0,
    )

    # Step 1: Mode A (distance -> GSD)
    assert calc.recalculate() is True
    initial_gsd = calc.image_density
    assert pytest.approx(initial_gsd, rel=1e-4) == 2.741227

    # Step 2: Switch to Mode B (GSD -> distance)
    calc.value_set_is_distance = False
    calc.image_density = initial_gsd
    assert calc.recalculate() is True
    assert pytest.approx(calc.distance_to_surface, rel=1e-4) == 100.0

    # Step 3: Switch back to Mode A
    calc.value_set_is_distance = True
    calc.distance_to_surface = 100.0
    assert calc.recalculate() is True
    assert pytest.approx(calc.image_density, rel=1e-4) == initial_gsd


def test_landscape_orientation_effect(sample_camera: CameraSpec):
    """Test the effect of landscape (True) vs portrait (False) on footprint orientations."""
    gsd = 2.741227  # cm/px (~100m alt)

    # Landscape mode (True): side = imageWidth * GSD, frontal = imageHeight * GSD
    sample_camera.landscape = True
    side_land, frontal_land = image_footprints(sample_camera, gsd)
    assert pytest.approx(side_land, rel=1e-3) == 150.0
    assert pytest.approx(frontal_land, rel=1e-3) == 100.0

    calc_land = CameraCalc(
        spec=sample_camera, value_set_is_distance=True, distance_to_surface=100.0
    )
    assert calc_land.recalculate() is True
    assert pytest.approx(calc_land.image_footprint_side, rel=1e-3) == 150.0
    assert pytest.approx(calc_land.image_footprint_frontal, rel=1e-3) == 100.0

    # Portrait mode (False): side = imageHeight * GSD, frontal = imageWidth * GSD
    sample_camera.landscape = False
    side_port, frontal_port = image_footprints(sample_camera, gsd)
    assert pytest.approx(side_port, rel=1e-3) == 100.0
    assert pytest.approx(frontal_port, rel=1e-3) == 150.0

    calc_port = CameraCalc(
        spec=sample_camera, value_set_is_distance=True, distance_to_surface=100.0
    )
    assert calc_port.recalculate() is True
    assert pytest.approx(calc_port.image_footprint_side, rel=1e-3) == 100.0
    assert pytest.approx(calc_port.image_footprint_frontal, rel=1e-3) == 150.0


def test_overlap_zero_and_eighty_five_percent(sample_camera: CameraSpec):
    """Test adjusted footprint and trigger distance calculations for 0% and 85% overlap."""
    calc = CameraCalc(
        spec=sample_camera,
        value_set_is_distance=True,
        distance_to_surface=100.0,
    )

    # 0% Overlap
    calc.side_overlap = 0.0
    calc.frontal_overlap = 0.0
    assert calc.recalculate() is True
    assert pytest.approx(calc.adjusted_footprint_side, rel=1e-3) == calc.image_footprint_side
    assert pytest.approx(calc.adjusted_footprint_frontal, rel=1e-3) == calc.image_footprint_frontal
    assert pytest.approx(calc.trigger_distance, rel=1e-3) == calc.image_footprint_frontal

    adj_s0, adj_f0 = adjusted_footprints(150.0, 100.0, 0.0, 0.0)
    assert pytest.approx(adj_s0) == 150.0
    assert pytest.approx(adj_f0) == 100.0

    # 85% Overlap
    calc.side_overlap = 85.0
    calc.frontal_overlap = 85.0
    assert calc.recalculate() is True
    expected_adj_side_85 = calc.image_footprint_side * (1.0 - 0.85)
    expected_adj_frontal_85 = calc.image_footprint_frontal * (1.0 - 0.85)
    assert pytest.approx(calc.adjusted_footprint_side, rel=1e-3) == expected_adj_side_85
    assert pytest.approx(calc.adjusted_footprint_frontal, rel=1e-3) == expected_adj_frontal_85
    assert pytest.approx(calc.trigger_distance, rel=1e-3) == expected_adj_frontal_85

    adj_s85, adj_f85 = adjusted_footprints(150.0, 100.0, 85.0, 85.0)
    assert pytest.approx(adj_s85) == 150.0 * 0.15
    assert pytest.approx(adj_f85) == 100.0 * 0.15


def test_gsd_and_altitude_conversion(sample_camera: CameraSpec):
    altitude = 100.0  # meters
    gsd = gsd_from_altitude(altitude, sample_camera)
    # Expected GSD = (100 * 100 * 13.2) / (5472 * 8.8) = 132000 / 48153.6 = 2.741227...
    assert pytest.approx(gsd, rel=1e-4) == 2.741227

    recalculated_alt = altitude_from_gsd(gsd, sample_camera)
    assert pytest.approx(recalculated_alt, rel=1e-4) == altitude


def test_invalid_inputs_and_guards(sample_camera: CameraSpec):
    """Test input validation and guards for invalid camera specs, non-positive distances, GSD, speed, and overlaps."""
    # Altitude <= 0
    assert gsd_from_altitude(0, sample_camera) == 0.0
    assert gsd_from_altitude(-10, sample_camera) == 0.0

    # GSD <= 0
    assert altitude_from_gsd(0, sample_camera) == 0.0
    assert altitude_from_gsd(-2.5, sample_camera) == 0.0

    # Invalid camera specs
    invalid_cam_zero = CameraSpec(canonicalName="InvalidZero", sensorWidth=0, focalLength=8.8)
    invalid_cam_neg = CameraSpec(canonicalName="InvalidNeg", sensorWidth=-5, focalLength=8.8)
    for inv_cam in (invalid_cam_zero, invalid_cam_neg):
        assert gsd_from_altitude(100, inv_cam) == 0.0
        assert altitude_from_gsd(2.5, inv_cam) == 0.0
        assert image_footprints(inv_cam, 2.5) == (0.0, 0.0)

    # None spec
    assert gsd_from_altitude(100, None) == 0.0
    assert altitude_from_gsd(2.5, None) == 0.0
    assert image_footprints(None, 2.5) == (0.0, 0.0)

    # Footprints and trigger distance guards
    assert adjusted_footprints(0, 100, 70, 70) == (0.0, 0.0)
    assert adjusted_footprints(150, -10, 70, 70) == (0.0, 0.0)
    assert trigger_distance(0) == 0.0
    assert trigger_distance(-5) == 0.0

    # Time between shots guards
    assert time_between_shots(20.0, 0) == 0.0
    assert time_between_shots(20.0, -5.0) == 0.0
    assert time_between_shots(0, 10.0) == 0.0
    assert time_between_shots(-10.0, 10.0) == 0.0

    # CameraCalc recalculate aborts on invalid state
    calc = CameraCalc(spec=sample_camera, value_set_is_distance=True, distance_to_surface=-10.0)
    assert calc.recalculate() is False

    calc.distance_to_surface = 100.0
    calc.value_set_is_distance = False
    calc.image_density = -5.0
    assert calc.recalculate() is False

    calc.spec = None
    assert calc.recalculate() is False
