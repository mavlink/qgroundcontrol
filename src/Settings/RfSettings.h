/****************************************************************************
 *
 * Copyright (C) 2019 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once
#include <QLocalSocket>
#include "SettingsGroup.h"

class RfSettings : public SettingsGroup
{
    Q_OBJECT

public:
    Q_PROPERTY(Fact* rfAuthentication     READ rfAuthentication   CONSTANT)

    RfSettings(QObject* parent = NULL);

    Fact* rfAuthentication (void);

    static const char* rfSettingsGroupName;
    static const char* rfAuthenticationSettingsName;

private slots:
    void _newRfAuthentication(QVariant value);

private:
    void _setWifiCountryCode(int value);
    void _setToD2dService(int value);

    SettingsFact* _rfAuthenticationFact;
    QString _rfConfigPropName;
    QLocalSocket _localSocket;
};
