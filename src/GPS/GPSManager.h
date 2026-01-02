#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

Q_DECLARE_LOGGING_CATEGORY(GPSManagerLog)

class GPSRtk;

class GPSManager : public QObject
{
    Q_OBJECT

public:
    GPSManager(QObject *parent = nullptr);
    ~GPSManager();

    static GPSManager *instance();

    GPSRtk *gpsRtk() { return _gpsRtk; }

private:
    GPSRtk *_gpsRtk = nullptr;
};
