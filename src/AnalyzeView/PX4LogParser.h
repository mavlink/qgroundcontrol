#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>

#include "GeoTagController.h"

class PX4LogParser
{
public:
    PX4LogParser();
    ~PX4LogParser();
    bool getTagsFromLog(QByteArray& log, QList<GeoTagWorker::cameraFeedbackPacket>& cameraFeedback);

private:

};
