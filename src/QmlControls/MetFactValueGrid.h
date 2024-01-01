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

class MetFactValueGrid : public FactValueGrid
{
    Q_OBJECT

public:
    MetFactValueGrid(QQuickItem *parent = nullptr);
    MetFactValueGrid(const QString& defaultSettingsGroup);

    Q_PROPERTY(QString metDataDefaultSettingsGroup MEMBER metDataDefaultSettingsGroup CONSTANT)

    static const QString metDataDefaultSettingsGroup;

private:
    Q_DISABLE_COPY(MetFactValueGrid)
};

QML_DECLARE_TYPE(MetFactValueGrid)
