/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef AirMapManager_H
#define AirMapManager_H

#include "QGCToolbox.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "MissionItem.h"
#include "MultiVehicleManager.h"

#include <qmqtt.h>

#include <QGeoCoordinate>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QUdpSocket>
#include <QHostInfo>
#include <QHostAddress>

#include <stdint.h>

Q_DECLARE_LOGGING_CATEGORY(AirMapManagerLog)


class AirspaceRestriction : public QObject
{
    Q_OBJECT

public:
    AirspaceRestriction(QObject* parent = NULL);
};

class PolygonAirspaceRestriction : public AirspaceRestriction
{
    Q_OBJECT

public:
    PolygonAirspaceRestriction(const QVariantList& polygon, QObject* parent = NULL);

    Q_PROPERTY(QVariantList polygon MEMBER _polygon CONSTANT)

    const QVariantList& getPolygon() const { return _polygon; }

private:
    QVariantList    _polygon;
};

class CircularAirspaceRestriction : public AirspaceRestriction
{
    Q_OBJECT

public:
    CircularAirspaceRestriction(const QGeoCoordinate& center, double radius, QObject* parent = NULL);

    Q_PROPERTY(QGeoCoordinate   center MEMBER _center CONSTANT)
    Q_PROPERTY(double           radius MEMBER _radius CONSTANT)

private:
    QGeoCoordinate  _center;
    double          _radius;
};


class AirMapLogin : public QObject
{
    Q_OBJECT
public:
    /**
     * @param networkManager
     * @param APIKey AirMap API key: this is stored as a reference, and thus must live as long as this object does
     */
    AirMapLogin(QNetworkAccessManager& networkManager, const QString& APIKey);

    void setCredentials(const QString& clientID, const QString& userName, const QString& password);

    /**
     * check if the credentials are set (not necessarily valid)
     */
    bool hasCredentials() const { return _userName != "" && _password != ""; }

    void login();
    void logout() { _JWTToken = ""; }

    /** get the JWT token. Empty if user not logged in */
    const QString& JWTToken() const { return _JWTToken; }

    bool isLoggedIn() const { return _JWTToken != ""; }

signals:
    void loginSuccess();
    void loginFailure(QNetworkReply::NetworkError error, const QString& errorString, const QString& serverErrorMessage);

private slots:
    void _requestFinished(void);
    void _requestError(QNetworkReply::NetworkError code);

private:
    void _post(QUrl url, const QByteArray& postData);

    QNetworkAccessManager& _networkManager;

    bool _isLoginInProgress = false;
    QString _JWTToken = ""; ///< JWT login token: empty when not logged in
    const QString& _APIKey;

    // login credentials
    QString _clientID;
    QString _userName;
    QString _password;
};

class AirMapNetworking : public QObject
{
    Q_OBJECT
public:

    struct SharedData {
        SharedData() : login(networkManager, airmapAPIKey) {}

        QNetworkAccessManager networkManager;
        QString airmapAPIKey;

        AirMapLogin login;
    };

    AirMapNetworking(SharedData& networkingData);

    /**
     * send a GET request
     * @param url
     * @param requiresLogin set to true if the user needs to be logged in for the request
     */
    void get(QUrl url, bool requiresLogin = false);

    /**
     * send a POST request
     * @param url
     * @param postData
     * @param isJsonData if true, content type is set to JSON, form data otherwise
     * @param requiresLogin set to true if the user needs to be logged in for the request
     */
    void post(QUrl url, const QByteArray& postData, bool isJsonData = false, bool requiresLogin = false);

    const QString& JWTLoginToken() const { return _networkingData.login.JWTToken(); }

    const AirMapLogin& getLogin() const { return _networkingData.login; }

signals:
    /// signal when the request finished (get or post). All requests are assumed to return JSON.
    void finished(QJsonParseError parseError, QJsonDocument document);
    void error(QNetworkReply::NetworkError code, const QString& errorString, const QString& serverErrorMessage);

private slots:
    void _loginSuccess();
    void _loginFailure(QNetworkReply::NetworkError networkError, const QString& errorString, const QString& serverErrorMessage);
    void _requestFinished(void);
private:
    SharedData& _networkingData;

    enum class RequestType {
        None,
        GET,
        POST
    };
    struct PendingRequest {
        RequestType type = RequestType::None;
        QUrl url;
        QByteArray postData;
        bool isJsonData;
        bool requiresLogin;
    };
    PendingRequest _pendingRequest;
};


