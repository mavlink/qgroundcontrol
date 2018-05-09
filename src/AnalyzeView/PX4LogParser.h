#ifndef PX4LOGPARSER_H
#define PX4LOGPARSER_H

#include <QGeoCoordinate>
#include <QDebug>

#include "GeoTagController.h"

class PX4LogParser
{
public:
    PX4LogParser();
    ~PX4LogParser();
    bool getTagsFromLog(QByteArray& log, QList<GeoTagWorker::cameraFeedbackPacket>& cameraFeedback);

private:

};

#endif // PX4LOGPARSER_H
