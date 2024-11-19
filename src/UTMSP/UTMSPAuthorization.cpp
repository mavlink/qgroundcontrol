/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

// #include <nlohmann/json.hpp>
#include <string>
#include <QByteArray>
#include <QBitArray>
#include <QString>

#include "UTMSPLogger.h"
#include "UTMSPAuthorization.h"
#include "parse/nlohmann/json.hpp"

using json = nlohmann::ordered_json;

UTMSPAuthorization::UTMSPAuthorization(QObject *parent):
    QObject(parent)
{

}

thread_local std::string clientToken = "";

bool UTMSPAuthorization::requestOAuth2Client(const QString &clientID, const QString &clientSecret)
{
    QString combinedCredential = clientID + ":" + clientSecret;
    QString encodedBasicToken = combinedCredential.toUtf8().toBase64();
    _utmspRestInterface.setBasicToken(encodedBasicToken);
    _utmspRestInterface.setHost(UTMSPRestInterface::HostTarget::AuthClient);
    const QString target = "/oauth/token/";
    QString body = "grant_type=client_credentials&scope=blender.write blender.read&audience=testflight.flightblender.com&client_id=" + clientID + "&client_secret=" + clientSecret + "\r\n\r\n";
    _utmspRestInterface.modifyRequest(target, QNetworkAccessManager::PostOperation, body);
    auto [status, response] = _utmspRestInterface.executeRequest();
    UTMSP_LOG_INFO() << "UTMSPAuthorization: Authorization Response: " << response;

    if(status == 200)
    {
        try {
            json responseJson = json::parse(response);
            clientToken = responseJson["access_token"];
            _isValidToken = true;
        }
        catch (const json::parse_error& e) {
            UTMSP_LOG_ERROR() << "UTMSPAuthorization: Invalid Token: " << e.what();
            _isValidToken = false;
        }
    }
    else
    {
        UTMSP_LOG_ERROR() << "UTMSPAuthorization: Invalid Status Code ";
        _isValidToken = false;
    }

    return _isValidToken;
}

const std::string &UTMSPAuthorization::getOAuth2Token()
{
    return clientToken;
}
