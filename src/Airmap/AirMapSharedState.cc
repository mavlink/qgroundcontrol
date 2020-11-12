/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapSharedState.h"
#include "AirMapManager.h"

#include "airmap/authenticator.h"
#include "qjsonwebtoken.h"

using namespace airmap;

void
AirMapSharedState::setSettings(const Settings& settings)
{
    logout();
    _settings = settings;
}

void
AirMapSharedState::doRequestWithLogin(const Callback& callback)
{
    if (isLoggedIn()) {
        callback(_loginToken);
    } else {
        login();
        _pendingRequests.enqueue(callback);
    }
}

//-- TODO:
//   For now, only anonymous login collects the (anonymous) pilot ID within login()
//   For autheticated logins, we need to collect it here as opposed to spread all over
//   the place as it is the case now.

void
AirMapSharedState::login()
{
    if (isLoggedIn() || _isLoginInProgress) {
        return;
    }
    _isLoginInProgress = true;
    if (_settings.userName == "") { //use anonymous login
        qCDebug(AirMapManagerLog) << "Anonymous authentication";
        Authenticator::AuthenticateAnonymously::Params params;
        params.id = "Anonymous";
        _client->authenticator().authenticate_anonymously(params,
                [this](const Authenticator::AuthenticateAnonymously::Result& result) {
            if (!_isLoginInProgress) { // was logout() called in the meanwhile?
                return;
            }
            if (result) {
                qCDebug(AirMapManagerLog) << "Successfully authenticated with AirMap: id="<< result.value().id.c_str();
                emit authStatus(AirspaceManager::AuthStatus::Anonymous);
                _loginToken = QString::fromStdString(result.value().id);
                QJsonWebToken token = QJsonWebToken::fromTokenAndSecret(_loginToken, QString());
                QJsonDocument doc = token.getPayloadJDoc();
                QJsonObject json = doc.object();
                _pilotID = json.value("sub").toString();
                qCDebug(AirMapManagerLog) << "Anonymous pilot id:" << _pilotID;
                _processPendingRequests();
            } else {
                _pendingRequests.clear();
                emit authStatus(AirspaceManager::AuthStatus::Error);
                QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
                emit error("Failed to authenticate with AirMap",
                        QString::fromStdString(result.error().message()), description);
            }
        });
    } else {
        Authenticator::AuthenticateWithPassword::Params params;
        params.oauth.username = _settings.userName.toStdString();
        params.oauth.password = _settings.password.toStdString();
        params.oauth.client_id = _settings.clientID.toStdString();
        params.oauth.device_id = "QGroundControl";
        qCDebug(AirMapManagerLog) << "User authentication" << _settings.userName;
        _client->authenticator().authenticate_with_password(params,
                [this](const Authenticator::AuthenticateWithPassword::Result& result) {
            if (!_isLoginInProgress) { // was logout() called in the meanwhile?
                return;
            }
            if (result) {
                qCDebug(AirMapManagerLog) << "Successfully authenticated with AirMap: id="<< result.value().id.c_str()<<", access="
                        <<result.value().access.c_str();
                emit authStatus(AirspaceManager::AuthStatus::Authenticated);
                _loginToken = QString::fromStdString(result.value().id);
                _processPendingRequests();
            } else {
                _pendingRequests.clear();
                QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
                emit authStatus(AirspaceManager::AuthStatus::Error);
                emit error("Failed to authenticate with AirMap",
                        QString::fromStdString(result.error().message()), description);
            }
        });
    }
}

void
AirMapSharedState::_processPendingRequests()
{
    while (!_pendingRequests.isEmpty()) {
        _pendingRequests.dequeue()(_loginToken);
    }
}

void
AirMapSharedState::logout()
{
    _isLoginInProgress = false; // cancel if we're currently trying to login
    if (!isLoggedIn()) {
        return;
    }
    _pilotID.clear();
    _loginToken.clear();
    _pendingRequests.clear();
}


