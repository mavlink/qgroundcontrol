/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactPanelController.h"
#include "SettingsFact.h"

class APMFollowComponentController : public FactPanelController
{
    Q_OBJECT

    Q_PROPERTY(Fact *angle          READ angleFact      CONSTANT)
    Q_PROPERTY(Fact *distance       READ distanceFact   CONSTANT)
    Q_PROPERTY(Fact *height         READ heightFact     CONSTANT)
    Q_PROPERTY(bool roverFirmware   READ roverFirmware  CONSTANT)

public:
    explicit APMFollowComponentController(QObject *parent = nullptr);
    ~APMFollowComponentController();

    Fact *angleFact() const { return _angleFact; }
    Fact *distanceFact() const { return _distanceFact; }
    Fact *heightFact() const { return _heightFact; }
    bool roverFirmware() const;

private:
    QMap<QString, FactMetaData*> _metaDataMap;

    SettingsFact *_angleFact = nullptr;
    SettingsFact *_distanceFact = nullptr;
    SettingsFact *_heightFact = nullptr;

    static constexpr const char *_angleName = "angle";
    static constexpr const char *_distanceName = "distance";
    static constexpr const char *_heightName = "height";
    static constexpr const char *_settingsGroup = "APMFollow";
};
