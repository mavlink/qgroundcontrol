#pragma once

#include "GeoTagController.h"
#include <QtCore/QByteArray>

class PX4LogParser
{
public:
    PX4LogParser();
    ~PX4LogParser();
    bool getTagsFromLog(QByteArray& log, QList<GeoTagWorker::cameraFeedbackPacket>& cameraFeedback);

private:

};
