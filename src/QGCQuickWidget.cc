/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#include "QGCQuickWidget.h"
#include "UASManager.h"
#include "AutoPilotPluginManager.h"

#include <QQmlContext>
#include <QQmlEngine>

/// @file
///     @brief Subclass of QQuickWidget which injects Facts and the Pallete object into
///             the QML context.
///
///     @author Don Gagne <don@thegagnes.com>

QGCQuickWidget::QGCQuickWidget(QWidget* parent) :
    QQuickWidget(parent)
{
    UASManagerInterface* uasMgr = UASManager::instance();
    Q_ASSERT(uasMgr);
    
    UASInterface* uas = uasMgr->getActiveUAS();
    Q_ASSERT(uas);
    
    AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(uas->getAutopilotType())->addFactsToQmlContext(rootContext(), uas);
    
    rootContext()->engine()->addImportPath("qrc:/qml");
    
    setSource(QUrl::fromLocalFile("/Users/Don/repos/qgroundcontrol/test.qml"));
}
