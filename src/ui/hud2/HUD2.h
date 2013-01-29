/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef HUD2_H
#define HUD2_H

#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QTimer>
#include <QImage>

#include "HUD2RenderNative.h"
#include "HUD2RenderGL.h"
#include "HUD2Drawer.h"
#include "HUD2Horizon.h"
#include "HUD2Data.h"
#include "HUD2Dialog.h"

#include "UASInterface.h"

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

#define HUD2_FPS_MIN        1
#define HUD2_FPS_DEFAULT    50
#define HUD2_FPS_MAX        100

typedef enum {
    RENDER_TYPE_NATIVE = 0,
    RENDER_TYPE_OPENGL = 1,
    RENDER_TYPE_ENUM_END = 2
}render_type;

class HUD2 : public QWidget
{
    Q_OBJECT

public:
    HUD2(QWidget *parent = 0);
    ~HUD2();

protected:
    UASInterface* uas; ///< The uas currently monitored
    void contextMenuEvent (QContextMenuEvent* event);

private:
    HUD2Data huddata;
    HUD2RenderNative *render_native;
    HUD2RenderGL *render_gl;
    HUD2Dialog *settings_dialog;

    QGridLayout *layout;
    QTimer fpsLimiter;
    int fpsLimit;
    bool repaintEnabled; // used in FPS limiter
    bool antiAliasing;
    int renderType;
    void paint(void);
    void createActions(void);

public slots:
    /** @brief Set the currently monitored UAS */
    virtual void setActiveUAS(UASInterface* uas);
    /** @brief Attitude from main autopilot / system state */
    void updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 timestamp);
    void updateGlobalPosition(UASInterface*,double,double,double,quint64);

    void toggleAntialising(bool aa);
    void switchRender(int type);
    void setFpsLimit(int limit);

private slots:
    void enableRepaint(void);

signals:
    //void visibilityChanged(bool visible);

private slots:

};

#endif
