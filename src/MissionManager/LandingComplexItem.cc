/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LandingComplexItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "SimpleMissionItem.h"
#include "PlanMasterController.h"
#include "FlightPathSegment.h"
#include "TakeoffMissionItem.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(LandingComplexItemLog, "LandingComplexItemLog")

const char* LandingComplexItem::_jsonFinalApproachCoordinateKey = "landingApproachCoordinate";
const char* LandingComplexItem::_jsonLoiterRadiusKey            = "loiterRadius";
const char* LandingComplexItem::_jsonLoiterClockwiseKey         = "loiterClockwise";
const char* LandingComplexItem::_jsonLandingCoordinateKey       = "landCoordinate";
const char* LandingComplexItem::_jsonAltitudesAreRelativeKey    = "altitudesAreRelative";
const char* LandingComplexItem::_jsonUseLoiterToAltKey          = "useLoiterToAlt";
const char* LandingComplexItem::_jsonStopTakingPhotosKey        = "stopTakingPhotos";
const char* LandingComplexItem::_jsonStopTakingVideoKey         = "stopVideoPhotos";

const char* LandingComplexItem::finalApproachToLandDistanceName = "LandingDistance";
const char* LandingComplexItem::landingHeadingName              = "LandingHeading";
const char* LandingComplexItem::finalApproachAltitudeName       = "FinalApproachAltitude";
const char* LandingComplexItem::loiterRadiusName                = "LoiterRadius";
const char* LandingComplexItem::loiterClockwiseName             = "LoiterClockwise";
const char* LandingComplexItem::landingAltitudeName             = "LandingAltitude";
const char* LandingComplexItem::useLoiterToAltName              = "UseLoiterToAlt";
const char* LandingComplexItem::stopTakingPhotosName            = "StopTakingPhotos";
const char* LandingComplexItem::stopTakingVideoName             = "StopTakingVideo";

// Deprecated keys

// Support for separate relative alt settings for land/loiter was removed. It now only has a single
// relative alt setting stored in _jsonAltitudesAreRelativeKey.
const char* LandingComplexItem::_jsonDeprecatedLandingAltitudeRelativeKey   = "landAltitudeRelative";
const char* LandingComplexItem::_jsonDeprecatedLoiterAltitudeRelativeKey    = "loiterAltitudeRelative";

// Name changed from _jsonDeprecatedLoiterCoordinateKey to _jsonFinalApproachCoordinateKey to reflect
// the new support for using either a loiter or just a waypoint as the approach entry point.
const char* LandingComplexItem::_jsonDeprecatedLoiterCoordinateKey          = "loiterCoordinate";

LandingComplexItem::LandingComplexItem(PlanMasterController* masterController, bool flyView)
    : ComplexMissionItem        (masterController, flyView)
{
    _isIncomplete = false;

    // The following is used to compress multiple recalc calls in a row to into a single call.
    connect(this, &LandingComplexItem::_updateFlightPathSegmentsSignal, this, &LandingComplexItem::_updateFlightPathSegmentsDontCallDirectly,   Qt::QueuedConnection);
    qgcApp()->addCompressedSignal(QMetaMethod::fromSignal(&LandingComplexItem::_updateFlightPathSegmentsSignal));
}

