/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "VisualMissionItemTest.h"
#include "LandingComplexItem.h"
#include "MultiSignalSpy.h"
#include "PlanMasterController.h"

class LandingComplexItemTest : public VisualMissionItemTest
{
    Q_OBJECT
    
public:
    LandingComplexItemTest(void);

    void cleanup(void) override;
    void init   (void) override;

private slots:
    void _testDirty                 (void);
    void _testItemCount             (void);
    void _testAppendSectionItems    (void);
    void _testScanForItems          (void);
    void _testSaveLoad              (void);

private:
    void _validateItem(LandingComplexItem* actualItem, LandingComplexItem* expectedItem);

    enum {
        finalApproachCoordinateChangedIndex = 0,
        loiterTangentCoordinateChangedIndex,
        landingCoordinateChangedIndex,
        landingCoordSetChangedIndex,
        altitudesAreRelativeChangedIndex,
        _updateFlightPathSegmentsSignalIndex,
        maxSignalIndex,
    };

    enum {
        finalApproachCoordinateChangedMask      = 1 << finalApproachCoordinateChangedIndex,
        loiterTangentCoordinateChangedMask      = 1 << loiterTangentCoordinateChangedIndex,
        landingCoordinateChangedMask            = 1 << landingCoordinateChangedIndex,
        landingCoordSetChangedMask              = 1 << landingCoordSetChangedIndex,
        altitudesAreRelativeChangedMask         = 1 << altitudesAreRelativeChangedIndex,
        _updateFlightPathSegmentsSignalMask     = 1 << _updateFlightPathSegmentsSignalIndex,
    };

    static const size_t cSignals = maxSignalIndex;
    const char*         rgSignals[cSignals];

    LandingComplexItem* _item                   = nullptr;
    MultiSignalSpy*     _multiSpy               = nullptr;
    MultiSignalSpy*     _viMultiSpy             = nullptr;
    SimpleMissionItem*  _validStopVideoItem     = nullptr;
    SimpleMissionItem*  _validStopDistanceItem  = nullptr;
    SimpleMissionItem*  _validStopTimeItem      = nullptr;
};

// Simple class used to test LandingComplexItem base class
class SimpleLandingComplexItem : public LandingComplexItem
{
    Q_OBJECT

public:
    SimpleLandingComplexItem(PlanMasterController* masterController, bool flyView);

    // Overrides from ComplexMissionItem
    QString patternName (void) const final { return QString(); }
    bool    load        (const QJsonObject& /*complexObject*/, int /*sequenceNumber*/, QString& /*errorString*/) final { return false; };
    QString mapVisualQML(void) const final { return QStringLiteral("FWLandingPatternMapVisual.qml"); }

    // Overrides from VisualMissionItem
    void    save        (QJsonArray&  /*missionItems*/) override { };

    static const QString name;

    static const char* jsonComplexItemTypeValue;

    static const char* settingsGroup;

private slots:
    void _updateFlightPathSegmentsDontCallDirectly(void) override;

private:
    static LandingComplexItem*  _createItem     (PlanMasterController* masterController, bool flyView) { return new SimpleLandingComplexItem(masterController, flyView); }
    static bool                 _isValidLandItem(const MissionItem& missionItem);

    // Overrides from LandingComplexItem
    const Fact*     _finalApproachAltitude  (void) const final { return &_finalApproachAltitudeFact; }
    const Fact*     _loiterRadius           (void) const final { return &_loiterRadiusFact; }
    const Fact*     _loiterClockwise        (void) const final { return &_loiterClockwiseFact; }
    const Fact*     _landingAltitude        (void) const final { return &_landingAltitudeFact; }
    const Fact*     _landingDistance        (void) const final { return &_landingDistanceFact; }
    const Fact*     _landingHeading         (void) const final { return &_landingHeadingFact; }
    const Fact*     _useLoiterToAlt         (void) const final { return &_useLoiterToAltFact; }
    const Fact*     _stopTakingPhotos       (void) const final { return &_stopTakingPhotosFact; }
    const Fact*     _stopTakingVideo        (void) const final { return &_stopTakingVideoFact; }
    void            _calcGlideSlope         (void) final { };
    MissionItem*    _createLandItem         (int seqNum, bool altRel, double lat, double lon, double alt, QObject* parent) final;

    QMap<QString, FactMetaData*> _metaDataMap;

    Fact            _landingDistanceFact;
    Fact            _finalApproachAltitudeFact;
    Fact            _loiterRadiusFact;
    Fact            _loiterClockwiseFact;
    Fact            _landingHeadingFact;
    Fact            _landingAltitudeFact;
    Fact            _useLoiterToAltFact;
    Fact            _stopTakingPhotosFact;
    Fact            _stopTakingVideoFact;

    friend class LandingComplexItemTest;
};
