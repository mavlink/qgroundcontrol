/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LogDownloadTypes.h"

void QGCLogEntry::setSelected(bool selected)
{
    if (_selected != selected) {
        _selected = selected;
        emit selectedChanged();
    }
}

void QGCLogEntry::setStatus(const QString& status)
{
    if (_status != status) {
        _status = status;
        emit statusChanged();
    }
}