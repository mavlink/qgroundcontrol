#include "SerialPortInfoCodec.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QString>

#include "QGCSerialPortTypes.h"
#include "SerialWireConstants.h"

namespace {

namespace keys = AndroidSerialWire::JsonKeys;

QString portNameFromDevicePath(const QString& source)
{
    return source.startsWith(kDevPrefix) ? source.mid(kDevPrefix.size()) : source;
}

}  // namespace

namespace SerialPortInfoCodec {

QList<QGCSerialPortInfo::Data> unpack(QByteArrayView packed)
{
    QList<QGCSerialPortInfo::Data> out;
    if (packed.isEmpty()) {
        return out;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(packed.data(), packed.size()));
    if (!doc.isObject()) {  // garbled buffer; callers log and proceed
        return out;
    }

    const QJsonArray ports = doc.object().value(keys::Ports).toArray();
    out.reserve(ports.size());
    for (const QJsonValue& portValue : ports) {
        const QJsonObject port = portValue.toObject();

        // A missing string key (absent in JSON) yields a null QString, distinct from an empty "".
        const QString deviceName = port.value(keys::DeviceName).toString();
        if (deviceName.isEmpty()) {
            continue;  // unusable port
        }

        QGCSerialPortInfo::Data data;
        data.portName = portNameFromDevicePath(deviceName);
        data.systemLocation = deviceName;
        data.description = port.value(keys::ProductName).toString();
        data.manufacturer = port.value(keys::ManufacturerName).toString();
        data.serialNumber = port.value(keys::SerialNumber).toString();
        data.productIdentifier = static_cast<quint16>(port.value(keys::ProductId).toInt());
        data.vendorIdentifier = static_cast<quint16>(port.value(keys::VendorId).toInt());
        // VID/PID enable LinkManager::_filterCompositePorts dedup, which also needs a non-empty, non-"0"
        // serialNumber to drop a board's secondary CDC port. Boards reporting no serial are not deduped.
        data.hasVendorIdentifier = (data.vendorIdentifier != 0);
        data.hasProductIdentifier = (data.productIdentifier != 0);

        const QJsonArray bauds = port.value(keys::BaudRates).toArray();
        QList<qint32> baudList;
        baudList.reserve(bauds.size());
        for (const QJsonValue& baud : bauds) {
            baudList.append(baud.toInt());
        }
        data.supportedBaudRates = std::move(baudList);

        out.emplace_back(std::move(data));
    }

    return out;
}

}  // namespace SerialPortInfoCodec
