/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapManager.h"
#include "AirMapSharedState.h"

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

void
AirMapSharedState::login()
{
    if (isLoggedIn() || _isLoginInProgress) {
        return;
    }
    _isLoginInProgress = true;
    if (_settings.userName == "") { //use anonymous login

        Authenticator::AuthenticateAnonymously::Params params;
        params.id = "";
        _client->authenticator().authenticate_anonymously(params,
                [this](const Authenticator::AuthenticateAnonymously::Result& result) {
            if (!_isLoginInProgress) { // was logout() called in the meanwhile?
                return;
            }
            if (result) {
                qCDebug(AirMapManagerLog)<<"Successfully authenticated with AirMap: id="<< result.value().id.c_str();
                _loginToken = QString::fromStdString(result.value().id);
                _processPendingRequests();
            } else {
                _pendingRequests.clear();
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
        _client->authenticator().authenticate_with_password(params,
                [this](const Authenticator::AuthenticateWithPassword::Result& result) {
            if (!_isLoginInProgress) { // was logout() called in the meanwhile?
                return;
            }
            if (result) {
                qCDebug(AirMapManagerLog)<<"Successfully authenticated with AirMap: id="<< result.value().id.c_str()<<", access="
                        <<result.value().access.c_str();
                _loginToken = QString::fromStdString(result.value().id);
                _processPendingRequests();
            } else {
                _pendingRequests.clear();
                QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
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
    _loginToken = "";
    _pendingRequests.clear();

}


