/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#ifndef ComplexMissionItem_H
#define ComplexMissionItem_H

#include "VisualMissionItem.h"
#include "MissionItem.h"

class ComplexMissionItem : public VisualMissionItem
{
    Q_OBJECT
    
public:
    ComplexMissionItem(Vehicle* vehicle, QObject* parent = NULL);
    ComplexMissionItem(const ComplexMissionItem& other, QObject* parent = NULL);

    Q_PROPERTY(QVariantList  polygonPath READ polygonPath NOTIFY polygonPathChanged)

    Q_INVOKABLE void clearPolygon(void);
    Q_INVOKABLE void addPolygonCoordinate(const QGeoCoordinate coordinate);

    QVariantList polygonPath(void);

    const QList<MissionItem*>& missionItems(void) { return _missionItems; }

    /// @return The next sequence number to use after this item. Takes into account child items of the complex item
    int nextSequenceNumber(void) const;

    // Overrides from VisualMissionItem

    bool            dirty                   (void) const final { return _dirty; }
    bool            isSimpleItem            (void) const final { return false; }
    bool            isStandaloneCoordinate  (void) const final { return false; }
    bool            specifiesCoordinate     (void) const final { return true; }
    QString         commandDescription      (void) const final { return "Survey"; }
    QString         commandName             (void) const final { return "Survey"; }
    QGeoCoordinate  coordinate              (void) const final { return _coordinate; }
    QGeoCoordinate  exitCoordinate          (void) const final { return _coordinate; }

    bool coordinateHasRelativeAltitude      (void) const final { return true; }
    bool exitCoordinateHasRelativeAltitude  (void) const final { return true; }
    bool exitCoordinateSameAsEntry          (void) const final { return false; }

    void setDirty           (bool dirty) final;
    void setCoordinate      (const QGeoCoordinate& coordinate);
    bool save               (QJsonObject& missionObject, QJsonArray& missionItems, QString& errorString) final;

signals:
    void polygonPathChanged(void);

private:
    bool                _dirty;
    QVariantList        _polygonPath;
    QList<MissionItem*> _missionItems;
    QGeoCoordinate      _coordinate;
};

#endif
