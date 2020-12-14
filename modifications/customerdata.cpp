#include "customerdata.h"
#include "Vehicle.h"

bool successfulLogin = false;
bool droneStatusCheck = false;
static QByteArray globalToken; // Global variable for token outside the class

// Singleton
static CustomerData* customer = nullptr;
CustomerData*
getInstance()
{
    if(!customer)
        customer = new CustomerData();
    return customer;
}
CustomerData::CustomerData(QObject* parent): QObject(parent)
{
}

void CustomerData::get(QString location)
{
    qInfo() << "Getting from Server.....";
    QNetworkReply* reply = manager.get(QNetworkRequest(QUrl(location)));
    connect(reply, &QNetworkReply::readyRead, this, &CustomerData::readyRead);
}

void CustomerData::postEmailPass(QString location, QByteArray data)
{

    qInfo() << "Posting to Server.....";
    QNetworkRequest request = QNetworkRequest(QUrl(location));
    request.setRawHeader("Content-Type", "application/json");
    QNetworkReply* reply = manager.post(request, data);
    connect(reply, &QNetworkReply::finished, this, &CustomerData::readyRead);
}


void CustomerData::postOTP(QString location, QByteArray data)
{
    qInfo() << "Posting OTP to Server.....";
    QNetworkRequest request = QNetworkRequest(QUrl(location));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("loingauth", authCode);
    QNetworkReply* reply = manager.post(request, data);
    connect(reply, &QNetworkReply::readyRead, this, &CustomerData::readyReadOTP);
}

// for posting drone number
void CustomerData::postDroneNo(QString location, QByteArray data)
{
    qInfo() <<"Posting drone number to Server...";
    QNetworkRequest request = QNetworkRequest(QUrl(location));
    request.setHeader(QNetworkRequest::ContentTypeHeader , "application/json");
    request.setRawHeader("auth",globalToken);
    QNetworkReply* reply = manager.post(request,data);
    connect(reply,&QNetworkReply::readyRead, this, &CustomerData::readyReadDroneNo);

}


void CustomerData::readyRead()
{
    qInfo() << "Ready Read";
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray replyArray = reply->readAll(), m_authCode;
    qInfo() << replyArray;
    int idx = replyArray.lastIndexOf("loginToken");
    int idx1 = replyArray.lastIndexOf("error");
    if(idx == -1 || idx1 != -1){
        emit wrongDetails();
        return;
    }
    idx += 13;
    for(int i=idx; i < replyArray.size()-2; i++){
        m_authCode.append(replyArray[i]);
    }
    this->authCode = m_authCode;
    emit correctDetails();

}

void CustomerData::readyReadOTP()
{
    qInfo() << "Ready Read OTP ";
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray replyArray = reply->readAll(), m_token;
    int idx = replyArray.lastIndexOf("error");
    int idx1 = replyArray.lastIndexOf("accessTokens");
    if(idx != -1){
        emit wrongOTP();
        return;
    }
    idx1 += 15;
    for(int i=idx1; i<replyArray.size()-1; i++){
        m_token.append(replyArray[i]);
    }
    qInfo() << m_token;
    // extracting tokenDroneNo for "auth":header session value
    tokenDroneNo.clear();
    for(int i =0;i<m_token.size()-1;i++){
        tokenDroneNo.append(m_token[i]);
    }
    this->token = m_token;
    this->authCode.clear();
    globalToken = tokenDroneNo;
    successfulLogin = true;
    emit correctOTP();
}

//extracting reply after validating drone number
void CustomerData::readyReadDroneNo()
{
    qInfo() <<"Ready Read Drone";
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray replyArray = reply->readAll();
    qInfo() << replyArray;
    if(replyArray == "Your drone is not registered"){
            emit droneNotRegistered();
        }
    else if(replyArray.contains("modal")){
            emit droneRegistered();
        }

}

void CustomerData::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(reply)
    Q_UNUSED(authenticator)
    qInfo() << "authenticationRequired";
}

void CustomerData::encrypted(QNetworkReply *reply)
{
    Q_UNUSED(reply)
    qInfo() << "encrypted";
}

void CustomerData::finished(QNetworkReply *reply)
{
    Q_UNUSED(reply)
    qInfo() << "finished";
}

void CustomerData::preSharedKeyAuthenticationRequired(QNetworkReply *reply, QSslPreSharedKeyAuthenticator *authenticator)
{
    Q_UNUSED(reply)
    Q_UNUSED(authenticator)
    qInfo() << "preSharedKeyAuthenticationRequired";
}

void CustomerData::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
    Q_UNUSED(proxy)
    Q_UNUSED(authenticator)
    qInfo() << "proxyAuthenticationRequired";
}

void CustomerData::sslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    Q_UNUSED(reply)
    Q_UNUSED(errors)
    qInfo() << "sslErrors";
}

extern QObject *singletonProvider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return getInstance();
}


