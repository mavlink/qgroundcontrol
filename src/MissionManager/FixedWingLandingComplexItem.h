/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

class FixedWingLandingComplexItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    FixedWingLandingComplexItem(Vehicle* vehicle, QObject* parent = NULL);

    Q_PROPERTY(Fact*            loiterAltitude          READ    loiterAltitude                                                  CONSTANT)
    Q_PROPERTY(Fact*            loiterRadius            READ    loiterRadius                                                    CONSTANT)
    Q_PROPERTY(Fact*            landingAltitude         READ    landingAltitude                                                 CONSTANT)
    Q_PROPERTY(Fact*            landingDistance         READ    landingDistance                                                 CONSTANT)
    Q_PROPERTY(Fact*            landingHeading          READ    landingHeading                                                  CONSTANT)
    Q_PROPERTY(Fact*            fallRate                READ    fallRate                                                    CONSTANT)
    Q_PROPERTY(bool             loiterClockwise         MEMBER  _loiterClockwise                                                NOTIFY loiterClockwiseChanged)
    Q_PROPERTY(bool             loiterAltitudeRelative  MEMBER  _loiterAltitudeRelative                                         NOTIFY loiterAltitudeRelativeChanged)
    Q_PROPERTY(bool             landingAltitudeRelative MEMBER  _landingAltitudeRelative                                        NOTIFY landingAltitudeRelativeChanged)
    Q_PROPERTY(QGeoCoordinate   loiterCoordinate        READ    loiterCoordinate            WRITE setLoiterCoordinate           NOTIFY loiterCoordinateChanged)
    Q_PROPERTY(QGeoCoordinate   loiterTangentCoordinate READ    loiterTangentCoordinate                                         NOTIFY loiterTangentCoordinateChanged)
    Q_PROPERTY(QGeoCoordinate   landingCoordinate       READ    landingCoordinate           WRITE setLandingCoordinate          NOTIFY landingCoordinateChanged)
    Q_PROPERTY(bool             landingCoordSet         MEMBER _landingCoordSet                                                 NOTIFY landingCoordSetChanged)

    Fact*           loiterAltitude          (void) { return &_loiterAltitudeFact; }
    Fact*           loiterRadius            (void) { return &_loiterRadiusFact; }
    Fact*           landingAltitude         (void) { return &_landingAltitudeFact; }
    Fact*           landingDistance         (void) { return &_landingDistanceFact; }
    Fact*           landingHeading          (void) { return &_landingHeadingFact; }
    Fact*           fallRate                (void) { return &_fallRateFact; }
    QGeoCoordinate  landingCoordinate       (void) const { return _landingCoordinate; }
    QGeoCoordinate  loiterCoordinate        (void) const { return _loiterCoordinate; }
    QGeoCoordinate  loiterTangentCoordinate (void) const { return _loiterTangentCoordinate; }

    void setLandingCoordinate       (const QGeoCoordinate& coordinate);
    void setLoiterCoordinate        (const QGeoCoordinate& coordinate);

    /// Scans the loaded items for a landing pattern complex item
    static bool scanForItem(QmlObjectListModel* visualItems, Vehicle* vehicle);

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
    void            appendMissionItems      (QList<MissionItem*>& items, QObject* missionItemParent) final;
    void            applyNewAltitude        (double newAltitude) final;

    bool coordinateHasRelativeAltitude      (void) const final { return _loiterAltitudeRelative; }
    bool exitCoordinateHasRelativeAltitude  (void) const final { return _landingAltitudeRelative; }
    bool exitCoordinateSameAsEntry          (void) const final { return false; }

    void setDirty           (bool dirty) final;
    void setCoordinate      (const QGeoCoordinate& coordinate) final { setLoiterCoordinate(coordinate); }
    void setSequenceNumber  (int sequenceNumber) final;
    void save               (QJsonArray&  missionItems) final;

    static const char* jsonComplexItemTypeValue;

signals:
    void loiterCoordinateChanged        (QGeoCoordinate coordinate);
    void loiterTangentCoordinateChanged (QGeoCoordinate coordinate);
    void landingCoordinateChanged       (QGeoCoordinate coordinate);
    void landingCoordSetChanged         (bool landingCoordSet);
    void loiterClockwiseChanged         (bool loiterClockwise);
    void loiterAltitudeRelativeChanged  (bool loiterAltitudeRelative);
    void landingAltitudeRelativeChanged (bool loiterAltitudeRelative);

private slots:
    void _recalcFromHeadingAndDistanceChange(void);
    void _recalcFromCoordinateChange(void);
    void _recalcFromRadiusChange(void);
    void _updateLoiterCoodinateAltitudeFromFact(void);
    void _updateLandingCoodinateAltitudeFromFact(void);
    double _mathematicAngleToHeading(double angle);
    double _headingToMathematicAngle(double heading);
    void _setDirty(void);

private:
    QPointF _rotatePoint(const QPointF& point, const QPointF& origin, double angle);

    int             _sequenceNumber;
    bool            _dirty;
    QGeoCoordinate  _loiterCoordinate;
    QGeoCoordinate  _loiterTangentCoordinate;
    QGeoCoordinate  _landingCoordinate;
    bool            _landingCoordSet;
    bool            _ignoreRecalcSignals;

    Fact            _landingDistanceFact;
    Fact            _loiterAltitudeFact;
    Fact            _loiterRadiusFact;
    Fact            _landingHeadingFact;
    Fact            _landingAltitudeFact;
    Fact            _fallRateFact;

    bool            _loiterClockwise;
    bool            _loiterAltitudeRelative;
    bool            _landingAltitudeRelative;

    static QMap<QString, FactMetaData*> _metaDataMap;

    static const char* _loiterToLandDistanceName;
    static const char* _loiterAltitudeName;
    static const char* _loiterRadiusName;
    static const char* _landingHeadingName;
    static const char* _landingAltitudeName;
    static const char* _fallRateName;

    static const char* _jsonLoiterCoordinateKey;
    static const char* _jsonLoiterRadiusKey;
    static const char* _jsonLoiterClockwiseKey;
    static const char* _jsonLoiterAltitudeRelativeKey;
    static const char* _jsonLandingCoordinateKey;
    static const char* _jsonLandingAltitudeRelativeKey;
    static const char* _jsonFallRateKey;
};

#endif
