/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include "FactGroup.h"

class CustomFactGroup : public FactGroup
{
    Q_OBJECT

public:
    GPSRTKFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* custom READ custom CONSTANT)

    Fact* custom(void) { return &_custom; }

private:
    const QString _customFactName = QStringLiteral("custom");
};
