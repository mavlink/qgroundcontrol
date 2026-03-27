#pragma once
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

class GPSDeviceFlags : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit GPSDeviceFlags(QObject *parent = nullptr) : QObject(parent) {}

    enum Flag : int {
        Trimble    = 0x01,
        Septentrio = 0x02,
        Femtomes   = 0x04,
        Ublox      = 0x08,
        Nmea       = 0x10,
        AllRtk     = Trimble | Septentrio | Femtomes | Ublox,
        All        = AllRtk | Nmea
    };
    Q_ENUM(Flag)

    // Manufacturer indices matching GcsGpsSettings.baseReceiverManufacturers enum order
    enum Manufacturer : int {
        ManufacturerAuto       = 0,
        ManufacturerTrimble    = 1,
        ManufacturerSeptentrio = 2,
        ManufacturerFemtomes   = 3,
        ManufacturerUblox      = 4,
        ManufacturerNmea       = 5,
    };
    Q_ENUM(Manufacturer)

    Q_INVOKABLE static int fromDeviceType(const QString &devType);
    Q_INVOKABLE static int fromManufacturer(int manufacturer);
    /// Resolves device flags from a GPSManager instance. QML passes QGroundControl.gpsManager.
    Q_INVOKABLE static int resolve(QObject *gpsMgr, int manufacturer);
};
