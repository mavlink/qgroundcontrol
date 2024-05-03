/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCToolbox.h"

#include <QtCore/QMap>

class AppSettings;
class QGCApplication;
class LinkInterface;
class QNetworkReply;

//-----------------------------------------------------------------------------
class AirLinkManager : public QGCTool
{
    Q_OBJECT

public:
    Q_PROPERTY(QStringList droneList READ droneList NOTIFY droneListChanged)
    Q_INVOKABLE void updateDroneList(const QString &login, const QString &pass);
    Q_INVOKABLE bool isOnline(const QString &drone);
    Q_INVOKABLE void connectToAirLinkServer(const QString &login, const QString &pass);
    Q_INVOKABLE void updateCredentials(const QString &login, const QString &pass);

    explicit AirLinkManager(QGCApplication* app, QGCToolbox* toolbox);
    ~AirLinkManager() override;

    void setToolbox (QGCToolbox* toolbox) override;
    QStringList droneList() const;

    static const QString airlinkHost;
    static constexpr int airlinkPort = 10000;

signals:
    void    droneListChanged();

private:
    void                _parseAnswer                (const QByteArray &ba);
    void                _processReplyAirlinkServer  (QNetworkReply &reply);

private:
    QMap<QString, bool> _vehiclesFromServer;
    QNetworkReply*      _reply;
};
