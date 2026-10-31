#include "MockVideoStreamServer.h"

// Builds without GStreamer serve no mock video stream: MockLink then advertises the requested stream as not running.

MockVideoStreamServer::~MockVideoStreamServer() = default;

bool MockVideoStreamServer::isStreamTypeAvailable(StreamType)
{
    return false;
}

bool MockVideoStreamServer::start(StreamType, const QString&, quint16)
{
    return false;
}

void MockVideoStreamServer::stop() {}
