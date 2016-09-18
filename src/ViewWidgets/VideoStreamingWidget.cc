/****************************************************************************
 *
 * Copyright (c) 2016, Intel Corporation
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

#include "VideoStreamingWidget.h"

VideoStreamingWidget::VideoStreamingWidget(const QString& title, QAction* action, QWidget *parent) :
    QGCQmlWidgetHolder(title, action, parent)
{
    Q_UNUSED(title)
    Q_UNUSED(action);

    resize(460, 328);

    setSource(QUrl::fromUserInput("qrc:/qml/VideoStreamingWidget.qml"));

    loadSettings();
}
