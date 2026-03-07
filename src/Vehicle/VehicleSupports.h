#pragma once

#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

class FirmwarePlugin;
class Vehicle;

class VehicleSupports : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit VehicleSupports(Vehicle *vehicle);

    Q_PROPERTY(bool throttleModeCenterZero          READ throttleModeCenterZero         CONSTANT)
    Q_PROPERTY(bool negativeThrust                  READ negativeThrust                 CONSTANT)
    Q_PROPERTY(bool jsButton                        READ jsButton                       CONSTANT)
    Q_PROPERTY(bool radio                           READ radio                          CONSTANT)
    Q_PROPERTY(bool motorInterference               READ motorInterference              CONSTANT)
    Q_PROPERTY(bool smartRTL                        READ smartRTL                       CONSTANT)
    Q_PROPERTY(bool terrainFrame                    READ terrainFrame                   NOTIFY terrainFrameChanged)
    Q_PROPERTY(bool guidedMode                      READ guidedMode                     CONSTANT)
    Q_PROPERTY(bool pauseVehicle                    READ pauseVehicle                   CONSTANT)
    Q_PROPERTY(bool orbitMode                       READ orbitMode                      CONSTANT)
    Q_PROPERTY(bool roiMode                         READ roiMode                        CONSTANT)
    Q_PROPERTY(bool takeoffMissionCommand           READ takeoffMissionCommand          CONSTANT)
    Q_PROPERTY(bool guidedTakeoffWithAltitude       READ guidedTakeoffWithAltitude      CONSTANT)
    Q_PROPERTY(bool guidedTakeoffWithoutAltitude    READ guidedTakeoffWithoutAltitude   CONSTANT)
    Q_PROPERTY(bool changeHeading                   READ changeHeading                  CONSTANT)

    bool throttleModeCenterZero() const;
    bool negativeThrust() const;
    bool jsButton() const;
    bool radio() const;
    bool motorInterference() const;
    bool smartRTL() const;
    bool terrainFrame() const;
    bool guidedMode() const;
    bool pauseVehicle() const;
    bool orbitMode() const;
    bool roiMode() const;
    bool takeoffMissionCommand() const;
    bool guidedTakeoffWithAltitude() const;
    bool guidedTakeoffWithoutAltitude() const;
    bool changeHeading() const;

signals:
    void terrainFrameChanged();

private:
    Vehicle *_vehicle = nullptr;
};