void LandingComplexItem::_init(void)
{
    if (_masterController->controllerVehicle()->apmFirmware()) {
        // ArduPilot does not support camera commands
        stopTakingVideo()->setRawValue(false);
        stopTakingPhotos()->setRawValue(false);
    }

    connect(landingDistance(),          &Fact::valueChanged,                                this, &LandingComplexItem::_recalcFromHeadingAndDistanceChange);
    connect(landingHeading(),           &Fact::valueChanged,                                this, &LandingComplexItem::_recalcFromHeadingAndDistanceChange);

    connect(loiterRadius(),             &Fact::valueChanged,                                this, &LandingComplexItem::_recalcFromRadiusChange);
    connect(loiterClockwise(),          &Fact::rawValueChanged,                             this, &LandingComplexItem::_recalcFromRadiusChange);

    connect(this,                       &LandingComplexItem::finalApproachCoordinateChanged,this, &LandingComplexItem::_recalcFromCoordinateChange);
    connect(this,                       &LandingComplexItem::landingCoordinateChanged,      this, &LandingComplexItem::_recalcFromCoordinateChange);
    connect(useLoiterToAlt(),           &Fact::rawValueChanged,                             this, &LandingComplexItem::_recalcFromCoordinateChange);

    connect(finalApproachAltitude(),    &Fact::valueChanged,                                this, &LandingComplexItem::_setDirty);
    connect(landingAltitude(),          &Fact::valueChanged,                                this, &LandingComplexItem::_setDirty);
    connect(landingDistance(),          &Fact::valueChanged,                                this, &LandingComplexItem::_setDirty);
    connect(landingHeading(),           &Fact::valueChanged,                                this, &LandingComplexItem::_setDirty);
    connect(loiterRadius(),             &Fact::valueChanged,                                this, &LandingComplexItem::_setDirty);
    connect(loiterClockwise(),          &Fact::valueChanged,                                this, &LandingComplexItem::_setDirty);
    connect(useLoiterToAlt(),           &Fact::valueChanged,                                this, &LandingComplexItem::_setDirty);
    connect(stopTakingPhotos(),         &Fact::valueChanged,                                this, &LandingComplexItem::_setDirty);
    connect(stopTakingVideo(),          &Fact::valueChanged,                                this, &LandingComplexItem::_setDirty);
    connect(this,                       &LandingComplexItem::finalApproachCoordinateChanged,this, &LandingComplexItem::_setDirty);
    connect(this,                       &LandingComplexItem::landingCoordinateChanged,      this, &LandingComplexItem::_setDirty);
    connect(this,                       &LandingComplexItem::altitudesAreRelativeChanged,   this, &LandingComplexItem::_setDirty);

    connect(stopTakingPhotos(),         &Fact::valueChanged,                                this, &LandingComplexItem::_signalLastSequenceNumberChanged);
    connect(stopTakingVideo(),          &Fact::valueChanged,                                this, &LandingComplexItem::_signalLastSequenceNumberChanged);

    connect(this,                       &LandingComplexItem::altitudesAreRelativeChanged,   this, &LandingComplexItem::_amslEntryAltChanged);
    connect(this,                       &LandingComplexItem::altitudesAreRelativeChanged,   this, &LandingComplexItem::_amslExitAltChanged);
    connect(finalApproachAltitude(),    &Fact::valueChanged,                                this, &LandingComplexItem::_amslEntryAltChanged);
    connect(landingAltitude(),          &Fact::valueChanged,                                this, &LandingComplexItem::_amslExitAltChanged);
    connect(this,                       &LandingComplexItem::amslEntryAltChanged,           this, &LandingComplexItem::maxAMSLAltitudeChanged);
    connect(this,                       &LandingComplexItem::amslExitAltChanged,            this, &LandingComplexItem::minAMSLAltitudeChanged);

    connect(this,                       &LandingComplexItem::landingCoordSetChanged,        this, &LandingComplexItem::readyForSaveStateChanged);
    connect(this,                       &LandingComplexItem::wizardModeChanged,             this, &LandingComplexItem::readyForSaveStateChanged);

    connect(this,                       &LandingComplexItem::finalApproachCoordinateChanged,this, &LandingComplexItem::complexDistanceChanged);
    connect(this,                       &LandingComplexItem::loiterTangentCoordinateChanged,this, &LandingComplexItem::complexDistanceChanged);
    connect(this,                       &LandingComplexItem::landingCoordinateChanged,      this, &LandingComplexItem::complexDistanceChanged);

    connect(this,                       &LandingComplexItem::loiterTangentCoordinateChanged,this, &LandingComplexItem::_updateFlightPathSegmentsSignal);
    connect(this,                       &LandingComplexItem::finalApproachCoordinateChanged,this, &LandingComplexItem::_updateFlightPathSegmentsSignal);
    connect(this,                       &LandingComplexItem::landingCoordinateChanged,      this, &LandingComplexItem::_updateFlightPathSegmentsSignal);
    connect(finalApproachAltitude(),    &Fact::valueChanged,                                this, &LandingComplexItem::_updateFlightPathSegmentsSignal);
    connect(landingAltitude(),          &Fact::valueChanged,                                this, &LandingComplexItem::_updateFlightPathSegmentsSignal);
    connect(this,                       &LandingComplexItem::altitudesAreRelativeChanged,   this, &LandingComplexItem::_updateFlightPathSegmentsSignal);
    connect(_missionController,         &MissionController::plannedHomePositionChanged,     this, &LandingComplexItem::_updateFlightPathSegmentsSignal);

    connect(finalApproachAltitude(),    &Fact::valueChanged,                                this, &LandingComplexItem::_updateFinalApproachCoodinateAltitudeFromFact);
    connect(landingAltitude(),          &Fact::valueChanged,                                this, &LandingComplexItem::_updateLandingCoodinateAltitudeFromFact);
}

