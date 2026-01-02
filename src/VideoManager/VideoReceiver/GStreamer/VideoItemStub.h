#pragma once

#include <QtQmlIntegration/QtQmlIntegration>
#include <QtQuick/QQuickItem>

class VideoItemStub : public QQuickItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(GstQt6VideoItem)

public:
    explicit VideoItemStub(QQuickItem *parent = nullptr)
        : QQuickItem(parent) {}
};
