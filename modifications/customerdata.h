#ifndef CUSTOMERDATA_H
#define CUSTOMERDATA_H

#include <QObject>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QNetworkProxy>
#include <QByteArray>
#include <QJsonValue>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>
#include "Vehicle.h"
class CustomerData: public QObject
{
    Q_OBJECT

public:
       explicit CustomerData(QObject* parent=nullptr);

signals:
    void correctDetails();
    void wrongDetails();
    void correctOTP();
    void wrongOTP();
    void droneRegistered();
    void droneNotRegistered();


public slots:
    void get(QString location);
    void postEmailPass(QString location, QByteArray data);
    void postOTP(QString location, QByteArray data);
    void postDroneNo(QString location, QByteArray data); // post droneNo to check whether it's registered or not

private slots:
    void readyRead();
    void readyReadOTP();
    void readyReadDroneNo();//getting droneNo validation reply

    void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
    void encrypted(QNetworkReply *reply);
    void finished(QNetworkReply *reply);
    void preSharedKeyAuthenticationRequired(QNetworkReply *reply, QSslPreSharedKeyAuthenticator *authenticator);
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
    void sslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
    QNetworkAccessManager manager;
    QByteArray authCode;
    QByteArray token;
    QByteArray tokenDroneNo;//storing 'auth': header session
    QByteArray tokenDroneReply;

};

extern QObject *singletonProvider(QQmlEngine *engine, QJSEngine *scriptEngine);
extern CustomerData *getInstance();
extern bool successfulLogin;
extern bool droneStatusCheck;
#endif // CUSTOMERDATA_H
