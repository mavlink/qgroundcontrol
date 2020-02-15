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

#include "ComplexMissionItem.h"
#include "MissionItem.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(FixedWingLandingComplexItemLog)

class FWLandingPatternTest;

class FixedWingLandingComplexItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    FixedWingLandingComplexItem(Vehicle* vehicle, bool flyView, QObject* parent);

    Q_PROPERTY(Fact*            loiterAltitude          READ    loiterAltitude                                              CONSTANT)
    Q_PROPERTY(Fact*            loiterRadius            READ    loiterRadius                                                CONSTANT)
    Q_PROPERTY(Fact*            landingAltitude         READ    landingAltitude                                             CONSTANT)
    Q_PROPERTY(Fact*            landingHeading          READ    landingHeading                                              CONSTANT)
    Q_PROPERTY(Fact*            valueSetIsDistance      READ    valueSetIsDistance                                          CONSTANT)
    Q_PROPERTY(Fact*            landingDistance         READ    landingDistance                                             CONSTANT)
    Q_PROPERTY(Fact*            glideSlope              READ    glideSlope                                                  CONSTANT)
    Q_PROPERTY(bool             loiterClockwise         MEMBER  _loiterClockwise                                            NOTIFY loiterClockwiseChanged)
    Q_PROPERTY(bool             altitudesAreRelative    MEMBER  _altitudesAreRelative                                       NOTIFY altitudesAreRelativeChanged)
    Q_PROPERTY(Fact*            stopTakingPhotos        READ    stopTakingPhotos                                            CONSTANT)
    Q_PROPERTY(Fact*            stopTakingVideo         READ    stopTakingVideo                                             CONSTANT)
    Q_PROPERTY(QGeoCoordinate   loiterCoordinate        READ    loiterCoordinate            WRITE setLoiterCoordinate       NOTIFY loiterCoordinateChanged)
    Q_PROPERTY(QGeoCoordinate   loiterTangentCoordinate READ    loiterTangentCoordinate                                     NOTIFY loiterTangentCoordinateChanged)
    Q_PROPERTY(QGeoCoordinate   landingCoordinate       READ    landingCoordinate           WRITE setLandingCoordinate      NOTIFY landingCoordinateChanged)
    Q_PROPERTY(bool             landingCoordSet         MEMBER _landingCoordSet                                             NOTIFY landingCoordSetChanged)
    Q_PROPERTY(bool             loiterDragAngleOnly     READ    loiterDragAngleOnly         WRITE setLoiterDragAngleOnly    NOTIFY loiterDragAngleOnlyChanged)

    Fact*           loiterAltitude          (void) { return &_loiterAltitudeFact; }
    Fact*           loiterRadius            (void) { return &_loiterRadiusFact; }
    Fact*           landingAltitude         (void) { return &_landingAltitudeFact; }
    Fact*           landingDistance         (void) { return &_landingDistanceFact; }
    Fact*           landingHeading          (void) { return &_landingHeadingFact; }
    Fact*           glideSlope              (void) { return &_glideSlopeFact; }
    Fact*           stopTakingPhotos        (void) { return &_stopTakingPhotosFact; }
    Fact*           stopTakingVideo         (void) { return &_stopTakingVideoFact; }
    Fact*           valueSetIsDistance      (void) { return &_valueSetIsDistanceFact; }
    QGeoCoordinate  landingCoordinate       (void) const { return _landingCoordinate; }
    QGeoCoordinate  loiterCoordinate        (void) const { return _loiterCoordinate; }
    QGeoCoordinate  loiterTangentCoordinate (void) const { return _loiterTangentCoordinate; }
    bool            loiterDragAngleOnly     (void) const { return _loiterDragAngleOnly; }

    void setLandingCoordinate       (const QGeoCoordinate& coordinate);
    void setLoiterCoordinate        (const QGeoCoordinate& coordinate);
    void setLoiterDragAngleOnly     (bool loiterDragAngleOnly);

    /// Scans the loaded items for a landing pattern complex item
    static bool scanForItem(QmlObjectListModel* visualItems, bool flyView, Vehicle* vehicle);

    static MissionItem* createDoLandStartItem   (int seqNum, QObject* parent);
    static MissionItem* createLoiterToAltItem   (int seqNum, bool altRel, double loiterRaidus, double lat, double lon, double alt, QObject* parent);
    static MissionItem* createLandItem          (int seqNum, bool altRel, double lat, double lon, double alt, QObject* parent);

    // Overrides from ComplexMissionItem
    double              complexDistance     (void) const final;
    int                 lastSequenceNumber  (void) const final;
    bool                load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    double              greatestDistanceTo  (const QGeoCoordinate &other) const final;
    QString             mapVisualQML        (void) const final { return QStringLiteral("FWLandingPatternMapVisual.qml"); }

    // Overrides from VisualMissionItem
    bool            dirty                   (void) const final { return _dirty; }
    bool            isSimpleItem            (void) const final { return false; }
    bool            isStandaloneCoordinate  (void) const final { return false; }
    bool            specifiesCoordinate     (void) const final;
    bool            specifiesAltitudeOnly   (void) const final { return false; }
    QString         commandDescription      (void) const final { return "Landing Pattern"; }
    QString         commandName             (void) const final { return "Landing Pattern"; }
    QString         abbreviation            (void) const final { return "L"; }
    QGeoCoordinate  coordinate              (void) const final { return _loiterCoordinate; }
    QGeoCoordinate  exitCoordinate          (void) const final { return _landingCoordinate; }
    int             sequenceNumber          (void) const final { return _sequenceNumber; }
    double          specifiedFlightSpeed    (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    double          specifiedGimbalYaw      (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    double          specifiedGimbalPitch    (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    void            appendMissionItems      (QList<MissionItem*>& items, QObject* missionItemParent) final;
    void            applyNewAltitude        (double newAltitude) final;
    double          additionalTimeDelay     (void) const final { return 0; }
    ReadyForSaveState readyForSaveState     (void) const final;

    bool coordinateHasRelativeAltitude      (void) const final { return _altitudesAreRelative; }
    bool exitCoordinateHasRelativeAltitude  (void) const final { return _altitudesAreRelative; }
    bool exitCoordinateSameAsEntry          (void) const final { return false; }

    void setDirty           (bool dirty) final;
    void setCoordinate      (const QGeoCoordinate& coordinate) final { setLoiterCoordinate(coordinate); }
    void setSequenceNumber  (int sequenceNumber) final;
    void save               (QJsonArray&  missionItems) final;

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
    void loiterCoordinateChanged        (QGeoCoordinate coordinate);
    void loiterTangentCoordinateChanged (QGeoCoordinate coordinate);
    void landingCoordinateChanged       (QGeoCoordinate coordinate);
    void landingCoordSetChanged         (bool landingCoordSet);
    void loiterClockwiseChanged         (bool loiterClockwise);
    void altitudesAreRelativeChanged    (bool altitudesAreRelative);
    void valueSetIsDistanceChanged      (bool valueSetIsDistance);
    void loiterDragAngleOnlyChanged     (bool loiterDragAngleOnly);

private slots:
    void    _recalcFromHeadingAndDistanceChange     (void);
    void    _recalcFromCoordinateChange             (void);
    void    _recalcFromRadiusChange                 (void);
    void    _updateLoiterCoodinateAltitudeFromFact  (void);
    void    _updateLandingCoodinateAltitudeFromFact (void);
    double  _mathematicAngleToHeading               (double angle);
    double  _headingToMathematicAngle               (double heading);
    void    _setDirty                               (void);
    void    _glideSlopeChanged                      (void);
    void    _signalLastSequenceNumberChanged        (void);

private:
    QPointF _rotatePoint    (const QPointF& point, const QPointF& origin, double angle);
    void    _calcGlideSlope (void);

    int             _sequenceNumber;
    bool            _dirty;
    QGeoCoordinate  _loiterCoordinate;
    QGeoCoordinate  _loiterTangentCoordinate;
    QGeoCoordinate  _landingCoordinate;
    bool            _landingCoordSet;
    bool            _ignoreRecalcSignals;
    bool            _loiterDragAngleOnly;

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

    bool            _loiterClockwise;
    bool            _altitudesAreRelative;

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
