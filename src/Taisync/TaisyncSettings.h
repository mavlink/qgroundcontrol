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

Q_DECLARE_LOGGING_CATEGORY(TaisyncSettingsLog)

class TaisyncSettings : public TaisyncHandler
{
    Q_OBJECT
public:

    Q_PROPERTY(bool     linkConnected       READ linkConnected              NOTIFY linkChanged)
    Q_PROPERTY(QString  linkVidFormat       READ linkVidFormat              NOTIFY linkChanged)
    Q_PROPERTY(int      uplinkRSSI          READ uplinkRSSI                 NOTIFY linkChanged)
    Q_PROPERTY(int      downlinkRSSI        READ downlinkRSSI               NOTIFY linkChanged)

    explicit TaisyncSettings            (QObject* parent = nullptr);
    void    start                       () override;
    bool    requestSettings             ();
    bool    requestFreqScan             ();

    bool     linkConnected              () { return _linkConnected; }
    QString  linkVidFormat              () { return _linkVidFormat; }
    int      uplinkRSSI                 () { return _downlinkRSSI; }
    int      downlinkRSSI               () { return _uplinkRSSI; }

signals:
    void    linkChanged                 ();

protected slots:
    void    _readBytes                  () override;

private:
    bool    _linkConnected  = false;
    QString _linkVidFormat;
    int     _downlinkRSSI   = 0;
    int     _uplinkRSSI     = 0;
};
