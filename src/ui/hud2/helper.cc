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

#include <QtGui>
#include "helper.h"

Helper::Helper()
{
    normalcolor = QColor(Qt::green);
    hudrect = QRect(0,0,0,0);
    background = QBrush(QColor(64, 32, 64));
    circlePen = QPen(Qt::black);
    circlePen.setWidth(1);
    textPen = QPen(Qt::white);
    textFont.setPixelSize(50);

    regularpen = QPen(normalcolor);
}

void Helper::paint(QPainter *painter, QPaintEvent *event, int elapsed)
{
    if (hudrect != painter->viewport()){
        hudrect = painter->viewport();
        updatesizes(hudrect);
    }
    painter->fillRect(event->rect(), background);
    painter->translate(hudrect.center());

//    painter->save();
//    painter->setBrush(circleBrush);
//    painter->setPen(circlePen);
//    painter->rotate(elapsed * 0.030);

//    qreal r = elapsed/1000.0;
//    int n = 30;
//    for (int i = 0; i < n; ++i) {
//        painter->rotate(30);
//        qreal radius = 0 + 120.0*((i+r)/n);
//        qreal circleRadius = 1 + ((i+r)/n)*20;
//        painter->drawEllipse(QRectF(radius, -circleRadius,
//                                    circleRadius*2, circleRadius*2));
//    }

//    painter->restore();
    painter->setPen(regularpen);
    painter->drawLine(0, 0, 100, elapsed);
    yaw.paint(painter);

//    painter->setPen(textPen);
//    painter->setFont(textFont);
//    painter->drawText(QRect(-50, -50, 100, 100), Qt::AlignCenter, "Qt");
}

void Helper::updatesizes(QRect rect){
    regularpen.setWidthF(rect.height()/100);
    yaw.updatesize(&rect);
}