void LandingComplexItem::setLandingHeadingToTakeoffHeading()
{
    TakeoffMissionItem* takeoffMissionItem = _missionController->takeoffMissionItem();
    if (takeoffMissionItem && takeoffMissionItem->specifiesCoordinate()) {
        qreal heading = takeoffMissionItem->launchCoordinate().azimuthTo(takeoffMissionItem->coordinate());
        landingHeading()->setRawValue(heading);
    }
}

double LandingComplexItem::complexDistance(void) const
{
    return finalApproachCoordinate().distanceTo(loiterTangentCoordinate()) + loiterTangentCoordinate().distanceTo(landingCoordinate());
}

void LandingComplexItem::setLandingCoordinate(const QGeoCoordinate& coordinate)
{
    if (coordinate != _landingCoordinate) {
        _landingCoordinate = coordinate;
        if (_landingCoordSet) {
            emit exitCoordinateChanged(coordinate);
            emit landingCoordinateChanged(coordinate);
        } else {
            _ignoreRecalcSignals = true;
            emit exitCoordinateChanged(coordinate);
            emit landingCoordinateChanged(coordinate);
            _ignoreRecalcSignals = false;
            _landingCoordSet = true;
            _recalcFromHeadingAndDistanceChange();
            emit landingCoordSetChanged(true);
        }
    }
}

void LandingComplexItem::setFinalApproachCoordinate(const QGeoCoordinate& coordinate)
{
    if (coordinate != _finalApproachCoordinate) {
        _finalApproachCoordinate = coordinate;
        emit coordinateChanged(coordinate);
        emit finalApproachCoordinateChanged(coordinate);
    }
}

QPointF LandingComplexItem::_rotatePoint(const QPointF& point, const QPointF& origin, double angle)
{
    QPointF rotated;
    double radians = (M_PI / 180.0) * angle;

    rotated.setX(((point.x() - origin.x()) * cos(radians)) - ((point.y() - origin.y()) * sin(radians)) + origin.x());
    rotated.setY(((point.x() - origin.x()) * sin(radians)) + ((point.y() - origin.y()) * cos(radians)) + origin.y());

    return rotated;
}

void LandingComplexItem::_recalcFromHeadingAndDistanceChange(void)
{
    // Fixed:
    //      land
    //      heading
    //      distance
    //      radius
    // Adjusted:
    //      loiter
    //      loiter tangent
    //      glide slope

    if (!_ignoreRecalcSignals && _landingCoordSet) {
        // These are our known values
        double radius = loiterRadius()->rawValue().toDouble();
        double landToTangentDistance = landingDistance()->rawValue().toDouble();
        double heading = landingHeading()->rawValue().toDouble();

        // Heading is from loiter to land, hence +180
        _loiterTangentCoordinate = _landingCoordinate.atDistanceAndAzimuth(landToTangentDistance, heading + 180);

        // Loiter coord is 90 degrees counter clockwise from tangent coord
        _finalApproachCoordinate = _loiterTangentCoordinate.atDistanceAndAzimuth(radius, heading - 180 + (_loiterClockwise()->rawValue().toBool() ? -90 : 90));
        _finalApproachCoordinate.setAltitude(finalApproachAltitude()->rawValue().toDouble());

        _ignoreRecalcSignals = true;
        emit loiterTangentCoordinateChanged(_loiterTangentCoordinate);
        emit finalApproachCoordinateChanged(_finalApproachCoordinate);
        emit coordinateChanged(_finalApproachCoordinate);
        _calcGlideSlope();
        _ignoreRecalcSignals = false;
    }
}

