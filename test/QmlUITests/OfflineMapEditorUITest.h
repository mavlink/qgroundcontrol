#pragma once

#include "QmlUITestBase.h"

class QGeoRectangle;
class QQuickItem;

/// UI tests for the Maps settings page: the choice of the map and the offline map set editor, on both map engines.
class OfflineMapEditorUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void initTestCase() override;

    void _newSetAreaFollowsMap_data();
    void _newSetAreaFollowsMap();
    void _newSetEstimateFollowsChosenType_data();
    void _newSetEstimateFollowsChosenType();
    void _newSetMapCentersOnSpecifiedPosition_data();
    void _newSetMapCentersOnSpecifiedPosition();
    void _newSetIsListedAndDownloads_data();
    void _newSetIsListedAndDownloads();
    void _defaultSetInfoOpensAndCloses_data();
    void _defaultSetInfoOpensAndCloses();
    void _setInfoShowsSetAreaAndMap_data();
    void _setInfoShowsSetAreaAndMap();
    void _mapEngineErrorOpensOneDialog();
    void _setCannotBeEditedDuringImport();
    void _renamedSetIsListedByItsNewName();
    void _deletedSetLeavesTheList();
    void _providerAndTypeCombosChooseTheMap_data();
    void _providerAndTypeCombosChooseTheMap();

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
    /// Types \a text into the text field \a field in place of what it shows, and ends the edit
    bool _typeIntoField(QQuickItem* field, const QString& text);
    /// Chooses \a text in the combo box of the labelled combo box \a objectName, as a click on its list does
    bool _chooseInCombo(const QString& objectName, const QString& text);
    static int _visibleItemCount(const QQuickItem* root, const QString& objectName);
};
