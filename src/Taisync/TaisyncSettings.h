/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "TaisyncHandler.h"

class TaisyncSettings : public TaisyncHandler
{
    Q_OBJECT
public:
    explicit TaisyncSettings            (QObject* parent = nullptr);
    bool    start                       () override;
    bool    requestLinkStatus           ();
    bool    requestDevInfo              ();
    bool    requestFreqScan             ();
    bool    requestVideoSettings        ();
    bool    requestRadioSettings        ();
    bool    requestIPSettings           ();
    bool    requestRTSPURISettings      ();
    bool    setRadioSettings            (const QString& mode, const QString& channel);
    bool    setVideoSettings            (const QString& output, const QString& mode, const QString& rate);
    bool    setRTSPSettings             (const QString& uri, const QString& account, const QString& password);
    bool    setIPSettings               (const QString& localIP, const QString& remoteIP, const QString& netMask);

signals:
    void    updateSettings              (QByteArray jSonData);

protected slots:
    void    _readBytes                  () override;

private:
    bool    _request                    (const QString& request);
    bool    _post                       (const QString& post, const QString& postPayload);
};
