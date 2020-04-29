/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef FixedWingLandingComplexItem_H
#define FixedWingLandingComplexItem_H

#include "LandingComplexItem.h"
#include "MissionItem.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(FixedWingLandingComplexItemLog)

class FWLandingPatternTest;
class PlanMasterController;

class FixedWingLandingComplexItem : public LandingComplexItem
{
    Q_OBJECT

public:
    FixedWingLandingComplexItem(PlanMasterController* masterController, bool flyView, QObject* parent);

    Q_PROPERTY(Fact*            valueSetIsDistance      READ    valueSetIsDistance                                          CONSTANT)
    Q_PROPERTY(Fact*            glideSlope              READ    glideSlope                                                  CONSTANT)

    Q_INVOKABLE void moveLandingPosition(const QGeoCoordinate& coordinate); // Maintains the current landing distance and heading

    Fact*           loiterAltitude          (void) final { return &_loiterAltitudeFact; }
    Fact*           loiterRadius            (void) final { return &_loiterRadiusFact; }
    Fact*           landingAltitude         (void) final { return &_landingAltitudeFact; }
    Fact*           landingDistance         (void) final { return &_landingDistanceFact; }
    Fact*           landingHeading          (void) final { return &_landingHeadingFact; }
    Fact*           stopTakingPhotos        (void) final { return &_stopTakingPhotosFact; }
    Fact*           stopTakingVideo         (void) final { return &_stopTakingVideoFact; }
    Fact*           glideSlope              (void) { return &_glideSlopeFact; }
    Fact*           valueSetIsDistance      (void) { return &_valueSetIsDistanceFact; }

    /// Scans the loaded items for a landing pattern complex item
    static bool scanForItem(QmlObjectListModel* visualItems, bool flyView, PlanMasterController* masterController);

    static MissionItem* createDoLandStartItem   (int seqNum, QObject* parent);
    static MissionItem* createLoiterToAltItem   (int seqNum, bool altRel, double loiterRaidus, double lat, double lon, double alt, QObject* parent);
    static MissionItem* createLandItem          (int seqNum, bool altRel, double lat, double lon, double alt, QObject* parent);

    // Overrides from ComplexMissionItem
    QString patternName         (void) const final { return name; }
    int     lastSequenceNumber  (void) const final;
    bool    load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    double  greatestDistanceTo  (const QGeoCoordinate &other) const final;
    QString mapVisualQML        (void) const final { return QStringLiteral("FWLandingPatternMapVisual.qml"); }

    // Overrides from VisualMissionItem
    bool                dirty                       (void) const final { return _dirty; }
    bool                isSimpleItem                (void) const final { return false; }
    bool                isStandaloneCoordinate      (void) const final { return false; }
    bool                specifiesCoordinate         (void) const final;
    bool                specifiesAltitudeOnly       (void) const final { return false; }
    QString             commandDescription          (void) const final { return "Landing Pattern"; }
    QString             commandName                 (void) const final { return "Landing Pattern"; }
    QString             abbreviation                (void) const final { return "L"; }
    QGeoCoordinate      coordinate                  (void) const final { return _loiterCoordinate; }
    QGeoCoordinate      exitCoordinate              (void) const final { return _landingCoordinate; }
    int                 sequenceNumber              (void) const final { return _sequenceNumber; }
    double              specifiedFlightSpeed        (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    double              specifiedGimbalYaw          (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    double              specifiedGimbalPitch        (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    void                appendMissionItems          (QList<MissionItem*>& items, QObject* missionItemParent) final;
    void                applyNewAltitude            (double newAltitude) final;
    double              additionalTimeDelay         (void) const final { return 0; }
    ReadyForSaveState   readyForSaveState           (void) const final;
    bool                exitCoordinateSameAsEntry   (void) const final { return false; }
    void                setDirty                    (bool dirty) final;
    void                setCoordinate               (const QGeoCoordinate& coordinate) final { setLoiterCoordinate(coordinate); }
    void                setSequenceNumber           (int sequenceNumber) final;
    void                save                        (QJsonArray&  missionItems) final;
    double              amslEntryAlt                (void) const final;
    double              amslExitAlt                 (void) const final;
    double              minAMSLAltitude             (void) const final { return amslExitAlt(); }
    double              maxAMSLAltitude             (void) const final { return amslEntryAlt(); }

    static const QString name;

    static const char* jsonComplexItemTypeValue;

    static const char* settingsGroup;
    static const char* loiterToLandDistanceName;
    static const char* loiterAltitudeName;
    static const char* loiterRadiusName;
    static const char* landingHeadingName;
    static const char* landingAltitudeName;
    static const char* glideSlopeName;
    static const char* stopTakingPhotosName;
    static const char* stopTakingVideoName;
    static const char* valueSetIsDistanceName;

signals:
    void valueSetIsDistanceChanged      (bool valueSetIsDistance);

private slots:
    void    _recalcFromHeadingAndDistanceChange         (void);
    void    _recalcFromCoordinateChange                 (void);
    void    _recalcFromRadiusChange                     (void);
    void    _updateLoiterCoodinateAltitudeFromFact      (void);
    void    _updateLandingCoodinateAltitudeFromFact     (void);
    double  _mathematicAngleToHeading                   (double angle);
    double  _headingToMathematicAngle                   (double heading);
    void    _setDirty                                   (void);
    void    _glideSlopeChanged                          (void);
    void    _signalLastSequenceNumberChanged            (void);

private:
    void    _calcGlideSlope                 (void);

    QMap<QString, FactMetaData*> _metaDataMap;

    Fact            _landingDistanceFact;
    Fact            _loiterAltitudeFact;
    Fact            _loiterRadiusFact;
    Fact            _landingHeadingFact;
    Fact            _landingAltitudeFact;
    Fact            _glideSlopeFact;
    Fact            _stopTakingPhotosFact;
    Fact            _stopTakingVideoFact;
    Fact            _valueSetIsDistanceFact;

    static const char* _jsonLoiterCoordinateKey;
    static const char* _jsonLoiterRadiusKey;
    static const char* _jsonLoiterClockwiseKey;
    static const char* _jsonLandingCoordinateKey;
    static const char* _jsonValueSetIsDistanceKey;
    static const char* _jsonAltitudesAreRelativeKey;
    static const char* _jsonStopTakingPhotosKey;
    static const char* _jsonStopTakingVideoKey;

    // Only in older file formats
    static const char* _jsonLandingAltitudeRelativeKey;
    static const char* _jsonLoiterAltitudeRelativeKey;
    static const char* _jsonFallRateKey;

    friend FWLandingPatternTest;
};

#endif
