/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class QmlObjectListModel;

/// Variant of SettingsGroup which manages a list of identically typed Settings groups.
class SettingsGroupArray : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Access via SettingsManager")
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(QmlObjectListModel* groups READ groups CONSTANT FINAL)

public:
    SettingsGroupArray(const QString& name, const QString& settingsGroupRoot, QObject* parent = nullptr);
    ~SettingsGroupArray() override;

    int count() const;
    QmlObjectListModel* groups() const;

    Q_INVOKABLE SettingsGroup* at(int index) const;
    Q_INVOKABLE SettingsGroup* append();
    Q_INVOKABLE void removeAt(int index);
    Q_INVOKABLE void clear();

    QString name() const { return _name; }
    QString settingsGroupRoot() const { return _settingsGroupRoot; }

signals:
    void countChanged();

protected:
    virtual SettingsGroup* _newGroupInstance(int indexKey);

private:
    struct Entry {
        int indexKey = -1;
        SettingsGroup* group = nullptr;
    };

    SettingsGroup* _createEntry(int indexKey);
    int _nextIndexKey() const;
    void _loadFromStore();
    void _persistPresence(int indexKey) const;
    void _removeFromStore(int indexKey) const;

    const QString _name;
    const QString _settingsGroupRoot;
    QList<Entry> _entries;
    QmlObjectListModel* _model = nullptr;
};
