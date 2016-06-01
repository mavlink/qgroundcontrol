/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef LogDownload_H
#define LogDownload_H

#include "QGCQmlWidgetHolder.h"

class LogDownload : public QGCQmlWidgetHolder
{
    Q_OBJECT
	
public:
    LogDownload(const QString& title, QAction* action, QWidget *parent = 0);
};

#endif