/// class to download polygons from AirMap
class AirspaceRestrictionManager : public QObject
{
    Q_OBJECT
public:
    AirspaceRestrictionManager(AirMapNetworking::SharedData& sharedData);

    void updateROI(const QGeoCoordinate& center, double radiusMeters);

    QmlObjectListModel* polygonRestrictions(void) { return &_polygonList; }
    QmlObjectListModel* circularRestrictions(void) { return &_circleList; }

signals:
    void networkError(QNetworkReply::NetworkError code, const QString& errorString, const QString& serverErrorMessage);
private slots:
    void _parseAirspaceJson(QJsonParseError parseError, QJsonDocument airspaceDoc);
    void _error(QNetworkReply::NetworkError code, const QString& errorString, const QString& serverErrorMessage);
private:

    enum class State {
        Idle,
        RetrieveList,
        RetrieveItems,
    };

    State                   _state = State::Idle;
    int                     _numAwaitingItems = 0;
    AirMapNetworking        _networking;

    QmlObjectListModel      _polygonList;
    QmlObjectListModel      _circleList;
    QList<PolygonAirspaceRestriction*> _nextPolygonList;
    QList<CircularAirspaceRestriction*> _nextcircleList;
};


class AirspaceAuthorization : public QObject {
    Q_OBJECT
public:
    enum PermitStatus {
        PermitUnknown = 0,
        PermitPending,
        PermitAccepted,
        PermitRejected,
    };
    Q_ENUMS(PermitStatus);
};

/// class to upload a flight
class AirMapFlightManager : public QObject
{
    Q_OBJECT
public:
    AirMapFlightManager(AirMapNetworking::SharedData& sharedData);

    /// Send flight path to AirMap
    void createFlight(const QList<MissionItem*>& missionItems);

    AirspaceAuthorization::PermitStatus flightPermitStatus() const { return _flightPermitStatus; }

    const QString& flightID() const { return _currentFlightId; }

    void setSitaUavRegistrationId(const QString& sitaUavRegistrationId) {
        _sitaUavRegistrationId = sitaUavRegistrationId;
    }
    void setSitaPilotRegistrationId(const QString& sitaPilotRegistrationId) {
        _sitaPilotRegistrationId = sitaPilotRegistrationId;
    }

public slots:
    void endFlight();

signals:
    void networkError(QNetworkReply::NetworkError code, const QString& errorString, const QString& serverErrorMessage);
    void flightPermitStatusChanged();

private slots:
    void _parseJson(QJsonParseError parseError, QJsonDocument doc);
    void _error(QNetworkReply::NetworkError code, const QString& errorString, const QString& serverErrorMessage);

    void _sendBriefingRequest();
private:

    /**
     * upload flight stored in _flight
     */
    void _uploadFlight();

    /**
     * implementation of endFlight()
     */
    void _endFlight(const QString& flightID);

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
    AirMapNetworking                    _networking;
    QString                             _currentFlightId; ///< Flight ID, empty if there is none
    QString                             _pendingFlightId; ///< current flight ID, not necessarily accepted yet (once accepted, it's equal to _currentFlightId)
    QString                             _pendingFlightPlan; ///< current flight plan, waiting to be submitted
    AirspaceAuthorization::PermitStatus _flightPermitStatus = AirspaceAuthorization::PermitUnknown;
    QString                             _pilotID; ///< Pilot ID in the form "auth0|abc123"
    bool                                _noFlightCreatedYet = true;
    QTimer                              _pollTimer; ///< timer to poll for approval check

    QString                             _sitaUavRegistrationId;
    QString                             _sitaPilotRegistrationId;
};

/// class to send telemetry data to AirMap
class AirMapTelemetry : public QObject
{
    Q_OBJECT
public:
    AirMapTelemetry(AirMapNetworking::SharedData& sharedData);
    virtual ~AirMapTelemetry();

    /**
     * Setup the connection to start sending telemetry
     */
    void startTelemetryStream(const QString& flightID);

    void stopTelemetryStream();

signals:
    void networkError(QNetworkReply::NetworkError code, const QString& errorString, const QString& serverErrorMessage);

public slots:
    void vehicleMavlinkMessageReceived(const mavlink_message_t& message);

private slots:
    void _parseJson(QJsonParseError parseError, QJsonDocument doc);
    void _error(QNetworkReply::NetworkError code, const QString& errorString, const QString& serverErrorMessage);
    void _udpTelemetryHostLookup(QHostInfo info);

private:

