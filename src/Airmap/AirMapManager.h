/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCToolbox.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "MissionItem.h"
#include "MultiVehicleManager.h"
#include "AirspaceManagement.h"

#include <QGeoCoordinate>
#include <QList>
#include <QQueue>
#include <QTimer>

#include <cstdint>
#include <functional>
#include <memory>

#include <airmap/qt/client.h>
#include <airmap/qt/logger.h>
#include <airmap/qt/types.h>
#include <airmap/traffic.h>

Q_DECLARE_LOGGING_CATEGORY(AirMapManagerLog)

/**
 * @class LifetimeChecker
 * Base class which helps to check if an object instance still exists.
 * A subclass can take a weak pointer from _instance and then check if the object was deleted.
 * This is used in callbacks that access 'this', but the instance might already be deleted (e.g. vehicle disconnect).
 */
class LifetimeChecker
{
public:
    LifetimeChecker() : _instance(this, [](void*){}) { }
    virtual ~LifetimeChecker() = default;

protected:
    std::shared_ptr<LifetimeChecker> _instance;
};

/**
 * @class AirMapSharedState
 * contains state & settings that need to be shared (such as login)
 */
class AirMapSharedState : public QObject
{
    Q_OBJECT
public:
    struct Settings {
        QString apiKey;

        // login credentials
        QString clientID;
        QString userName; ///< use anonymous login if empty
        QString password;
    };

    void setSettings(const Settings& settings);
    const Settings& settings() const { return _settings; }

    void setClient(airmap::qt::Client* client) { _client = client; }

    /**
     * Get the current client instance. It can be NULL. If not NULL, it implies
     * there's an API key set.
     */
    airmap::qt::Client* client() const { return _client; }

    bool hasAPIKey() const { return _settings.apiKey != ""; }

    bool isLoggedIn() const { return _loginToken != ""; }

    using Callback = std::function<void(const QString& /* login_token */)>;

    /**
     * Do a request that requires user login: if not yet logged in, the request is queued and
     * processed after successful login, otherwise it's executed directly.
     */
    void doRequestWithLogin(const Callback& callback);

    void login();

    void logout();

    const QString& loginToken() const { return _loginToken; }

signals:
    void error(const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);

private:
    void _processPendingRequests();

    bool _isLoginInProgress = false;
    QString _loginToken; ///< login token: empty when not logged in

    airmap::qt::Client* _client = nullptr;

    Settings _settings;

    QQueue<Callback> _pendingRequests; ///< pending requests that are processed after a successful login
};


/// class to download polygons from AirMap
class AirMapRestrictionManager : public AirspaceRestrictionProvider, public LifetimeChecker
{
    Q_OBJECT
public:
    AirMapRestrictionManager(AirMapSharedState& shared);

    void setROI(const QGeoCoordinate& center, double radiusMeters) override;

signals:
    void error(const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);

private:

    static void _addPolygonToList(const airmap::Geometry::Polygon& polygon, QList<PolygonAirspaceRestriction*>& list);

    enum class State {
        Idle,
        RetrieveItems,
    };

    State                   _state = State::Idle;
    AirMapSharedState&      _shared;
};


/// class to upload a flight
class AirMapFlightManager : public QObject, public LifetimeChecker
{
    Q_OBJECT
public:
    AirMapFlightManager(AirMapSharedState& shared);

    /// Send flight path to AirMap
    void createFlight(const QList<MissionItem*>& missionItems);

    AirspaceAuthorization::PermitStatus flightPermitStatus() const { return _flightPermitStatus; }

    const QString& flightID() const { return _currentFlightId; }

public slots:
    void endFlight();

signals:
    void error(const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);
    void flightPermitStatusChanged();

private slots:
    void _pollBriefing();

private:

    /**
     * upload flight stored in _flight
     */
    void _uploadFlight();

    /**
     * query the active flights and end the first one (because only a single flight can be active at a time).
     */
    void _endFirstFlight();

    /**
     * implementation of endFlight()
     */
    void _endFlight(const QString& flightID);

    /**
     * check if the briefing response is valid and call _submitPendingFlightPlan() if it is.
     */
    void _checkForValidBriefing();

    void _submitPendingFlightPlan();

    enum class State {
        Idle,
        GetPilotID,
        FlightUpload,
        FlightBrief,
        FlightSubmit,
        FlightPolling, // poll & check for approval
        FlightEnd,
        EndFirstFlight, // get a list of open flights & end the first one (because there can only be 1 active at a time)
    };
    struct Flight {
        QList<QGeoCoordinate> coords;
        QGeoCoordinate takeoffCoord;
        float maxAltitude = 0;

