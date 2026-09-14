#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtQmlIntegration/QtQmlIntegration>

class QmlObjectListModel;

/// \brief A single sysid -> role/nickname assignment.
///
/// Mutated only through VehicleRoleController (mirrors StagedExclusionZone's
/// controller-owns-mutation style) so every change goes through one place that also persists it.
class VehicleRoleEntry : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by VehicleRoleController")

    Q_PROPERTY(int sysid READ sysid CONSTANT)
    Q_PROPERTY(QString role READ role NOTIFY roleChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)

public:
    VehicleRoleEntry(int sysid, const QString& role, const QString& name, QObject* parent = nullptr);

    int sysid() const { return _sysid; }

    QString role() const { return _role; }

    QString name() const { return _name; }

    void setRole(const QString& role);
    void setName(const QString& name);

signals:
    void roleChanged(QString role);
    void nameChanged(QString name);

private:
    int _sysid = 0;
    QString _role;
    QString _name;
};

/// \brief Remembers which MAVLink system ID (sysid) corresponds to which of the program's
/// permanent vehicles (Rover, Stallion, Hex), plus an optional operator nickname.
///
/// QGC already reads each vehicle's sysid from its heartbeat (Vehicle::id(), set on the vehicle
/// side by the ArduPilot SYSID_THISMAV parameter, the same value Mission Planner uses to
/// deconflict autopilots on a shared link) but has no persisted concept of which sysid means
/// which vehicle, and no way to show an operator-facing name anywhere. This is presentation-layer
/// bookkeeping only - it does not gate any safety-relevant behavior (rover takeover continues to
/// use Vehicle::rover()/MAV_TYPE, unchanged).
class VehicleRoleController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_MOC_INCLUDE("QmlObjectListModel.h")

    Q_PROPERTY(QmlObjectListModel* roleEntries READ roleEntries CONSTANT)
    Q_PROPERTY(QStringList availableRoles READ availableRoles CONSTANT)

public:
    explicit VehicleRoleController(QObject* parent = nullptr);

    QmlObjectListModel* roleEntries() const { return _roleEntries; }

    /// The program's fixed set of vehicle roles. Not user-extensible: the program has exactly
    /// three permanent vehicles.
    QStringList availableRoles() const
    {
        return {QStringLiteral("Rover"), QStringLiteral("Stallion"), QStringLiteral("Hex")};
    }

    /// Adds a new sysid/role/name assignment, or updates the existing entry for that sysid if one
    /// is already present. Ignored if sysid is outside the valid MAVLink range [1,255] or role
    /// isn't one of availableRoles().
    Q_INVOKABLE void addEntry(int sysid, const QString& role, const QString& name);

    Q_INVOKABLE void removeEntry(int index);
    Q_INVOKABLE void setRole(int index, const QString& role);
    Q_INVOKABLE void setName(int index, const QString& name);

    /// Returns the assigned name for sysid, or an empty string if unassigned. Callers should fall
    /// back to a generic label.
    Q_INVOKABLE QString nameForSysid(int sysid) const;
    /// Returns the assigned role for sysid, or an empty string if unassigned.
    Q_INVOKABLE QString roleForSysid(int sysid) const;

private:
    void _load();
    void _save();
    int _indexForSysid(int sysid) const;

    QmlObjectListModel* _roleEntries = nullptr;
};
