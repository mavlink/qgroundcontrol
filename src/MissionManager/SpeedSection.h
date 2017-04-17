/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "Section.h"
#include "FactSystem.h"
#include "QmlObjectListModel.h"

class SpeedSection : public Section
{
    Q_OBJECT

public:
    SpeedSection(Vehicle* vehicle, QObject* parent = NULL);

    Q_PROPERTY(bool     specifyFlightSpeed  READ specifyFlightSpeed WRITE setSpecifyFlightSpeed NOTIFY specifyFlightSpeedChanged)
    Q_PROPERTY(Fact*    flightSpeed         READ flightSpeed                                    CONSTANT)

    bool    specifyFlightSpeed      (void) const { return _specifyFlightSpeed; }
    Fact*   flightSpeed             (void) { return &_flightSpeedFact; }
    void    setSpecifyFlightSpeed   (bool specifyFlightSpeed);

    // Overrides from Section
    bool available          (void) const override { return _available; }
    bool dirty              (void) const override { return _dirty; }
    void setAvailable       (bool available) override;
    void setDirty           (bool dirty) override;
    bool scanForSection     (QmlObjectListModel* visualItems, int scanIndex) override;
    void appendSectionItems (QList<MissionItem*>& items, QObject* missionItemParent, int& seqNum) override;
    int  itemCount          (void) const override;
    bool settingsSpecified  (void) const override;

signals:
    void specifyFlightSpeedChanged(bool specifyFlightSpeed);

private slots:
    void _setDirty(void);

private:
    bool    _available;
    bool    _dirty;
    bool    _specifyFlightSpeed;
    Fact    _flightSpeedFact;

    static QMap<QString, FactMetaData*> _metaDataMap;

    static const char* _flightSpeedName;
};
