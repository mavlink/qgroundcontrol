"""Tests for mission item expansion utilities in qgc4qgis.core.missionitems."""

from qgc4qgis.core.missionitems import (
    DistanceMode,
    MavCmd,
    MavFrame,
    create_camera_trigger_distance_item,
    create_waypoint_item,
    expand_transects_to_mission_items,
    mav_frame_from_distance_mode,
)


def test_mav_frame_from_distance_mode():
    """Verify MAV_FRAME derivation from DistanceMode enum values."""
    assert mav_frame_from_distance_mode(DistanceMode.RELATIVE) == MavFrame.GLOBAL_RELATIVE_ALT
    assert mav_frame_from_distance_mode(DistanceMode.ABSOLUTE) == MavFrame.GLOBAL
    assert mav_frame_from_distance_mode(DistanceMode.CALC_ABOVE_TERRAIN) == MavFrame.GLOBAL
    assert mav_frame_from_distance_mode(DistanceMode.TERRAIN) == MavFrame.GLOBAL_TERRAIN_ALT
    # Unknown mode fallback
    assert mav_frame_from_distance_mode(99) == MavFrame.GLOBAL_RELATIVE_ALT


def test_create_waypoint_item():
    """Verify NAV_WAYPOINT SimpleItem dictionary structure."""
    item = create_waypoint_item(
        seq=1,
        lat=-23.55,
        lon=-46.63,
        alt=100.0,
        frame=MavFrame.GLOBAL_RELATIVE_ALT,
        hold_time=5.0,
    )
    assert item["type"] == "SimpleItem"
    assert item["command"] == MavCmd.NAV_WAYPOINT
    assert item["doJumpId"] == 1
    assert item["frame"] == MavFrame.GLOBAL_RELATIVE_ALT
    assert item["autoContinue"] is True
    assert item["params"] == [5.0, 0.0, 0.0, None, -23.55, -46.63, 100.0]


def test_create_camera_trigger_distance_item():
    """Verify DO_SET_CAM_TRIGG_DIST SimpleItem dictionary structure."""
    start_item = create_camera_trigger_distance_item(seq=2, trigger_distance=25.0)
    assert start_item["type"] == "SimpleItem"
    assert start_item["command"] == MavCmd.DO_SET_CAM_TRIGG_DIST
    assert start_item["doJumpId"] == 2
    assert start_item["frame"] == MavFrame.MISSION
    assert start_item["params"] == [25.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0]

    stop_item = create_camera_trigger_distance_item(seq=3, trigger_distance=0.0)
    assert stop_item["params"][0] == 0.0
    assert stop_item["params"][2] == 0.0


def test_expand_transects_standard_without_turnaround():
    """Verify expansion when camera_trigger_in_turnaround=False and has_turnaround=False."""
    transects = [
        [(-23.550, -46.630, 50.0), (-23.551, -46.631, 50.0)],
        [(-23.552, -46.632, 50.0), (-23.553, -46.633, 50.0)],
    ]
    items = expand_transects_to_mission_items(
        transects=transects,
        trigger_distance=20.0,
        camera_trigger_in_turnaround=False,
        has_turnaround=False,
        distance_mode=DistanceMode.RELATIVE,
    )
    commands = [item["command"] for item in items]
    expected = [
        MavCmd.NAV_WAYPOINT,
        MavCmd.DO_SET_CAM_TRIGG_DIST,  # Start transect 1
        MavCmd.NAV_WAYPOINT,
        MavCmd.DO_SET_CAM_TRIGG_DIST,  # Stop transect 1
        MavCmd.NAV_WAYPOINT,
        MavCmd.DO_SET_CAM_TRIGG_DIST,  # Start transect 2
        MavCmd.NAV_WAYPOINT,
        MavCmd.DO_SET_CAM_TRIGG_DIST,  # Stop transect 2
    ]
    assert commands == expected
    assert items[0]["frame"] == MavFrame.GLOBAL_RELATIVE_ALT
    assert items[1]["params"][0] == 20.0
    assert items[3]["params"][0] == 0.0


