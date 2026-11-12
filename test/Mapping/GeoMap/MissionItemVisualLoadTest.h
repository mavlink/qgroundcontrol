#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include "UnitTest.h"

/// Stands in for a mission item: what the maps read of one to load its visual
class MissionItemVisualLoadTestItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString mapVisualQML READ visualQml CONSTANT)
    Q_PROPERTY(QString geoMapVisualQML READ visualQml CONSTANT)
    Q_PROPERTY(bool isSimpleItem READ isSimpleItem CONSTANT)

public:
    explicit MissionItemVisualLoadTestItem(const QString& visualQml, QObject* parent = nullptr)
        : QObject(parent)
        , _visualQml(visualQml)
    {}

    QString visualQml() const { return _visualQml; }

    bool isSimpleItem() const { return false; }

private:
    const QString _visualQml;
};

/// Tests how the maps of both engines load the visuals of mission items
class MissionItemVisualLoadTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _itemRemovedWhileVisualLoads_data();
    void _itemRemovedWhileVisualLoads();
};
