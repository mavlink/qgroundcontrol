/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef WifiSettings_H
#define WifiSettings_H

#include <QObject>

class VideoSettings;

class WifiSettings : public QObject
{
    Q_OBJECT

public:
    WifiSettings();
    static void setNativeMethods(void);

    Q_PROPERTY(QString videoShareSSID       READ videoShareSSID         CONSTANT)
    Q_PROPERTY(int videoShareAuthType       READ videoShareAuthType     CONSTANT)
    Q_PROPERTY(QString videoSharePasswd     READ videoSharePasswd       CONSTANT)
    Q_PROPERTY(QString rtspURL              READ rtspURL                CONSTANT)
    Q_INVOKABLE bool setVideoShareApConfig(QString name, QString passwd, int authType, bool restart);
    Q_INVOKABLE bool setVideoShareApEnabled(bool enabled);
    Q_INVOKABLE void setCountryCode(QString country, bool persist);
    Q_INVOKABLE QString getCountryCode();

    QString videoShareSSID    (void);
    int videoShareAuthType    (void);
    QString videoSharePasswd  (void);
    QString rtspURL           (void);

    static const char* _rtspURL;

signals:
    void wifiAPStateChanged   (int state);

private:
};

#endif
