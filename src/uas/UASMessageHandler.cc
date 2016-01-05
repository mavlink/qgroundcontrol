/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/*!
 * @file
 *   @brief Message Handler
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "QGCApplication.h"
#include "UASMessageHandler.h"
#include "MultiVehicleManager.h"
#include "UAS.h"

UASMessage::UASMessage(int componentid, int severity, QString text)
{
    _compId   = componentid;
    _severity = severity;
    _text     = text;
}

bool UASMessage::severityIsError()
{
    switch (_severity) {
        case MAV_SEVERITY_EMERGENCY:
        case MAV_SEVERITY_ALERT:
        case MAV_SEVERITY_CRITICAL:
        case MAV_SEVERITY_ERROR:
            return true;
        default:
            return false;
    }
}

UASMessageHandler::UASMessageHandler(QGCApplication* app)
    : QGCTool(app)
    , _activeUAS(NULL)
    , _errorCount(0)
    , _errorCountTotal(0)
    , _warningCount(0)
    , _normalCount(0)
    , _showErrorsInToolbar(false)
    , _multiVehicleManager(NULL)
{

}

UASMessageHandler::~UASMessageHandler()
{
    clearMessages();
}

void UASMessageHandler::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);

   _multiVehicleManager = _toolbox->multiVehicleManager();

   connect(_multiVehicleManager, &MultiVehicleManager::activeVehicleChanged, this, &UASMessageHandler::_activeVehicleChanged);
   emit textMessageReceived(NULL);
   emit textMessageCountChanged(0);
}

void UASMessageHandler::clearMessages()
{
    _mutex.lock();
    while(_messages.count()) {
        delete _messages.last();
        _messages.pop_back();
    }
    _errorCount   = 0;
    _warningCount = 0;
    _normalCount  = 0;
    _mutex.unlock();
    emit textMessageCountChanged(0);
}

void UASMessageHandler::_activeVehicleChanged(Vehicle* vehicle)
{
    // If we were already attached to an autopilot, disconnect it.
    if (_activeUAS)
    {
        disconnect(_activeUAS, &UASInterface::textMessageReceived, this, &UASMessageHandler::handleTextMessage);
        _activeUAS = NULL;
        clearMessages();
        emit textMessageReceived(NULL);
    }
    // And now if there's an autopilot to follow, set up the UI.
    if (vehicle)
    {
        UAS* uas = vehicle->uas();

        // Connect to the new UAS.
        clearMessages();
        _activeUAS = uas;
        connect(uas, &UASInterface::textMessageReceived, this, &UASMessageHandler::handleTextMessage);
    }
}

void UASMessageHandler::handleTextMessage(int, int compId, int severity, QString text)
{

    // Color the output depending on the message severity. We have 3 distinct cases:
    // 1: If we have an ERROR or worse, make it bigger, bolder, and highlight it red.
    // 2: If we have a warning or notice, just make it bold and color it orange.
    // 3: Otherwise color it the standard color, white.

    _mutex.lock();

    // So first determine the styling based on the severity.
    QString style;
    switch (severity)
    {
    case MAV_SEVERITY_EMERGENCY:
    case MAV_SEVERITY_ALERT:
    case MAV_SEVERITY_CRITICAL:
    case MAV_SEVERITY_ERROR:
        style = QString("color: #f95e5e; font-weight:bold");
        _errorCount++;
        _errorCountTotal++;
        break;
    case MAV_SEVERITY_NOTICE:
    case MAV_SEVERITY_WARNING:
        style = QString("color: #f9b55e; font-weight:bold");
        _warningCount++;
        break;
    default:
        style = QString("color: #ffffff; font-weight:bold");
        _normalCount++;
        break;
    }

    // And determine the text for the severitie
    QString severityText("");
    switch (severity)
    {
    case MAV_SEVERITY_EMERGENCY:
        severityText = QString(tr(" EMERGENCY:"));
        break;
    case MAV_SEVERITY_ALERT:
        severityText = QString(tr(" ALERT:"));
        break;
    case MAV_SEVERITY_CRITICAL:
        severityText = QString(tr(" Critical:"));
        break;
    case MAV_SEVERITY_ERROR:
        severityText = QString(tr(" Error:"));
        break;
    case MAV_SEVERITY_WARNING:
        severityText = QString(tr(" Warning:"));
        break;
    case MAV_SEVERITY_NOTICE:
        severityText = QString(tr(" Notice:"));
        break;
    case MAV_SEVERITY_INFO:
        severityText = QString(tr(" Info:"));
        break;
    case MAV_SEVERITY_DEBUG:
        severityText = QString(tr(" Debug:"));
        break;
    default:
        severityText = QString(tr(""));
        break;
    }

    // Finally preppend the properly-styled text with a timestamp.
    QString dateString = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    UASMessage* message = new UASMessage(compId, severity, text);
    message->_setFormatedText(QString("<p style=\"color:#e0e0f0\">[%2 - COMP:%3]<font style=\"%1\">%4 %5</font></p>").arg(style).arg(dateString).arg(compId).arg(severityText).arg(text));
    _messages.append(message);
    int count = _messages.count();
    if (message->severityIsError()) {
        _latestError = severityText + " " + text;
    }
    _mutex.unlock();
    emit textMessageReceived(message);
    emit textMessageCountChanged(count);

    if (_showErrorsInToolbar && message->severityIsError()) {
        _app->showMessage(message->getText());
    }
}

int UASMessageHandler::getErrorCountTotal() {
    _mutex.lock();
    int c = _errorCountTotal;
    _mutex.unlock();
    return c;
}

int UASMessageHandler::getErrorCount() {
    _mutex.lock();
    int c = _errorCount;
    _errorCount = 0;
    _mutex.unlock();
    return c;
}

int UASMessageHandler::getWarningCount() {
    _mutex.lock();
    int c = _warningCount;
    _warningCount = 0;
    _mutex.unlock();
    return c;
}

int UASMessageHandler::getNormalCount() {
    _mutex.lock();
    int c = _normalCount;
    _normalCount = 0;
    _mutex.unlock();
    return c;
}
