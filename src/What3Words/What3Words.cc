#include "What3Words.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QUrl>
#include <QVariantMap>

class What3Words::What3WordsImpl {
public:
    bool convertToGeoCoordinate(const QString &address, QGeoCoordinate &geoCoordinate)
    {
        if (!validateWhat3WordsAddress(address)) {
            setLastError("validateWhat3WordsAddress failed: " + lastError());
            return false;
        }

        const QString requestUrl = QString("https://api.what3words.com/v3/convert-to-coordinates?words=%1&key=%2")
                .arg(address, _apiKey);

        QString responseContent { };

        if (!getApiResponse(requestUrl, responseContent)) {
            setLastError("getApiResponse failed: " + lastError());
            return false;
        }

        if (!convertApiResponseToGeoCoordinate(responseContent, geoCoordinate)) {
            setLastError("convertApiResponseToGeoCoordinate failed: " + lastError());
            return false;
        }

        return true;
    }

    bool convertToWhat3WordsAddress(const QGeoCoordinate &geoCoordinate, QString &what3wordsAddress)
    {
        if (!validateGeoCordinate(geoCoordinate)) {
            setLastError("validateGeoCordinate failed: " + lastError());
            return false;
        }

        const QString requestUrl = QString("https://api.what3words.com/v3/convert-to-3wa?coordinates=%1,%2&key=%3")
                .arg(QString::number(geoCoordinate.latitude()), QString::number(geoCoordinate.longitude()), _apiKey);

        QString responseContent { };

        if (!getApiResponse(requestUrl, responseContent)) {
            setLastError("getApiResponse failed: " + lastError());
            return false;
        }

        if (!convertApiResponseToWhat3WordsAddress(responseContent, what3wordsAddress)) {
            setLastError("convertApiResponseToWhat3WordsAddress failed: " + lastError());
            return false;
        }

        return true;
    }

    void setApiKey(const QString &apiKey)
    {
        _apiKey = apiKey;
    }

    void setLastError(const QString &error)
    {
        _errorMessage = error;
    }

    QString lastError() const
    {
        return _errorMessage;
    }

private:
    bool validateWhat3WordsAddress(const QString &address)
    {
        if (address.isEmpty() || address.isNull()) {
            setLastError("address is empty or null");
            return false;
        }

        if (!_addressRegex.match(address).hasMatch()) {
            setLastError("wrong format");
            return false;
        }

        return true;
    }

    bool validateGeoCordinate(const QGeoCoordinate &geoCoordinate)
    {
        if (!geoCoordinate.isValid()) {
            setLastError("geocoordinate is not valid");
            return false;
        }

        return true;
    }

    bool getApiResponse(const QString &url, QString &requestResponse)
    {
        QNetworkRequest networkRequest(url);
        QNetworkReply *networkReply = _networkAccessManager.get(networkRequest);

        QEventLoop loop;
        QObject::connect(networkReply, SIGNAL(readyRead()), &loop, SLOT(quit()));
        loop.exec();

        int httpStatusCode = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        switch (httpStatusCode)
        {
        case 200:
            requestResponse = networkReply->readAll();
            return true;
        case 400:
            setLastError("bad request (duplicate/invalid parameter)");
            return false;
        case 401:
            setLastError("missing api key");
            return false;
        case 404:
            setLastError("url not found");
            return false;
        case 405:
            setLastError("method not allowed");
            return false;
        case 500:
            setLastError("server error");
            return false;
        }
        return true;
    }

    bool convertApiResponseToGeoCoordinate(const QString &response, QGeoCoordinate &coordinate)
    {
        QJsonDocument jsonDocument = QJsonDocument::fromJson(response.toUtf8());
        QJsonObject rootObject = jsonDocument.object();
        QJsonObject coordinatesObject = rootObject["coordinates"].toObject();
        QVariantMap coordinatesVarMap = coordinatesObject.toVariantMap();

        double lat = coordinatesVarMap["lat"].toDouble();
        coordinate.setLatitude(lat);

        double lng = coordinatesVarMap["lng"].toDouble();
        coordinate.setLongitude(lng);

        return true;
    }

    bool convertApiResponseToWhat3WordsAddress(const QString &response, QString &address)
    {
        QJsonDocument jsonDocument = QJsonDocument::fromJson(response.toUtf8());
        QJsonObject rootObject = jsonDocument.object();
        QVariantMap rootObjectVarMap = rootObject.toVariantMap();

        address = rootObjectVarMap["words"].toString();

        return true;
    }

    QString                 _apiKey;
    QString                 _errorMessage;
    QNetworkAccessManager   _networkAccessManager;
    QRegularExpression      _addressRegex {"[a-z]+\\.[a-z]+\\.[a-z]+"};
};

What3Words::What3Words(QObject *parent) :
    QObject (parent),
    impl_(new What3WordsImpl())
{
}

What3Words::~What3Words()
{
}

bool What3Words::convertToGeoCoordinate(const QString &address, QGeoCoordinate &geoCoordinate)
{
    return impl_->convertToGeoCoordinate(address, geoCoordinate);
}

bool What3Words::convertToWhat3WordsAddress(const QGeoCoordinate &geoCoordinate, QString &what3wordsAddress)
{
    return impl_->convertToWhat3WordsAddress(geoCoordinate, what3wordsAddress);
}

void What3Words::setApiKey(const QString &apiKey)
{
    return impl_->setApiKey(apiKey);
}

QString What3Words::lastError()
{
    return impl_->lastError();
}
