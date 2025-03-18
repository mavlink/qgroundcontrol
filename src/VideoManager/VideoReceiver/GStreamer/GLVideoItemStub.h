/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtQuick/QQuickItem>

class GLVideoItemStub : public QQuickItem
{
    Q_OBJECT
    // QML_NAMED_ELEMENT(GstGLQt6VideoItem)

public:
    GLVideoItemStub(QQuickItem *parent = nullptr) :
        QQuickItem(parent) {}
};
