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

#include "ComplexMissionItem.h"

Q_DECLARE_LOGGING_CATEGORY(LandingComplexItemLog)

class PlanMasterController;
class LandingComplexItemTest;
class Fact;
class MissionItem;

// Base class for landing patterns complex items.
class LandingComplexItem : public ComplexMissionItem
{
    Q_OBJECT
    Q_MOC_INCLUDE("Fact.h")
    Q_MOC_INCLUDE("MissionItem.h")

public:
    LandingComplexItem(PlanMasterController* masterController, bool flyView);

    Q_PROPERTY(Fact*            finalApproachAltitude   READ    finalApproachAltitude                                           CONSTANT)
    Q_PROPERTY(Fact*            useDoChangeSpeed        READ    useDoChangeSpeed                                                CONSTANT)
    Q_PROPERTY(Fact*            finalApproachSpeed      READ    finalApproachSpeed                                              CONSTANT)
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
    const Fact* useDoChangeSpeed        (void) const { return _useDoChangeSpeed(); }
    const Fact* finalApproachSpeed      (void) const { return _finalApproachSpeed(); }
    const Fact* loiterRadius            (void) const { return _loiterRadius(); }
    const Fact* loiterClockwise         (void) const { return _loiterClockwise(); }
    const Fact* landingAltitude         (void) const { return _landingAltitude(); }
    const Fact* landingDistance         (void) const { return _landingDistance(); }
    const Fact* landingHeading          (void) const { return _landingHeading(); }
    const Fact* useLoiterToAlt          (void) const { return _useLoiterToAlt(); }
    const Fact* stopTakingPhotos        (void) const { return _stopTakingPhotos(); }
    const Fact* stopTakingVideo         (void) const { return _stopTakingVideo(); }

    Fact* finalApproachAltitude (void) { return const_cast<Fact*>(const_cast<const LandingComplexItem*>(this)->_finalApproachAltitude()); };
    Fact* useDoChangeSpeed      (void) { return const_cast<Fact*>(const_cast<const LandingComplexItem*>(this)->_useDoChangeSpeed()); };
    Fact* finalApproachSpeed    (void) { return const_cast<Fact*>(const_cast<const LandingComplexItem*>(this)->_finalApproachSpeed()); };
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
    bool                isLandCommand               (void) const final { return true; }
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

    static constexpr const char* finalApproachToLandDistanceName    = "LandingDistance";
    static constexpr const char* landingHeadingName                 = "LandingHeading";
    static constexpr const char* finalApproachAltitudeName          = "FinalApproachAltitude";
    static constexpr const char* useDoChangeSpeedName               = "UseDoChangeSpeed";
    static constexpr const char* finalApproachSpeedName             = "FinalApproachSpeed";
    static constexpr const char* loiterRadiusName                   = "LoiterRadius";
    static constexpr const char* loiterClockwiseName                = "LoiterClockwise";
    static constexpr const char* landingAltitudeName                = "LandingAltitude";
    static constexpr const char* useLoiterToAltName                 = "UseLoiterToAlt";
    static constexpr const char* stopTakingPhotosName               = "StopTakingPhotos";
    static constexpr const char* stopTakingVideoName                = "StopTakingVideo";

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
    virtual const Fact*     _useDoChangeSpeed       (void) const = 0;
    virtual const Fact*     _finalApproachSpeed     (void) const = 0;
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
    MissionItem*    _createDoChangeSpeedItem(int speedType, int speedValue, int throttlePercentage, int seqNum, QObject* parent);
    MissionItem*    _createFinalApproachItem(int seqNum, QObject* parent);
    QJsonObject     _save                   (void);
    bool            _load                   (const QJsonObject& complexObject, int sequenceNumber, const QString& jsonComplexItemTypeValue, bool useDeprecatedRelAltKeys, QString& errorString);

    typedef bool                (*IsLandItemFunc)(const MissionItem& missionItem);
    typedef LandingComplexItem* (*CreateItemFunc)(PlanMasterController* masterController, bool flyView);

    static bool _scanForItems(QmlObjectListModel* visualItems, bool flyView, PlanMasterController* masterController, IsLandItemFunc isLandItemFunc, CreateItemFunc createItemFunc);
    static bool _scanForItem(QmlObjectListModel* visualItems, int& startIndex, bool flyView, PlanMasterController* masterController, IsLandItemFunc isLandItemFunc, CreateItemFunc createItemFunc);

    int             _sequenceNumber             = 0;
    bool            _dirty                      = false;
    QGeoCoordinate  _finalApproachCoordinate;
    QGeoCoordinate  _loiterTangentCoordinate;
    QGeoCoordinate  _landingCoordinate;
    bool            _landingCoordSet            = false;
    bool            _ignoreRecalcSignals        = false;
    bool            _altitudesAreRelative       = true;

    // Support for separate relative alt settings for land/loiter was removed. It now only has a single
    // relative alt setting stored in _jsonAltitudesAreRelativeKey.
    static constexpr const char* _jsonDeprecatedLandingAltitudeRelativeKey   = "landAltitudeRelative";
    static constexpr const char* _jsonDeprecatedLoiterAltitudeRelativeKey    = "loiterAltitudeRelative";

    // Name changed from _jsonDeprecatedLoiterCoordinateKey to _jsonFinalApproachCoordinateKey to reflect
    // the new support for using either a loiter or just a waypoint as the approach entry point.
    static constexpr const char* _jsonDeprecatedLoiterCoordinateKey          = "loiterCoordinate";

    static constexpr const char* _jsonFinalApproachCoordinateKey    = "landingApproachCoordinate";
    static constexpr const char* _jsonUseDoChangeSpeedKey           = "useDoChangeSpeed";
    static constexpr const char* _jsonFinalApproachSpeedKey         = "finalApproachSpeed";
    static constexpr const char* _jsonLoiterRadiusKey               = "loiterRadius";
    static constexpr const char* _jsonLoiterClockwiseKey            = "loiterClockwise";
    static constexpr const char* _jsonLandingCoordinateKey          = "landCoordinate";
    static constexpr const char* _jsonAltitudesAreRelativeKey       = "altitudesAreRelative";
    static constexpr const char* _jsonUseLoiterToAltKey             = "useLoiterToAlt";
    static constexpr const char* _jsonStopTakingPhotosKey           = "stopTakingPhotos";
    static constexpr const char* _jsonStopTakingVideoKey            = "stopVideoPhotos";

private slots:
    void    _recalcFromRadiusChange                         (void);
    void    _signalLastSequenceNumberChanged                (void);
    void    _updateFinalApproachCoodinateAltitudeFromFact   (void);
    void    _updateLandingCoodinateAltitudeFromFact     (void);

    friend class LandingComplexItemTest;
};
