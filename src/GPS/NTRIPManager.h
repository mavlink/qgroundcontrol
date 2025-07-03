/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

Q_DECLARE_LOGGING_CATEGORY(NTRIPManagerLog)

class NTRIPTCPLink;
class RTCMMavlink;

class NTRIPManager : public QObject
{
    Q_OBJECT

public:
    explicit NTRIPManager(QObject *parent = nullptr);
    ~NTRIPManager();

public slots:
    static void _tcpError(const QString &errorMsg);

private:
    NTRIPTCPLink *_tcpLink = nullptr;
    RTCMMavlink *_rtcmMavlink = nullptr;
};
