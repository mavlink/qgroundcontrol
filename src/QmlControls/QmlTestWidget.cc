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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QmlTestWidget.h"

#include <QColorDialog>

QmlTestWidget::QmlTestWidget(void)
    : QGCQmlWidgetHolder(QString(), NULL, NULL)
{
    setAttribute(Qt::WA_DeleteOnClose);
    resize(900, 500);
    setVisible(true);

    setContextPropertyObject("controller", this);

    setSource(QUrl::fromUserInput("qrc:qml/QmlTest.qml"));
}

void QmlTestWidget::showColorDialog(QQuickItem* item)
{
    Q_UNUSED(item)
    QColorDialog colorDialog(this);
    connect(&colorDialog, &QColorDialog::colorSelected, this, &QmlTestWidget::_colorSelected);
    colorDialog.open();
}

void QmlTestWidget::_colorSelected(const QColor & color)
{
    Q_UNUSED(color);
}

