/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QString>
#include <string>

#include "UTMSPRestInterface.h"

class UTMSPAuthorization: public QObject
{
    Q_OBJECT
public:
    UTMSPAuthorization(QObject *parent = nullptr);
    virtual ~UTMSPAuthorization() = default;

    const std::string& getOAuth2Token();

protected slots:
    bool requestOAuth2Client(const QString& clientID, const QString& clientSecret);

private:
    bool                 _isValidToken;
    UTMSPRestInterface   _utmspRestInterface;
};
