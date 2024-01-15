/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "ComplexMissionItem.h"
#include "MissionItem.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(LandingComplexItemLog)

class PlanMasterController;
class LandingComplexItemTest;

// Base class for landing patterns complex items.
class LandingComplexItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    LandingComplexItem(PlanMasterController* masterController, bool flyView);

    Q_PROPERTY(Fact*            finalApproachAltitude   READ    finalApproachAltitude                                           CONSTANT)
    Q_PROPERTY(Fact*            loiterRadius            READ    loiterRadius                                                    CONSTANT)
    Q_PROPERTY(Fact*            landingAltitude         READ    landingAltitude                                                 CONSTANT)
    Q_PROPERTY(Fact*            landingHeading          READ    landingHeading                                                  CONSTANT)
    Q_PROPERTY(Fact*            landingDistance         READ    landingDistance                                                 CONSTANT)
    Q_PROPERTY(Fact*            loiterClockwise         READ    loiterClockwise                                                 CONSTANT)
    Q_PROPERTY(Fact*            useLoiterToAlt          READ    useLoiterToAlt                                                  CONSTANT)
    Q_PROPERTY(Fact*            stopTakingPhotos        READ    stopTakingPhotos                                                CONSTANT)
    Q_PROPERTY(Fact*            stopTakingVideo         READ    stopTakingVideo                                                 CONSTANT)
    Q_PROPERTY(QGeoCoordinate   finalApproachCoordinate READ    finalApproachCoordinate     WRITE setFinalApproachCoordinate    NOTIFY finalApproachCoordinateChanged)
    Q_PROPERTY(QGeoCoordinate   loiterTangentCoordinate READ    loiterTangentCoordinate                                         NOTIFY loiterTangentCoordinateChanged)
    Q_PROPERTY(QGeoCoordinate   landingCoordinate       READ    landingCoordinate           WRITE setLandingCoordinate          NOTIFY landingCoordinateChanged)
    Q_PROPERTY(bool             altitudesAreRelative    READ    altitudesAreRelative        WRITE setAltitudesAreRelative       NOTIFY altitudesAreRelativeChanged)
    Q_PROPERTY(bool             landingCoordSet         READ    landingCoordSet                                                 NOTIFY landingCoordSetChanged)

    Q_INVOKABLE void setLandingHeadingToTakeoffHeading();

    const Fact* finalApproachAltitude   (void) const { return _finalApproachAltitude(); }
    const Fact* loiterRadius            (void) const { return _loiterRadius(); }
    const Fact* loiterClockwise         (void) const { return _loiterClockwise(); }
    const Fact* landingAltitude         (void) const { return _landingAltitude(); }
    const Fact* landingDistance         (void) const { return _landingDistance(); }
    const Fact* landingHeading          (void) const { return _landingHeading(); }
    const Fact* useLoiterToAlt          (void) const { return _useLoiterToAlt(); }
    const Fact* stopTakingPhotos        (void) const { return _stopTakingPhotos(); }
    const Fact* stopTakingVideo         (void) const { return _stopTakingVideo(); }

    Fact* finalApproachAltitude (void) { return const_cast<Fact*>(const_cast<const LandingComplexItem*>(this)->_finalApproachAltitude()); };
    Fact* loiterRadius          (void) { return const_cast<Fact*>(const_cast<const LandingComplexItem*>(this)->_loiterRadius()); };
    Fact* loiterClockwise       (void) { return const_cast<Fact*>(const_cast<const LandingComplexItem*>(this)->_loiterClockwise()); };
    Fact* landingAltitude       (void) { return const_cast<Fact*>(const_cast<const LandingComplexItem*>(this)->_landingAltitude()); };
    Fact* landingDistance       (void) { return const_cast<Fact*>(const_cast<const LandingComplexItem*>(this)->_landingDistance()); };
    Fact* landingHeading        (void) { return const_cast<Fact*>(const_cast<const LandingComplexItem*>(this)->_landingHeading()); };
    Fact* useLoiterToAlt        (void) { return const_cast<Fact*>(const_cast<const LandingComplexItem*>(this)->_useLoiterToAlt()); };
    Fact* stopTakingPhotos      (void) { return const_cast<Fact*>(const_cast<const LandingComplexItem*>(this)->_stopTakingPhotos()); };
    Fact* stopTakingVideo       (void) { return const_cast<Fact*>(const_cast<const LandingComplexItem*>(this)->_stopTakingVideo()); };

    bool            altitudesAreRelative    (void) const { return _altitudesAreRelative; }
    bool            landingCoordSet         (void) const { return _landingCoordSet; }
    QGeoCoordinate  landingCoordinate       (void) const { return _landingCoordinate; }
    QGeoCoordinate  finalApproachCoordinate (void) const { return _finalApproachCoordinate; }
    QGeoCoordinate  loiterTangentCoordinate (void) const { return _loiterTangentCoordinate; }

    void setLandingCoordinate       (const QGeoCoordinate& coordinate);
    void setFinalApproachCoordinate (const QGeoCoordinate& coordinate);
    void setAltitudesAreRelative    (bool altitudesAreRelative);

    // Overrides from ComplexMissionItem
    double  complexDistance     (void) const final;
    double  greatestDistanceTo  (const QGeoCoordinate &other) const final;
    int     lastSequenceNumber  (void) const final;

    // Overrides from VisualMissionItem
    bool                dirty                       (void) const final { return _dirty; }
    bool                isSimpleItem                (void) const final { return false; }
    bool                isStandaloneCoordinate      (void) const final { return false; }
    bool                specifiesCoordinate         (void) const final { return true; }
    bool                specifiesAltitudeOnly       (void) const final { return false; }
    QString             commandDescription          (void) const final { return "Landing Pattern"; }
    QString             commandName                 (void) const final { return "Landing Pattern"; }
    QString             abbreviation                (void) const final { return "L"; }
    QGeoCoordinate      coordinate                  (void) const final { return _finalApproachCoordinate; }
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
    void                setCoordinate               (const QGeoCoordinate& coordinate) final { setFinalApproachCoordinate(coordinate); }
    void                setSequenceNumber           (int sequenceNumber) final;
    double              amslEntryAlt                (void) const final;
    double              amslExitAlt                 (void) const final;
    double              minAMSLAltitude             (void) const final { return amslExitAlt(); }
    double              maxAMSLAltitude             (void) const final { return amslEntryAlt(); }

    static const char* finalApproachToLandDistanceName;
    static const char* finalApproachAltitudeName;
    static const char* loiterRadiusName;
    static const char* loiterClockwiseName;
    static const char* landingHeadingName;
    static const char* landingAltitudeName;
    static const char* useLoiterToAltName;
    static const char* stopTakingPhotosName;
    static const char* stopTakingVideoName;

