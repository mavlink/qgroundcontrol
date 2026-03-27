#include "GPSDeviceFlags.h"
#include "GPSManager.h"
#include "GPSRtk.h"
#include "GPSTypes.h"

// Single source of truth: kGPSTypeRegistry drives both the manufacturer id
// namespace (GPSDeviceFlags::Manufacturer) and the bitmask namespace
// (GPSDeviceFlags::Flag). These static_asserts catch silent drift when a new
// receiver is added to kGPSTypeRegistry without a matching Flag/Manufacturer
// entry here.
static_assert(GPSDeviceFlags::Trimble    == 0x01, "Flag::Trimble must match kGPSTypeRegistry");
static_assert(GPSDeviceFlags::Septentrio == 0x02, "Flag::Septentrio must match kGPSTypeRegistry");
static_assert(GPSDeviceFlags::Femtomes   == 0x04, "Flag::Femtomes must match kGPSTypeRegistry");
static_assert(GPSDeviceFlags::Ublox      == 0x08, "Flag::Ublox must match kGPSTypeRegistry");
static_assert(GPSDeviceFlags::Nmea       == 0x10, "Flag::Nmea must match kGPSTypeRegistry");

int GPSDeviceFlags::fromDeviceType(const QString &devType)
{
    if (devType.isEmpty()) {
        return All;
    }
    const QString dt = devType.toLower();
    for (const auto &entry : kGPSTypeRegistry) {
        if (dt.contains(QLatin1String(entry.matchString))) {
            return entry.deviceFlag;
        }
    }
    return All;
}

int GPSDeviceFlags::fromManufacturer(int manufacturer)
{
    if (manufacturer == ManufacturerAuto) {
        return All;
    }
    for (const auto &entry : kGPSTypeRegistry) {
        if (entry.manufacturerId == manufacturer) {
            return entry.deviceFlag;
        }
    }
    return All;
}

int GPSDeviceFlags::resolve(QObject *gpsMgr, int manufacturer)
{
    auto *mgr = qobject_cast<GPSManager*>(gpsMgr);
    if (mgr && mgr->deviceCount() > 0) {
        GPSRtk *primary = mgr->gpsRtk();
        if (primary && !primary->deviceType().isEmpty()) {
            return fromDeviceType(primary->deviceType());
        }
    }
    return fromManufacturer(manufacturer);
}
