# GPS / RTK implementation TODO

Recover useful work from `feature/gps-updates`, `feature/gps-driver`, and
`feature/gpsrtk-improvements` as focused changes against current master.
Do not merge the stale branches wholesale.

## Implementation order

- [x] **1. Remove the GPS/RTK core dependency on serial support.** Build the manager,
  receiver driver, provider, Facts, resources, and QML exposure with
  `QGC_NO_SERIAL_LINK=ON`. Create transports on the receiver worker through a
  transport factory. Keep the serial adapter optional. Verify serial-enabled and
  serial-disabled builds and the core tests. TCP implementation is item 4.
- [ ] **2. Move GPS connection management out of Comms.** Move RTK discovery,
  auto-connect, unplug handling, serial NMEA ownership, UDP NMEA ownership, and
  source switching from `LinkManager` into GPS-owned components. Keep fix health
  in `PositionManager`; share serial discovery and port reservations with MAVLink
  links, including Android constraints. Keep generic networking utilities shared
  and preserve saved settings keys.
- [ ] **3. Receiver recovery.** Report actual connection state, retry failed
  connections with bounded backoff, and clear stale receiver state. Ensure retiring
  workers cannot update a newer session or outlive their cancellation state.
- [ ] **4. TCP receiver support.** Add cancellable TCP transport for network receivers
  and serial bridges, host/port settings, and connect/disconnect controls.
- [ ] **5. Correction diagnostics.** Show correction freshness, rates, bounded events,
  and a warning when corrections arrive without a reported vehicle RTK fix.
- [ ] **6. Richer vehicle telemetry.** Expose GPS1/GPS2 accuracy, altitude, speed,
  heading, and GPS_RTK/GPS2_RTK health and baseline information. Clear unavailable
  fields when message sources change.
- [ ] **7. Satellite diagnostics.** Show constellation, signal strength, and
  satellites used by local receivers and GCS positioning sources. Respect the
  exact driver's ID mappings, units, and unavailable data.
- [ ] **8. GCS position health.** Expire stale fixes, expose source and permission
  status, and allow the local receiver to supply GCS position. Preserve source
  ownership and invalidate old updates on switches.
- [ ] **9. Receiver configuration.** Expose supported output modes, u-blox operating
  and dynamic modes, constellations, and heading offsets. Preserve defaults,
  convert units at the driver boundary, and apply changes on reconnect.
- [ ] **10. Base map overlay.** Show the base position, accuracy circle, and line to
  the active vehicle in both map engines. Hide stale or disconnected base data.

## Optional later work

- [ ] Multiple local receivers.
- [ ] Correction injection into a local receiver.

## Constraints

- Preserve existing RTCM fragmentation, per-link deduplication, and the shared
  sequence-ID domain across correction sources.
- Keep the existing pinned GPS driver revision unless an independently reviewed
  feature requires an update.
- Build and test evidence is separate from physical receiver and flight validation.
- Leave implementation changes uncommitted until explicitly requested.

## Item 1 validation

Implemented; validation recorded below.

- Debug build succeeds with serial enabled and with `QGC_NO_SERIAL_LINK=ON`.
- The serial-disabled build also disables Qt SerialPort package discovery and has
  no Qt SerialPort link dependency.
- GPS driver, provider, RTK resource/QML exposure, JSON resource audit, and GPS popup
  tests pass in both builds. The serial transport test passes in the normal build.
- Full repository lint remains blocked by existing violations. Scoped static
  analysis and formatting checks for changed C++ regions pass.
- Full Unit suites: 263/264 pass with serial enabled; 260/261 pass with serial
  disabled. Both retain the existing BluetoothWorkerTest failure on host adapter
  warnings. An initial cache-worker timeout passed on the final rerun.
- Physical receiver validation remains outstanding.
