#pragma once

#include "APM/APMAutoPilotPlugin.h"
#include "OnboardComputersManager.h"
#include "VioGpsComparer/VioGpsComparer.h"
class Vehicle;
class FoxFourCameraControl;
class FoxFourAutoPilotPlugin : public APMAutoPilotPlugin
{
    Q_OBJECT
    Q_PROPERTY(OnboardComputersManager* onboardComputersManager READ onboardComputersManager MEMBER _onboardComputersMngr)
    Q_PROPERTY(VioGpsComparer *vioGpsComparer MEMBER _vioGpsComparer)
    Q_PROPERTY(QString storageCapacity READ storageCapacity NOTIFY storageCapacityChanged)

public:
    explicit FoxFourAutoPilotPlugin(Vehicle *vehicle, QObject *parent = nullptr);
    ~FoxFourAutoPilotPlugin();
    /// This allows us to hide Vehicle Setup pages if needed
    const QVariantList &vehicleComponents() final;
    QString storageCapacity();
    /// Reboot all onboard computers
    Q_INVOKABLE void rebootOnboardComputers();
    Q_INVOKABLE void setEK3Source(int index);

    OnboardComputersManager* onboardComputersManager();
signals:
    void storageCapacityChanged();
private slots:
    void handleStorageCapacityChanged(uint32_t total, uint32_t free);
private:
    QVariantList _components;
    OnboardComputersManager *_onboardComputersMngr = nullptr;
    VioGpsComparer *_vioGpsComparer = nullptr;
    QString _storageCapacityStr = "0 / 0 MB";
    QMetaObject::Connection _cameraConnection;
};