void LandingComplexItem::_recalcFromRadiusChange(void)
{
    // Fixed:
    //      land
    //      loiter tangent
    //      distance
    //      radius
    //      heading
    // Adjusted:
    //      loiter

    if (!_ignoreRecalcSignals) {
        // These are our known values
        double radius  = loiterRadius()->rawValue().toDouble();
        double landToTangentDistance = landingDistance()->rawValue().toDouble();
        double heading = landingHeading()->rawValue().toDouble();

        double landToLoiterDistance = _landingCoordinate.distanceTo(_finalApproachCoordinate);
        if (landToLoiterDistance < radius) {
            // Degnenerate case: Move tangent to loiter point
            _loiterTangentCoordinate = _finalApproachCoordinate;

            double heading = _landingCoordinate.azimuthTo(_loiterTangentCoordinate);

            _ignoreRecalcSignals = true;
            landingHeading()->setRawValue(heading);
            emit loiterTangentCoordinateChanged(_loiterTangentCoordinate);
            _ignoreRecalcSignals = false;
        } else {
            double landToLoiterDistance = qSqrt(qPow(radius, 2) + qPow(landToTangentDistance, 2));
            double angleLoiterToTangent = qRadiansToDegrees(qAsin(radius/landToLoiterDistance)) * (_loiterClockwise()->rawValue().toBool() ? -1 : 1);

            _finalApproachCoordinate = _landingCoordinate.atDistanceAndAzimuth(landToLoiterDistance, heading + 180 + angleLoiterToTangent);
            _finalApproachCoordinate.setAltitude(finalApproachAltitude()->rawValue().toDouble());

            _ignoreRecalcSignals = true;
            emit finalApproachCoordinateChanged(_finalApproachCoordinate);
            emit coordinateChanged(_finalApproachCoordinate);
            _ignoreRecalcSignals = false;
        }
    }
}

void LandingComplexItem::_recalcFromCoordinateChange(void)
{
    // Fixed:
    //      land
    //      loiter
    //      radius
    // Adjusted:
    //      loiter tangent
    //      heading
    //      distance
    //      glide slope

    if (!_ignoreRecalcSignals && _landingCoordSet) {
        // These are our known values
        double radius = loiterRadius()->rawValue().toDouble();
        double landToLoiterDistance = _landingCoordinate.distanceTo(_finalApproachCoordinate);
        double landToLoiterHeading = _landingCoordinate.azimuthTo(_finalApproachCoordinate);

        double landToTangentDistance;
        if (landToLoiterDistance < radius) {
            // Degenerate case, set tangent to loiter coordinate
            _loiterTangentCoordinate = _finalApproachCoordinate;
            landToTangentDistance = _landingCoordinate.distanceTo(_loiterTangentCoordinate);
        } else {
            double loiterToTangentAngle = qRadiansToDegrees(qAsin(radius/landToLoiterDistance)) * (_loiterClockwise()->rawValue().toBool() ? 1 : -1);
            landToTangentDistance = qSqrt(qPow(landToLoiterDistance, 2) - qPow(radius, 2));

            _loiterTangentCoordinate = _landingCoordinate.atDistanceAndAzimuth(landToTangentDistance, landToLoiterHeading + loiterToTangentAngle);

        }

        double heading = _loiterTangentCoordinate.azimuthTo(_landingCoordinate);

        _ignoreRecalcSignals = true;
        landingHeading()->setRawValue(heading);
        landingDistance()->setRawValue(landToTangentDistance);
        emit loiterTangentCoordinateChanged(_loiterTangentCoordinate);
        _calcGlideSlope();
        _ignoreRecalcSignals = false;
    }
}

int LandingComplexItem::lastSequenceNumber(void) const
{
    // Fixed items are:
    //  land start, loiter, land
    // Optional items are:
    //  stop photos/video
    return _sequenceNumber + 2 + (stopTakingPhotos()->rawValue().toBool() ? 2 : 0) + (stopTakingVideo()->rawValue().toBool() ? 1 : 0);
}

void LandingComplexItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    int seqNum = _sequenceNumber;

    // IMPORTANT NOTE: Any changes here must also be taken into account in scanForItem

    MissionItem* item = _createDoLandStartItem(seqNum++, missionItemParent);
    items.append(item);


    if (stopTakingPhotos()->rawValue().toBool()) {
        CameraSection::appendStopTakingPhotos(items, seqNum, missionItemParent);
    }

    if (stopTakingVideo()->rawValue().toBool()) {
        CameraSection::appendStopTakingVideo(items, seqNum, missionItemParent);
    }

    item = _createFinalApproachItem(seqNum++, missionItemParent);
    items.append(item);

    item = _createLandItem(seqNum++,
                           _altitudesAreRelative,
                           _landingCoordinate.latitude(), _landingCoordinate.longitude(), landingAltitude()->rawValue().toDouble(),
                           missionItemParent);
    items.append(item);
}

