#include "VehicleRoleController.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "AppSettings.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(VehicleRoleLog, "Custom.VehicleRoles")

namespace {
const char* kRolesFileName = "VehicleRoles.json";
}

VehicleRoleEntry::VehicleRoleEntry(int sysid, const QString& role, const QString& name, QObject* parent)
    : QObject(parent), _sysid(sysid), _role(role), _name(name)
{}

void VehicleRoleEntry::setRole(const QString& role)
{
    if (_role != role) {
        _role = role;
        emit roleChanged(_role);
    }
}

void VehicleRoleEntry::setName(const QString& name)
{
    if (_name != name) {
        _name = name;
        emit nameChanged(_name);
    }
}

VehicleRoleController::VehicleRoleController(QObject* parent)
    : QObject(parent), _roleEntries(new QmlObjectListModel(this))
{
    _load();
}

int VehicleRoleController::_indexForSysid(int sysid) const
{
    for (int i = 0; i < _roleEntries->count(); i++) {
        if (qobject_cast<VehicleRoleEntry*>(_roleEntries->get(i))->sysid() == sysid) {
            return i;
        }
    }
    return -1;
}

void VehicleRoleController::addEntry(int sysid, const QString& role, const QString& name)
{
    if (sysid < 1 || sysid > 255 || !availableRoles().contains(role)) {
        qCWarning(VehicleRoleLog) << "Ignoring invalid entry - sysid:" << sysid << "role:" << role;
        return;
    }

    const int existingIndex = _indexForSysid(sysid);
    if (existingIndex >= 0) {
        setRole(existingIndex, role);
        setName(existingIndex, name);
        return;
    }

    _roleEntries->append(new VehicleRoleEntry(sysid, role, name, this));
    _save();
}

void VehicleRoleController::removeEntry(int index)
{
    if (auto* entry = qobject_cast<VehicleRoleEntry*>(_roleEntries->get(index))) {
        _roleEntries->removeOne(entry);
        entry->deleteLater();
        _save();
    }
}

void VehicleRoleController::setRole(int index, const QString& role)
{
    if (!availableRoles().contains(role)) {
        qCWarning(VehicleRoleLog) << "Ignoring invalid role:" << role;
        return;
    }
    if (auto* entry = qobject_cast<VehicleRoleEntry*>(_roleEntries->get(index))) {
        entry->setRole(role);
        _save();
    }
}

void VehicleRoleController::setName(int index, const QString& name)
{
    if (auto* entry = qobject_cast<VehicleRoleEntry*>(_roleEntries->get(index))) {
        entry->setName(name);
        _save();
    }
}

QString VehicleRoleController::nameForSysid(int sysid) const
{
    const int index = _indexForSysid(sysid);
    return index >= 0 ? qobject_cast<VehicleRoleEntry*>(_roleEntries->get(index))->name() : QString();
}

QString VehicleRoleController::roleForSysid(int sysid) const
{
    const int index = _indexForSysid(sysid);
    return index >= 0 ? qobject_cast<VehicleRoleEntry*>(_roleEntries->get(index))->role() : QString();
}

void VehicleRoleController::_load()
{
    const QString filePath =
        SettingsManager::instance()->appSettings()->settingsSavePath() + QStringLiteral("/") + kRolesFileName;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isArray()) {
        qCWarning(VehicleRoleLog) << "Ignoring malformed roles file:" << filePath;
        return;
    }

    for (const QJsonValue& value : doc.array()) {
        const QJsonObject obj = value.toObject();
        const int sysid = obj.value(QStringLiteral("sysid")).toInt();
        const QString role = obj.value(QStringLiteral("role")).toString();
        if (sysid < 1 || sysid > 255 || !availableRoles().contains(role)) {
            qCWarning(VehicleRoleLog) << "Skipping invalid saved entry - sysid:" << sysid << "role:" << role;
            continue;
        }
        _roleEntries->append(new VehicleRoleEntry(sysid, role, obj.value(QStringLiteral("name")).toString(), this));
    }
}

void VehicleRoleController::_save()
{
    QJsonArray array;
    for (int i = 0; i < _roleEntries->count(); i++) {
        auto* entry = qobject_cast<VehicleRoleEntry*>(_roleEntries->get(i));
        QJsonObject obj;
        obj[QStringLiteral("sysid")] = entry->sysid();
        obj[QStringLiteral("role")] = entry->role();
        obj[QStringLiteral("name")] = entry->name();
        array.append(obj);
    }

    const QString filePath =
        SettingsManager::instance()->appSettings()->settingsSavePath() + QStringLiteral("/") + kRolesFileName;
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(VehicleRoleLog) << "Failed to open roles file for write:" << filePath;
        return;
    }
    file.write(QJsonDocument(array).toJson());
}
