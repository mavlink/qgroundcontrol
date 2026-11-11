#pragma once

#include <QtCore/QRectF>

#include "QmlUITestBase.h"

class QQuickItem;

/// UI tests for the credit of imagery and terrain that a map shows, on both map engines.
class MapAttributionUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _flyViewMapCreditsItsSources_data();
    void _flyViewMapCreditsItsSources();

private:
    static QRectF _sceneRect(const QQuickItem* item);
};
