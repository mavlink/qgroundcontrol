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
    explicit MicrohardSettings          (QObject* parent = nullptr);
    bool    start                       () override;
    bool    requestLinkStatus           ();
    bool    setIPSettings               (const QString& localIP, const QString& remoteIP, const QString& netMask);

signals:
    void    updateSettings              (QByteArray jSonData);

protected slots:
    void    _readBytes                  () override;

private:
    bool    _request                    (const QString& request);
    bool    _post                       (const QString& post, const QString& postPayload);
};
