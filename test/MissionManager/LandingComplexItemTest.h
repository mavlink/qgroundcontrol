#pragma once

#include "VisualMissionItemTest.h"
#include "LandingComplexItem.h"
#include "Fact.h"

class MultiSignalSpy;
class PlanMasterController;

class LandingComplexItemTest : public VisualMissionItemTest
{
    Q_OBJECT

public:
    LandingComplexItemTest() = default;

    void cleanup() override;
    void init() override;

private slots:
    void _testDirty();
    void _testItemCount();
    void _testAppendSectionItems();
    void _testScanForItems();
    void _testSaveLoad();

private:
    void _validateItem(LandingComplexItem* actualItem, LandingComplexItem* expectedItem);

    LandingComplexItem* _item = nullptr;
    MultiSignalSpy* _multiSpy = nullptr;
    MultiSignalSpy* _viMultiSpy = nullptr;
    SimpleMissionItem* _validStopVideoItem = nullptr;
    SimpleMissionItem* _validStopDistanceItem = nullptr;
    SimpleMissionItem* _validStopTimeItem = nullptr;
};

// Simple class used to test LandingComplexItem base class
class SimpleLandingComplexItem : public LandingComplexItem
{
    Q_OBJECT

public:
    SimpleLandingComplexItem(PlanMasterController* masterController, bool flyView);

    // Overrides from ComplexMissionItem
    QString patternName() const final { return QString(); }
    bool load(const QJsonObject& /*complexObject*/, int /*sequenceNumber*/, QString& /*errorString*/) final { return false; }
    QString mapVisualQML() const final { return QStringLiteral("FWLandingPatternMapVisual.qml"); }

    // Overrides from VisualMissionItem
    void save(QJsonArray& /*missionItems*/) override { }

    static const QString name;

    static constexpr const char* jsonComplexItemTypeValue = "utSimpleLandingPattern";

    static constexpr const char* settingsGroup = "SimpleLandingComplexItemUnitTest";

private slots:
    void _updateFlightPathSegmentsDontCallDirectly() override;

private:
    static LandingComplexItem* _createItem(PlanMasterController* masterController, bool flyView) { return new SimpleLandingComplexItem(masterController, flyView); }
    static bool _isValidLandItem(const MissionItem& missionItem);

    // Overrides from LandingComplexItem
    const Fact* _finalApproachAltitude() const final { return &_finalApproachAltitudeFact; }
    const Fact* _useDoChangeSpeed() const final { return &_useDoChangeSpeedFact; }
    const Fact* _finalApproachSpeed() const final { return &_finalApproachSpeedFact; }
    const Fact* _loiterRadius() const final { return &_loiterRadiusFact; }
    const Fact* _loiterClockwise() const final { return &_loiterClockwiseFact; }
    const Fact* _landingAltitude() const final { return &_landingAltitudeFact; }
    const Fact* _landingDistance() const final { return &_landingDistanceFact; }
    const Fact* _landingHeading() const final { return &_landingHeadingFact; }
    const Fact* _useLoiterToAlt() const final { return &_useLoiterToAltFact; }
    const Fact* _stopTakingPhotos() const final { return &_stopTakingPhotosFact; }
    const Fact* _stopTakingVideo() const final { return &_stopTakingVideoFact; }
    void _calcGlideSlope() final { }
    MissionItem* _createLandItem(int seqNum, bool altRel, double lat, double lon, double alt, QObject* parent) final;

    QMap<QString, FactMetaData*> _metaDataMap;

    Fact _landingDistanceFact;
    Fact _finalApproachAltitudeFact;
    Fact _useDoChangeSpeedFact;
    Fact _finalApproachSpeedFact;
    Fact _loiterRadiusFact;
    Fact _loiterClockwiseFact;
    Fact _landingHeadingFact;
    Fact _landingAltitudeFact;
    Fact _useLoiterToAltFact;
    Fact _stopTakingPhotosFact;
    Fact _stopTakingVideoFact;

    friend class LandingComplexItemTest;
};
