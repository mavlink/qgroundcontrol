/****************************************************************************
 *
 * Copyright (c) 2016, Intel Corporation
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Video Streaming Widget Controller
 *   @author Ricardo de Almeida Gonzaga <ricardo.gonzaga@intel.com>
 */

#include "VideoStreamingWidgetController.h"

#include "QGroundControlQmlGlobal.h"

VideoStreamingWidgetController::VideoStreamingWidgetController(void)
    : _rtspEnabled(false)
    , _serverLabel(NULL)
    , _ipField(NULL)
    , _portField(NULL)
    , _streamsComboBox(NULL)
    , _formatComboBox(NULL)
    , _frameSizeWidthField(NULL)
    , _frameSizeHeightField(NULL)
    , _nameField(NULL)
{
    if (QGroundControlQmlGlobal::streamingSources()->rawValue().toUInt() == QGroundControlQmlGlobal::StreamingSources::StreamingSourcesRTSP)
        _rtspEnabled = true;
}

VideoStreamingWidgetController::~VideoStreamingWidgetController(void)
{

}

void VideoStreamingWidgetController::getServerInfo(void)
{
    Q_ASSERT(_serverLabel);
    Q_ASSERT(_ipField);
    Q_ASSERT(_portField);
    Q_ASSERT(_streamsComboBox);

    // TODO: Use custom MAVLink message to get server information.

    QStringList streams;
    streams << "rtsp://0.0.0.0:8554/stream0" << "rtsp://0.0.0.0:8554/stream1";

    _serverLabel->setProperty("text", "Server: 0.0.0.0:8554");
    _ipField->setProperty("text", "0.0.0.0");
    _portField->setProperty("text", "8554");
    _streamsComboBox->setProperty("model", streams);
}

void VideoStreamingWidgetController::getStreamInfo(void)
{
    Q_ASSERT(_streamsComboBox);
    Q_ASSERT(_formatComboBox);
    Q_ASSERT(_frameSizeWidthField);
    Q_ASSERT(_frameSizeHeightField);
    Q_ASSERT(_nameField);

    // TODO: Use custom MAVLink message to get stream information.

    QStringList formats;
    formats << "YUYV" << "MJPG";

    _formatComboBox->setProperty("model", formats);
    _formatComboBox->setProperty("currentIndex", 0);
    _frameSizeWidthField->setProperty("text", "1280");
    _frameSizeHeightField->setProperty("text", "720");
    _nameField->setProperty("text", "stream0");
}

void VideoStreamingWidgetController::refresh(void)
{
    getServerInfo();
    getStreamInfo();
}

void VideoStreamingWidgetController::setIp(void)
{
    Q_ASSERT(_ipField);

    // TODO: Use custom MAVLink message to set ip.

    _ipField->setProperty("text", "0.0.0.0");
}

void VideoStreamingWidgetController::setPort(void)
{
    Q_ASSERT(_portField);

    // TODO: Use custom MAVLink message to set port.

    _portField->setProperty("text", "8554");
}

void VideoStreamingWidgetController::setActiveStream(void)
{
    getStreamInfo();
}

void VideoStreamingWidgetController::setFormat(void)
{
    Q_ASSERT(_formatComboBox);

    // TODO: Use custom MAVLink message to set format.

    QStringList formats;
    formats << "YUYV" << "MJPG";

    _formatComboBox->setProperty("model", formats);
    _formatComboBox->setProperty("currentIndex", 0);
}

void VideoStreamingWidgetController::setFrameSize(void)
{
    Q_ASSERT(_frameSizeWidthField);
    Q_ASSERT(_frameSizeHeightField);

    // TODO: Use custom MAVLink message to set frame size.

    _frameSizeWidthField->setProperty("text", "1280");
    _frameSizeHeightField->setProperty("text", "720");
}

void VideoStreamingWidgetController::setName(void)
{
    Q_ASSERT(_nameField);

    // TODO: Use custom MAVLink message to set name.

    _nameField->setProperty("text", "stream0");
}
