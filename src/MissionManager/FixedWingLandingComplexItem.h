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
    FixedWingLandingComplexItem(Vehicle* vehicle, QGeoCoordinate mapClickCoordinate, QObject* parent = NULL);

    Q_PROPERTY(QVariantList     textFieldFacts      MEMBER  _textFieldFacts     CONSTANT)
    Q_PROPERTY(Fact*            loiterClockwise     READ    loiterClockwise     CONSTANT)
    Q_PROPERTY(QGeoCoordinate   loiterCoordinate    MEMBER  _loiterCoordinate   NOTIFY loiterCoordinateChanged)

    Fact* loiterClockwise(void) { return &_loiterClockwiseFact; }

    // Overrides from ComplexMissionItem

    double              complexDistance     (void) const final;
    int                 lastSequenceNumber  (void) const final;
    QmlObjectListModel* getMissionItems     (void) const final;
    bool                load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    double              greatestDistanceTo  (const QGeoCoordinate &other) const final;
    void                setCruiseSpeed      (double cruiseSpeed) final;
    QString             mapVisualQML        (void) const final { return QStringLiteral("FWLandingPatternMapVisual.qml"); }

    // Overrides from VisualMissionItem

    bool            dirty                   (void) const final { return _dirty; }
    bool            isSimpleItem            (void) const final { return false; }
    bool            isStandaloneCoordinate  (void) const final { return false; }
    bool            specifiesCoordinate     (void) const final;
    QString         commandDescription      (void) const final { return "Landing Pattern"; }
    QString         commandName             (void) const final { return "Landing Pattern"; }
    QString         abbreviation            (void) const final { return "L"; }
    QGeoCoordinate  coordinate              (void) const final { return _coordinate; }
    QGeoCoordinate  exitCoordinate          (void) const final { return _exitCoordinate; }
    int             sequenceNumber          (void) const final { return _sequenceNumber; }
    double          flightSpeed             (void) final { return std::numeric_limits<double>::quiet_NaN(); }

    bool coordinateHasRelativeAltitude      (void) const final { return true; }
    bool exitCoordinateHasRelativeAltitude  (void) const final { return true; }
    bool exitCoordinateSameAsEntry          (void) const final { return true; }

    void setDirty           (bool dirty) final;
    void setCoordinate      (const QGeoCoordinate& coordinate) final;
    void setSequenceNumber  (int sequenceNumber) final;
    void save               (QJsonObject& saveObject) const final;

    static const char* jsonComplexItemTypeValue;

signals:
    void loiterCoordinateChanged(QGeoCoordinate coordinate);

private slots:
    void _recalcLoiterPosition(void);

private:
    void _setExitCoordinate(const QGeoCoordinate& coordinate);
    QPointF _rotatePoint(const QPointF& point, const QPointF& origin, double angle);

    int             _sequenceNumber;
    bool            _dirty;
    QGeoCoordinate  _coordinate;
    QGeoCoordinate  _exitCoordinate;
    QGeoCoordinate  _loiterCoordinate;

    Fact            _loiterToLandDistanceFact;
    Fact            _loiterAltitudeFact;
    Fact            _loiterRadiusFact;
    Fact            _loiterClockwiseFact;
    Fact            _landingHeadingFact;

    static QMap<QString, FactMetaData*> _metaDataMap;

    QVariantList _textFieldFacts;

    static const char* _loiterToLandDistanceName;
    static const char* _loiterAltitudeName;
    static const char* _loiterRadiusName;
    static const char* _loiterClockwiseName;
    static const char* _landingHeadingName;
};

#endif
