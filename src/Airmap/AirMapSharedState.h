/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QQueue>

#include "AirspaceManager.h"

#include <Airmap/services/client.h>

/**
 * @class AirMapSharedState
 * Contains state & settings that need to be shared (such as login)
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

    void                setSettings         (const Settings& settings);
    const Settings&     settings            () const { return _settings; }
    void                setClient           (airmap::services::Client* client) { _client = client; }

    QString             pilotID             () { return _pilotID; }
    void                setPilotID          (const QString& pilotID) { _pilotID = pilotID; }

    /**
     * Get the current client instance. It can be NULL. If not NULL, it implies
     * there's an API key set.
     */
    airmap::services::Client*   client              () const { return _client; }
    bool                        hasAPIKey           () const { return _settings.apiKey != ""; }
    bool                        isLoggedIn          () const { return _loginToken != ""; }

    using Callback = std::function<void(const QString& /* login_token */)>;

    /**
     * Do a request that requires user login: if not yet logged in, the request is queued and
     * processed after successful login, otherwise it's executed directly.
     */
    void                doRequestWithLogin  (const Callback& callback);
    void                login               ();
    void                logout              ();
    const QString&      loginToken          () const { return _loginToken; }

signals:
    void    error       (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);
    void    authStatus  (AirspaceManager::AuthStatus status);

private:
    void _processPendingRequests            ();

private:
    bool                        _isLoginInProgress = false;
    QString                     _loginToken;        ///< login token: empty when not logged in
    QString                     _pilotID;
    airmap::services::Client*   _client = nullptr;
    Settings                    _settings;
    QQueue<Callback>            _pendingRequests;   ///< pending requests that are processed after a successful login
};

