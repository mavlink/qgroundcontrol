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

#ifndef CustomCommandWidget_H
#define CustomCommandWidget_H

#include "QGCQmlWidgetHolder.h"

class CustomCommandWidget : public QGCQmlWidgetHolder
{
    Q_OBJECT
	
public:
    CustomCommandWidget(const QString& title, QAction* action, QWidget *parent = 0);
};

#endif
