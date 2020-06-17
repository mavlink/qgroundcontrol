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
#include <QUdpSocket>
#include <QTimer>

class TaisyncTelemetry : public TaisyncHandler
{
    Q_OBJECT
public:

    explicit TaisyncTelemetry           (QObject* parent = nullptr);
    bool    close                       () override;
    bool    start                       () override;
    void    writeBytes                  (QByteArray bytes);

signals:
    void    bytesReady                   (QByteArray bytes);

private slots:
    void    _newConnection              () override;
    void    _readBytes                  () override;

};