MissionItem* LandingComplexItem::_createDoLandStartItem(int seqNum, QObject* parent)
{
    return new MissionItem(seqNum,                              // sequence number
                           MAV_CMD_DO_LAND_START,               // MAV_CMD
                           MAV_FRAME_MISSION,                   // MAV_FRAME
                           0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // param 1-7
                           true,                                // autoContinue
                           false,                               // isCurrentItem
                           parent);
}

MissionItem* LandingComplexItem::_createFinalApproachItem(int seqNum, QObject* parent)
{
    if (useLoiterToAlt()->rawValue().toBool()) {
        return new MissionItem(seqNum,
                               MAV_CMD_NAV_LOITER_TO_ALT,
                               _altitudesAreRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,
                               1.0,             // Heading required = true
                               loiterRadius()->rawValue().toDouble() * (_loiterClockwise()->rawValue().toBool() ? 1.0 : -1.0),
                               0.0,             // param 3 - unused
                               1.0,             // Exit crosstrack - tangent of loiter to land point
                               _finalApproachCoordinate.latitude(),
                               _finalApproachCoordinate.longitude(),
                               _finalApproachAltitude()->rawValue().toFloat(),
                               true,            // autoContinue
                               false,           // isCurrentItem
                               parent);
    } else {
        return new MissionItem(seqNum,
                               MAV_CMD_NAV_WAYPOINT,
                               _altitudesAreRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,
                               0,               // No hold time
                               0,               // Use default acceptance radius
                               0,               // Pass through waypoint
                               qQNaN(),         // Yaw not specified
                               _finalApproachCoordinate.latitude(),
                               _finalApproachCoordinate.longitude(),
                               _finalApproachAltitude()->rawValue().toFloat(),
                               true,            // autoContinue
                               false,           // isCurrentItem
                               parent);
    }
}

