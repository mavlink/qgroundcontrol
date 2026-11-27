#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

class MavlinkCameraControlInterface;
class QGCVideoStreamInfo;
class QGeoCoordinate;
class Vehicle;

/// The video streams the cameras of a vehicle advertise, and what those cameras are told of how QGC plays them: which
/// of a camera's streams play besides its current one, and to stop transmitting the current camera's stream while
/// the video is off. The vehicle is the active one once followActiveVehicle(). VideoManager, which gives the streams
/// their sources, knows the vehicle and its cameras through this class only.
class VehicleVideoStreams : public QObject
{
    Q_OBJECT

public:
    /// A stream a camera of the vehicle advertises.
    struct Stream
    {
        QPointer<QGCVideoStreamInfo> streamInfo;
        QPointer<MavlinkCameraControlInterface> camera;
        /// For the user: the camera's name, when the vehicle has several, and the stream's.
        QString name;
    };

    explicit VehicleVideoStreams(QObject* parent = nullptr);

    /// Takes MultiVehicleManager's active vehicle, now and whenever it changes.
    void followActiveVehicle();
    /// Leaves the cameras of the vehicle so far without a stream to transmit, then takes @p vehicle, null for none:
    /// vehicleChanged(), then streamsChanged() with what its cameras advertise.
    void setVehicle(Vehicle* vehicle);

    bool hasVehicle() const;
    /// There is a vehicle, and it is disarmed.
    bool disarmed() const;
    /// There is a vehicle, and its link isn't lost.
    bool connected() const;
    /// Where the vehicle is; invalid without one.
    QGeoCoordinate position() const;

    /// Tells the current camera to transmit its stream: setVehicle() told it to stop when its vehicle was last left.
    void resumeCurrentStream();
    /// Tells the current camera to stop transmitting its stream while the video isn't @p playing, and to resume once
    /// it is: a stream pushed to a UDP port would cross the vehicle's link all the same.
    void setPlaying(bool playing);
    /// Tells each camera which of its streams play besides its current one: those of @p inTiles, and its thermal
    /// stream when it is one of @p mainStreams, the streams the main picture and the view over it play.
    void setExtraStreams(const QList<Stream>& inTiles, const QList<const QGCVideoStreamInfo*>& mainStreams);
    /// Makes @p stream its camera's current stream, and that camera the current one, as the camera panel selects
    /// them. By value: the cameras advertise their streams anew as each is told.
    void select(Stream stream);

signals:
    void vehicleChanged();
    /// The cameras' streams, read anew. @p current is the current camera's current stream, or its thermal stream
    /// when it advertises no other; without a stream info when there is none. @p thermal is the thermal stream to
    /// show over it: the current camera's or, when that has none, that of the first other camera that has one; null
    /// when @p current is a thermal stream. @p others are the remaining streams that aren't thermal: the current
    /// camera's first, then the other cameras', in the cameras' order.
    void streamsChanged(const VehicleVideoStreams::Stream& current, QGCVideoStreamInfo* thermal,
                        const QList<VehicleVideoStreams::Stream>& others);
    void armedChanged(bool armed);
    void communicationLostChanged(bool communicationLost);

private:
    void _readStreams();

    QPointer<Vehicle> _vehicle;
    /// The camera setPlaying() told to stop transmitting.
    QPointer<MavlinkCameraControlInterface> _stoppedCamera;
};
