---
applyTo: "src/**/*.h"
description: "When performing a code review on C++ headers, enforce the MAVLink lightweight include hierarchy to keep moc parse times fast."
---

# MAVLink Include Rules for Headers

C++ header files must use the **lightest possible** MAVLink include. Heavy includes in headers slow Qt moc parsing dramatically (~2.5s vs ~8ms per header) and every translation unit that includes them. `MAVLinkLib.h` skips the dialect under moc (`Q_MOC_RUN`), so moc sees only the `MAVLinkMessageType.h` base types from it.

## Include Hierarchy (lightest to heaviest)

| Header | Provides | Preprocessed Lines |
| ------ | -------- | ------------------ |
| `QGCMAVLinkTypes.h` | `FirmwareClass_t`, `VehicleClass_t`, `maxRcChannels` | ~50 |
| `MAVLinkMessageType.h` | `mavlink_message_t`, `mavlink_channel_t`, base types | ~2,700 |
| `MAVLinkEnums.h` | All MAVLink enum typedefs (MAV_CMD, MAV_TYPE, MAV_RESULT, etc.), generated at build time | ~5,500 |
| `VehicleTypes.h` | `MavCmdResultFailureCode_t`, `RequestMessageResultHandlerFailureCode_t` (includes `MAVLinkEnums.h` and `QGCMAVLinkTypes.h`) | ~5,500 |
| `QGCMAVLink.h` | `MAVLinkEnums.h`, `MAVLinkMessageType.h`, and `QGCMAVLinkTypes.h` combined | ~8,300 |
| `MAVLinkLib.h` | Full MAVLink with every message pack/unpack header | ~180,000 |

## Rules

1. **Never include `MAVLinkLib.h` in a `.h` file** unless the header uses specific MAVLink message struct types in its declarations (e.g., `mavlink_command_long_t`, `mavlink_command_ack_t`). Move message struct usage to `.cc` files when possible.

2. **Never include `Vehicle.h` just for type aliases.** Use `VehicleTypes.h` + `class Vehicle;` forward declaration instead, when only `MavCmdResultFailureCode_t` or `RequestMessageResultHandlerFailureCode_t` are needed.

3. **Pick the lightest include that satisfies the header's needs:**
   - Only enum constants? → `MAVLinkEnums.h`
   - `mavlink_message_t` in signatures? → `MAVLinkMessageType.h`
   - Both enums and message type? → `MAVLinkEnums.h` + `MAVLinkMessageType.h` (still lighter than `QGCMAVLink.h`)
   - `FirmwareClass_t` / `VehicleClass_t`? → `QGCMAVLinkTypes.h`

4. **Headers must be self-contained.** Every type used in a header's declarations must be provided by an explicit `#include`, not by the precompiled header (PCH). The PCH provides `MAVLinkLib.h` for `.cc` files, but moc processes headers independently.

5. **`MAVLinkLib.h` is fine in `.cc` files** — it's provided by the PCH anyway. Only header includes affect moc parse time.

6. **Keep the `Q_MOC_RUN` guard in `MAVLinkLib.h`.** Preprocessing the full dialect after some Qt headers makes moc take seconds per header. Moc never needs the message definitions; the compiler still sees them when it builds the generated `moc_*.cpp`.
