#include "VehicleProfile.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QSettings>

QGC_LOGGING_CATEGORY(VehicleProfileLog, "Vehicle.VehicleProfile")

//-----------------------------------------------------------------------------
VehicleProfile::VehicleProfile(Vehicle* vehicle, QObject* parent)
    : QObject(parent)
    , _vehicle(vehicle)
{
    if (!_vehicle) {
        qCWarning(VehicleProfileLog) << "VehicleProfile created with null vehicle";
    }
}

//-----------------------------------------------------------------------------
VehicleProfile::~VehicleProfile()
{
}

//-----------------------------------------------------------------------------
void
VehicleProfile::setName(const QString& name)
{
    if (_name != name) {
        _name = name;
        emit nameChanged();
    }
}

//-----------------------------------------------------------------------------
void
VehicleProfile::setDescription(const QString& description)
{
    if (_description != description) {
        _description = description;
        emit descriptionChanged();
    }
}

//-----------------------------------------------------------------------------
void
VehicleProfile::_setIsActive(bool active)
{
    if (_isActive != active) {
        _isActive = active;
        emit isActiveChanged();
    }
}

//-----------------------------------------------------------------------------
bool
VehicleProfile::loadProfile(const QString& profileName)
{
    if (!_vehicle) {
        qCWarning(VehicleProfileLog) << "Cannot load profile: no vehicle";
        return false;
    }

    if (profileName.isEmpty()) {
        qCWarning(VehicleProfileLog) << "Cannot load profile: empty profile name";
        return false;
    }

    QSettings settings;
    settings.beginGroup(QStringLiteral("VehicleProfiles"));
    settings.beginGroup(profileName);

    if (!settings.contains("name")) {
        qCWarning(VehicleProfileLog) << "Profile not found:" << profileName;
        settings.endGroup();
        settings.endGroup();
        return false;
    }

    _name = settings.value("name").toString();
    _description = settings.value("description").toString();

    settings.endGroup();
    settings.endGroup();

    emit nameChanged();
    emit descriptionChanged();
    emit profileLoaded(profileName);

    qCDebug(VehicleProfileLog) << "Profile loaded:" << profileName;
    return true;
}

//-----------------------------------------------------------------------------
bool
VehicleProfile::saveProfile()
{
    if (!_vehicle) {
        qCWarning(VehicleProfileLog) << "Cannot save profile: no vehicle";
        return false;
    }

    if (_name.isEmpty()) {
        qCWarning(VehicleProfileLog) << "Cannot save profile: empty profile name";
        return false;
    }

    QSettings settings;
    settings.beginGroup(QStringLiteral("VehicleProfiles"));
    settings.beginGroup(_name);

    settings.setValue("name", _name);
    settings.setValue("description", _description);

    settings.endGroup();
    settings.endGroup();

    emit profileSaved(_name);

    qCDebug(VehicleProfileLog) << "Profile saved:" << _name;
    return true;
}

//-----------------------------------------------------------------------------
bool
VehicleProfile::deleteProfile(const QString& profileName)
{
    if (profileName.isEmpty()) {
        qCWarning(VehicleProfileLog) << "Cannot delete profile: empty profile name";
        return false;
    }

    QSettings settings;
    settings.beginGroup(QStringLiteral("VehicleProfiles"));

    if (!settings.childGroups().contains(profileName)) {
        qCWarning(VehicleProfileLog) << "Profile not found:" << profileName;
        settings.endGroup();
        return false;
    }

    settings.remove(profileName);
    settings.endGroup();

    emit profileDeleted(profileName);

    qCDebug(VehicleProfileLog) << "Profile deleted:" << profileName;
    return true;
}

//-----------------------------------------------------------------------------
QStringList
VehicleProfile::availableProfiles() const
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("VehicleProfiles"));
    QStringList profiles = settings.childGroups();
    settings.endGroup();

    return profiles;
}

//-----------------------------------------------------------------------------
void
VehicleProfile::applyProfile()
{
    if (!_vehicle) {
        qCWarning(VehicleProfileLog) << "Cannot apply profile: no vehicle";
        return;
    }

    if (_name.isEmpty()) {
        qCWarning(VehicleProfileLog) << "Cannot apply profile: empty profile name";
        return;
    }

    _setIsActive(true);

    // TODO: Implement profile application logic
    // This would typically involve setting vehicle parameters,
    // flight modes, or other configuration based on the profile

    qCDebug(VehicleProfileLog) << "Profile applied:" << _name;
}