bool LandingComplexItem::_scanForItem(QmlObjectListModel* visualItems, bool flyView, PlanMasterController* masterController, IsLandItemFunc isLandItemFunc, CreateItemFunc createItemFunc)
{
    qCDebug(LandingComplexItemLog) << "VTOLLandingComplexItem::scanForItem count" << visualItems->count();

    if (visualItems->count() < 3) {
        return false;
    }

    // A valid landing pattern is comprised of the follow commands in this order at the end of the item list:
    //  MAV_CMD_DO_LAND_START - required
    //  Stop taking photos sequence - optional
    //  Stop taking video sequence - optional
    //  MAV_CMD_NAV_LOITER_TO_ALT or MAV_CMD_NAV_WAYPOINT
    //  MAV_CMD_NAV_LAND or MAV_CMD_NAV_VTOL_LAND

    // Start looking for the commands in reverse order from the end of the list

    int scanIndex = visualItems->count() - 1;

    if (scanIndex < 0 || scanIndex > visualItems->count() - 1) {
        return false;
    }
    SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(scanIndex--);
    if (!item) {
        return false;
    }
    MissionItem& missionItemLand = item->missionItem();
    if (!isLandItemFunc(missionItemLand)) {
        return false;
    }
    MAV_FRAME landPointFrame = missionItemLand.frame();

    if (scanIndex < 0 || scanIndex > visualItems->count() - 1) {
        return false;
    }
    item = visualItems->value<SimpleMissionItem*>(scanIndex);
    if (!item) {
        return false;
    }
    bool useLoiterToAlt = true;
    MissionItem& missionItemFinalApproach = item->missionItem();
    if (missionItemFinalApproach.command() == MAV_CMD_NAV_LOITER_TO_ALT) {
        if (missionItemFinalApproach.frame() != landPointFrame ||
                missionItemFinalApproach.param1() != 1.0 || missionItemFinalApproach.param3() != 0 || missionItemFinalApproach.param4() != 1.0) {
            return false;
        }
    } else if (missionItemFinalApproach.command() == MAV_CMD_NAV_WAYPOINT) {
        if (missionItemFinalApproach.frame() != landPointFrame ||
                missionItemFinalApproach.param1() != 0 || missionItemFinalApproach.param2() != 0 || missionItemFinalApproach.param3() != 0 ||
                !qIsNaN(missionItemFinalApproach.param4()) ||
                qIsNaN(missionItemFinalApproach.param5()) || qIsNaN(missionItemFinalApproach.param6()) || qIsNaN(missionItemFinalApproach.param6())) {
            return false;
        }
        useLoiterToAlt = false;
    } else {
        return false;
    }

    scanIndex -= CameraSection::stopTakingVideoCommandCount();
    bool stopTakingVideo = CameraSection::scanStopTakingVideo(visualItems, scanIndex, false /* removeScannedItems */);
    if (!stopTakingVideo) {
        scanIndex += CameraSection::stopTakingVideoCommandCount();
    }

    scanIndex -= CameraSection::stopTakingPhotosCommandCount();
    bool stopTakingPhotos = CameraSection::scanStopTakingPhotos(visualItems, scanIndex, false /* removeScannedItems */);
    if (!stopTakingPhotos) {
        scanIndex += CameraSection::stopTakingPhotosCommandCount();
    }

    scanIndex--;
    if (scanIndex < 0 || scanIndex > visualItems->count() - 1) {
        return false;
    }
    item = visualItems->value<SimpleMissionItem*>(scanIndex);
    if (!item) {
        return false;
    }
    MissionItem& missionItemDoLandStart = item->missionItem();
    if (missionItemDoLandStart.command() != MAV_CMD_DO_LAND_START ||
            missionItemDoLandStart.frame() != MAV_FRAME_MISSION ||
            missionItemDoLandStart.param1() != 0 || missionItemDoLandStart.param2() != 0 || missionItemDoLandStart.param3() != 0 || missionItemDoLandStart.param4() != 0 ||
            missionItemDoLandStart.param5() != 0 || missionItemDoLandStart.param6() != 0 || missionItemDoLandStart.param7() != 0) {
        return false;
    }

    // We made it this far so we do have a Fixed Wing Landing Pattern item at the end of the mission.
    // Since we have scanned it we need to remove the items for it fromt the list
    int deleteCount = 3;
    if (stopTakingPhotos) {
        deleteCount += CameraSection::stopTakingPhotosCommandCount();
    }
    if (stopTakingVideo) {
        deleteCount += CameraSection::stopTakingVideoCommandCount();
    }
    int firstItem = visualItems->count() - deleteCount;
    while (deleteCount--) {
        visualItems->removeAt(firstItem)->deleteLater();
    }

    // Now stuff all the scanned information into the item

    LandingComplexItem* complexItem = createItemFunc(masterController, flyView);

    complexItem->_ignoreRecalcSignals = true;

    complexItem->_altitudesAreRelative = landPointFrame == MAV_FRAME_GLOBAL_RELATIVE_ALT;
    complexItem->setFinalApproachCoordinate(QGeoCoordinate(missionItemFinalApproach.param5(), missionItemFinalApproach.param6()));
    complexItem->finalApproachAltitude()->setRawValue(missionItemFinalApproach.param7());
    complexItem->useLoiterToAlt()->setRawValue(useLoiterToAlt);

    if (useLoiterToAlt) {
        complexItem->loiterRadius()->setRawValue(qAbs(missionItemFinalApproach.param2()));
        complexItem->loiterClockwise()->setRawValue(missionItemFinalApproach.param2() > 0);
    }

    complexItem->_landingCoordinate.setLatitude(missionItemLand.param5());
    complexItem->_landingCoordinate.setLongitude(missionItemLand.param6());
    complexItem->landingAltitude()->setRawValue(missionItemLand.param7());

    complexItem->stopTakingPhotos()->setRawValue(stopTakingPhotos);
    complexItem->stopTakingVideo()->setRawValue(stopTakingVideo);

    complexItem->_landingCoordSet = true;

    complexItem->_ignoreRecalcSignals = false;

    complexItem->_recalcFromCoordinateChange();
    complexItem->setDirty(false);

    visualItems->append(complexItem);

    return true;
}

void LandingComplexItem::applyNewAltitude(double newAltitude)
{
    finalApproachAltitude()->setRawValue(newAltitude);
}

LandingComplexItem::ReadyForSaveState LandingComplexItem::readyForSaveState(void) const
{
    return _landingCoordSet && !_wizardMode ? ReadyForSave : NotReadyForSaveData;
}

