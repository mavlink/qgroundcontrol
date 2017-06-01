/****************************************************************************
 *
 * Copyright (c) 2017, Intel Corporation
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Video Streaming Widget
 *   @author Ricardo de Almeida Gonzaga <ricardo.gonzaga@intel.com>
 */

#ifndef VideoStreamingWidget_H
#define VideoStreamingWidget_H

#include "QGCQmlWidgetHolder.h"

class VideoStreamingWidget : public QGCQmlWidgetHolder
{
    Q_OBJECT

public:
    VideoStreamingWidget(const QString& title, QAction* action, QWidget *parent = 0);
};

#endif // VideoStreamingWidget_H
