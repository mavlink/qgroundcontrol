/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

signals:
    void    updateSettings              (QByteArray jSonData);

protected slots:
    void    _readBytes                  () override;

private:
    bool    _request                    (const QString& request);
};
