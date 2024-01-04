/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include<QString>
#include <string>

#include "UTMSPRestInterface.h"

class UTMSPAuthorization: public QObject, public UTMSPRestInterface
{
    Q_OBJECT
public:
    UTMSPAuthorization();
    ~UTMSPAuthorization();

    const std::string& getOAuth2Token();

protected slots:
    bool requestOAuth2Client(const QString& clientID, const QString& clientSecret);

protected:
    http::request<http::string_body> _request;

private:
    std::string          _clientID;
    std::string          _clientSecret;
    bool                 _isValidToken;
};
