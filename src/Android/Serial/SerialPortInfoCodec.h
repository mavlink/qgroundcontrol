#pragma once

// JNI-free decoder for the JSON USB-port enumeration buffer produced Java-side by
// QGCUsbSerialManager.packPortsInfo. Holds no JNI/QObject state, so it builds and unit-tests on the
// host (SerialPortInfoCodecTest). Keeping the wire format in one decoder — instead of inlined in
// AndroidSerialPort::availableDevices — means it is exercised by a round-trip test mirrored by the
// Java packer test (UsbPortInfoPackingTest), so a format change on either side fails a test.
//
// Shape (UTF-8 JSON): {"ports":[{deviceName, productName, manufacturerName, serialNumber, productId,
// vendorId, baudRates:[...]}]}. An absent string key is a null QString (distinct from an empty "");
// a missing baudRates yields no supported rates.

#include <QtCore/QByteArrayView>
#include <QtCore/QList>

#include "QGCSerialPortInfo.h"

namespace SerialPortInfoCodec {

// Decodes `packed` into per-port Data records. Returns the ports parsed before any malformity; a
// truncated/garbled buffer yields the prefix that parsed cleanly (callers log and proceed).
QList<QGCSerialPortInfo::Data> unpack(QByteArrayView packed);

}  // namespace SerialPortInfoCodec
