/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>

#include "LandingComplexItem.h"
#include "Fact.h"

Q_DECLARE_LOGGING_CATEGORY(FixedWingLandingComplexItemLog)

class FWLandingPatternTest;
class PlanMasterController;
class MissionItem;

class FixedWingLandingComplexItem : public LandingComplexItem
{
    Q_OBJECT
    Q_MOC_INCLUDE("MissionItem.h")

public:
    FixedWingLandingComplexItem(PlanMasterController* masterController, bool flyView);

    Q_PROPERTY(Fact*            valueSetIsDistance      READ    valueSetIsDistance                                          CONSTANT)
    Q_PROPERTY(Fact*            glideSlope              READ    glideSlope                                                  CONSTANT)

    Q_INVOKABLE void moveLandingPosition(const QGeoCoordinate& coordinate); // Maintains the current landing distance and heading

    Fact*           glideSlope              (void) { return &_glideSlopeFact; }
    Fact*           valueSetIsDistance      (void) { return &_valueSetIsDistanceFact; }

    /// Scans the loaded items for a landing pattern complex item
    static bool scanForItems(QmlObjectListModel* visualItems, bool flyView, PlanMasterController* masterController);

    // Overrides from ComplexMissionItem
    QString patternName         (void) const final;
    bool    load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    QString mapVisualQML        (void) const final { return QStringLiteral("FWLandingPatternMapVisual.qml"); }

    // Overrides from VisualMissionItem
    void                save                        (QJsonArray&  missionItems) final;

    static const QString name;

    static constexpr const char* settingsGroup                      = "FixedWingLanding";
    static constexpr const char* jsonComplexItemTypeValue           = "fwLandingPattern";

    static constexpr const char* glideSlopeName                     = "GlideSlope";
    static constexpr const char* valueSetIsDistanceName             = "ValueSetIsDistance";

private slots:
    void _updateFlightPathSegmentsDontCallDirectly  (void) override;
    void _glideSlopeChanged                         (void);

private:
    static LandingComplexItem*  _createItem     (PlanMasterController* masterController, bool flyView) { return new FixedWingLandingComplexItem(masterController, flyView); }
    static bool                 _isValidLandItem(const MissionItem& missionItem);

    // Overrides from LandingComplexItem
    const Fact*     _finalApproachAltitude  (void) const final { return &_finalApproachAltitudeFact; }
    const Fact*     _useDoChangeSpeed       (void) const final { return &_useDoChangeSpeedFact; }
    const Fact*     _finalApproachSpeed     (void) const final { return &_finalApproachSpeedFact; }
    const Fact*     _loiterRadius           (void) const final { return &_loiterRadiusFact; }
    const Fact*     _loiterClockwise        (void) const final { return &_loiterClockwiseFact; }
    const Fact*     _landingAltitude        (void) const final { return &_landingAltitudeFact; }
    const Fact*     _landingDistance        (void) const final { return &_landingDistanceFact; }
    const Fact*     _landingHeading         (void) const final { return &_landingHeadingFact; }
    const Fact*     _useLoiterToAlt         (void) const final { return &_useLoiterToAltFact; }
    const Fact*     _stopTakingPhotos       (void) const final { return &_stopTakingPhotosFact; }
    const Fact*     _stopTakingVideo        (void) const final { return &_stopTakingVideoFact; }
    void            _calcGlideSlope         (void) final;
    MissionItem*    _createLandItem         (int seqNum, bool altRel, double lat, double lon, double alt, QObject* parent) final;

    QMap<QString, FactMetaData*> _metaDataMap;

    Fact            _landingDistanceFact;
    Fact            _finalApproachAltitudeFact;
    Fact            _useDoChangeSpeedFact;
    Fact            _finalApproachSpeedFact;
    Fact            _loiterRadiusFact;
    Fact            _loiterClockwiseFact;
    Fact            _landingHeadingFact;
    Fact            _landingAltitudeFact;
    Fact            _glideSlopeFact;
    Fact            _useLoiterToAltFact;
    Fact            _stopTakingPhotosFact;
    Fact            _stopTakingVideoFact;
    Fact            _valueSetIsDistanceFact;

    static constexpr const char* _jsonValueSetIsDistanceKey         = "valueSetIsDistance";

    friend FWLandingPatternTest;
};
