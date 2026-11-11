#pragma once

#include "QmlUITestBase.h"

class QQuickItem;

/// UI tests for the offline map set editor of the Maps settings page, on both map engines.
class OfflineMapEditorUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _newSetAreaFollowsMap_data();
    void _newSetAreaFollowsMap();
    void _defaultSetInfoOpensAndCloses_data();
    void _defaultSetInfoOpensAndCloses();
    void _renamedSetIsListedByItsNewName();

private:
    void _addEngineRows();
    /// Starts the UI on the fetched engine and opens the Maps settings page
    void _openMapsSettings(bool geoMapEngine);
    /// The editor's map, once it is laid out. Fails the test when the editor shows the other engine's map.
    QQuickItem* _editorMap(bool geoMapEngine);
    void _mouseDrag(Qt::MouseButton button, const QPoint& from, const QPoint& delta);
    static int _visibleItemCount(const QQuickItem* root, const QString& objectName);

    QVariant _savedEngine;
};
