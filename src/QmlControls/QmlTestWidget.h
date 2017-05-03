/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef QmlTestWidget_h
#define QmlTestWidget_h

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QGCQmlWidgetHolder.h"

/// This is used to create widgets which are implemented in QML.

class QmlTestWidget : public QGCQmlWidgetHolder
{
    Q_OBJECT

public:
    QmlTestWidget(void);

    Q_INVOKABLE void showColorDialog(QQuickItem* item);

private slots:
    void _colorSelected(const QColor & color);

};

#endif
