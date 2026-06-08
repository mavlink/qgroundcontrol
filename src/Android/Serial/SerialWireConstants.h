#pragma once

// Hand-maintained: keep every value in sync with its twin in SerialWireConstants.java. Both sides pin the same
// literals via twin tests (SerialWireContractTest / SerialWireConstantsTest); FlowControl is also checked vs mik3y at runtime.

#include <QtCore/QLatin1StringView>
#include <QtCore/qtypes.h>

#include <limits>

namespace AndroidSerialWire {

constexpr qint64 MAX_CHUNK_BYTES = 16384;
constexpr int BAD_DEVICE_ID = 0;

// jint is spec-guaranteed 32-bit signed; use qint32 so this header compiles host-side (test) without JNI types.
static_assert(MAX_CHUNK_BYTES <= std::numeric_limits<qint32>::max(),
              "MAX_CHUNK_BYTES must fit the JNI jint length field");

enum class JavaExceptionKind : int
{
    Unknown = 0,  // Unclassified failure
    Resource = 1,  // IOException at runtime — hot-unplug
    Permission = 2,  // USB permission denied
    OpenFailed = 3,  // Open-path failure (driver / port / connection)
    Last = OpenFailed,
};

// FlowControl wire values match mik3y UsbSerialPort.FlowControl.ordinal() / SerialConstants.FC_*.
enum FlowControl
{
    NoFlowControl = 0,
    RtsCtsFlowControl = 1,
    DtrDsrFlowControl = 2,
    XonXoffFlowControl = 3,
    XonXoffInlineFlowControl = 4,
};

// JSON key names for the USB-port enumeration blob; twin of KEY_* in SerialWireConstants.java. The shared
// golden fixture (test/Comms/Serial/data/PortInfoGolden.json) pins these literals across both languages.
namespace JsonKeys {
constexpr auto Ports = QLatin1StringView("ports");
constexpr auto DeviceName = QLatin1StringView("deviceName");
constexpr auto ProductName = QLatin1StringView("productName");
constexpr auto ManufacturerName = QLatin1StringView("manufacturerName");
constexpr auto SerialNumber = QLatin1StringView("serialNumber");
constexpr auto ProductId = QLatin1StringView("productId");
constexpr auto VendorId = QLatin1StringView("vendorId");
constexpr auto BaudRates = QLatin1StringView("baudRates");
}  // namespace JsonKeys

}  // namespace AndroidSerialWire