signals:
    void finalApproachCoordinateChanged (QGeoCoordinate coordinate);
    void loiterTangentCoordinateChanged (QGeoCoordinate coordinate);
    void landingCoordinateChanged       (QGeoCoordinate coordinate);
    void landingCoordSetChanged         (bool landingCoordSet);
    void altitudesAreRelativeChanged    (bool altitudesAreRelative);
    void _updateFlightPathSegmentsSignal(void);

protected slots:
    virtual void _updateFlightPathSegmentsDontCallDirectly(void) = 0;

    void _recalcFromHeadingAndDistanceChange        (void);
    void _recalcFromCoordinateChange                (void);
    void _setDirty                                  (void);

protected:
    virtual const Fact*     _finalApproachAltitude  (void) const = 0;
    virtual const Fact*     _loiterRadius           (void) const = 0;
    virtual const Fact*     _loiterClockwise        (void) const = 0;
    virtual const Fact*     _landingAltitude        (void) const = 0;
    virtual const Fact*     _landingDistance        (void) const = 0;
    virtual const Fact*     _landingHeading         (void) const = 0;
    virtual const Fact*     _useLoiterToAlt         (void) const = 0;
    virtual const Fact*     _stopTakingPhotos       (void) const = 0;
    virtual const Fact*     _stopTakingVideo        (void) const = 0;
    virtual void            _calcGlideSlope         (void) = 0;
    virtual MissionItem*    _createLandItem         (int seqNum, bool altRel, double lat, double lon, double alt, QObject* parent) = 0;

    void            _init                   (void);
    QPointF         _rotatePoint            (const QPointF& point, const QPointF& origin, double angle);
    MissionItem*    _createDoLandStartItem  (int seqNum, QObject* parent);
    MissionItem*    _createFinalApproachItem(int seqNum, QObject* parent);
    QJsonObject     _save                   (void);
    bool            _load                   (const QJsonObject& complexObject, int sequenceNumber, const QString& jsonComplexItemTypeValue, bool useDeprecatedRelAltKeys, QString& errorString);

    typedef bool                (*IsLandItemFunc)(const MissionItem& missionItem);
    typedef LandingComplexItem* (*CreateItemFunc)(PlanMasterController* masterController, bool flyView);

    static bool _scanForItem(QmlObjectListModel* visualItems, bool flyView, PlanMasterController* masterController, IsLandItemFunc isLandItemFunc, CreateItemFunc createItemFunc);

    int             _sequenceNumber             = 0;
    bool            _dirty                      = false;
    QGeoCoordinate  _finalApproachCoordinate;
    QGeoCoordinate  _loiterTangentCoordinate;
    QGeoCoordinate  _landingCoordinate;
    bool            _landingCoordSet            = false;
    bool            _ignoreRecalcSignals        = false;
    bool            _altitudesAreRelative       = true;

    static const char* _jsonFinalApproachCoordinateKey;
    static const char* _jsonLoiterRadiusKey;
    static const char* _jsonLoiterClockwiseKey;
    static const char* _jsonLandingCoordinateKey;
    static const char* _jsonAltitudesAreRelativeKey;
    static const char* _jsonUseLoiterToAltKey;
    static const char* _jsonStopTakingPhotosKey;
    static const char* _jsonStopTakingVideoKey;

    // Only in older file formats
    static const char* _jsonDeprecatedLandingAltitudeRelativeKey;
    static const char* _jsonDeprecatedLoiterAltitudeRelativeKey;
    static const char* _jsonDeprecatedLoiterCoordinateKey;

private slots:
    void    _recalcFromRadiusChange                         (void);
    void    _signalLastSequenceNumberChanged                (void);
    void    _updateFinalApproachCoodinateAltitudeFromFact   (void);
    void    _updateLandingCoodinateAltitudeFromFact     (void);

    friend class LandingComplexItemTest;
};
