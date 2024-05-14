/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(GPSManagerLog)

class GPSRtk;

class GPSManager : public QObject
{
    Q_OBJECT

public:
    GPSManager(QObject* parent = nullptr);
    ~GPSManager();

    static GPSManager* instance();

    GPSRtk* gpsRtk() { return m_gpsRtk; }

private:
    GPSRtk* m_gpsRtk = nullptr;
};