void LandingComplexItem::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void LandingComplexItem::_setDirty(void)
{
    setDirty(true);
}

void LandingComplexItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}

double LandingComplexItem::amslEntryAlt(void) const
{
    return finalApproachAltitude()->rawValue().toDouble() + (_altitudesAreRelative ? _missionController->plannedHomePosition().altitude() : 0);
}

double LandingComplexItem::amslExitAlt(void) const
{
    return landingAltitude()->rawValue().toDouble() + (_altitudesAreRelative ? _missionController->plannedHomePosition().altitude() : 0);

}

void LandingComplexItem::_signalLastSequenceNumberChanged(void)
{
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

void LandingComplexItem::_updateFinalApproachCoodinateAltitudeFromFact(void)
{
    _finalApproachCoordinate.setAltitude(finalApproachAltitude()->rawValue().toDouble());
    emit finalApproachCoordinateChanged(_finalApproachCoordinate);
    emit coordinateChanged(_finalApproachCoordinate);
}

void LandingComplexItem::_updateLandingCoodinateAltitudeFromFact(void)
{
    _landingCoordinate.setAltitude(landingAltitude()->rawValue().toDouble());
    emit landingCoordinateChanged(_landingCoordinate);
}

double LandingComplexItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    return qMax(_finalApproachCoordinate.distanceTo(other),_landingCoordinate.distanceTo(other));
}

QJsonObject LandingComplexItem::_save(void)
{
    QJsonObject saveObject;

    QGeoCoordinate coordinate;
    QJsonValue jsonCoordinate;

    coordinate = _finalApproachCoordinate;
    coordinate.setAltitude(finalApproachAltitude()->rawValue().toDouble());
    JsonHelper::saveGeoCoordinate(coordinate, true /* writeAltitude */, jsonCoordinate);
    saveObject[_jsonFinalApproachCoordinateKey] = jsonCoordinate;

    coordinate = _landingCoordinate;
    coordinate.setAltitude(landingAltitude()->rawValue().toDouble());
    JsonHelper::saveGeoCoordinate(coordinate, true /* writeAltitude */, jsonCoordinate);
    saveObject[_jsonLandingCoordinateKey] = jsonCoordinate;

    saveObject[_jsonLoiterRadiusKey]            = loiterRadius()->rawValue().toDouble();
    saveObject[_jsonStopTakingPhotosKey]        = stopTakingPhotos()->rawValue().toBool();
    saveObject[_jsonStopTakingVideoKey]         = stopTakingVideo()->rawValue().toBool();
    saveObject[_jsonLoiterClockwiseKey]         = loiterClockwise()->rawValue().toBool();
    saveObject[_jsonUseLoiterToAltKey]          = useLoiterToAlt()->rawValue().toBool();
    saveObject[_jsonAltitudesAreRelativeKey]    = _altitudesAreRelative;

    return saveObject;
}

