#pragma once

#include <QtQuick/QQuickItem>

class GLVideoItemStub : public QQuickItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(GstGLQt6VideoItem)

public:
    GLVideoItemStub(QQuickItem *parent = nullptr) :
        QQuickItem(parent)
    {}
    ~GLVideoItemStub() = default;
};
