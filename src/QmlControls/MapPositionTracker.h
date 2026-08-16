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
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(MapPositionTrackerLog)

/// Map auto-centering policy shared by the map engines (FlightMap/QtLocation and
/// GeoMap). Owns the decisions of when and where to center:
///   - one-time centering on the first valid vehicle or GCS position
///   - continuous vehicle following (keepVehicleCentered)
///   - inset-rect following: recenter when the vehicle drifts outside the
///     unobstructed center area of the viewport (evaluateInsetFollow)
/// Following pauses while the user interacts with the map and resumes after a
/// cooldown. The consumer owns everything engine-specific: coordinate/screen
/// projection, camera movement and recenter animation.
class MapPositionTracker : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QGeoCoordinate gcsPosition READ gcsPosition WRITE setGcsPosition NOTIFY gcsPositionChanged)
    Q_PROPERTY(QGeoCoordinate vehicleCoordinate READ vehicleCoordinate WRITE setVehicleCoordinate NOTIFY
                   vehicleCoordinateChanged)
    Q_PROPERTY(bool allowGCSLocationCenter READ allowGCSLocationCenter WRITE setAllowGCSLocationCenter NOTIFY
                   allowGCSLocationCenterChanged)
    Q_PROPERTY(bool allowVehicleLocationCenter READ allowVehicleLocationCenter WRITE setAllowVehicleLocationCenter
                   NOTIFY allowVehicleLocationCenterChanged)
    Q_PROPERTY(bool centerGCSWhenVehicleValid READ centerGCSWhenVehicleValid WRITE setCenterGCSWhenVehicleValid NOTIFY
                   centerGCSWhenVehicleValidChanged)
    Q_PROPERTY(bool keepVehicleCentered READ keepVehicleCentered WRITE setKeepVehicleCentered NOTIFY
                   keepVehicleCenteredChanged)
    Q_PROPERTY(bool userInteracting READ userInteracting WRITE setUserInteracting NOTIFY userInteractingChanged)
    Q_PROPERTY(bool animating READ animating WRITE setAnimating NOTIFY animatingChanged)
    Q_PROPERTY(bool firstVehiclePositionReceived READ firstVehiclePositionReceived WRITE setFirstVehiclePositionReceived
                   NOTIFY firstVehiclePositionReceivedChanged)

public:
    explicit MapPositionTracker(QObject* parent = nullptr);

    /// Following resumes this long after the user stops interacting
    static constexpr int kResumeDelayMs = 10000;

    bool active() const { return _active; }

    void setActive(bool active);

    QGeoCoordinate gcsPosition() const { return _gcsPosition; }

    void setGcsPosition(const QGeoCoordinate& position);

    QGeoCoordinate vehicleCoordinate() const { return _vehicleCoordinate; }

    void setVehicleCoordinate(const QGeoCoordinate& coordinate);

    bool allowGCSLocationCenter() const { return _allowGCSLocationCenter; }

    void setAllowGCSLocationCenter(bool allow);

    bool allowVehicleLocationCenter() const { return _allowVehicleLocationCenter; }

    void setAllowVehicleLocationCenter(bool allow);

    /// One-time GCS centering also fires while a valid vehicle position exists
    /// (FlightMap centers on GCS in that case when the map will keep following
    /// the vehicle anyway)
    bool centerGCSWhenVehicleValid() const { return _centerGCSWhenVehicleValid; }

    void setCenterGCSWhenVehicleValid(bool center);

    bool keepVehicleCentered() const { return _keepVehicleCentered; }

    void setKeepVehicleCentered(bool keep);

    bool userInteracting() const { return _userInteracting; }

    void setUserInteracting(bool interacting);

    /// Consumer's recenter animation is running: inset evaluation is skipped
    bool animating() const { return _animating; }

    void setAnimating(bool animating);

    /// Writable: consumers set it true to suppress the one-time vehicle
    /// centering (e.g. after fitting the view to mission items instead)
    bool firstVehiclePositionReceived() const { return _firstVehiclePositionReceived; }

    void setFirstVehiclePositionReceived(bool received);

    /// Inset-follow evaluation, called periodically by the consumer with the
    /// vehicle's current screen position, the unobstructed center rect and the
    /// (possibly empty) list of corner rects covered by UI elements. Emits
    /// recenterVehicleTo(centerRect.center()) when the vehicle is outside the
    /// center rect or under a corner rect.
    Q_INVOKABLE void evaluateInsetFollow(const QPointF& vehicleScreenPoint, const QRectF& centerRect,
                                         const QVariantList& cornerRects);

#ifdef QGC_UNITTEST_BUILD
    void setResumeDelayMsForTest(int ms) { _resumeTimer.setInterval(ms); }
#endif

signals:
    /// Consumer sets its map center. firstPosition: first vehicle position,
    /// also apply the initial zoom.
    void centerMap(const QGeoCoordinate& coordinate, bool firstPosition);
    /// Consumer moves the map (animated) so the vehicle lands at screenPoint
    void recenterVehicleTo(const QPointF& screenPoint);

    void activeChanged();
    void gcsPositionChanged();
    void vehicleCoordinateChanged();
    void allowGCSLocationCenterChanged();
    void allowVehicleLocationCenterChanged();
    void centerGCSWhenVehicleValidChanged();
    void keepVehicleCenteredChanged();
    void userInteractingChanged();
    void animatingChanged();
    void firstVehiclePositionReceivedChanged();

private:
    bool _paused() const { return _userInteracting || _resumeTimer.isActive(); }

    /// One-time vehicle centering; returns true when it centered
    bool _evaluateVehicleOneShot();
    void _evaluateGCSOneShot();
    void _resumeFollowing();

    bool _active = true;
    QGeoCoordinate _gcsPosition;
    QGeoCoordinate _vehicleCoordinate;
    bool _allowGCSLocationCenter = false;
    bool _allowVehicleLocationCenter = false;
    bool _centerGCSWhenVehicleValid = false;
    bool _keepVehicleCentered = false;
    bool _userInteracting = false;
    bool _animating = false;
    bool _firstVehiclePositionReceived = false;
    bool _firstGCSPositionReceived = false;
    QTimer _resumeTimer;
};