bool LandingComplexItem::_load(const QJsonObject& complexObject, int sequenceNumber, const QString& jsonComplexItemTypeValue, bool useDeprecatedRelAltKeys, QString& errorString)
{
    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey,                   QJsonValue::Double, true },
        { VisualMissionItem::jsonTypeKey,               QJsonValue::String, true },
        { ComplexMissionItem::jsonComplexItemTypeKey,   QJsonValue::String, true },
        { _jsonDeprecatedLoiterCoordinateKey,           QJsonValue::Array,  false }, // Loiter changed to Final Approach
        { _jsonFinalApproachCoordinateKey,              QJsonValue::Array,  false },
        { _jsonLoiterRadiusKey,                         QJsonValue::Double, true },
        { _jsonLoiterClockwiseKey,                      QJsonValue::Bool,   true },
        { _jsonLandingCoordinateKey,                    QJsonValue::Array,  true },
        { _jsonStopTakingPhotosKey,                     QJsonValue::Bool,   false },
        { _jsonStopTakingVideoKey,                      QJsonValue::Bool,   false },
        { _jsonUseLoiterToAltKey,                       QJsonValue::Bool,   false },
    };
    if (!JsonHelper::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    if (!complexObject.contains(_jsonDeprecatedLoiterCoordinateKey) && !complexObject.contains(_jsonFinalApproachCoordinateKey)) {
        QList<JsonHelper::KeyValidateInfo> keyInfoList = {
            { _jsonFinalApproachCoordinateKey, QJsonValue::Array, true },
        };
        if (!JsonHelper::validateKeys(complexObject, keyInfoList, errorString)) {
            return false;
        }
    }

    QString itemType = complexObject[VisualMissionItem::jsonTypeKey].toString();
    QString complexType = complexObject[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if (itemType != VisualMissionItem::jsonTypeComplexItemValue || complexType != jsonComplexItemTypeValue) {
        errorString = tr("%1 does not support loading this complex mission item type: %2:%3").arg(qgcApp()->applicationName()).arg(itemType).arg(complexType);
        return false;
    }

    setSequenceNumber(sequenceNumber);

    _ignoreRecalcSignals = true;

    if (useDeprecatedRelAltKeys) {
        QList<JsonHelper::KeyValidateInfo> v1KeyInfoList = {
            { _jsonDeprecatedLoiterAltitudeRelativeKey,   QJsonValue::Bool,  true },
            { _jsonDeprecatedLandingAltitudeRelativeKey,  QJsonValue::Bool,  true },
        };
        if (!JsonHelper::validateKeys(complexObject, v1KeyInfoList, errorString)) {
            return false;
        }

        bool loiterAltitudeRelative = complexObject[_jsonDeprecatedLoiterAltitudeRelativeKey].toBool();
        bool landingAltitudeRelative = complexObject[_jsonDeprecatedLandingAltitudeRelativeKey].toBool();
        if (loiterAltitudeRelative != landingAltitudeRelative) {
            qgcApp()->showAppMessage(tr("Fixed Wing Landing Pattern: "
                                        "Setting the loiter and landing altitudes with different settings for altitude relative is no longer supported. "
                                        "Both have been set to relative altitude. Be sure to adjust/check your plan prior to flight."));
            _altitudesAreRelative = true;
        } else {
            _altitudesAreRelative = loiterAltitudeRelative;
        }
    } else {
        QList<JsonHelper::KeyValidateInfo> v2KeyInfoList = {
            { _jsonAltitudesAreRelativeKey, QJsonValue::Bool,  true },
        };
        if (!JsonHelper::validateKeys(complexObject, v2KeyInfoList, errorString)) {
            _ignoreRecalcSignals = false;
            return false;
        }
        _altitudesAreRelative = complexObject[_jsonAltitudesAreRelativeKey].toBool();
    }

    QGeoCoordinate coordinate;
    QString finalApproachKey = complexObject.contains(_jsonFinalApproachCoordinateKey) ? _jsonFinalApproachCoordinateKey : _jsonDeprecatedLoiterCoordinateKey;
    if (!JsonHelper::loadGeoCoordinate(complexObject[finalApproachKey], true /* altitudeRequired */, coordinate, errorString)) {
        return false;
    }
    _finalApproachCoordinate = coordinate;
    finalApproachAltitude()->setRawValue(coordinate.altitude());

    if (!JsonHelper::loadGeoCoordinate(complexObject[_jsonLandingCoordinateKey], true /* altitudeRequired */, coordinate, errorString)) {
        return false;
    }
    _landingCoordinate = coordinate;
    landingAltitude()->setRawValue(coordinate.altitude());

    loiterRadius()->setRawValue(complexObject[_jsonLoiterRadiusKey].toDouble());
    loiterClockwise()->setRawValue(complexObject[_jsonLoiterClockwiseKey].toBool());
    useLoiterToAlt()->setRawValue(complexObject[_jsonUseLoiterToAltKey].toBool(true));
    stopTakingPhotos()->setRawValue(complexObject[_jsonStopTakingPhotosKey].toBool(false));
    stopTakingVideo()->setRawValue(complexObject[_jsonStopTakingVideoKey].toBool(false));

    _calcGlideSlope();

    _landingCoordSet        = true;
    _ignoreRecalcSignals    = false;

    _recalcFromCoordinateChange();
    emit coordinateChanged(this->coordinate());    // This will kick off terrain query

    return true;
}

void LandingComplexItem::setAltitudesAreRelative(bool altitudesAreRelative)
{
    if (altitudesAreRelative != _altitudesAreRelative) {
        _altitudesAreRelative = altitudesAreRelative;
        emit altitudesAreRelativeChanged(_altitudesAreRelative);
    }
}