        void reset() {
            coords.clear();
            maxAltitude = 0;
        }
    };
    Flight                              _flight; ///< flight pending to be uploaded

    State                               _state = State::Idle;
    AirMapSharedState&                  _shared;
    QString                             _currentFlightId; ///< Flight ID, empty if there is none
    QString                             _pendingFlightId; ///< current flight ID, not necessarily accepted yet (once accepted, it's equal to _currentFlightId)
    QString                             _pendingFlightPlan; ///< current flight plan, waiting to be submitted
    AirspaceAuthorization::PermitStatus _flightPermitStatus = AirspaceAuthorization::PermitUnknown;
    QString                             _pilotID; ///< Pilot ID in the form "auth0|abc123"
    bool                                _noFlightCreatedYet = true;
    QTimer                              _pollTimer; ///< timer to poll for approval check
};

/// class to send telemetry data to AirMap
class AirMapTelemetry : public QObject, public LifetimeChecker
{
    Q_OBJECT
public:
    AirMapTelemetry(AirMapSharedState& shared);
    virtual ~AirMapTelemetry() = default;

    /**
     * Setup the connection to start sending telemetry
     */
    void startTelemetryStream(const QString& flightID);

    void stopTelemetryStream();

    bool isTelemetryStreaming() const;

signals:
    void error(const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);

public slots:
    void vehicleMavlinkMessageReceived(const mavlink_message_t& message);

private:

    void _handleGlobalPositionInt(const mavlink_message_t& message);
    void _handleGPSRawInt(const mavlink_message_t& message);

    enum class State {
        Idle,
        StartCommunication,
        EndCommunication,
        Streaming,
    };

    State                   _state = State::Idle;

    AirMapSharedState&      _shared;
    std::string              _key; ///< key for AES encryption (16 bytes)
    QString                 _flightID;

    float                   _lastHdop = 1.f;
};


class AirMapTrafficMonitor : public QObject, public LifetimeChecker
{
    Q_OBJECT
public:
    AirMapTrafficMonitor(AirMapSharedState& shared)
    : _shared(shared)
    {
    }
    virtual ~AirMapTrafficMonitor();

    void startConnection(const QString& flightID);

    void stop();

signals:
    void error(const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);
    void trafficUpdate(QString traffic_id, QString vehicle_id, QGeoCoordinate location, float heading);

private:
    void _update(airmap::Traffic::Update::Type type, const std::vector<airmap::Traffic::Update>& update);

private:
    QString                                               _flightID;
    AirMapSharedState&                                    _shared;
    std::shared_ptr<airmap::Traffic::Monitor>             _monitor;
    std::shared_ptr<airmap::Traffic::Monitor::Subscriber> _subscriber;
};



/// AirMap per vehicle management class.
class AirMapManagerPerVehicle : public AirspaceManagerPerVehicle
{
    Q_OBJECT
public:
    AirMapManagerPerVehicle(AirMapSharedState& shared, const Vehicle& vehicle, QGCToolbox& toolbox);
    virtual ~AirMapManagerPerVehicle() = default;


    void createFlight(const QList<MissionItem*>& missionItems) override;

    AirspaceAuthorization::PermitStatus flightPermitStatus() const override;

    void startTelemetryStream() override;

    void stopTelemetryStream() override;

    bool isTelemetryStreaming() const override;

signals:
    void error(const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);

public slots:
    void endFlight() override;

protected slots:
    virtual void vehicleMavlinkMessageReceived(const mavlink_message_t& message) override;
private slots:
    void _flightPermitStatusChanged();
private:
    AirMapSharedState&           _shared;

    AirMapFlightManager          _flightManager;
    AirMapTelemetry              _telemetry;
    AirMapTrafficMonitor         _trafficMonitor;

    QGCToolbox&                  _toolbox;
};


class AirMapManager : public AirspaceManager
{
    Q_OBJECT
    
public:
    AirMapManager(QGCApplication* app, QGCToolbox* toolbox);
    virtual ~AirMapManager();

    void setToolbox(QGCToolbox* toolbox) override;

    AirspaceManagerPerVehicle* instantiateVehicle(const Vehicle& vehicle) override;

    AirspaceRestrictionProvider* instantiateRestrictionProvider() override;

    QString name() const override { return "AirMap"; }

    void requestWeatherUpdate(const QGeoCoordinate& coordinate) override;

private slots:
    void _error(const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);

    void _settingsChanged();
private:

    AirMapSharedState _shared;

    std::shared_ptr<airmap::qt::Logger> _logger;
    std::shared_ptr<airmap::qt::DispatchingLogger> _dispatchingLogger;
};