    void _handleGlobalPositionInt(const mavlink_message_t& message);
    void _handleGPSRawInt(const mavlink_message_t& message);

    bool _isTelemetryStreaming() const;

    enum class State {
        Idle,
        StartCommunication,
        EndCommunication,
        Streaming,
    };

    State                   _state = State::Idle;

    AirMapNetworking        _networking;
    QByteArray              _key; ///< key for AES encryption, 16 bytes
    QString                 _flightID;
    uint32_t                _seqNum = 1;
    QUdpSocket*             _socket = nullptr;
    QHostAddress            _udpHost;
    static constexpr int    _udpPort = 32003;

    float                   _lastHdop = 1.f;
};


class AirMapTrafficAlertClient : public QMQTT::Client
{
    Q_OBJECT
public:
    AirMapTrafficAlertClient(const QString& host, const quint16 port, QObject* parent = NULL)
        : QMQTT::Client(host, port, QSslConfiguration::defaultConfiguration(), true, parent)
    {
        connect(this, &AirMapTrafficAlertClient::connected, this, &AirMapTrafficAlertClient::onConnected);
        connect(this, &AirMapTrafficAlertClient::subscribed, this, &AirMapTrafficAlertClient::onSubscribed);
        connect(this, &AirMapTrafficAlertClient::received, this, &AirMapTrafficAlertClient::onReceived);
        connect(this, &AirMapTrafficAlertClient::error, this, &AirMapTrafficAlertClient::onError);
    }
    virtual ~AirMapTrafficAlertClient() = default;

    void startConnection(const QString& flightID, const QString& password);

signals:
    void trafficUpdate(QString traffic_id, QString vehicle_id, QGeoCoordinate location, float heading);

private slots:

    void onError(const QMQTT::ClientError error);

    void onConnected();

    void onSubscribed(const QString& topic);

    void onReceived(const QMQTT::Message& message);

private:
    QString _flightID;
};




/// AirMap server communication support.
class AirMapManager : public QGCTool
{
    Q_OBJECT
    
public:
    AirMapManager(QGCApplication* app, QGCToolbox* toolbox);
    ~AirMapManager();

    /// Set the ROI for airspace information
    ///     @param center Center coordinate for ROI
    ///     @param radiusMeters Radius in meters around center which is of interest
    void setROI(QGeoCoordinate& center, double radiusMeters);

    /// Send flight path to AirMap
    void createFlight(const QList<MissionItem*>& missionItems);


    QmlObjectListModel* polygonRestrictions(void) { return _airspaceRestrictionManager.polygonRestrictions(); }
    QmlObjectListModel* circularRestrictions(void) { return _airspaceRestrictionManager.circularRestrictions(); }

    void setToolbox(QGCToolbox* toolbox) override;

    AirspaceAuthorization::PermitStatus flightPermitStatus() const { return _flightManager.flightPermitStatus(); }

signals:
    void flightPermitStatusChanged();

    void trafficUpdate(QString traffic_id, QString vehicle_id, QGeoCoordinate location, float heading);

private slots:
    void _updateToROI(void);
    void _networkError(QNetworkReply::NetworkError code, const QString& errorString, const QString& serverErrorMessage);

    void _activeVehicleChanged(Vehicle* activeVehicle);
    void _vehicleArmedChanged(bool armed);

    void _flightPermitStatusChanged();

private:
    bool _hasAPIKey() const { return _networkingData.airmapAPIKey != ""; }

    /**
     * A new vehicle got connected, listen to its state
     */
    void _connectVehicle(Vehicle* vehicle);

    AirMapNetworking::SharedData _networkingData;
    AirspaceRestrictionManager   _airspaceRestrictionManager;
    AirMapFlightManager          _flightManager;
    AirMapTelemetry              _telemetry;
    AirMapTrafficAlertClient     _trafficAlerts;

    QGeoCoordinate          _roiCenter;
    double                  _roiRadius;

    QTimer                  _updateTimer;

    Vehicle*                _vehicle = nullptr; ///< current vehicle
    bool                    _vehicleWasInMissionMode = false; ///< true if the vehicle was in mission mode when arming
};

#endif
