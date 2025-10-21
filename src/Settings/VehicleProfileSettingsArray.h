/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "SettingsGroupArray.h"

class VehicleProfileSettings;

/// Wraps a SettingsGroupArray specialized for VehicleProfileSettings entries.
class VehicleProfileSettingsArray : public SettingsGroupArray
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Access via SettingsManager")

public:
    explicit VehicleProfileSettingsArray(QObject* parent = nullptr);

protected:
    SettingsGroup* _newGroupInstance(int indexKey) override;
};
