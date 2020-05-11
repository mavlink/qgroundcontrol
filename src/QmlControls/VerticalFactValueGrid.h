/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactSystem.h"
#include "QmlObjectListModel.h"
#include "QGCApplication.h"
#include "FactValueGrid.h"

class InstrumentValueData;

class VerticalFactValueGrid : public FactValueGrid
{
    Q_OBJECT

public:
    VerticalFactValueGrid(QQuickItem *parent = nullptr);
    VerticalFactValueGrid(const QString& defaultSettingsGroup);

    Q_PROPERTY(QString              valuePageDefaultSettingsGroup   MEMBER valuePageDefaultSettingsGroup                    CONSTANT)
    Q_PROPERTY(QString              valuePageUserSettingsGroup      MEMBER _valuePageUserSettingsGroup                      CONSTANT)

    static const QString valuePageDefaultSettingsGroup;

private:
    Q_DISABLE_COPY(VerticalFactValueGrid)

    static const QString _valuePageUserSettingsGroup;
};

QML_DECLARE_TYPE(VerticalFactValueGrid)
