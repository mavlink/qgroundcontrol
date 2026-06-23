---
applyTo: "src/**/*.h"
description: "When performing a code review on C++ headers, check that heavy headers like Vehicle.h and QGCMAVLink.h are not included when a lightweight alternative or forward declaration suffices."
---

# Forward Declaration Rules for Headers

Prefer forward declarations and lightweight type headers over including full class definitions.

## Vehicle.h

`Vehicle.h` is one of the heaviest headers in the codebase. Most headers that reference `Vehicle` only need:
- A pointer/reference to Vehicle → `class Vehicle;` forward declaration
- Vehicle type aliases → `#include "VehicleTypes.h"`

Only include `Vehicle.h` if the header calls Vehicle methods, accesses Vehicle members, or inherits from Vehicle.

## QGCMAVLink.h

`QGCMAVLink.h` combines MAVLink enums, message types, and QGC-specific type aliases. Most headers only need one of these. Use the specific lightweight header instead:
- `MAVLinkEnums.h` for enum constants
- `MAVLinkMessageType.h` for `mavlink_message_t`
- `QGCMAVLinkTypes.h` for `FirmwareClass_t`, `VehicleClass_t`
