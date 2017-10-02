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
#include "AirspaceManagement.h"

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

/**
 * @class AirMapNetworking
 * Handles networking requests (GET & POST), with login if required.
 * There can only be one active request per object instance.
 */
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

    /**
     * abort the current request (_requestFinished() or _requestError() will not be emitted)
     */
    void abort();

    bool hasAPIKey() const { return _networkingData.airmapAPIKey != ""; }

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

    QNetworkReply* _currentNetworkReply = nullptr;
};


/// class to download polygons from AirMap
class AirMapRestrictionManager : public AirspaceRestrictionProvider
{
    Q_OBJECT
public:
    AirMapRestrictionManager(AirMapNetworking::SharedData& sharedData);

    void setROI(const QGeoCoordinate& center, double radiusMeters) override;

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

    /**
     * abort the current operation
     */
    void abort();

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

    bool isTelemetryStreaming() const;

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



/// AirMap per vehicle management class.
class AirMapManagerPerVehicle : public AirspaceManagerPerVehicle
{
    Q_OBJECT
public:
    AirMapManagerPerVehicle(AirMapNetworking::SharedData& sharedData, const Vehicle& vehicle, QGCToolbox& toolbox);
    virtual ~AirMapManagerPerVehicle() = default;


    void createFlight(const QList<MissionItem*>& missionItems) override;

    AirspaceAuthorization::PermitStatus flightPermitStatus() const override;

    void startTelemetryStream() override;

    void stopTelemetryStream() override;

    bool isTelemetryStreaming() const override;

signals:
    void networkError(QNetworkReply::NetworkError code, const QString& errorString, const QString& serverErrorMessage);

public slots:
    void endFlight() override;

    void settingsChanged();

protected slots:
    virtual void vehicleMavlinkMessageReceived(const mavlink_message_t& message) override;
private slots:
    void _flightPermitStatusChanged();
private:
    AirMapNetworking             _networking;

    AirMapFlightManager          _flightManager;
    AirMapTelemetry              _telemetry;
    AirMapTrafficAlertClient     _trafficAlerts;

    QGCToolbox&                  _toolbox;
};


class AirMapManager : public AirspaceManager
{
    Q_OBJECT
    
public:
    AirMapManager(QGCApplication* app, QGCToolbox* toolbox);
    virtual ~AirMapManager() = default;

    void setToolbox(QGCToolbox* toolbox) override;

    AirspaceManagerPerVehicle* instantiateVehicle(const Vehicle& vehicle) override;

    AirspaceRestrictionProvider* instantiateRestrictionProvider() override;

    QString name() const override { return "AirMap"; }

signals:
    void settingsChanged();

private slots:
    void _networkError(QNetworkReply::NetworkError code, const QString& errorString, const QString& serverErrorMessage);

    void _settingsChanged();
private:

    AirMapNetworking::SharedData _networkingData;
};

#endif
