/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MicrohardHandler.h"

class MicrohardSettings : public MicrohardHandler
{
    Q_OBJECT
public:
    explicit MicrohardSettings          (QString address, QObject* parent = nullptr, bool setEncryptionKey = false);
    bool    start                       () override;
    void    getStatus                   ();
    void    setEncryptionKey            (QString key);
    bool    loggedIn                    () { return _loggedIn; }

protected slots:
    void    _readBytes                  () override;

signals:
    void    updateRSSI                  (int rssi);

private:
    bool    _loggedIn;
    int     _rssiVal;
    QString _address;
    bool    _setEncryptionKey;
};
