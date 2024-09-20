/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCMAVLink.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QMap>

class MissionCommandUIInfo;

/// Maintains a list of MissionCommandUIInfo objects loaded from a json file.
class MissionCommandList : public QObject
{
    Q_OBJECT
    
public:
    /// @param baseCommandList true: bottomost level of mission command hierarchy (partial spec allowed), false: override level of hierarchy
    MissionCommandList(const QString& jsonFilename, bool baseCommandList, QObject* parent = nullptr);

    /// Returns list of categories in this list
    QStringList& categories(void) { return _categories; }

    /// Returns the ui info for specified command, NULL if command not found
    MissionCommandUIInfo* getUIInfo(MAV_CMD command) const;

    const QList<MAV_CMD>& commandIds(void) const { return _ids; }
    
    static constexpr const char* qgcFileType = "MavCmdInfo";

private:
    void _loadMavCmdInfoJson(const QString& jsonFilename, bool baseCommandList);

    QMap<MAV_CMD, MissionCommandUIInfo*>    _infoMap;
    QList<MAV_CMD>                          _ids;
    QStringList                             _categories;

    static constexpr const char* _versionJsonKey =       "version";
    static constexpr const char* _mavCmdInfoJsonKey =    "mavCmdInfo";
};
