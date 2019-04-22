/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SysStatusManager.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"

#include <QSettings>


SysStatusManager::SysStatusManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
    QSettings settings;

    setBoardTemputure("--.--");
}

//-----------------------------------------------------------------------------
SysStatusManager::~SysStatusManager()
{

}

//-----------------------------------------------------------------------------
void SysStatusManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<SysStatusManager>("QGroundControl.SysStatusManager", 1, 0, "SysStatusManager", "Reference only");
}


void SysStatusManager::setBoardTemputure(const QString &boardTemputure)
{
    if (boardTemputure == m_boardTemputure)
        return;

    m_boardTemputure = boardTemputure;
    emit boardTemputureChanged();
}
