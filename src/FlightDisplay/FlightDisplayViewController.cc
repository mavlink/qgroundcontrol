/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>

#include <VideoItem.h>

#include "ScreenToolsController.h"
#include "FlightDisplayViewController.h"

const char* kMainFlightDisplayViewControllerGroup = "FlightDisplayViewController";

FlightDisplayViewController::FlightDisplayViewController(QObject *parent)
    : QObject(parent)
    , _videoRunning(false)
{
    /*
     * This is the receiving end of an UDP RTP stream. The sender can be setup with this command:
     *
     * gst-launch-1.0 uvch264src initial-bitrate=1000000 average-bitrate=1000000 iframe-period=1000 name=src auto-start=true src.vidsrc ! \
     * video/x-h264,width=1280,height=720,framerate=24/1 ! h264parse ! rtph264pay ! udpsink host=192.168.1.9 port=5600
     *
     * Where the main parameters are:
     *
     *  uvch264src:         Your h264 video source (the example above uses a Logitech C920 on an Raspberry PI 2+ or Odroid C1
     *  host=192.168.1.9    This is the IP address of QGC. You can use Avahi/Zeroconf to find QGC using the "_qgroundcontrol._udp" service.
     *
     * Advanced settings (you should probably read the gstreamer documentation before changing these):
     *
     * initial-bitrate=1000000 average-bitrate=1000000
     * The bit rate to use. The greater, the better quality at the cost of higher bandwidth.
     *
     * width=1280,height=720,framerate=24/1
     * The video resolution and frame rate. This depends on the camera used.
     *
     * iframe-period=1000
     * Interval between iFrames. The greater the interval the lesser bandwidth at the cost of a longer time to recover from lost packets.
     *
     * Do not change anything else unless you know what you are doing. Any other change will require a matching change on the receiving end.
     *
     */
    _videoSurface = new VideoSurface;
    _videoReceiver = new VideoReceiver(this);
    _videoReceiver->setUri(QLatin1Literal("udp://0.0.0.0:5600"));   // Port 5600=Solo UDP port, if you change you will break Solo video support
#if defined(QGC_GST_STREAMING)
    _videoReceiver->setVideoSink(_videoSurface->videoSink());
    connect(&_frameTimer, &QTimer::timeout, this, &FlightDisplayViewController::_updateTimer);
    _frameTimer.start(1000);
#endif
}

FlightDisplayViewController::~FlightDisplayViewController()
{

}

#if defined(QGC_GST_STREAMING)
void FlightDisplayViewController::_updateTimer(void)
{
    if(_videoRunning)
    {
        time_t elapsed = 0;
        if(_videoSurface)
        {
            elapsed = time(0) - _videoSurface->lastFrame();
        }
        if(elapsed > 2)
        {
            _videoRunning = false;
            _videoSurface->setLastFrame(0);
            emit videoRunningChanged();
        }
    }
    else
    {
        if(_videoSurface && _videoSurface->lastFrame()) {
            if(!_videoRunning)
            {
                _videoRunning = true;
                emit videoRunningChanged();
            }
        }
    }
}
#endif
