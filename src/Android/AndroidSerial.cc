#include "AndroidSerial.h"
#include "QGCSerialPortInfo.h"
#include "qandroidserialengine_p.h"

#include <QtCore/QJniArray>
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/qjnitypes.h>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(AndroidSerialLog, "Android.Serial");

Q_DECLARE_JNI_CLASS(QGCUsbSerialManager, "org/mavlink/qgroundcontrol/serial/QGCUsbSerialManager")
Q_DECLARE_JNI_CLASS(UsbPortInfo, "org/mavlink/qgroundcontrol/serial/UsbPortInfo")

namespace AndroidSerial {

void initialize()
{
    QAndroidSerialEngine::initialize();
}

namespace {

// JNI deviceName arrives as "/dev/bus/usb/<bus>/<dev>" (absolute) — strip /dev/ for
// portName like the upstream Qt path does, otherwise leave as-is.
QString portNameFromDevicePath(const QString &source)
{
    return source.startsWith(QLatin1String("/dev/")) ? source.mid(5) : source;
}

}  // namespace

QList<QGCSerialPortInfo> availableDevices()
{
    QList<QGCSerialPortInfo> serialPortInfoList;
    QJniEnvironment env;

    const auto array = QJniObject::callStaticMethod<QtJniTypes::QGCUsbSerialManager,
                                                    QJniArray<QtJniTypes::UsbPortInfo>>("availablePortsInfo");
    if (env.checkAndClearExceptions() || !array.isValid()) {
        if (!array.isValid()) {
            qCDebug(AndroidSerialLog) << "availablePortsInfo returned null/empty";
        } else {
            qCWarning(AndroidSerialLog) << "Exception occurred while calling availablePortsInfo";
        }
        return serialPortInfoList;
    }

    qsizetype index = 0;
    for (const QJniObject &portInfo : array) {
        const qsizetype currentIndex = index++;
        if (!portInfo.isValid()) {
            qCWarning(AndroidSerialLog) << "Null UsbPortInfo element at index" << currentIndex;
            continue;
        }

        const QString deviceName = portInfo.callMethod<jstring>("deviceName").toString();
        const QString productName = portInfo.callMethod<jstring>("productName").toString();
        const QString manufacturerName = portInfo.callMethod<jstring>("manufacturerName").toString();
        const QString serialNumber = portInfo.callMethod<jstring>("serialNumber").toString();
        const jint productId = portInfo.callMethod<jint>("productId");
        const jint vendorId = portInfo.callMethod<jint>("vendorId");

        if (env.checkAndClearExceptions()) {
            qCWarning(AndroidSerialLog) << "Exception reading UsbPortInfo at index" << currentIndex;
            continue;
        }

        QGCSerialPortInfo::Data data;
        data.portName             = portNameFromDevicePath(deviceName);
        data.systemLocation       = deviceName;
        data.description          = productName;
        data.manufacturer         = manufacturerName;
        data.serialNumber         = serialNumber;
        data.productIdentifier    = static_cast<quint16>(productId);
        data.hasProductIdentifier = (productId != INVALID_DEVICE_ID);
        data.vendorIdentifier     = static_cast<quint16>(vendorId);
        data.hasVendorIdentifier  = (vendorId  != INVALID_DEVICE_ID);

        serialPortInfoList.append(QGCSerialPortInfo(std::move(data)));
    }

    (void) env.checkAndClearExceptions();

    return serialPortInfoList;
}

}  // namespace AndroidSerial
