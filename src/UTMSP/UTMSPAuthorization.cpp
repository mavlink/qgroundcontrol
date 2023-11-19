/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <boost/beast/version.hpp>
#include <nlohmann/json.hpp>

#include "UTMSPLogger.h"
#include "UTMSPAuthorization.h"

using json = nlohmann::ordered_json;

std::string UTMSPAuthorization::_clientID;
std::string UTMSPAuthorization::_clientSecret;
std::string UTMSPAuthorization::_clientToken;

UTMSPAuthorization::UTMSPAuthorization():
    UTMSPRestInterface("passport.utm.dev.airoplatform.com")
{

}

UTMSPAuthorization::~UTMSPAuthorization()
{

}

bool UTMSPAuthorization::requestOAuth2Client(const QString &clientID, const QString &clientSecret)
{
    // Convert QString to std::string
    _clientID = clientID.toStdString();
    _clientSecret = clientSecret.toStdString();

    // Generate the basic Token
    std::string combinedCredential = _clientID + ":" + _clientSecret;
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, combinedCredential.c_str(), combinedCredential.length());
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    BIO_set_close(bio, BIO_NOCLOSE);
    BIO_free_all(bio);

    std::string encodedBasicToken(bufferPtr->data, bufferPtr->length);
    BUF_MEM_free(bufferPtr);
    setBasicToken(encodedBasicToken);

    // Get the Access Token
    setHost("AuthClient");
    connectNetwork();
    const std::string target = "/oauth/token/";
    std::string body = "grant_type=client_credentials&scope=blender.write blender.read&audience=blender.utm.dev.airoplatform.com&client_id=" + _clientID + "&client_secret=" + _clientSecret + "\r\n\r\n";
    modifyRequest(target, http::verb::post, body);
    auto [status, response] = executeRequest();
    UTMSP_LOG_INFO() << "UTMSPAuthorization: Authorization Response: " << response;

    if(status == 200)
    {
        try {
            json responseJson = json::parse(response);
            _clientToken = responseJson["access_token"];
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

std::string UTMSPAuthorization::getOAuth2Token()
{
    return _clientToken;
}
