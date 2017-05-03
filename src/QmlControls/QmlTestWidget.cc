/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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

