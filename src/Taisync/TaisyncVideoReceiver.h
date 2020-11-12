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

class TaisyncVideoReceiver : public TaisyncHandler
{
    Q_OBJECT
public:

    explicit TaisyncVideoReceiver       (QObject* parent = nullptr);
    bool start                          () override;
    bool close                          () override;

private slots:
    void    _readBytes                  () override;

private:
    QUdpSocket*     _udpVideoSocket     = nullptr;
};
