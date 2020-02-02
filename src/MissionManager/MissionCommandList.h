/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef MissionCommandList_H
#define MissionCommandList_H

#include "QGCToolbox.h"
#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonValue>

class MissionCommandUIInfo;

/// Maintains a list of MissionCommandUIInfo objects loaded from a json file.
class MissionCommandList : public QObject
{
    Q_OBJECT
    
public:
    /// @param jsonFilename Json file which contains commands
    /// @param baseCommandList true: bottomost level of mission command hierarchy (partial not allowed), false: mid-level of command hierarchy
    MissionCommandList(const QString& jsonFilename, bool baseCommandList, QObject* parent = nullptr);

    /// Returns list of categories in this list
    QStringList& categories(void) { return _categories; }

    /// Returns the ui info for specified command, NULL if command not found
    MissionCommandUIInfo* getUIInfo(MAV_CMD command) const;

    const QList<MAV_CMD>& commandIds(void) const { return _ids; }
    
private:
    void _loadMavCmdInfoJson(const QString& jsonFilename, bool baseCommandList);

    QMap<MAV_CMD, MissionCommandUIInfo*>    _infoMap;
    QList<MAV_CMD>                          _ids;
    QStringList                             _categories;

    static const char* _versionJsonKey;
    static const char* _mavCmdInfoJsonKey;
};

#endif