def test_expand_transects_standard_with_turnaround():
    """Verify expansion when camera_trigger_in_turnaround=False and has_turnaround=True."""
    transects = [
        [(-23.549, -46.629), (-23.550, -46.630), (-23.551, -46.631), (-23.552, -46.632)],
        [(-23.553, -46.633), (-23.554, -46.634), (-23.555, -46.635), (-23.556, -46.636)],
    ]
    items = expand_transects_to_mission_items(
        transects=transects,
        trigger_distance=15.0,
        camera_trigger_in_turnaround=False,
        has_turnaround=True,
        distance_mode=DistanceMode.ABSOLUTE,
        default_altitude=60.0,
    )
    commands = [item["command"] for item in items]
    expected = [
        MavCmd.NAV_WAYPOINT,  # t_entry 1
        MavCmd.NAV_WAYPOINT,  # s_entry 1
        MavCmd.DO_SET_CAM_TRIGG_DIST,  # start trigger (15m)
        MavCmd.NAV_WAYPOINT,  # s_exit 1
        MavCmd.DO_SET_CAM_TRIGG_DIST,  # stop trigger (0m)
        MavCmd.NAV_WAYPOINT,  # t_exit 1
        MavCmd.NAV_WAYPOINT,  # t_entry 2
        MavCmd.NAV_WAYPOINT,  # s_entry 2
        MavCmd.DO_SET_CAM_TRIGG_DIST,  # start trigger (15m)
        MavCmd.NAV_WAYPOINT,  # s_exit 2
        MavCmd.DO_SET_CAM_TRIGG_DIST,  # stop trigger (0m)
        MavCmd.NAV_WAYPOINT,  # t_exit 2
    ]
    assert commands == expected
    assert items[0]["frame"] == MavFrame.GLOBAL
    assert items[0]["params"][6] == 60.0  # default altitude applied


def test_expand_transects_images_in_turnaround_with_turnaround():
    """Verify expansion when camera_trigger_in_turnaround=True and has_turnaround=True."""
    transects = [
        [(-23.549, -46.629), (-23.550, -46.630), (-23.551, -46.631), (-23.552, -46.632)],
        [(-23.553, -46.633), (-23.554, -46.634), (-23.555, -46.635), (-23.556, -46.636)],
    ]
    items = expand_transects_to_mission_items(
        transects=transects,
        trigger_distance=25.0,
        camera_trigger_in_turnaround=True,
        has_turnaround=True,
        distance_mode=DistanceMode.TERRAIN,
    )
    commands = [item["command"] for item in items]
    expected = [
        MavCmd.NAV_WAYPOINT,  # t_entry 1
        MavCmd.DO_SET_CAM_TRIGG_DIST,  # trigger start at first turnaround
        MavCmd.NAV_WAYPOINT,  # s_entry 1
        MavCmd.DO_SET_CAM_TRIGG_DIST,  # trigger start at survey entry
        MavCmd.NAV_WAYPOINT,  # s_exit 1
        MavCmd.NAV_WAYPOINT,  # t_exit 1
        MavCmd.NAV_WAYPOINT,  # t_entry 2
        MavCmd.NAV_WAYPOINT,  # s_entry 2
        MavCmd.DO_SET_CAM_TRIGG_DIST,  # trigger start at survey entry
        MavCmd.NAV_WAYPOINT,  # s_exit 2
        MavCmd.NAV_WAYPOINT,  # t_exit 2
        MavCmd.DO_SET_CAM_TRIGG_DIST,  # trigger stop at final turnaround
    ]
    assert commands == expected
    assert items[0]["frame"] == MavFrame.GLOBAL_TERRAIN_ALT


def test_expand_transects_no_trigger_when_dist_zero():
    """Verify no DO_SET_CAM_TRIGG_DIST items are added when trigger_distance=0."""
    transects = [
        [(-23.550, -46.630, 50.0), (-23.551, -46.631, 50.0)],
    ]
    items = expand_transects_to_mission_items(
        transects=transects,
        trigger_distance=0.0,
    )
    assert len(items) == 2
    assert all(item["command"] == MavCmd.NAV_WAYPOINT for item in items)
