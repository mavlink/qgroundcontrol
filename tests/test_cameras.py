"""Tests for camera catalog loader."""

from qgc4qgis.core.cameras import CUSTOM_CAMERA_NAME, CameraSpec, load_cameras


def test_load_cameras():
    cameras = load_cameras()
    assert len(cameras) > 1
    assert cameras[0].canonicalName == CUSTOM_CAMERA_NAME
    assert isinstance(cameras[0], CameraSpec)

    catalog = cameras[1:]
    assert len(catalog) == 35  # Based on 35 cameras in cameras.json

    # Check sorting by brand/model
    for i in range(len(catalog) - 1):
        prev_key = (catalog[i].brand.lower(), catalog[i].model.lower())
        next_key = (catalog[i + 1].brand.lower(), catalog[i + 1].model.lower())
        assert prev_key <= next_key
