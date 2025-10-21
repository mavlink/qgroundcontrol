/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SettingsGroupArray.h"
#include "QmlObjectListModel.h"

#include <QtCore/QSettings>
#include <QtCore/QSet>

#include <algorithm>

namespace {

//! Helper that maps each array entry to a unique QSettings group path.
class SettingsGroupArrayIndexedGroup : public SettingsGroup
{
public:
    SettingsGroupArrayIndexedGroup(const QString& name, const QString& settingsGroupRoot, int indexKey, QObject* parent)
        : SettingsGroup(name, settingsGroupRoot, parent)
        , _index(indexKey)
    {
        if (settingsGroupRoot.isEmpty()) {
            _settingsGroup = QString::number(indexKey);
        } else {
            _settingsGroup = QStringLiteral("%1/%2").arg(settingsGroupRoot, QString::number(indexKey));
        }
    }

    int index() const { return _index; }

private:
    int _index = -1;
};

} // namespace

SettingsGroupArray::SettingsGroupArray(const QString& name, const QString& settingsGroupRoot, QObject* parent)
    : QObject(parent)
    , _name(name)
    , _settingsGroupRoot(settingsGroupRoot)
    , _model(new QmlObjectListModel(this))
{
    connect(_model, &QmlObjectListModel::countChanged, this, [this](int) {
        emit countChanged();
    });
    _loadFromStore();
}

SettingsGroupArray::~SettingsGroupArray() = default;

int SettingsGroupArray::count() const
{
    return _model ? _model->count() : 0;
}

QmlObjectListModel* SettingsGroupArray::groups() const
{
    return _model;
}

SettingsGroup* SettingsGroupArray::at(int index) const
{
    if (index < 0 || index >= _entries.count()) {
        return nullptr;
    }
    return _entries.at(index).group;
}

SettingsGroup* SettingsGroupArray::append()
{
    const int indexKey = _nextIndexKey();
    SettingsGroup* group = _createEntry(indexKey);
    _entries.append({ indexKey, group });
    _model->append(group);
    _persistPresence(indexKey);
    return group;
}

void SettingsGroupArray::removeAt(int index)
{
    if (index < 0 || index >= _entries.count()) {
        return;
    }

    const Entry entry = _entries.takeAt(index);
    (void)_model->removeAt(index);
    _removeFromStore(entry.indexKey);
    if (entry.group) {
        entry.group->deleteLater();
    }
}

void SettingsGroupArray::clear()
{
    while (!_entries.isEmpty()) {
        removeAt(_entries.count() - 1);
    }
}

SettingsGroup* SettingsGroupArray::_createEntry(int indexKey)
{
    SettingsGroup* group = _newGroupInstance(indexKey);
    if (!group) {
        return nullptr;
    }

    if (_settingsGroupRoot.isEmpty()) {
        group->_settingsGroup = QString::number(indexKey);
    } else {
        group->_settingsGroup = QStringLiteral("%1/%2").arg(_settingsGroupRoot, QString::number(indexKey));
    }

    // Pre-create the facts to make them immediately accessible from QML.
    const QStringList facts = group->factNames();
    for (const QString& factName : facts) {
        group->fact(factName);
    }

    return group;
}

SettingsGroup* SettingsGroupArray::_newGroupInstance(int indexKey)
{
    return new SettingsGroupArrayIndexedGroup(_name, _settingsGroupRoot, indexKey, this);
}

int SettingsGroupArray::_nextIndexKey() const
{
    QSet<int> used;
    used.reserve(_entries.count());
    for (const Entry& entry : _entries) {
        used.insert(entry.indexKey);
    }

    int candidate = 0;
    while (used.contains(candidate)) {
        ++candidate;
    }
    return candidate;
}

void SettingsGroupArray::_loadFromStore()
{
    QSettings settings;
    if (!_settingsGroupRoot.isEmpty()) {
        settings.beginGroup(_settingsGroupRoot);
    }

    QList<int> discoveredIndexes;
    const QStringList childGroups = settings.childGroups();
    discoveredIndexes.reserve(childGroups.size());
    for (const QString& groupName : childGroups) {
        bool ok = false;
        const int idx = groupName.toInt(&ok);
        if (ok) {
            discoveredIndexes.append(idx);
        }
    }

    std::sort(discoveredIndexes.begin(), discoveredIndexes.end());

    if (!_settingsGroupRoot.isEmpty()) {
        settings.endGroup();
    }

    for (const Entry& entry : _entries) {
        if (entry.group) {
            entry.group->deleteLater();
        }
    }
    _entries.clear();
    if (_model) {
        _model->clear();
    }

    for (int idx : discoveredIndexes) {
        SettingsGroup* group = _createEntry(idx);
        if (!group) {
            continue;
        }
        _entries.append({ idx, group });
        if (_model) {
            _model->append(group);
        }
    }
    if (_model) {
        _model->setDirty(false);
    }
}

void SettingsGroupArray::_persistPresence(int indexKey) const
{
    QSettings settings;
    if (!_settingsGroupRoot.isEmpty()) {
        settings.beginGroup(_settingsGroupRoot);
    }

    settings.beginGroup(QString::number(indexKey));
    settings.setValue(QStringLiteral("_active"), true);
    settings.endGroup();

    if (!_settingsGroupRoot.isEmpty()) {
        settings.endGroup();
    }
}

void SettingsGroupArray::_removeFromStore(int indexKey) const
{
    QSettings settings;
    if (!_settingsGroupRoot.isEmpty()) {
        settings.beginGroup(_settingsGroupRoot);
    }

    settings.remove(QString::number(indexKey));

    if (!_settingsGroupRoot.isEmpty()) {
        settings.endGroup();
    }
}

