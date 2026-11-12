#pragma once

#include "QmlUITestBase.h"

class QGeoRectangle;
class QQuickItem;

/// UI tests for the offline map set editor of the Maps settings page, on both map engines.
class OfflineMapEditorUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _newSetAreaFollowsMap_data();
    void _newSetAreaFollowsMap();
    void _newSetEstimateFollowsChosenType_data();
    void _newSetEstimateFollowsChosenType();
    void _defaultSetInfoOpensAndCloses_data();
    void _defaultSetInfoOpensAndCloses();
    void _setInfoShowsSetAreaAndMap_data();
    void _setInfoShowsSetAreaAndMap();
    void _mapEngineErrorOpensOneDialog();
    void _setCannotBeEditedDuringImport();
    void _renamedSetIsListedByItsNewName();

private:
    void _addEngineRows();
    /// Saves a set of \a area that is only in the database, like one of an earlier run: nothing downloads its tiles.
    /// \a shownType is the map type that the set names, which need not be one of the map list.
    /// Check QTest::currentTestFailed() on return. \a setId is for _deleteSet(), also after a failure.
    void _saveSet(const QString& name, const QString& shownType, const QGeoRectangle& area, quint64& setId);
    static void _deleteSet(quint64 setId);
    /// Starts the UI on the fetched engine and opens the Maps settings page
    void _openMapsSettings(bool geoMapEngine);
    /// Clicks Edit of a set in the list of the Maps settings page, once the page has the set
    bool _clickSetEditButton(const QString& setName);
    /// The editor's map, once it is laid out. Fails the test when the editor shows the other engine's map.
    QQuickItem* _editorMap(bool geoMapEngine);
    void _mouseDrag(Qt::MouseButton button, const QPoint& from, const QPoint& delta);
    static int _visibleItemCount(const QQuickItem* root, const QString& objectName);

    QVariant _savedEngine;
};
