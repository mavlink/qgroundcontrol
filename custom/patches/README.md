# Custom Patches for QGroundControl v5.0.8

These patches are applied automatically by `setup_and_build_qgc.sh` before building.
They are applied via `git apply` inside the `qgroundcontrol/` directory and are
idempotent — they will be skipped if already applied.

---

## gps-drivers-pin.patch

**Problem:**
QGroundControl v5.0.8 has a bug in `src/GPS/CMakeLists.txt` where the PX4-GPSDrivers
dependency is pinned to `GIT_TAG main` instead of a specific commit hash. This is a
floating reference — when v5.0.8 was released, `main` pointed to a compatible version,
but `main` has since moved forward to a newer version with a breaking API change:

The `GPSDriverUBX` constructor now requires a 6th `Settings` argument, but
`GPSProvider.cc` in v5.0.8 only passes 5 arguments, causing a compile error:

```
error: no matching function for call to
'GPSDriverUBX::GPSDriverUBX(GPSHelper::Interface, int (*)(GPSCallbackType, void*, int, void*),
GPSProvider*, sensor_gps_s*, satellite_info_s*)'
```

**Fix:**
Pin `GIT_TAG` to the specific commit hash that `main` pointed to at the time v5.0.8
was built, so the build uses the same GPS driver version the QGC developers tested against.

**Commit pinned:** `0b9695881bd1e8f830ab4538ab3acc0050019eba`

This hash was identified by looking at the PX4-GPSDrivers commit history on GitHub
filtered to the v5.0.8 release date (October 8, 2025):

https://github.com/PX4/PX4-GPSDrivers/commits/main?until=2025-10-08

The top commit on that page is the last commit that was on `main` at the time v5.0.8
was released, making it the version QGC v5.0.8 was built and tested against.

**Upstream issue:** This is a known bug in the v5.0.8 release. It has not been patched
upstream as of the time of writing. If a future QGC version pins this dependency
properly, this patch can be removed.
