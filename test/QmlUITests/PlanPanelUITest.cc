#include "PlanPanelUITest.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>

#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSize>
#include <QtCore/QTemporaryDir>
#include <QtCore/QVariant>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>
#include <QtCore/QtNumeric>
#include <QtPositioning/QGeoCoordinate>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AppSettings.h"
#include "Fact.h"
#include "FactMetaData.h"
#include "GeoFenceController.h"
#include "GeoMapCamera.h"
#include "GeoMapItem.h"
#include "GeoMapProjectedPath.h"
#include "MissionController.h"
#include "MockLink.h"
#include "PlanMasterController.h"
#include "PlanViewSettings.h"
#include "QGCFileDialogController.h"
#include "QGCMAVLink.h"
#include "QGCMapCircle.h"
#include "QGroundControlQmlGlobal.h"
#include "QmlObjectListModel.h"
#include "RallyPoint.h"
#include "RallyPointController.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VisualMissionItem.h"

UT_REGISTER_TEST(PlanPanelUITest, TestLabel::Integration, TestLabel::MissionManager)

namespace {
constexpr int kPanelLayerPlan = 0;
constexpr int kPanelLayerMission = 1;
constexpr int kPanelLayerFence = 2;
constexpr int kPanelLayerRally = 3;

// Tall enough that the panel is above the height where the list and inspector take turns
constexpr int kTallWindowHeight = 1400;

const char* kPlanPanel = "planPanel";
const char* kListInspector = "planPanel_listInspector";
const char* kMissionList = "planPanel_missionList";
const char* kInspectorLoader = "planPanel_inspectorLoader";
const char* kInspectorBack = "planPanel_inspectorBack";
const char* kInspectorPrevious = "planPanel_inspectorPrevious";
const char* kInspectorNext = "planPanel_inspectorNext";
const char* kInspectorDelete = "planPanel_inspectorDelete";
const char* kInspectorMenu = "planPanel_inspectorMenu";
const char* kInspectorTitle = "planPanel_inspectorTitle";
const char* kTemplatesColumn = "planPanel_templatesColumn";
const char* kHomeStatus = "planPanel_homeStatus";
const char* kGeoFenceVisuals = "planView_geoFenceMapVisuals";
const char* kRallyVisuals = "planView_rallyPointMapVisuals";
const char* kStatusHint = "planToolbar_statusHint";
const char* kPanelLayerSwitcher = "planView_layerSwitcher";
const char* kPlanTree = "planView_planTree";
const char* kTakeoffButton = "planToolStrip_takeoffButton";
const char* kWaypointButton = "planToolStrip_waypointButton";

PlanViewSettings* planViewSettings()
{
    return SettingsManager::instance()->planViewSettings();
}

// The GeoMap items live under Loaders, which only the item tree reaches
QList<QQuickItem*> findGeoMapItems(QQuickItem* parent, const QString& objectName)
{
    QList<QQuickItem*> items;
    const auto children = parent->childItems();
    for (QQuickItem* const child : children) {
        if (child->objectName() == objectName) {
            items.append(child);
        }
        items.append(findGeoMapItems(child, objectName));
    }
    return items;
}

QList<QQuickItem*> findGeoMapWaypointMarkers(QQuickItem* parent)
{
    return findGeoMapItems(parent, QStringLiteral("geoMapWaypointMarker"));
}

/// Position in \a map of \a coordinate
QPointF mapScreenPoint(QQuickItem* map, const QGeoCoordinate& coordinate)
{
    QVariant screenPoint;
    const bool projected =
        QMetaObject::invokeMethod(map, "fromCoordinate", Q_RETURN_ARG(QVariant, screenPoint),
                                  Q_ARG(QVariant, QVariant::fromValue(coordinate)), Q_ARG(QVariant, false));
    return projected ? screenPoint.toPointF() : QPointF(-1, -1);
}

QString tabObjectName(const QString& tab)
{
    return QStringLiteral("planPanel_tab_") + tab;
}

QString pageObjectName(const QString& tab)
{
    return QStringLiteral("planPanel_page_") + tab;
}

QString rowObjectName(int sequenceNumber)
{
    return QStringLiteral("planPanel_missionRow_") + QString::number(sequenceNumber);
}

QQuickItem* findFactTextField(QQuickItem* root)
{
    if (!root || !root->isVisible()) {
        return nullptr;
    }
    if (root->property("validationError").isValid() && root->property("fact").isValid()) {
        return root;
    }
    const auto children = root->childItems();
    for (QQuickItem* child : children) {
        if (QQuickItem* const found = findFactTextField(child)) {
            return found;
        }
    }
    return nullptr;
}

/// Leaves an unparsable value in \a textField and validates it as losing focus would
bool enterInvalidValue(QObject* textField)
{
    textField->setProperty("text", QStringLiteral("not a number"));
    return QMetaObject::invokeMethod(textField, "_onEditingFinished") &&
           textField->property("validationError").toBool();
}

/// Plans for an offline vehicle of firmwareClass and vehicleClass until destroyed
class OfflineVehicleScope
{
public:
    OfflineVehicleScope(int firmwareClass, int vehicleClass)
        : _firmware(SettingsManager::instance()->appSettings()->offlineEditingFirmwareClass())
        , _vehicle(SettingsManager::instance()->appSettings()->offlineEditingVehicleClass())
        , _savedFirmware(_firmware->rawValue())
        , _savedVehicle(_vehicle->rawValue())
    {
        _firmware->setRawValue(firmwareClass);
        _vehicle->setRawValue(vehicleClass);
    }

    ~OfflineVehicleScope()
    {
        _firmware->setRawValue(_savedFirmware);
        _vehicle->setRawValue(_savedVehicle);
    }

    Q_DISABLE_COPY_MOVE(OfflineVehicleScope)

private:
    Fact* const _firmware;
    Fact* const _vehicle;
    const QVariant _savedFirmware;
    const QVariant _savedVehicle;
};

bool restoreValidValue(QObject* textField, const QVariant& validText)
{
    textField->setProperty("text", validText);
    return QMetaObject::invokeMethod(textField, "_onEditingFinished") &&
           !textField->property("validationError").toBool();
}
}  // namespace

void PlanPanelUITest::_startPlanPanel(bool geoMapEngine)
{
    setUseTabbedPlanPanel(true);
    setUseGeoMapPlanEngine(geoMapEngine);

    startUI();
    if (QTest::currentTestFailed())
        return;

    // Without an RHI backend the GeoMap View3D warns when it renders
    if (geoMapEngine) {
        const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
        QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");
    }

    _openPlanView();
}

void PlanPanelUITest::_openPlanView()
{
    QVERIFY2(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewPlan")), "Failed to navigate to Plan view");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("mainView_plan"), 2000), "Plan view did not appear");
    QTest::qWait(_viewDelay);

    // Resizing while the Fly view is showing makes its map log a binding loop
    QVERIFY(_window);
    _window->resize(_window->width(), kTallWindowHeight);

    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY2(map, "planView_map not found");
    const QGeoCoordinate mapCenter(47.397742, 8.545594);
    map->setProperty("center", QVariant::fromValue(mapCenter));
    map->setProperty("zoomLevel", 15.0);
    QVERIFY2(waitForCondition(
                 [&] {
                     // The map re-centers on its settings binding while it first gets a size, so keep asking
                     map->setProperty("center", QVariant::fromValue(mapCenter));
                     const QGeoCoordinate c = map->property("center").value<QGeoCoordinate>();
                     return c.isValid() && (c.distanceTo(mapCenter) < 1.0);
                 },
                 3000),
             "Map did not settle on the requested center");

    QVERIFY2(findVisibleItem(_rootItem, QLatin1String(kPlanPanel), 2000), "Tabbed plan panel did not appear");
}

void PlanPanelUITest::_clickMap(qreal fractionX, qreal fractionY)
{
    QVERIFY2(clickItemFraction(QStringLiteral("planView_map"), fractionX, fractionY), "Failed to click planView_map");
}

void PlanPanelUITest::_setHome()
{
    _clickMap(0.4, 0.5);
    if (QTest::currentTestFailed())
        return;
    QVERIFY2(waitForCondition(
                 [&] {
                     MissionController* const controller = _missionController();
                     return controller && controller->homePositionSet();
                 },
                 2000, QStringLiteral("home position set")),
             "Map click on the Plan tab did not set home");
}

MissionController* PlanPanelUITest::_missionController()
{
    QQuickItem* const planView = findVisibleItem(_rootItem, QStringLiteral("mainView_plan"));
    if (!planView) {
        return nullptr;
    }
    return qobject_cast<MissionController*>(planView->property("_missionController").value<QObject*>());
}

QString PlanPanelUITest::_currentTab()
{
    QQuickItem* const panel = findVisibleItem(_rootItem, QLatin1String(kPlanPanel), 0);
    return panel ? panel->property("currentTab").toString() : QString();
}

int PlanPanelUITest::_editingLayer()
{
    QQuickItem* const planView = findVisibleItem(_rootItem, QStringLiteral("mainView_plan"));
    return planView ? planView->property("_editingLayer").toInt() : -1;
}

QString PlanPanelUITest::_layoutMode()
{
    QQuickItem* const listInspector = findItem(_rootItem, QLatin1String(kListInspector));
    return listInspector ? listInspector->property("layoutMode").toString() : QString();
}

QString PlanPanelUITest::_tabStyle()
{
    QQuickItem* const tabBar = findItem(_rootItem, QStringLiteral("planPanel_tabBar"));
    return tabBar ? tabBar->property("style").toString() : QString();
}

bool PlanPanelUITest::_selectTab(const QString& tab)
{
    if (!clickButton(tabObjectName(tab))) {
        return false;
    }
    return waitForCondition([&] { return _currentTab() == tab; }, 2000, QStringLiteral("current tab %1").arg(tab));
}

void PlanPanelUITest::_setHomeAndOpenMission()
{
    _setHome();
    if (QTest::currentTestFailed())
        return;
    QVERIFY2(_selectTab(QStringLiteral("mission")), "Failed to open the Mission tab");
}

void PlanPanelUITest::_buildMission(int waypointCount)
{
    _setHomeAndOpenMission();
    if (QTest::currentTestFailed())
        return;

    QVERIFY2(clickButton(QLatin1String(kTakeoffButton)), "Failed to click Takeoff button");

    QVERIFY2(clickButton(QLatin1String(kWaypointButton)), "Failed to arm the Waypoint tool");
    for (int i = 0; i < waypointCount; i++) {
        _clickMap(0.2 + (0.1 * i), 0.3 + (0.08 * i));
        if (QTest::currentTestFailed())
            return;
    }

    const int expectedItemCount = 2 + waypointCount;  // mission settings item and takeoff included
    QVERIFY2(waitForCondition(
                 [&] {
                     MissionController* const controller = _missionController();
                     return controller && controller->visualItems()->count() == expectedItemCount;
                 },
                 3000, QStringLiteral("mission built")),
             "Mission items were not inserted");
}

void PlanPanelUITest::_testFlagDefaults()
{
    QVERIFY2(planViewSettings()->useNewPlanPanel()->rawDefaultValue().toBool(),
             "The tabbed plan panel must be the default");
    QVERIFY2(!planViewSettings()->useGeoMapEngine()->rawDefaultValue().toBool(), "The GeoMap Plan map must be opt-in");
}

void PlanPanelUITest::_testFlagOffShowsTreeView()
{
    setUseTabbedPlanPanel(false);

    startUI();
    if (QTest::currentTestFailed())
        return;
    QVERIFY2(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewPlan")), "Failed to navigate to Plan view");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("mainView_plan"), 2000), "Plan view did not appear");

    QVERIFY2(findVisibleItem(_rootItem, QLatin1String(kPlanTree), 2000), "Tree view missing with the flag off");
    QVERIFY2(!findVisibleItem(_rootItem, QLatin1String(kPlanPanel), 0), "Tabbed panel shown with the flag off");
}

void PlanPanelUITest::_testEmptyPlanStartsOnPlanTab()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;

    QCOMPARE(_currentTab(), QStringLiteral("plan"));
    QVERIFY2(waitForCondition([&] { return _editingLayer() == kPanelLayerPlan; }, 2000, QStringLiteral("plan layer")),
             "Empty plan did not start on the plan editing layer");
    verifyVisibility(QLatin1String(kPanelLayerSwitcher), false, QStringLiteral("layer switcher hidden"));
    verifyVisibility(QLatin1String(kPlanTree), false, QStringLiteral("tree hidden"));
    verifyVisibility(QString::fromLatin1(kTakeoffButton), false,
                     QStringLiteral("tool strip hidden off the Mission tab"));
    for (const QString& tab :
         {QStringLiteral("plan"), QStringLiteral("mission"), QStringLiteral("fence"), QStringLiteral("rally")}) {
        verifyVisibility(QStringLiteral("planPanel_tabBadge_") + tab, false,
                         QStringLiteral("no warning badge on %1 for an empty plan").arg(tab));
    }
    if (QTest::currentTestFailed())
        return;

    // Templates need a home position, and a map click on the Plan tab sets it
    verifyVisibility(QLatin1String(kTemplatesColumn), true, QStringLiteral("templates shown for an empty plan"));
    verifyEnabled(QLatin1String(kTemplatesColumn), false, QStringLiteral("templates before home"));
    if (QTest::currentTestFailed())
        return;

    _clickMap(0.4, 0.5);
    if (QTest::currentTestFailed())
        return;
    QVERIFY2(waitForCondition(
                 [&] {
                     MissionController* const controller = _missionController();
                     return controller && controller->homePositionSet();
                 },
                 2000, QStringLiteral("home position set")),
             "Map click on the Plan tab did not set home");
    verifyEnabled(QLatin1String(kTemplatesColumn), true, QStringLiteral("templates after home"));
    QVERIFY2(waitForCondition(
                 [&] {
                     QQuickItem* const status = findVisibleItem(_rootItem, QLatin1String(kHomeStatus), 0);
                     return status && status->property("text").toString().startsWith(QStringLiteral("Set"));
                 },
                 2000, QStringLiteral("home status")),
             "Plan tab did not report the home position as set");

    // A failed terrain lookup leaves the planned home altitude for the user to enter
    verifyVisibility(QStringLiteral("planPanel_homeAltitude"), false, QStringLiteral("no altitude field by default"));
    QObject* const settingsItem = _missionController()->visualItems()->get(0);
    QVERIFY(settingsItem);
    QVERIFY(QMetaObject::invokeMethod(settingsItem, "_terrainDataReceived", Q_ARG(bool, false),
                                      Q_ARG(QList<double>, QList<double>())));
    verifyVisibility(QStringLiteral("planPanel_homeAltitude"), true,
                     QStringLiteral("altitude field after a failed terrain query"));
    QCOMPARE(_currentTab(), QStringLiteral("plan"));
    if (QTest::currentTestFailed())
        return;

    // An invalid Plan tab field blocks creating a plan from a template, which would switch to Mission
    QObject* const homeAltitude = findItem(_rootItem, QStringLiteral("planPanel_homeAltitude"));
    QVERIFY(homeAltitude);
    const QVariant validHomeAltitude = homeAltitude->property("fact").value<QObject*>()->property("valueString");
    QVERIFY2(enterInvalidValue(homeAltitude), "Non-numeric home altitude was accepted");
    const int itemCount = _missionController()->visualItems()->count();
    QVERIFY2(clickButton(QStringLiteral("planCreator_Survey")), "Failed to click the Survey template");
    QCOMPARE(_missionController()->visualItems()->count(), itemCount);
    QCOMPARE(_currentTab(), QStringLiteral("plan"));
    QVERIFY(restoreValidValue(homeAltitude, validHomeAltitude));

    // Choosing no template starts manual mission editing
    QVERIFY2(clickButton(QStringLiteral("planCreator_No Template")), "Failed to click No Template");
    QVERIFY2(
        waitForCondition([&] { return _currentTab() == QStringLiteral("mission"); }, 2000, QStringLiteral("mission")),
        "No Template did not open the Mission tab");
    QCOMPARE(_editingLayer(), kPanelLayerMission);
}

void PlanPanelUITest::_testTabSelectionDrivesEditingLayer()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;

    struct TabState
    {
        const char* tab;
        int layer;
        bool toolStripVisible;
        bool fenceInteractive;
        bool rallyInteractive;
        const char* statusHint;  // Empty: no hint pill
    };

    // One UI session walks all tabs since each step starts from the state the previous one left
    const TabState states[] = {
        {"mission", kPanelLayerMission, true, false, false, ""},
        {"fence", kPanelLayerFence, false, true, false, ""},
        {"rally", kPanelLayerRally, false, false, true, "Click in map to add rally points"},
        {"plan", kPanelLayerPlan, false, false, false, "Click in map to move Home"},
    };

    // Only the Plan tab, where home is set, can be opened until there is a home position
    verifyText(QLatin1String(kStatusHint), QStringLiteral("Click in map to set Home"),
               QStringLiteral("plan tab without home"));
    for (const QString& tab : {QStringLiteral("mission"), QStringLiteral("fence"), QStringLiteral("rally")}) {
        verifyEnabled(tabObjectName(tab), false, QStringLiteral("%1 tab without home").arg(tab));
    }
    if (QTest::currentTestFailed())
        return;
    _setHome();
    if (QTest::currentTestFailed())
        return;

    for (const TabState& state : states) {
        const QString tab = QLatin1String(state.tab);
        QVERIFY2(_selectTab(tab), qPrintable(QStringLiteral("Failed to select the %1 tab").arg(tab)));

        QVERIFY2(
            waitForCondition([&] { return _editingLayer() == state.layer; }, 2000, QStringLiteral("editing layer")),
            qPrintable(QStringLiteral("%1 tab: expected editing layer %2 but found %3")
                           .arg(tab)
                           .arg(state.layer)
                           .arg(_editingLayer())));
        verifyProperty(tabObjectName(tab), "current", true, tab);
        verifyVisibility(pageObjectName(tab), true, tab);
        verifyVisibility(QString::fromLatin1(kTakeoffButton), state.toolStripVisible, tab);
        verifyProperty(QLatin1String(kGeoFenceVisuals), "interactive", state.fenceInteractive, tab);
        verifyProperty(QLatin1String(kRallyVisuals), "interactive", state.rallyInteractive, tab);
        const QString statusHint = QLatin1String(state.statusHint);
        verifyVisibility(QLatin1String(kStatusHint), !statusHint.isEmpty(), tab);
        if (!statusHint.isEmpty()) {
            verifyText(QLatin1String(kStatusHint), statusHint, tab);
        }
        for (const QString& other :
             {QStringLiteral("plan"), QStringLiteral("mission"), QStringLiteral("fence"), QStringLiteral("rally")}) {
            if (other != tab) {
                verifyVisibility(pageObjectName(other), false,
                                 QStringLiteral("%1 page hidden on %2 tab").arg(other, tab));
                verifyProperty(tabObjectName(other), "current", false, tab);
            }
        }
        if (QTest::currentTestFailed())
            return;
    }

    // The tree created by turning the tabbed panel off must not keep the Plan tab's layer
    setUseTabbedPlanPanel(false);
    QVERIFY2(findVisibleItem(_rootItem, QLatin1String(kPlanTree), 2000),
             "Tree view missing after turning the flag off");
    QVERIFY2(
        waitForCondition([&] { return _editingLayer() == kPanelLayerMission; }, 2000, QStringLiteral("mission layer")),
        qPrintable(QStringLiteral("Tree view left the editing layer at %1").arg(_editingLayer())));
}

void PlanPanelUITest::_testDisabledTabsDoNotPassClicksToTheMap_data()
{
    QTest::addColumn<bool>("geoMapEngine");

    QTest::newRow("QtLocation map") << false;
    QTest::newRow("GeoMap") << true;
}

void PlanPanelUITest::_testDisabledTabsDoNotPassClicksToTheMap()
{
    QFETCH(bool, geoMapEngine);

    _startPlanPanel(geoMapEngine);
    if (QTest::currentTestFailed())
        return;

    MissionController* const controller = _missionController();
    QVERIFY(controller);
    QVERIFY(!controller->homePositionSet());

    // A map click on the Plan tab sets home, so one passed through a disabled tab would set it behind the tab
    for (const QString& tab : {QStringLiteral("mission"), QStringLiteral("fence"), QStringLiteral("rally")}) {
        verifyEnabled(tabObjectName(tab), false, QStringLiteral("%1 tab without home").arg(tab));
        if (QTest::currentTestFailed())
            return;
        QVERIFY2(clickButton(tabObjectName(tab)), qPrintable(QStringLiteral("Failed to click the %1 tab").arg(tab)));
        QVERIFY2(!controller->homePositionSet(),
                 qPrintable(QStringLiteral("Clicking the disabled %1 tab set home").arg(tab)));
        QCOMPARE(_currentTab(), QStringLiteral("plan"));
    }

    // The gap between two pills
    QQuickItem* const missionTab = findVisibleItem(_rootItem, tabObjectName(QStringLiteral("mission")));
    QQuickItem* const fenceTab = findVisibleItem(_rootItem, tabObjectName(QStringLiteral("fence")));
    QVERIFY(missionTab && fenceTab);
    const QPointF gap((missionTab->mapToScene(QPointF(missionTab->width() / 2, missionTab->height())) +
                       fenceTab->mapToScene(QPointF(fenceTab->width() / 2, 0))) /
                      2);
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, gap.toPoint());
    QVERIFY2(!controller->homePositionSet(), "Clicking between two tabs set home");

    // The other buttons stay on the tab bar too: on the GeoMap a right drag would orbit the camera
    if (geoMapEngine) {
        QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
        QVERIFY(map);
        auto* const camera = map->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
        QVERIFY2(camera, "GeoMapCamera not found");
        const double headingBefore = camera->heading();
        const QPoint start = fenceTab->mapToScene(QPointF(fenceTab->width() / 2, fenceTab->height() / 2)).toPoint();
        QTest::mousePress(_window, Qt::RightButton, Qt::NoModifier, start);
        constexpr int steps = 10;
        for (int i = 1; i <= steps; i++) {
            QTest::mouseMove(_window, start + (QPoint(-80, 0) * i / steps));
        }
        QTest::mouseRelease(_window, Qt::RightButton, Qt::NoModifier, start + QPoint(-80, 0));
        QVERIFY2(
            qFuzzyCompare(camera->heading() + 1, headingBefore + 1),
            qPrintable(
                QStringLiteral("A right drag on the tab bar turned the map to heading %1").arg(camera->heading())));
    }
}

void PlanPanelUITest::_testDefaultAltitudeChipOpensPlanTab()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;

    _setHomeAndOpenMission();
    if (QTest::currentTestFailed())
        return;
    QVERIFY2(clickButton(QStringLiteral("planPanel_defaultAltitudeChip")), "Failed to click the default altitude chip");
    QVERIFY2(
        waitForCondition([&] { return _currentTab() == QStringLiteral("plan"); }, 2000, QStringLiteral("chip to plan")),
        "Default altitude chip did not open the Plan tab");
}

void PlanPanelUITest::_testMissionListFollowsItemsAndSelection()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;

    _buildMission(2);
    if (QTest::currentTestFailed())
        return;

    MissionController* const controller = _missionController();
    QVERIFY(controller);
    QCOMPARE(_currentTab(), QStringLiteral("mission"));

    // Settings item, takeoff and two waypoints each get a row
    for (int seq = 0; seq < 4; seq++) {
        QVERIFY2(findVisibleItem(_rootItem, rowObjectName(seq), 2000),
                 qPrintable(QStringLiteral("Row %1 missing from the mission list").arg(seq)));
    }
    if (QTest::currentTestFailed())
        return;

    // The newest item is current; its inspector title follows it
    QVERIFY2(
        waitForCondition([&] { return controller->currentPlanViewSeqNum() == 3; }, 2000,
                         QStringLiteral("newest item current")),
        qPrintable(
            QStringLiteral("Newest item is not current (current is %1)").arg(controller->currentPlanViewSeqNum())));
    verifyProperty(rowObjectName(3), "_current", true, QStringLiteral("newest row current"));
    verifyText(QString::fromLatin1(kInspectorTitle), controller->currentPlanViewItem()->commandName(),
               QStringLiteral("inspector title"));
    if (QTest::currentTestFailed())
        return;

    QVERIFY2(clickButton(rowObjectName(1)), "Failed to click row 1");
    QVERIFY2(waitForCondition([&] { return controller->currentPlanViewSeqNum() == 1; }, 2000,
                              QStringLiteral("row selects item")),
             "Clicking a row did not select its item");
    verifyProperty(rowObjectName(1), "_current", true, QStringLiteral("clicked row current"));
    verifyProperty(rowObjectName(3), "_current", false, QStringLiteral("old row no longer current"));
    verifyText(QString::fromLatin1(kInspectorTitle), controller->currentPlanViewItem()->commandName(),
               QStringLiteral("inspector title after click"));
    if (QTest::currentTestFailed())
        return;

    // The mission settings item cannot be deleted
    QVERIFY2(clickButton(rowObjectName(0)), "Failed to click row 0");
    QVERIFY2(waitForCondition([&] { return controller->currentPlanViewSeqNum() == 0; }, 2000,
                              QStringLiteral("settings row selected")),
             "Clicking the settings row did not select it");
    verifyVisibility(QLatin1String(kInspectorDelete), false, QStringLiteral("delete hidden for the settings item"));
    verifyVisibility(QLatin1String(kInspectorMenu), false, QStringLiteral("menu hidden for the settings item"));
    if (QTest::currentTestFailed())
        return;

    QVERIFY2(clickButton(rowObjectName(3)), "Failed to click row 3");
    QVERIFY2(waitForCondition([&] { return controller->currentPlanViewSeqNum() == 3; }, 2000,
                              QStringLiteral("row 3 selected")),
             "Clicking row 3 did not select it");

    // An invalid field in the editor blocks deleting the item until it is corrected
    QQuickItem* editorField = nullptr;
    QVERIFY2(waitForCondition(
                 [&] {
                     editorField =
                         findFactTextField(findVisibleItem(_rootItem, QStringLiteral("planPanel_inspectorEditor"), 0));
                     return editorField != nullptr;
                 },
                 2000, QStringLiteral("editor field")),
             "The item editor shows no Fact text field");
    const QVariant validValue = editorField->property("fact").value<QObject*>()->property("valueString");
    QVERIFY2(enterInvalidValue(editorField), "Non-numeric value was accepted");
    QVERIFY2(clickButton(QLatin1String(kInspectorDelete)), "Failed to click the inspector delete button");
    QCOMPARE(controller->visualItems()->count(), 4);
    QObject* const mapIndicators = findItem(_rootItem, QStringLiteral("planView_missionItemIndicators"));
    QVERIFY(mapIndicators);
    QVERIFY(QMetaObject::invokeMethod(mapIndicators, "itemSelected", Q_ARG(int, 1)));
    QCOMPARE(controller->currentPlanViewSeqNum(), 3);
    QVERIFY(restoreValidValue(editorField, validValue));

    QVERIFY2(clickButton(QLatin1String(kInspectorDelete)), "Failed to click the inspector delete button");
    QVERIFY2(
        waitForCondition([&] { return controller->visualItems()->count() == 3; }, 2000, QStringLiteral("item deleted")),
        "Inspector delete did not remove the item");
    QVERIFY2(waitForCondition([&] { return !findVisibleItem(_rootItem, rowObjectName(3), 0); }, 2000,
                              QStringLiteral("row removed")),
             "Deleted item still has a row");
}

void PlanPanelUITest::_testItemsAddedOnPlanTabSwitchToMission()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;

    _clickMap(0.4, 0.5);
    if (QTest::currentTestFailed())
        return;
    MissionController* const controller = _missionController();
    QVERIFY(controller);
    QVERIFY2(waitForCondition([&] { return controller->homePositionSet(); }, 2000, QStringLiteral("home set")),
             "Home was not set");
    QCOMPARE(_currentTab(), QStringLiteral("plan"));

    controller->insertSimpleMissionItem(QGeoCoordinate(47.3979, 8.5459), controller->visualItems()->count(),
                                        true /* makeCurrentItem */);
    QVERIFY2(waitForCondition([&] { return _currentTab() == QStringLiteral("mission"); }, 2000,
                              QStringLiteral("switch to mission")),
             "Adding an item while on the Plan tab did not open the Mission tab");

    // Items added while already on another tab leave the tab alone
    QVERIFY2(_selectTab(QStringLiteral("fence")), "Failed to open the Fence tab");
    controller->insertSimpleMissionItem(QGeoCoordinate(47.3981, 8.5461), controller->visualItems()->count(),
                                        true /* makeCurrentItem */);
    QVERIFY2(
        waitForCondition([&] { return controller->visualItems()->count() == 3; }, 2000, QStringLiteral("second item")),
        "Second item was not inserted");
    QCOMPARE(_currentTab(), QStringLiteral("fence"));

    // Clearing the plan returns to the Plan tab, even when rally points are cleared after the mission
    QQuickItem* const planView = findVisibleItem(_rootItem, QStringLiteral("mainView_plan"));
    QVERIFY(planView);
    PlanMasterController* const masterController =
        qvariant_cast<PlanMasterController*>(planView->property("_planMasterController"));
    QVERIFY(masterController);
    masterController->rallyPointController()->addPoint(QGeoCoordinate(47.3983, 8.5463));
    QVERIFY(masterController->rallyPointController()->containsItems());
    masterController->removeAll();
    QVERIFY2(
        waitForCondition([&] { return _currentTab() == QStringLiteral("plan"); }, 2000, QStringLiteral("cleared plan")),
        "Clearing the plan did not return to the Plan tab");
}

void PlanPanelUITest::_testIncompleteItemsAreFlagged()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;

    _setHomeAndOpenMission();
    if (QTest::currentTestFailed())
        return;

    MissionController* const controller = _missionController();
    QVERIFY(controller);
    const QVariantList complexItems = controller->property("complexMissionItems").toList();
    QVERIFY2(!complexItems.isEmpty(), "No complex mission items available");
    const QString canonicalName = complexItems.first().toMap().value(QStringLiteral("canonicalName")).toString();

    // A pattern without its area defined cannot be saved yet
    VisualMissionItem* const complexItem =
        controller->insertComplexMissionItem(canonicalName, QGeoCoordinate(47.3977, 8.5455), 1, true);
    QVERIFY(complexItem);
    QVERIFY2(complexItem->readyForSaveState() != VisualMissionItem::ReadyForSave,
             "Pattern item without an area should be incomplete");

    const int sequenceNumber = complexItem->sequenceNumber();
    QVERIFY2(findVisibleItem(_rootItem, rowObjectName(sequenceNumber), 2000), "Row for the pattern item missing");
    verifyProperty(QStringLiteral("planPanel_missionRowIndex_") + QString::number(sequenceNumber), "text",
                   QStringLiteral("?"), QStringLiteral("incomplete row marker"));
    QVERIFY2(waitForCondition(
                 [&] {
                     QQuickItem* const summary =
                         findVisibleItem(_rootItem, QStringLiteral("planPanel_missionSummary"), 0);
                     return summary && summary->property("text").toString().contains(QStringLiteral("1"));
                 },
                 2000, QStringLiteral("summary counts incomplete")),
             "Mission summary does not report the incomplete item");
    verifyVisibility(QStringLiteral("planPanel_tabBadge_mission"), true, QStringLiteral("mission tab warning badge"));
}

void PlanPanelUITest::_testDrillInShowsListOrInspector()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;

    _buildMission(2);
    if (QTest::currentTestFailed())
        return;
    MissionController* const controller = _missionController();
    QVERIFY(controller);

    // Split layout: list and inspector together, no back or step buttons
    QCOMPARE(_layoutMode(), QStringLiteral("split"));
    verifyVisibility(QLatin1String(kMissionList), true, QStringLiteral("split list"));
    verifyVisibility(QLatin1String(kInspectorLoader), true, QStringLiteral("split inspector"));
    verifyVisibility(QLatin1String(kInspectorBack), false, QStringLiteral("split back button"));
    if (QTest::currentTestFailed())
        return;

    QQuickItem* const listInspector = findVisibleItem(_rootItem, QLatin1String(kListInspector));
    QVERIFY(listInspector);
    listInspector->setProperty("layoutModeOverride", QStringLiteral("drillIn"));
    QVERIFY2(waitForCondition([&] { return _layoutMode() == QStringLiteral("drillIn"); }, 2000,
                              QStringLiteral("drill-in layout")),
             "Layout override did not apply");

    // Drill-in starts on the list even though an item is current
    verifyVisibility(QLatin1String(kMissionList), true, QStringLiteral("drill-in list"));
    verifyVisibility(QLatin1String(kInspectorLoader), false, QStringLiteral("drill-in inspector hidden"));
    if (QTest::currentTestFailed())
        return;

    QVERIFY2(clickButton(rowObjectName(2)), "Failed to click row 2");
    verifyVisibility(QLatin1String(kInspectorLoader), true, QStringLiteral("inspector after row click"));
    verifyVisibility(QLatin1String(kMissionList), false, QStringLiteral("list hidden while inspecting"));
    verifyVisibility(QLatin1String(kInspectorBack), true, QStringLiteral("back button"));
    QCOMPARE(controller->currentPlanViewSeqNum(), 2);
    if (QTest::currentTestFailed())
        return;

    // Stepping moves the selection without returning to the list
    QVERIFY2(clickButton(QLatin1String(kInspectorNext)), "Failed to click Next");
    QVERIFY2(
        waitForCondition([&] { return controller->currentPlanViewSeqNum() == 3; }, 2000, QStringLiteral("next item")),
        "Next did not select the following item");
    verifyEnabled(QLatin1String(kInspectorNext), false, QStringLiteral("next disabled on the last item"));
    verifyEnabled(QLatin1String(kInspectorPrevious), true, QStringLiteral("previous enabled"));
    verifyVisibility(QLatin1String(kMissionList), false, QStringLiteral("list stays hidden while stepping"));
    if (QTest::currentTestFailed())
        return;

    QVERIFY2(clickButton(QLatin1String(kInspectorPrevious)), "Failed to click Previous");
    QVERIFY2(waitForCondition([&] { return controller->currentPlanViewSeqNum() == 2; }, 2000,
                              QStringLiteral("previous item")),
             "Previous did not select the preceding item");

    QVERIFY2(clickButton(QLatin1String(kInspectorBack)), "Failed to click Back");
    verifyVisibility(QLatin1String(kMissionList), true, QStringLiteral("list after back"));
    verifyVisibility(QLatin1String(kInspectorLoader), false, QStringLiteral("inspector after back"));
    QCOMPARE(controller->currentPlanViewSeqNum(), 2);
}

void PlanPanelUITest::_testPanelHeightSelectsLayout()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;

    _buildMission(1);
    if (QTest::currentTestFailed())
        return;
    QVERIFY(_window);

    const QSize originalSize = _window->size();
    QCOMPARE(_layoutMode(), QStringLiteral("split"));
    QCOMPARE(_tabStyle(), QStringLiteral("text"));

    _window->resize(originalSize.width(), qRound(originalSize.height() * 0.2));
    QVERIFY2(waitForCondition([&] { return _layoutMode() == QStringLiteral("drillIn"); }, 3000,
                              QStringLiteral("short window drill-in")),
             qPrintable(
                 QStringLiteral("A %1x%2 window kept the split layout").arg(_window->width()).arg(_window->height())));
    QVERIFY2(waitForCondition([&] { return _tabStyle() == QStringLiteral("icon"); }, 3000,
                              QStringLiteral("short window icon tabs")),
             "A short window kept the text tabs");

    _window->resize(originalSize);
    QVERIFY2(waitForCondition([&] { return _layoutMode() == QStringLiteral("split"); }, 3000,
                              QStringLiteral("restored window split")),
             "Restoring the window height did not restore the split layout");
    QVERIFY2(waitForCondition([&] { return _tabStyle() == QStringLiteral("text"); }, 3000,
                              QStringLiteral("restored window text tabs")),
             "Restoring the window height did not restore the text tabs");
}

void PlanPanelUITest::_testDividerResizesListAndInspector()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;

    _buildMission(2);
    if (QTest::currentTestFailed())
        return;

    QQuickItem* const listInspector = findItem(_rootItem, QLatin1String(kListInspector));
    QVERIFY(listInspector);
    QCOMPARE(_layoutMode(), QStringLiteral("split"));

    const auto dividerCenter = [&]() {
        QQuickItem* const divider = findVisibleItem(_rootItem, QStringLiteral("planPanel_listInspectorDivider"), 2000);
        return divider ? divider->mapToScene(QPointF(divider->width() / 2, divider->height() / 2)).toPoint()
                       : QPoint(-1, -1);
    };
    const auto drag = [&](int deltaY) {
        const QPoint start = dividerCenter();
        QTest::mousePress(_window, Qt::LeftButton, Qt::NoModifier, start);
        QTest::mouseMove(_window, start + QPoint(0, deltaY / 2));
        QTest::mouseMove(_window, start + QPoint(0, deltaY));
        QTest::mouseRelease(_window, Qt::LeftButton, Qt::NoModifier, start + QPoint(0, deltaY));
    };
    const auto listHeight = [&]() { return listInspector->property("_splitListHeight").toReal(); };

    QVERIFY2(dividerCenter().x() >= 0, "Divider not visible with a selected item in the split layout");
    const qreal initialHeight = listHeight();
    QVERIFY2(listInspector->property("userListHeight").toReal() < 0, "List height should start automatic");

    drag(30);
    QVERIFY2(waitForCondition([&] { return qAbs(listHeight() - (initialHeight + 30)) < 2; }, 2000,
                              QStringLiteral("drag down")),
             qPrintable(
                 QStringLiteral("Dragging down 30 moved the list from %1 to %2").arg(initialHeight).arg(listHeight())));
    QVERIFY2(listInspector->property("userListHeight").toReal() >= 0, "Dragging did not set a user list height");

    drag(-15);
    QVERIFY2(waitForCondition([&] { return qAbs(listHeight() - (initialHeight + 15)) < 2; }, 2000,
                              QStringLiteral("drag up")),
             qPrintable(QStringLiteral("Dragging up 15 left the list at %1").arg(listHeight())));

    // Neither side can be dragged below its minimum height
    const qreal minList = listInspector->property("_minListHeight").toReal();
    const qreal maxList = listInspector->height() - listInspector->property("_dividerHeight").toReal() -
                          listInspector->property("_minInspectorHeight").toReal();
    drag(5 - dividerCenter().y());
    QVERIFY2(
        waitForCondition([&] { return qAbs(listHeight() - minList) < 2; }, 2000, QStringLiteral("minimum list height")),
        qPrintable(
            QStringLiteral("Dragging to the top left the list at %1, minimum is %2").arg(listHeight()).arg(minList)));
    drag(_window->height() - 5 - dividerCenter().y());
    QVERIFY2(waitForCondition([&] { return qAbs(listHeight() - maxList) < 2; }, 2000,
                              QStringLiteral("minimum inspector height")),
             qPrintable(QStringLiteral("Dragging to the bottom left the list at %1, maximum is %2")
                            .arg(listHeight())
                            .arg(maxList)));

    // Double click returns to the height of the list content
    const QPoint center = dividerCenter();
    QTest::mouseDClick(_window, Qt::LeftButton, Qt::NoModifier, center);
    QVERIFY2(waitForCondition([&] { return qAbs(listHeight() - initialHeight) < 2; }, 2000, QStringLiteral("reset")),
             qPrintable(
                 QStringLiteral("Double click left the list at %1, expected %2").arg(listHeight()).arg(initialHeight)));
}

void PlanPanelUITest::_testItemMenuActsOnInspectedItem()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;

    _buildMission(2);
    if (QTest::currentTestFailed())
        return;

    MissionController* const controller = _missionController();
    QVERIFY(controller);
    VisualMissionItem* const item = controller->currentPlanViewItem();
    QVERIFY(item);
    const int itemIndex = controller->visualItems()->indexOf(item);
    QVERIFY(itemIndex > 0);
    const QmlObjectListModel* const visualItems = controller->visualItems();
    const QGeoCoordinate previousCoordinate = visualItems->value<VisualMissionItem*>(itemIndex - 1)->coordinate();
    QVERIFY2(item->coordinate().distanceTo(previousCoordinate) > 1.0,
             "The inspected item must start away from the previous item");

    QVERIFY2(clickButton(QLatin1String(kInspectorMenu)), "Failed to click the inspector menu button");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("missionItemMenu_editPosition"), 2000),
             "Item menu did not open");
    verifyEnabled(QStringLiteral("missionItemMenu_editPosition"), true, QStringLiteral("edit position"));
    // Offline there is no vehicle position to move to
    verifyEnabled(QStringLiteral("missionItemMenu_moveToVehicle"), false, QStringLiteral("move to vehicle"));
    verifyEnabled(QStringLiteral("missionItemMenu_moveToPrevious"), true, QStringLiteral("move to previous"));
    if (QTest::currentTestFailed())
        return;

    QVERIFY2(clickButton(QStringLiteral("missionItemMenu_moveToPrevious")), "Failed to click Move to previous");
    QVERIFY2(waitForCondition([&] { return item->coordinate().distanceTo(previousCoordinate) < 1.0; }, 2000,
                              QStringLiteral("item moved")),
             "Move to previous did not move the inspected item");
}

void PlanPanelUITest::_testGeoMapMarkerClickSelectsItem()
{
    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    _buildMission(2);
    if (QTest::currentTestFailed())
        return;
    // Disarm the Waypoint tool so a marker click can only select
    QVERIFY2(clickButton(QLatin1String(kWaypointButton)), "Failed to disarm the Waypoint tool");

    MissionController* const controller = _missionController();
    QVERIFY(controller);
    const int itemCount = controller->visualItems()->count();
    VisualMissionItem* const target = controller->visualItems()->value<VisualMissionItem*>(1);
    QVERIFY(target);
    QVERIFY2(controller->currentPlanViewItem() != target, "The target item must start unselected");

    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    QVariant screenPoint;
    QVERIFY(QMetaObject::invokeMethod(map, "fromCoordinate", Q_RETURN_ARG(QVariant, screenPoint),
                                      Q_ARG(QVariant, QVariant::fromValue(target->coordinate())),
                                      Q_ARG(QVariant, false)));
    const QPointF point = screenPoint.toPointF();
    QVERIFY2(QRectF(QPointF(0, 0), map->size()).contains(point), "The target item is not on screen");
    QVERIFY2(clickItemFraction(QStringLiteral("planView_map"), point.x() / map->width(), point.y() / map->height()),
             "Failed to click the item marker");

    QVERIFY2(waitForCondition([&] { return controller->currentPlanViewItem() == target; }, 2000,
                              QStringLiteral("marker selects item")),
             "Clicking the item marker did not select the item");
    QCOMPARE(controller->visualItems()->count(), itemCount);
    verifyText(QString::fromLatin1(kInspectorTitle), target->commandName(), QStringLiteral("inspector title"));
}

void PlanPanelUITest::_testGeoMapMarkerDragMovesOnlyTheCurrentItem_data()
{
    QTest::addColumn<bool>("dragCurrentItem");
    QTest::addColumn<QString>("tab");
    QTest::addColumn<int>("modifier");

    QTest::newRow("current item moves") << true << QStringLiteral("mission") << int(Qt::NoModifier);
    QTest::newRow("other item pans the map") << false << QStringLiteral("mission") << int(Qt::NoModifier);
    QTest::newRow("current item off the mission layer pans the map")
        << true << QStringLiteral("fence") << int(Qt::NoModifier);
    QTest::newRow("shift drag of the current item orbits the map")
        << true << QStringLiteral("mission") << int(Qt::ShiftModifier);
    QTest::newRow("control drag of the current item looks around")
        << true << QStringLiteral("mission") << int(Qt::ControlModifier);
}

void PlanPanelUITest::_testGeoMapMarkerDragMovesOnlyTheCurrentItem()
{
    QFETCH(bool, dragCurrentItem);
    QFETCH(QString, tab);
    QFETCH(int, modifier);
    const auto modifiers = Qt::KeyboardModifiers(modifier);
    const bool expectMove = dragCurrentItem && (tab == QStringLiteral("mission")) && !modifiers;

    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    _buildMission(2);
    if (QTest::currentTestFailed())
        return;
    // Disarm the Waypoint tool so a marker press can only select or drag
    QVERIFY2(clickButton(QLatin1String(kWaypointButton)), "Failed to disarm the Waypoint tool");
    if (tab != QStringLiteral("mission")) {
        QVERIFY2(_selectTab(tab), "Failed to open the tab");
    }

    MissionController* const controller = _missionController();
    QVERIFY(controller);
    const int itemCount = controller->visualItems()->count();
    VisualMissionItem* const current = controller->currentPlanViewItem();
    VisualMissionItem* const target =
        dragCurrentItem ? current : controller->visualItems()->value<VisualMissionItem*>(itemCount - 2);
    QVERIFY(target);
    QCOMPARE(target == current, dragCurrentItem);

    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    const auto windowPoint = [map](const QGeoCoordinate& coordinate) {
        QVariant screenPoint;
        const bool projected =
            QMetaObject::invokeMethod(map, "fromCoordinate", Q_RETURN_ARG(QVariant, screenPoint),
                                      Q_ARG(QVariant, QVariant::fromValue(coordinate)), Q_ARG(QVariant, false));
        return projected ? map->mapToScene(screenPoint.toPointF()).toPoint() : QPoint(-1, -1);
    };

    const QGeoCoordinate before = target->coordinate();
    const double altitudeBefore = target->property("amslEntryAlt").toDouble();
    const QPoint start = windowPoint(before);
    QVERIFY2(map->contains(map->mapFromScene(start)), "The marker is not on screen");
    const QPoint delta(-70, 50);

    QTest::mousePress(_window, Qt::LeftButton, modifiers, start);
    constexpr int steps = 10;
    for (int i = 1; i <= steps; i++) {
        QTest::mouseMove(_window, start + ((delta * i) / steps));
    }
    QTest::mouseRelease(_window, Qt::LeftButton, modifiers, start + delta);

    if (expectMove) {
        QVERIFY2(waitForCondition([&] { return target->coordinate().distanceTo(before) > 1.0; }, 2000,
                                  QStringLiteral("item moved")),
                 "Dragging the current item's marker did not move the item");
        // The marker ends up under the cursor, with the item's altitude untouched
        const QPoint end = windowPoint(target->coordinate());
        QVERIFY2((end - (start + delta)).manhattanLength() <= 4,
                 qPrintable(QStringLiteral("Marker ended at %1,%2 instead of under the cursor at %3,%4")
                                .arg(end.x())
                                .arg(end.y())
                                .arg(start.x() + delta.x())
                                .arg(start.y() + delta.y())));
        QVERIFY(qFuzzyCompare(target->property("amslEntryAlt").toDouble(), altitudeBefore));
        QCOMPARE(controller->currentPlanViewItem(), target);
    } else {
        // The press went to the map gesture: the map followed the cursor and the item stayed put
        if (!modifiers) {
            QVERIFY2(waitForCondition([&] { return (windowPoint(before) - start).manhattanLength() > 20; }, 2000,
                                      QStringLiteral("map panned")),
                     "Dragging another item's marker did not pan the map");
        }
        QVERIFY2(target->coordinate().distanceTo(before) < 0.01, "Dragging a non-current marker moved its item");
    }
    QCOMPARE(controller->visualItems()->count(), itemCount);
}

void PlanPanelUITest::_testGeoMapRallyMarkerClickSelectsPoint()
{
    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;
    _setHome();
    if (QTest::currentTestFailed())
        return;

    RallyPointController* const rally = _planMasterController()->rallyPointController();
    QVERIFY2(rally->supported(), "Offline vehicle should support rally points");
    QVERIFY2(_selectTab(QStringLiteral("rally")), "Failed to open the Rally tab");
    _clickMap(0.3, 0.4);
    _clickMap(0.45, 0.6);
    QVERIFY2(waitForCondition([&] { return rally->points()->count() == 2; }, 2000, QStringLiteral("points added")),
             "Map clicks did not add rally points");
    QObject* const first = rally->points()->get(0);
    QCOMPARE(rally->currentRallyPoint(), rally->points()->get(1));

    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    const QPointF point = mapScreenPoint(map, first->property("coordinate").value<QGeoCoordinate>());
    QVERIFY2(QRectF(QPointF(0, 0), map->size()).contains(point), "The rally point is not on screen");
    QVERIFY2(clickItemFraction(QStringLiteral("planView_map"), point.x() / map->width(), point.y() / map->height()),
             "Failed to click the rally point marker");

    QVERIFY2(waitForCondition([&] { return rally->currentRallyPoint() == first; }, 2000,
                              QStringLiteral("marker selects point")),
             "Clicking the rally point marker did not select the point");
    QCOMPARE(rally->points()->count(), 2);
}

void PlanPanelUITest::_testGeoMapRallyMarkerDragMovesOnlyTheCurrentPoint_data()
{
    QTest::addColumn<bool>("dragCurrentPoint");
    QTest::addColumn<QString>("tab");

    QTest::newRow("current point moves") << true << QStringLiteral("rally");
    QTest::newRow("other point pans the map") << false << QStringLiteral("rally");
    QTest::newRow("current point off the rally layer pans the map") << true << QStringLiteral("fence");
}

void PlanPanelUITest::_testGeoMapRallyMarkerDragMovesOnlyTheCurrentPoint()
{
    QFETCH(bool, dragCurrentPoint);
    QFETCH(QString, tab);
    const bool expectMove = dragCurrentPoint && (tab == QStringLiteral("rally"));

    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;
    _setHome();
    if (QTest::currentTestFailed())
        return;

    RallyPointController* const rally = _planMasterController()->rallyPointController();
    QVERIFY2(rally->supported(), "Offline vehicle should support rally points");
    QVERIFY2(_selectTab(QStringLiteral("rally")), "Failed to open the Rally tab");
    _clickMap(0.3, 0.4);
    _clickMap(0.45, 0.6);
    QVERIFY2(waitForCondition([&] { return rally->points()->count() == 2; }, 2000, QStringLiteral("points added")),
             "Map clicks did not add rally points");
    if (tab != QStringLiteral("rally")) {
        QVERIFY2(_selectTab(tab), "Failed to open the tab");
    }

    QObject* const current = rally->currentRallyPoint();
    QObject* const target = dragCurrentPoint ? current : rally->points()->get(0);
    QVERIFY(target);
    QCOMPARE(target == current, dragCurrentPoint);

    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    const auto windowPoint = [&](const QGeoCoordinate& coordinate) {
        return map->mapToScene(mapScreenPoint(map, coordinate)).toPoint();
    };
    const auto targetCoordinate = [&]() { return target->property("coordinate").value<QGeoCoordinate>(); };

    const QGeoCoordinate before = targetCoordinate();
    const QPoint start = windowPoint(before);
    QVERIFY2(map->contains(map->mapFromScene(start)), "The marker is not on screen");
    const QPoint delta(-70, 50);

    QTest::mousePress(_window, Qt::LeftButton, Qt::NoModifier, start);
    constexpr int steps = 10;
    for (int i = 1; i <= steps; i++) {
        QTest::mouseMove(_window, start + ((delta * i) / steps));
    }
    QTest::mouseRelease(_window, Qt::LeftButton, Qt::NoModifier, start + delta);

    if (expectMove) {
        QVERIFY2(waitForCondition([&] { return targetCoordinate().distanceTo(before) > 1.0; }, 2000,
                                  QStringLiteral("point moved")),
                 "Dragging the current point's marker did not move the point");
        // The marker ends up under the cursor, with the point's altitude untouched
        const QPoint end = windowPoint(targetCoordinate());
        QVERIFY2((end - (start + delta)).manhattanLength() <= 4,
                 qPrintable(QStringLiteral("Marker ended at %1,%2 instead of under the cursor at %3,%4")
                                .arg(end.x())
                                .arg(end.y())
                                .arg(start.x() + delta.x())
                                .arg(start.y() + delta.y())));
        QVERIFY(qFuzzyCompare(targetCoordinate().altitude(), before.altitude()));
        QCOMPARE(rally->currentRallyPoint(), target);
    } else {
        // The press went to the map gesture: the map followed the cursor and the point stayed put
        QVERIFY2(waitForCondition([&] { return (windowPoint(before) - start).manhattanLength() > 20; }, 2000,
                                  QStringLiteral("map panned")),
                 "Dragging a marker that is not draggable did not pan the map");
        QVERIFY2(targetCoordinate().distanceTo(before) < 0.01, "Dragging a non-current marker moved its point");
    }
    QCOMPARE(rally->points()->count(), 2);
}

void PlanPanelUITest::_addGeoMapRallyPoint(QQuickItem*& marker)
{
    marker = nullptr;
    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    _clickMap(0.4, 0.5);
    if (QTest::currentTestFailed())
        return;
    MissionController* const controller = _missionController();
    QVERIFY(controller);
    QVERIFY2(waitForCondition([&] { return controller->homePositionSet(); }, 2000, QStringLiteral("home set")),
             "Map click on the Plan tab did not set home");

    RallyPointController* const rally = _planMasterController()->rallyPointController();
    QVERIFY2(rally->supported(), "Offline vehicle should support rally points");
    QVERIFY2(_selectTab(QStringLiteral("rally")), "Failed to open the Rally tab");
    _clickMap(0.5, 0.5);
    if (QTest::currentTestFailed())
        return;
    QVERIFY2(waitForCondition([&] { return rally->points()->count() == 1; }, 2000, QStringLiteral("point added")),
             "Map click did not add a rally point");

    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    QVERIFY2(waitForCondition(
                 [&] {
                     const auto markers = findGeoMapItems(map, QStringLiteral("geoMapRallyPoint"));
                     marker = markers.isEmpty() ? nullptr : markers.first();
                     return marker != nullptr;
                 },
                 2000, QStringLiteral("rally marker")),
             "The rally point has no map marker");
}

void PlanPanelUITest::_testGeoMapRallyMarkerHeightFollowsFirmwareAndFrame_data()
{
    QTest::addColumn<int>("firmwareClass");
    QTest::addColumn<int>("frame");
    QTest::addColumn<double>("altitude");
    QTest::addColumn<bool>("onGround");

    // A PX4 rally altitude is the ground elevation, an ArduPilot one a flight altitude
    QTest::newRow("px4 point lies on the terrain")
        << int(QGCMAVLink::FirmwareClassPX4) << int(QGroundControlQmlGlobal::AltitudeFrameRelative) << 50.0 << true;
    QTest::newRow("ardupilot relative point floats above home")
        << int(QGCMAVLink::FirmwareClassArduPilot) << int(QGroundControlQmlGlobal::AltitudeFrameRelative) << 50.0
        << false;
    QTest::newRow("ardupilot amsl point floats at its altitude")
        << int(QGCMAVLink::FirmwareClassArduPilot) << int(QGroundControlQmlGlobal::AltitudeFrameAbsolute) << 900.0
        << false;
    QTest::newRow("ardupilot terrain point floats above the terrain")
        << int(QGCMAVLink::FirmwareClassArduPilot) << int(QGroundControlQmlGlobal::AltitudeFrameTerrain) << 50.0
        << false;
}

void PlanPanelUITest::_testGeoMapRallyMarkerHeightFollowsFirmwareAndFrame()
{
    QFETCH(int, firmwareClass);
    QFETCH(int, frame);
    QFETCH(double, altitude);
    QFETCH(bool, onGround);

    if ((firmwareClass == QGCMAVLink::FirmwareClassArduPilot) && !apmFirmwareSupported()) {
        QSKIP("ArduPilot firmware support is not built");
    }
    Fact* const firmwareFact = SettingsManager::instance()->appSettings()->offlineEditingFirmwareClass();
    const QVariant savedFirmwareClass = firmwareFact->rawValue();
    const auto restoreGuard = qScopeGuard([=] { firmwareFact->setRawValue(savedFirmwareClass); });
    firmwareFact->setRawValue(firmwareClass);

    QQuickItem* marker = nullptr;
    _addGeoMapRallyPoint(marker);
    if (QTest::currentTestFailed())
        return;

    QCOMPARE(marker->property("groundMarker").toBool(), onGround);
    QCOMPARE(marker->property("altitudeMode").toInt(),
             int(onGround ? GeoMapItem::AltitudeMode::ClampToGround : GeoMapItem::AltitudeMode::Absolute));
    if (onGround) {
        return;
    }

    auto* const point = qobject_cast<RallyPoint*>(_planMasterController()->rallyPointController()->points()->get(0));
    QVERIFY(point);
    point->setAltitudeFrame(static_cast<QGroundControlQmlGlobal::AltitudeFrame>(frame));
    point->altitude()->setRawValue(altitude);

    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    QObject* const surfaceModel = map->property("surfaceModel").value<QObject*>();
    QVERIFY(surfaceModel);
    MissionController* const controller = _missionController();
    QVERIFY(controller);
    const auto expectedAltitude = [&]() {
        switch (frame) {
            case QGroundControlQmlGlobal::AltitudeFrameAbsolute:
                return altitude;
            case QGroundControlQmlGlobal::AltitudeFrameTerrain: {
                double terrainHeight = qQNaN();
                QMetaObject::invokeMethod(surfaceModel, "terrainHeightAt", Q_RETURN_ARG(double, terrainHeight),
                                          Q_ARG(QGeoCoordinate, point->coordinate()));
                return terrainHeight + altitude;
            }
            default:
                return controller->plannedHomePosition().altitude() + altitude;
        }
    };
    const auto markerAltitude = [&]() { return marker->property("coordinate").value<QGeoCoordinate>().altitude(); };
    QVERIFY2(waitForCondition([&] { return qAbs(markerAltitude() - expectedAltitude()) < 0.01; }, 2000,
                              QStringLiteral("marker altitude")),
             qPrintable(
                 QStringLiteral("Marker drawn at %1 m instead of %2 m").arg(markerAltitude()).arg(expectedAltitude())));
}

void PlanPanelUITest::_testGeoMapRallyGroundMarkingMatchesMarkerSize_data()
{
    QTest::addColumn<int>("firmwareClass");
    QTest::addColumn<QString>("markingName");
    QTest::addColumn<double>("altitude");  // NaN: the default altitude
    QTest::addColumn<double>("zoomLevel");

    QTest::newRow("px4 ground circle") << int(QGCMAVLink::FirmwareClassPX4)
                                       << QStringLiteral("geoMapGroundMarkerCircle") << qQNaN() << 15.0;
    QTest::newRow("ardupilot drop shadow")
        << int(QGCMAVLink::FirmwareClassArduPilot) << QStringLiteral("geoMapDropShadow") << qQNaN() << 15.0;
    // The shadow is much farther from the camera than the marker it belongs to
    QTest::newRow("ardupilot drop shadow under a high point close up")
        << int(QGCMAVLink::FirmwareClassArduPilot) << QStringLiteral("geoMapDropShadow") << 400.0 << 18.0;
}

void PlanPanelUITest::_testGeoMapRallyGroundMarkingMatchesMarkerSize()
{
    QFETCH(int, firmwareClass);
    QFETCH(QString, markingName);
    QFETCH(double, altitude);
    QFETCH(double, zoomLevel);

    if ((firmwareClass == QGCMAVLink::FirmwareClassArduPilot) && !apmFirmwareSupported()) {
        QSKIP("ArduPilot firmware support is not built");
    }
    Fact* const firmwareFact = SettingsManager::instance()->appSettings()->offlineEditingFirmwareClass();
    const QVariant savedFirmwareClass = firmwareFact->rawValue();
    const auto restoreGuard = qScopeGuard([=] { firmwareFact->setRawValue(savedFirmwareClass); });
    firmwareFact->setRawValue(firmwareClass);

    QQuickItem* marker = nullptr;
    _addGeoMapRallyPoint(marker);
    if (QTest::currentTestFailed())
        return;

    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    auto* const point = qobject_cast<RallyPoint*>(_planMasterController()->rallyPointController()->points()->get(0));
    QVERIFY(point);
    if (!qIsNaN(altitude)) {
        point->altitude()->setRawValue(altitude);
    }
    QVERIFY(map->setProperty("zoomLevel", zoomLevel));
    QVERIFY(map->setProperty("center", QVariant::fromValue(point->coordinate())));
    auto* const camera = map->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(camera, "GeoMapCamera not found");
    camera->setMode(GeoMapCamera::Mode::Mode3D);
    QVERIFY2(waitForCondition([&] { return qFuzzyCompare(camera->tilt(), GeoMapCamera::kDefault3DTilt); }, 3000,
                              QStringLiteral("3D transition")),
             "The camera did not finish the transition to 3D");

    const QList<QQuickItem*> markings = findGeoMapItems(marker, markingName);
    QVERIFY2(!markings.isEmpty(), qPrintable(QStringLiteral("No %1 under the rally marker").arg(markingName)));
    auto* const path = markings.first()->findChild<GeoMapProjectedPath*>();
    QVERIFY(path);

    // Its across-the-view width is not foreshortened by the tilt, so it matches the marker diameter
    const auto markingWidth = [&]() {
        const QVariantList points = path->screenPoints();
        if (points.isEmpty()) {
            return 0.0;
        }
        qreal minX = points.first().toPointF().x();
        qreal maxX = minX;
        for (const QVariant& point : points) {
            minX = std::min(minX, point.toPointF().x());
            maxX = std::max(maxX, point.toPointF().x());
        }
        return maxX - minX;
    };
    const qreal markerSize = marker->property("_indicatorSize").toReal();
    QVERIFY(markerSize > 0);
    QVERIFY2(
        waitForCondition([&] { return qAbs(markingWidth() - markerSize) < markerSize * 0.1; }, 3000,
                         QStringLiteral("marking size")),
        qPrintable(
            QStringLiteral("%1 is %2 px wide, the marker %3 px").arg(markingName).arg(markingWidth()).arg(markerSize)));
}

void PlanPanelUITest::_testGeoMapTerrainRelativeDragKeepsTheMarkerUnderTheCursor_data()
{
    QTest::addColumn<bool>("rallyPoint");

    QTest::newRow("waypoint") << false;
    QTest::newRow("ardupilot terrain rally point") << true;
}

void PlanPanelUITest::_testGeoMapTerrainRelativeDragKeepsTheMarkerUnderTheCursor()
{
    QFETCH(bool, rallyPoint);

    if (rallyPoint && !apmFirmwareSupported()) {
        QSKIP("ArduPilot firmware support is not built");
    }
    Fact* const firmwareFact = SettingsManager::instance()->appSettings()->offlineEditingFirmwareClass();
    const QVariant savedFirmwareClass = firmwareFact->rawValue();
    const auto restoreGuard = qScopeGuard([=] { firmwareFact->setRawValue(savedFirmwareClass); });

    // A terrain-relative marker stands at its height above the terrain, which changes as it is dragged
    QQuickItem* map = nullptr;
    std::function<QGeoCoordinate()> targetCoordinate;
    std::function<QQuickItem*()> findMarker;
    if (rallyPoint) {
        firmwareFact->setRawValue(QGCMAVLink::FirmwareClassArduPilot);
        QQuickItem* rallyMarker = nullptr;
        _addGeoMapRallyPoint(rallyMarker);
        if (QTest::currentTestFailed())
            return;
        auto* const point =
            qobject_cast<RallyPoint*>(_planMasterController()->rallyPointController()->points()->get(0));
        QVERIFY(point);
        point->setAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrameTerrain);
        point->altitude()->setRawValue(50.0);
        targetCoordinate = [point] { return point->coordinate(); };
        findMarker = [rallyMarker] { return rallyMarker; };
    } else {
        _startPlanPanel(true /* geoMapEngine */);
        if (QTest::currentTestFailed())
            return;
        MissionController* const controller = _missionController();
        QVERIFY(controller);
        controller->setGlobalAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrameCalcAboveTerrain);
        _buildMission(2);
        if (QTest::currentTestFailed())
            return;
        // Disarm the Waypoint tool so a marker press can only select or drag
        QVERIFY2(clickButton(QLatin1String(kWaypointButton)), "Failed to disarm the Waypoint tool");
        VisualMissionItem* const target = controller->currentPlanViewItem();
        QVERIFY(target);
        targetCoordinate = [target] { return target->coordinate(); };
        findMarker = [&map, target]() -> QQuickItem* {
            for (QQuickItem* const marker : findGeoMapWaypointMarkers(map)) {
                if (marker->property("item").value<QObject*>() == target) {
                    return marker;
                }
            }
            return nullptr;
        };
    }

    map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    auto* const camera = map->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(camera, "GeoMapCamera not found");

    // Tilted, close up and on terrain that rises 200 m, around the marker being dragged
    QObject* const surfaceModel = map->property("surfaceModel").value<QObject*>();
    QVERIFY(surfaceModel);
    QVERIFY(surfaceModel->setProperty("debugHills", true));
    QVERIFY(map->setProperty("zoomLevel", 17.0));
    QVERIFY(map->setProperty("center", QVariant::fromValue(targetCoordinate())));
    camera->setMode(GeoMapCamera::Mode::Mode3D);
    QVERIFY2(waitForCondition([&] { return qFuzzyCompare(camera->tilt(), GeoMapCamera::kDefault3DTilt); }, 3000,
                              QStringLiteral("3D transition")),
             "The camera did not finish the transition to 3D");

    const auto markerPoint = [&]() {
        QQuickItem* const marker = findMarker();
        return marker ? marker->mapToScene(QPointF(marker->width() / 2, marker->height() / 2)).toPoint()
                      : QPoint(-1, -1);
    };
    // The rebuilt surface takes a moment to be there for the pick to find
    QVERIFY2(waitForCondition(
                 [&] {
                     const QPoint point = markerPoint();
                     return map->contains(map->mapFromScene(point));
                 },
                 5000, QStringLiteral("marker on screen")),
             "The marker is not on screen");

    const QGeoCoordinate before = targetCoordinate();
    const QPoint start = markerPoint();
    const QPoint delta(-160, 110);
    QTest::mousePress(_window, Qt::LeftButton, Qt::NoModifier, start);
    // One big step, so the marker's new height has to come from the pick and not from following the cursor
    QTest::mouseMove(_window, start + delta);
    QTest::mouseRelease(_window, Qt::LeftButton, Qt::NoModifier, start + delta);

    QVERIFY2(waitForCondition([&] { return targetCoordinate().distanceTo(before) > 1.0; }, 2000,
                              QStringLiteral("marker moved")),
             "Dragging the current marker did not move it");
    const QPoint end = markerPoint();
    QVERIFY2((end - (start + delta)).manhattanLength() <= 6,
             qPrintable(QStringLiteral("Marker ended at %1,%2 instead of under the cursor at %3,%4")
                            .arg(end.x())
                            .arg(end.y())
                            .arg(start.x() + delta.x())
                            .arg(start.y() + delta.y())));
}

void PlanPanelUITest::_testGeoMapMarkerShowsWhileTerrainAltitudePending()
{
    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    MissionController* const controller = _missionController();
    QVERIFY(controller);
    // Calculated above terrain altitudes have no AMSL altitude until a terrain query answers
    controller->setGlobalAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrameCalcAboveTerrain);
    _buildMission(2);
    if (QTest::currentTestFailed())
        return;

    VisualMissionItem* const waypoint =
        controller->visualItems()->value<VisualMissionItem*>(controller->visualItems()->count() - 1);
    QVERIFY(waypoint);
    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);

    const std::function<QQuickItem*(QQuickItem*)> findMarker = [&](QQuickItem* parent) -> QQuickItem* {
        const auto children = parent->childItems();
        for (QQuickItem* const child : children) {
            if (child->objectName() == QStringLiteral("geoMapWaypointMarker") &&
                child->property("item").value<QObject*>() == waypoint) {
                return child;
            }
            if (QQuickItem* const found = findMarker(child)) {
                return found;
            }
        }
        return nullptr;
    };
    QQuickItem* marker = nullptr;
    QVERIFY2(waitForCondition([&] { return (marker = findMarker(map)) != nullptr; }, 3000,
                              QStringLiteral("waypoint marker created")),
             "No marker was created for the waypoint");

    // The direction arrows show once every altitude of their segment is known, and not while one is pending
    const auto arrowsVisible = [&](bool visible) {
        const auto arrows = findGeoMapItems(map, QStringLiteral("geoMapDirectionArrow"));
        return !arrows.isEmpty() && std::all_of(arrows.cbegin(), arrows.cend(),
                                                [&](QQuickItem* arrow) { return arrow->isVisible() == visible; });
    };
    QVERIFY2(
        waitForCondition([&] { return !qIsNaN(waypoint->property("amslEntryAlt").toDouble()) && arrowsVisible(true); },
                         8000, QStringLiteral("direction arrows with known altitudes")),
        "The direction arrows are not shown once the terrain altitudes are known");

    // Back to waiting on the terrain query
    QVERIFY(QMetaObject::invokeMethod(waypoint, "_terrainDataReceived", Q_ARG(bool, false),
                                      Q_ARG(QList<double>, QList<double>())));
    QVERIFY2(qIsNaN(waypoint->property("amslEntryAlt").toDouble()), "The waypoint must be waiting on terrain data");
    QVERIFY2(waitForCondition([&] { return arrowsVisible(false); }, 3000, QStringLiteral("direction arrows hidden")),
             "The direction arrows are not hidden while a terrain altitude is pending");

    // Without an altitude the marker would sit at sea level, below the terrain surface, until the query answers
    QObject* const surfaceModel = marker->property("surfaceModel").value<QObject*>();
    QVERIFY(surfaceModel);
    const auto markerStandsOnTerrain = [&] {
        const QGeoCoordinate coordinate = marker->property("coordinate").value<QGeoCoordinate>();
        if (qIsNaN(coordinate.altitude())) {
            return false;
        }
        double terrainHeight = qQNaN();
        const bool queried = QMetaObject::invokeMethod(
            surfaceModel, "terrainHeightAt", Q_RETURN_ARG(double, terrainHeight), Q_ARG(QGeoCoordinate, coordinate));
        return queried && (coordinate.altitude() - terrainHeight) >= 1.0;
    };
    QVERIFY2(markerStandsOnTerrain(), "The waypoint marker is not drawn on the terrain while its altitude is pending");

    // Terrain tiles that arrive while the query is still pending lift the marker and the route with the terrain
    QVERIFY(surfaceModel->setProperty("debugHills", true));
    const QGeoCoordinate waypointCoordinate = waypoint->coordinate();
    const auto terrainHeightAt = [&](const QGeoCoordinate& coordinate) {
        double terrainHeight = qQNaN();
        QMetaObject::invokeMethod(surfaceModel, "terrainHeightAt", Q_RETURN_ARG(double, terrainHeight),
                                  Q_ARG(QGeoCoordinate, coordinate));
        return terrainHeight;
    };
    QVERIFY2(waitForCondition([&] { return terrainHeightAt(waypointCoordinate) >= 5.0; }, 3000,
                              QStringLiteral("hills under the waypoint")),
             "The debug hills did not raise the terrain under the waypoint");
    const double heightAboveTerrain = qvariant_cast<Fact*>(waypoint->property("altitude"))->rawValue().toDouble();
    const auto standsAboveTerrain = [&](const QGeoCoordinate& coordinate, double bias) {
        return qAbs(coordinate.altitude() + bias - terrainHeightAt(coordinate) - heightAboveTerrain) < 0.5;
    };
    QVERIFY2(
        waitForCondition([&] { return standsAboveTerrain(marker->property("coordinate").value<QGeoCoordinate>(), 0); },
                         3000, QStringLiteral("marker follows the terrain")),
        "The waypoint marker did not follow the terrain that arrived while its altitude is pending");

    QQuickItem* const missionPath = findVisibleItem(_rootItem, QStringLiteral("geoMapMissionPath"), 3000);
    QVERIFY(missionPath);
    const auto routeFollowsTerrain = [&] {
        const QVariantList points = missionPath->property("_routePoints").toList();
        const double bias = missionPath->property("homeTerrainBias").toDouble();
        for (const QVariant& point : points) {
            const QVariantMap pointMap = point.toMap();
            const QGeoCoordinate coordinate = pointMap.value(QStringLiteral("coordinate")).value<QGeoCoordinate>();
            if (pointMap.value(QStringLiteral("pendingAmsl")).toBool() &&
                coordinate.distanceTo(waypointCoordinate) < 1.0) {
                return standsAboveTerrain(coordinate, bias);
            }
        }
        return false;
    };
    QVERIFY2(waitForCondition(routeFollowsTerrain, 3000, QStringLiteral("route follows the terrain")),
             "The mission route did not follow the terrain that arrived while an altitude is pending");
}

void PlanPanelUITest::_testGeoMapMissionPathReturnsToPlannedHome()
{
    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    _buildMission(1);
    if (QTest::currentTestFailed())
        return;

    MissionController* const controller = _missionController();
    QVERIFY(controller);
    const int itemCount = controller->visualItems()->count();
    const QGeoCoordinate lastWaypoint =
        controller->visualItems()->value<VisualMissionItem*>(itemCount - 1)->coordinate();
    QVERIFY(controller->insertLandItem(lastWaypoint, itemCount));
    const QGeoCoordinate plannedHome = controller->plannedHomePosition();
    QVERIFY(plannedHome.isValid());

    QQuickItem* const missionPath = findVisibleItem(_rootItem, QStringLiteral("geoMapMissionPath"), 3000);
    QVERIFY(missionPath);
    const auto routeEnd = [&] {
        const QVariantList points = missionPath->property("_routePoints").toList();
        return points.isEmpty() ? QGeoCoordinate()
                                : points.last().toMap().value(QStringLiteral("coordinate")).value<QGeoCoordinate>();
    };
    // The offline editing vehicle has no home position of its own, only the planned one
    QVERIFY2(waitForCondition([&] { return routeEnd().isValid() && routeEnd().distanceTo(plannedHome) < 0.5; }, 3000,
                              QStringLiteral("route returns home")),
             "The route of a mission ending in Return does not go back to the planned home");
}

void PlanPanelUITest::_testFenceTabAddsFenceInsideTheGeoMapViewport_data()
{
    QTest::addColumn<bool>("steepTilt");
    QTest::addColumn<double>("heading");

    QTest::newRow("sky above the horizon") << true << 0.0;
    QTest::newRow("south up") << false << 180.0;
    QTest::newRow("east up") << false << 90.0;
    QTest::newRow("diagonal heading") << false << 45.0;
}

void PlanPanelUITest::_testFenceTabAddsFenceInsideTheGeoMapViewport()
{
    QFETCH(bool, steepTilt);
    QFETCH(double, heading);

    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;
    _setHome();
    if (QTest::currentTestFailed())
        return;

    PlanMasterController* const master = _planMasterController();
    QVERIFY(master);
    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    auto* const camera = map->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(camera, "GeoMapCamera not found");

    QVERIFY2(_selectTab(QStringLiteral("fence")), "Failed to open the Fence tab");

    const auto groundUnderTopLeft = [&] {
        QVariant coordinate;
        QMetaObject::invokeMethod(
            map, "toCoordinate", Q_RETURN_ARG(QVariant, coordinate),
            Q_ARG(QVariant, QVariant::fromValue(map->property("centerViewport").toRectF().topLeft())),
            Q_ARG(QVariant, false));
        return coordinate.value<QGeoCoordinate>().isValid();
    };
    if (steepTilt) {
        // At the steepest tilt the upper viewport corners point into the sky; the tilt is set once the 3D
        // transition ended
        camera->setMode(GeoMapCamera::Mode::Mode3D);
        QVERIFY2(waitForCondition([&] { return qFuzzyCompare(camera->tilt(), GeoMapCamera::kDefault3DTilt); }, 3000,
                                  QStringLiteral("3D transition")),
                 "The camera did not finish the transition to 3D");
        camera->setTilt(GeoMapCamera::kMaxTilt);
        QVERIFY2(waitForCondition([&] { return !groundUnderTopLeft(); }, 3000, QStringLiteral("sky above the horizon")),
                 "The upper viewport corner must point into the sky for this test");
    }
    camera->setHeading(heading);
    QVERIFY2(waitForCondition([&] { return qAbs(std::remainder(camera->heading() - heading, 360.0)) < 0.01; }, 3000,
                              QStringLiteral("heading")),
             "The camera did not take the heading");

    GeoFenceController* const fence = master->geoFenceController();
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceAddPolygon")), "Failed to click + Polygon");
    QVERIFY2(!steepTilt || !groundUnderTopLeft(),
             "The upper viewport corner must still point into the sky when the polygon is added");
    QVERIFY2(waitForCondition([&] { return fence->polygons()->count() == 1; }, 2000, QStringLiteral("polygon added")),
             "+ Polygon did not add a polygon");

    // Every vertex is a real coordinate, and the polygon is where the viewport shows ground
    QObject* const polygon = fence->polygons()->get(0);
    const int vertexCount = polygon->property("count").toInt();
    QVERIFY(vertexCount >= 3);
    const QRectF viewport = map->property("centerViewport").toRectF();
    for (int i = 0; i < vertexCount; i++) {
        QGeoCoordinate vertex;
        QMetaObject::invokeMethod(polygon, "vertexCoordinate", Q_RETURN_ARG(QGeoCoordinate, vertex), Q_ARG(int, i));
        QVERIFY2(vertex.isValid(), qPrintable(QStringLiteral("Fence vertex %1 is not a valid coordinate").arg(i)));
        QVariant screenPoint;
        QVERIFY(QMetaObject::invokeMethod(map, "fromCoordinate", Q_RETURN_ARG(QVariant, screenPoint),
                                          Q_ARG(QVariant, QVariant::fromValue(vertex)), Q_ARG(QVariant, false)));
        QVERIFY2(viewport.adjusted(-2, -2, 2, 2).contains(screenPoint.toPointF()),
                 qPrintable(QStringLiteral("Fence vertex %1 is outside the viewport at %2,%3")
                                .arg(i)
                                .arg(screenPoint.toPointF().x())
                                .arg(screenPoint.toPointF().y())));
    }
}

void PlanPanelUITest::_testFenceTabAddsFenceInsideTheViewportOverTheDateline_data()
{
    QTest::addColumn<double>("zoomLevel");
    QTest::addColumn<double>("latitude");
    QTest::addColumn<double>("longitude");
    QTest::addColumn<bool>("wide");

    QTest::newRow("over the dateline") << 3.0 << 20.0 << 180.0 << false;
    QTest::newRow("wider than a half turn") << 0.0 << 0.0 << 0.0 << true;
}

void PlanPanelUITest::_testFenceTabAddsFenceInsideTheViewportOverTheDateline()
{
    QFETCH(double, zoomLevel);
    QFETCH(double, latitude);
    QFETCH(double, longitude);
    QFETCH(bool, wide);

    // The QtLocation map is the one that shows both sides of the antimeridian
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;
    _setHome();
    if (QTest::currentTestFailed())
        return;

    PlanMasterController* const master = _planMasterController();
    QVERIFY(master);
    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    if (wide) {
        // The map cannot zoom out past the world filling its height, so only a wider window shows more than half
        // of the world's width
        QVERIFY(_window);
        _window->resize(_window->width() + 600, _window->height());
    }
    // The panel decides how wide the viewport is, so the fence tab is open before the map is moved
    QVERIFY2(_selectTab(QStringLiteral("fence")), "Failed to open the Fence tab");
    QVERIFY(map->setProperty("zoomLevel", zoomLevel));
    QVERIFY(map->setProperty("center", QVariant::fromValue(QGeoCoordinate(latitude, longitude))));
    const auto mapCenter = [&] { return map->property("center").value<QGeoCoordinate>(); };
    QVERIFY2(waitForCondition(
                 [&] {
                     return qAbs(mapCenter().latitude() - latitude) < 0.5 &&
                            qAbs(std::remainder(mapCenter().longitude() - longitude, 360.0)) < 1.0;
                 },
                 3000, QStringLiteral("map centered")),
             "The map did not move to the longitude");

    // The viewport must show the world beyond the antimeridian, or more than a half turn of it, for this test
    const auto cornerLongitude = [&](const QPointF& corner) {
        QGeoCoordinate coordinate;
        QMetaObject::invokeMethod(map, "toCoordinate", Q_RETURN_ARG(QGeoCoordinate, coordinate), Q_ARG(QPointF, corner),
                                  Q_ARG(bool, false));
        return coordinate.longitude();
    };
    const QRectF mapViewport = map->property("centerViewport").toRectF();
    const double leftLongitude = cornerLongitude(mapViewport.topLeft());
    const double rightLongitude = cornerLongitude(mapViewport.bottomRight());
    const QString shown =
        QStringLiteral("The viewport shows longitudes %1 to %2").arg(leftLongitude).arg(rightLongitude);
    if (wide) {
        QVERIFY2(rightLongitude - leftLongitude > 180.0, qPrintable(shown));
    } else {
        QVERIFY2(leftLongitude > 90.0 && rightLongitude < -90.0, qPrintable(shown));
    }
    GeoFenceController* const fence = master->geoFenceController();
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceAddPolygon")), "Failed to click + Polygon");
    QVERIFY2(waitForCondition([&] { return fence->polygons()->count() == 1; }, 2000, QStringLiteral("polygon added")),
             "+ Polygon did not add a polygon");

    QObject* const polygon = fence->polygons()->get(0);
    const QRectF viewport = map->property("centerViewport").toRectF();
    const int vertexCount = polygon->property("count").toInt();
    QVERIFY(vertexCount >= 3);
    for (int i = 0; i < vertexCount; i++) {
        QGeoCoordinate vertex;
        QMetaObject::invokeMethod(polygon, "vertexCoordinate", Q_RETURN_ARG(QGeoCoordinate, vertex), Q_ARG(int, i));
        QPointF screenPoint;
        QVERIFY(QMetaObject::invokeMethod(map, "fromCoordinate", Q_RETURN_ARG(QPointF, screenPoint),
                                          Q_ARG(QGeoCoordinate, vertex), Q_ARG(bool, false)));
        QVERIFY2(viewport.adjusted(-2, -2, 2, 2).contains(screenPoint),
                 qPrintable(QStringLiteral("Fence vertex %1 at %2,%3 is outside the viewport at %4,%5")
                                .arg(i)
                                .arg(vertex.latitude())
                                .arg(vertex.longitude())
                                .arg(screenPoint.x())
                                .arg(screenPoint.y())));
    }
}

void PlanPanelUITest::_testGeoMapDimsMissionOutsideTheMissionLayer()
{
    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    _buildMission(1);
    if (QTest::currentTestFailed())
        return;

    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    auto* const camera = map->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(camera, "GeoMapCamera not found");
    QQuickItem* const missionPath = findVisibleItem(_rootItem, QStringLiteral("geoMapMissionPath"), 3000);
    QVERIFY(missionPath);

    // The 3D nodes do not follow the item opacity the 2D content is dimmed with
    const auto nodeOpacity = [](QQuickItem* item) {
        QObject* const node = item->property("node3D").value<QObject*>();
        return node ? node->property("opacity").toReal() : qQNaN();
    };
    camera->setMode(GeoMapCamera::Mode::Mode3D);
    QVERIFY2(waitForCondition([&] { return qFuzzyCompare(camera->tilt(), GeoMapCamera::kDefault3DTilt); }, 3000,
                              QStringLiteral("3D transition")),
             "The camera did not finish the transition to 3D");
    // The crossfade settles on 1 asymptotically
    const auto allMarkersAt = [&](double opacity) {
        const auto markers = findGeoMapWaypointMarkers(map);
        return !markers.isEmpty() && std::all_of(markers.cbegin(), markers.cend(), [&](QQuickItem* marker) {
            return qAbs(nodeOpacity(marker) - opacity) < 0.01;
        });
    };
    QVERIFY2(waitForCondition([&] { return allMarkersAt(1.0); }, 3000, QStringLiteral("opaque 3D markers")),
             "The 3D markers must be opaque while the mission is edited");
    QCOMPARE(nodeOpacity(missionPath), 1.0);

    QVERIFY2(_selectTab(QStringLiteral("fence")), "Failed to open the Fence tab");
    QVERIFY2(waitForCondition([&] { return qFuzzyCompare(nodeOpacity(missionPath), 0.5); }, 3000,
                              QStringLiteral("dimmed route")),
             "The 3D route is not dimmed outside the mission layer");
    QVERIFY2(waitForCondition([&] { return allMarkersAt(0.5); }, 3000, QStringLiteral("dimmed 3D markers")),
             "The 3D markers are not dimmed outside the mission layer");
}

void PlanPanelUITest::_testGeoMapControlsReceiveClicksOverMissionMarkers()
{
    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    _buildMission(2);
    if (QTest::currentTestFailed())
        return;
    // Disarm the Waypoint tool so a click can only select
    QVERIFY2(clickButton(QLatin1String(kWaypointButton)), "Failed to disarm the Waypoint tool");

    MissionController* const controller = _missionController();
    QVERIFY(controller);
    VisualMissionItem* const current = controller->currentPlanViewItem();
    VisualMissionItem* const target = controller->visualItems()->value<VisualMissionItem*>(1);
    QVERIFY(target && target != current);

    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    auto* const camera = map->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(camera, "GeoMapCamera not found");
    QQuickItem* const button = findVisibleItem(_rootItem, QStringLiteral("geoMapModeButton"));
    QVERIFY(button);

    // A waypoint marker panned under the camera control
    const QPointF buttonCenter =
        map->mapFromScene(button->mapToScene(QPointF(button->width() / 2, button->height() / 2)));
    QVariant coordinate;
    QVERIFY(QMetaObject::invokeMethod(map, "toCoordinate", Q_RETURN_ARG(QVariant, coordinate),
                                      Q_ARG(QVariant, QVariant::fromValue(buttonCenter)), Q_ARG(QVariant, false)));
    QVERIFY(coordinate.value<QGeoCoordinate>().isValid());
    target->setCoordinate(coordinate.value<QGeoCoordinate>());
    const auto markerIsUnderButton = [&] {
        for (QQuickItem* const marker : findGeoMapWaypointMarkers(map)) {
            if (marker->property("item").value<QObject*>() == target) {
                const QPointF center = marker->mapToScene(QPointF(marker->width() / 2, marker->height() / 2));
                return (center - button->mapToScene(QPointF(button->width() / 2, button->height() / 2)))
                           .manhattanLength() < 4;
            }
        }
        return false;
    };
    QVERIFY2(waitForCondition(markerIsUnderButton, 3000, QStringLiteral("marker under the control")),
             "The waypoint marker did not move under the camera control");

    QVERIFY2(clickButton(QStringLiteral("geoMapModeButton")), "Failed to click the 2D/3D button");
    QVERIFY2(
        waitForCondition([&] { return camera->mode() == GeoMapCamera::Mode::Mode3D; }, 2000, QStringLiteral("3D mode")),
        "The marker under the 2D/3D button took its click");
    QCOMPARE(controller->currentPlanViewItem(), current);
}

void PlanPanelUITest::_testGeoMapDoesNotRecenterOnOffscreenVehicleInPlanView()
{
    setUseTabbedPlanPanel(true);
    setUseGeoMapPlanEngine(true);
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [this](QPointer<MockLink>, Vehicle* vehicle) {
            QVERIFY2(expectSoftwareBackendWarnings(/*strict*/ false).has_value(),
                     "No renderer interface on the main window");
            _openPlanView();
            if (QTest::currentTestFailed())
                return;

            QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
            QVERIFY(map);
            auto* const camera = map->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
            QVERIFY2(camera, "GeoMapCamera not found");
            QObject* const tracker = map->property("positionTracker").value<QObject*>();
            QVERIFY(tracker);
            QVERIFY2(waitForCondition([&] { return tracker->property("firstVehiclePositionReceived").toBool(); }, 5000,
                                      QStringLiteral("first vehicle position")),
                     "The map never took the vehicle position");

            // Editing the plan away from the vehicle: the map stays where the user put it
            QSignalSpy recenterSpy(tracker, SIGNAL(recenterVehicleTo(QPointF)));
            QVERIFY(recenterSpy.isValid());
            QGeoCoordinate away = vehicle->coordinate();
            away.setLatitude(away.latitude() + 0.5);
            camera->setCenter(away);
            QVERIFY2(!recenterSpy.wait(2000), "The Plan map was recentered on the vehicle that left the viewport");
            QVERIFY(camera->center().distanceTo(away) < 1000.0);
        });
}

void PlanPanelUITest::_testGeoMapMarkerClickOutsideTheMissionLayerReachesTheMap()
{
    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    _buildMission(1);
    if (QTest::currentTestFailed())
        return;

    MissionController* const controller = _missionController();
    PlanMasterController* const master = _planMasterController();
    QVERIFY(controller && master);
    RallyPointController* const rally = master->rallyPointController();
    QVERIFY2(rally->supported(), "Offline vehicle should support rally points");
    VisualMissionItem* const target =
        controller->visualItems()->value<VisualMissionItem*>(controller->visualItems()->count() - 1);
    QVERIFY(target);

    QVERIFY2(_selectTab(QStringLiteral("rally")), "Failed to open the Rally tab");
    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    QVariant screenPoint;
    QVERIFY(QMetaObject::invokeMethod(map, "fromCoordinate", Q_RETURN_ARG(QVariant, screenPoint),
                                      Q_ARG(QVariant, QVariant::fromValue(target->coordinate())),
                                      Q_ARG(QVariant, false)));
    const QPointF point = screenPoint.toPointF();
    QVERIFY2(QRectF(QPointF(0, 0), map->size()).contains(point), "The waypoint is not on screen");

    // The click belongs to the map, which adds the rally point, and not to the dimmed marker under it
    const int rallyPointCount = rally->points()->count();
    QVERIFY2(clickItemFraction(QStringLiteral("planView_map"), point.x() / map->width(), point.y() / map->height()),
             "Failed to click the map");
    QVERIFY2(waitForCondition([&] { return rally->points()->count() == rallyPointCount + 1; }, 2000,
                              QStringLiteral("rally point added")),
             "The click on a dimmed marker did not reach the map");
    QCOMPARE(_currentTab(), QStringLiteral("rally"));
}

void PlanPanelUITest::_testGeoMapDimsLandingPatternOutsideTheMissionLayer()
{
    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    _buildMission(1);
    if (QTest::currentTestFailed())
        return;

    MissionController* const controller = _missionController();
    QVERIFY(controller);
    const int itemCount = controller->visualItems()->count();
    QVERIFY(controller->insertComplexMissionItem(
        QStringLiteral("Fixed Wing Landing"),
        controller->visualItems()->value<VisualMissionItem*>(itemCount - 1)->coordinate(), itemCount));

    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    auto* const camera = map->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(camera, "GeoMapCamera not found");
    camera->setMode(GeoMapCamera::Mode::Mode3D);
    QVERIFY2(waitForCondition([&] { return qFuzzyCompare(camera->tilt(), GeoMapCamera::kDefault3DTilt); }, 3000,
                              QStringLiteral("3D transition")),
             "The camera did not finish the transition to 3D");

    // The landing pattern's markers show the sequence numbers of its items; the fence and rally labels show none
    const auto labelsAt = [&](double opacity) {
        QList<QQuickItem*> labels;
        for (QQuickItem* const label : findGeoMapItems(map, QStringLiteral("geoMapMissionLabel"))) {
            if (label->property("index").toInt() >= 0) {
                labels.append(label);
            }
        }
        return !labels.isEmpty() && std::all_of(labels.cbegin(), labels.cend(), [&](QQuickItem* label) {
            QObject* const node = label->property("node3D").value<QObject*>();
            return node && qAbs(node->property("opacity").toReal() - opacity) < 0.01;
        });
    };
    QVERIFY2(waitForCondition([&] { return labelsAt(1.0); }, 3000, QStringLiteral("opaque 3D landing markers")),
             "The landing pattern markers must be opaque while the mission is edited");
    QVERIFY2(_selectTab(QStringLiteral("fence")), "Failed to open the Fence tab");
    QVERIFY2(waitForCondition([&] { return labelsAt(0.5); }, 3000, QStringLiteral("dimmed 3D landing markers")),
             "The landing pattern markers are not dimmed outside the mission layer");
}

void PlanPanelUITest::_testGeoMapFenceIsEditedOnlyOnTheFenceLayer_data()
{
    QTest::addColumn<QString>("addButton");
    QTest::addColumn<QString>("handleName");
    QTest::addColumn<QPoint>("delta");
    QTest::addColumn<bool>("edgeLengths");

    // The radius handle stays east of the center, so it is only dragged east
    QTest::newRow("circle radius") << QStringLiteral("planPanel_fenceAddCircle")
                                   << QStringLiteral("geoMapCircleRadiusHandle") << QPoint(40, 0) << false;
    QTest::newRow("circle center") << QStringLiteral("planPanel_fenceAddCircle")
                                   << QStringLiteral("geoMapCircleCenterHandle") << QPoint(40, 30) << false;
    QTest::newRow("polygon vertex") << QStringLiteral("planPanel_fenceAddPolygon")
                                    << QStringLiteral("geoMapPolygonVertexHandle") << QPoint(40, 30) << true;
    QTest::newRow("polygon center") << QStringLiteral("planPanel_fenceAddPolygon")
                                    << QStringLiteral("geoMapPolygonCenterHandle") << QPoint(40, 30) << false;
}

void PlanPanelUITest::_testGeoMapFenceIsEditedOnlyOnTheFenceLayer()
{
    QFETCH(QString, addButton);
    QFETCH(QString, handleName);
    QFETCH(QPoint, delta);
    QFETCH(bool, edgeLengths);

    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    _setHomeAndOpenMission();
    if (QTest::currentTestFailed())
        return;

    PlanMasterController* const master = _planMasterController();
    QVERIFY(master);
    GeoFenceController* const fence = master->geoFenceController();
    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);

    QVERIFY2(_selectTab(QStringLiteral("fence")), "Failed to open the Fence tab");
    QVERIFY2(clickButton(addButton), "Failed to add the fence");
    QVERIFY2(waitForCondition([&] { return (fence->polygons()->count() + fence->circles()->count()) == 1; }, 2000,
                              QStringLiteral("fence added")),
             "The fence was not added");

    const auto fenceVisuals = [&]() -> QQuickItem* {
        const auto items = findGeoMapItems(map, QLatin1String(kGeoFenceVisuals));
        return items.isEmpty() ? nullptr : items.first();
    };
    const auto visibleHandles = [&] {
        QList<QQuickItem*> handles = findGeoMapItems(map, handleName);
        handles.removeIf([](QQuickItem* handle) { return !handle->isVisible(); });
        return handles;
    };
    const auto anchorInWindow = [](QQuickItem* handle) {
        return handle->mapToScene(handle->property("anchorPoint").toPointF()).toPoint();
    };
    // The wall's 3D node does not follow the 2D opacity the fence visuals are dimmed with. Rings without a wall, such
    // as the drag handles' ground shadows, have a hidden one.
    const auto wallsAt = [&](double opacity) {
        QList<QQuickItem*> walls = findGeoMapItems(map, QStringLiteral("geoMapFenceWall"));
        walls.removeIf([](QQuickItem* wall) { return !wall->isVisible(); });
        return !walls.isEmpty() && std::all_of(walls.cbegin(), walls.cend(), [&](QQuickItem* wall) {
            QObject* const node = wall->property("node3D").value<QObject*>();
            return node && qAbs(node->property("opacity").toReal() - opacity) < 0.01;
        });
    };
    QVERIFY2(fenceVisuals(), "Fence visuals not found on the GeoMap");
    QCOMPARE(fenceVisuals()->opacity(), 1.0);
    QVERIFY2(waitForCondition([&] { return wallsAt(1.0); }, 3000, QStringLiteral("opaque fence walls")),
             "The fence walls must be opaque while the fence is edited");
    QVERIFY2(waitForCondition([&] { return !visibleHandles().isEmpty(); }, 2000, QStringLiteral("handle shown")),
             "The selected fence has no handle on the Fence tab");
    // Polygons also get the polygon tools toolbar
    const bool isPolygon = fence->polygons()->count() == 1;
    verifyVisibility(QStringLiteral("geoMapPolygonToolbar"), isPolygon,
                     QStringLiteral("polygon toolbar on the Fence tab"));
    if (QTest::currentTestFailed())
        return;

    const QPoint start = anchorInWindow(visibleHandles().first());
    QTest::mousePress(_window, Qt::LeftButton, Qt::NoModifier, start);
    constexpr int steps = 10;
    for (int i = 1; i <= steps; i++) {
        QTest::mouseMove(_window, start + ((delta * i) / steps));
    }
    // Only a polygon vertex drag shows the length of every edge
    const auto visibleEdgeLengths = [&] {
        QList<QQuickItem*> labels = findGeoMapItems(map, QStringLiteral("geoMapPolygonEdgeLength"));
        labels.removeIf([](QQuickItem* label) { return !label->isVisible(); });
        return labels.count();
    };
    const int edgeCount = (fence->polygons()->count() == 1) ? fence->polygons()->get(0)->property("count").toInt() : 0;
    QVERIFY2(waitForCondition([&] { return visibleEdgeLengths() == (edgeLengths ? edgeCount : 0); }, 2000,
                              QStringLiteral("edge lengths while dragging")),
             qPrintable(QStringLiteral("%1 edge lengths show while dragging").arg(visibleEdgeLengths())));
    QTest::mouseRelease(_window, Qt::LeftButton, Qt::NoModifier, start + delta);
    QVERIFY2(waitForCondition([&] { return visibleEdgeLengths() == 0; }, 2000, QStringLiteral("edge lengths hidden")),
             "Edge lengths still show after the drag");

    // The fence follows the handle to the cursor
    const QPoint end = start + delta;
    const auto handleUnderCursor = [&] {
        const auto handles = visibleHandles();
        return std::any_of(handles.cbegin(), handles.cend(),
                           [&](QQuickItem* handle) { return (anchorInWindow(handle) - end).manhattanLength() <= 4; });
    };
    QVERIFY2(waitForCondition(handleUnderCursor, 2000, QStringLiteral("handle dragged")),
             "The dragged handle did not end up under the cursor");
    // A polygon left in its drag state stops notifying path changes
    if (fence->polygons()->count() == 1) {
        QObject* const polygon = fence->polygons()->get(0);
        QVERIFY(!polygon->property("vertexDrag").toBool());
        QVERIFY(!polygon->property("centerDrag").toBool());
    }

    // Off the Fence tab the fence is dimmed and cannot be edited
    QVERIFY2(_selectTab(QStringLiteral("mission")), "Failed to open the Mission tab");
    QVERIFY2(waitForCondition([&] { return qFuzzyCompare(fenceVisuals()->opacity(), 0.5); }, 2000,
                              QStringLiteral("dimmed fence")),
             "The fence is not dimmed outside the fence layer");
    QVERIFY2(waitForCondition([&] { return wallsAt(0.5); }, 3000, QStringLiteral("dimmed fence walls")),
             "The fence walls are not dimmed outside the fence layer");
    QVERIFY2(visibleHandles().isEmpty(), "The fence handle shows outside the fence layer");
    verifyVisibility(QStringLiteral("geoMapPolygonToolbar"), false,
                     QStringLiteral("polygon toolbar off the Fence tab"));
}

void PlanPanelUITest::_testGeoMapFencePolygonSplitHandleInsertsVertex()
{
    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    _setHomeAndOpenMission();
    if (QTest::currentTestFailed())
        return;

    PlanMasterController* const master = _planMasterController();
    QVERIFY(master);
    GeoFenceController* const fence = master->geoFenceController();
    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);

    QVERIFY2(_selectTab(QStringLiteral("fence")), "Failed to open the Fence tab");
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceAddPolygon")), "Failed to click + Polygon");
    QVERIFY2(waitForCondition([&] { return fence->polygons()->count() == 1; }, 2000, QStringLiteral("polygon added")),
             "+ Polygon did not add a polygon");
    QObject* const polygon = fence->polygons()->get(0);
    const int vertexCount = polygon->property("count").toInt();

    // One split handle per edge, each halfway between its two vertices
    const auto splitHandles = [&] { return findGeoMapItems(map, QStringLiteral("geoMapPolygonSplitHandle")); };
    QVERIFY2(
        waitForCondition([&] { return splitHandles().count() == vertexCount; }, 2000, QStringLiteral("split handles")),
        qPrintable(QStringLiteral("%1 split handles for %2 edges").arg(splitHandles().count()).arg(vertexCount)));
    const auto handles = splitHandles();
    const auto firstEdge = std::find_if(handles.cbegin(), handles.cend(),
                                        [](QQuickItem* splitHandle) { return splitHandle->property("index") == 0; });
    QVERIFY(firstEdge != handles.cend());
    QQuickItem* const handle = *firstEdge;
    QGeoCoordinate first;
    QGeoCoordinate second;
    QMetaObject::invokeMethod(polygon, "vertexCoordinate", Q_RETURN_ARG(QGeoCoordinate, first), Q_ARG(int, 0));
    QMetaObject::invokeMethod(polygon, "vertexCoordinate", Q_RETURN_ARG(QGeoCoordinate, second), Q_ARG(int, 1));
    const QGeoCoordinate midpoint = handle->property("coordinate").value<QGeoCoordinate>();
    QVERIFY2(qAbs(midpoint.distanceTo(first) - midpoint.distanceTo(second)) < 1.0,
             "The split handle is not halfway along its edge");

    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier,
                      handle->mapToScene(handle->property("anchorPoint").toPointF()).toPoint());
    QVERIFY2(waitForCondition([&] { return polygon->property("count").toInt() == vertexCount + 1; }, 2000,
                              QStringLiteral("vertex inserted")),
             "Clicking the split handle did not insert a vertex");
    QGeoCoordinate inserted;
    QMetaObject::invokeMethod(polygon, "vertexCoordinate", Q_RETURN_ARG(QGeoCoordinate, inserted), Q_ARG(int, 1));
    QVERIFY2(inserted.distanceTo(midpoint) < 1.0, "The vertex was not inserted at the split handle");
}

void PlanPanelUITest::_testGeoMapFencePolygonMenuActsOnClickedHandle_data()
{
    QTest::addColumn<QString>("handleName");
    QTest::addColumn<QString>("menuItem");
    QTest::addColumn<int>("vertexCountBefore");

    QTest::newRow("remove vertex") << QStringLiteral("geoMapPolygonVertexHandle")
                                   << QStringLiteral("geoMapPolygonRemoveVertex") << 4;
    QTest::newRow("no removal below three vertices") << QStringLiteral("geoMapPolygonVertexHandle") << QString() << 3;
    QTest::newRow("edit vertex position")
        << QStringLiteral("geoMapPolygonVertexHandle") << QStringLiteral("geoMapPolygonEditPosition") << 4;
    QTest::newRow("edit center position")
        << QStringLiteral("geoMapPolygonCenterHandle") << QStringLiteral("geoMapPolygonEditPosition") << 4;
}

void PlanPanelUITest::_testGeoMapFencePolygonMenuActsOnClickedHandle()
{
    QFETCH(QString, handleName);
    QFETCH(QString, menuItem);
    QFETCH(int, vertexCountBefore);

    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    _setHomeAndOpenMission();
    if (QTest::currentTestFailed())
        return;

    PlanMasterController* const master = _planMasterController();
    QVERIFY(master);
    GeoFenceController* const fence = master->geoFenceController();
    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);

    QVERIFY2(_selectTab(QStringLiteral("fence")), "Failed to open the Fence tab");
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceAddPolygon")), "Failed to click + Polygon");
    QVERIFY2(waitForCondition([&] { return fence->polygons()->count() == 1; }, 2000, QStringLiteral("polygon added")),
             "+ Polygon did not add a polygon");
    QObject* const polygon = fence->polygons()->get(0);
    const auto vertexCount = [polygon] { return polygon->property("count").toInt(); };
    while (vertexCount() > vertexCountBefore) {
        QMetaObject::invokeMethod(polygon, "removeVertex", Q_ARG(int, vertexCount() - 1));
    }
    QCOMPARE(vertexCount(), vertexCountBefore);

    const auto path = [polygon] { return polygon->property("path").toList(); };
    // Vertex 1, so an action applied to the default index 0 shows; or the center
    constexpr int clickedVertex = 1;
    const auto clickedHandle = [&]() -> QQuickItem* {
        const auto handles = findGeoMapItems(map, handleName);
        for (QQuickItem* const handle : handles) {
            if (handle->isVisible() &&
                (!handle->property("index").isValid() || handle->property("index") == clickedVertex)) {
                return handle;
            }
        }
        return nullptr;
    };
    QVERIFY2(waitForCondition([&] { return clickedHandle() != nullptr; }, 2000, QStringLiteral("handle shown")),
             "The handle is not shown");
    QQuickItem* const handle = clickedHandle();
    const QVariantList pathBefore = path();
    // A click that wobbles a little, as taps and trackpad clicks do, is still a click
    const QPoint clickPoint = handle->mapToScene(handle->property("anchorPoint").toPointF()).toPoint();
    QTest::mousePress(_window, Qt::LeftButton, Qt::NoModifier, clickPoint);
    QTest::mouseMove(_window, clickPoint + QPoint(2, 1));
    QTest::mouseRelease(_window, Qt::LeftButton, Qt::NoModifier, clickPoint + QPoint(2, 1));
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("geoMapPolygonEditPosition"), 2000),
             "Clicking the handle did not open its menu");
    QVERIFY2(path() == pathBefore, "Clicking the handle moved the polygon");

    // A polygon keeps at least three vertices, and the center has none to remove
    const bool removable = (handleName == QStringLiteral("geoMapPolygonVertexHandle")) && (vertexCountBefore > 3);
    verifyVisibility(QStringLiteral("geoMapPolygonRemoveVertex"), removable, QStringLiteral("Remove vertex"));
    if (QTest::currentTestFailed() || menuItem.isEmpty())
        return;

    const QGeoCoordinate vertex = pathBefore[clickedVertex].value<QGeoCoordinate>();
    const QGeoCoordinate center = polygon->property("center").value<QGeoCoordinate>();
    // Whether the polygon has a vertex at coordinate, within a meter
    const auto hasVertexAt = [&](const QGeoCoordinate& coordinate) {
        const QVariantList vertices = path();
        return std::any_of(vertices.cbegin(), vertices.cend(), [&](const QVariant& other) {
            return other.value<QGeoCoordinate>().distanceTo(coordinate) < 1.0;
        });
    };
    // The vertices other than the clicked one are all still there
    const auto othersKept = [&] {
        for (int i = 0; i < pathBefore.count(); i++) {
            if ((i != clickedVertex) && !hasVertexAt(pathBefore[i].value<QGeoCoordinate>())) {
                return false;
            }
        }
        return true;
    };
    QVERIFY2(clickButton(menuItem), "Failed to click the menu item");

    if (menuItem == QStringLiteral("geoMapPolygonRemoveVertex")) {
        QVERIFY2(waitForCondition([&] { return vertexCount() == vertexCountBefore - 1; }, 2000,
                                  QStringLiteral("vertex removed")),
                 "Remove vertex did not remove the vertex");
        QVERIFY2(!hasVertexAt(vertex) && othersKept(), "Remove vertex removed another vertex than the clicked one");
        return;
    }

    const bool editsCenter = (handleName == QStringLiteral("geoMapPolygonCenterHandle"));
    const QString dialogName = editsCenter ? QStringLiteral("geoMapPolygonCenterPositionDialog")
                                           : QStringLiteral("geoMapPolygonVertexPositionDialog");
    const QString dialogTitle =
        editsCenter ? QStringLiteral("Edit Center Position") : QStringLiteral("Edit Vertex Position");
    QVERIFY2(waitForDialog(dialogTitle), "Edit position did not open the position dialog");
    // The popup has no QObject parent to search from; the title label's context names it
    QObject* dialog = nullptr;
    for (QQuickItem* const title : findGeoMapItems(_rootItem, QStringLiteral("popupDialog_title"))) {
        if (title->isVisible() && (title->property("text").toString() == dialogTitle)) {
            if (QQmlContext* const context = qmlContext(title)) {
                dialog = context->objectForName(QStringLiteral("root"));
            }
        }
    }
    QVERIFY2(dialog && (dialog->objectName() == dialogName), "The position dialog was not found");
    const QGeoCoordinate edited = (editsCenter ? center : vertex).atDistanceAndAzimuth(100, 45);
    dialog->setProperty("coordinate", QVariant::fromValue(edited));
    if (editsCenter) {
        QVERIFY2(waitForCondition(
                     [&] { return polygon->property("center").value<QGeoCoordinate>().distanceTo(edited) < 1.0; }, 2000,
                     QStringLiteral("center moved")),
                 "The polygon center did not move to the edited position");
        QVERIFY(!polygon->property("centerDrag").toBool());
    } else {
        // Winding may reorder the vertices, so they are compared as a set
        QVERIFY2(waitForCondition([&] { return hasVertexAt(edited); }, 2000, QStringLiteral("vertex moved")),
                 "No vertex moved to the edited position");
        QVERIFY2(othersKept(), "Edit position moved another vertex than the clicked one");
        QCOMPARE(vertexCount(), vertexCountBefore);
    }
    QMetaObject::invokeMethod(dialog, "close");
}

void PlanPanelUITest::_testGeoMapFencePolygonToolsReshapeThePolygon_data()
{
    QTest::addColumn<QString>("tool");

    QTest::newRow("basic") << QStringLiteral("basic");
    QTest::newRow("circular") << QStringLiteral("circular");
    QTest::newRow("circle radius drag") << QStringLiteral("radiusDrag");
    QTest::newRow("circle set radius") << QStringLiteral("setRadius");
    QTest::newRow("trace") << QStringLiteral("trace");
    QTest::newRow("trace with too few vertices") << QStringLiteral("traceTooFew");
    QTest::newRow("load file") << QStringLiteral("loadFile");
    QTest::newRow("load file from a toolbar too wide for the map") << QStringLiteral("loadFileNarrow");
    QTest::newRow("basic starts a polygon from none") << QStringLiteral("basicFromEmpty");
}

void PlanPanelUITest::_testGeoMapFencePolygonToolsReshapeThePolygon()
{
    QFETCH(QString, tool);

    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    _setHomeAndOpenMission();
    if (QTest::currentTestFailed())
        return;

    PlanMasterController* const master = _planMasterController();
    QVERIFY(master);
    GeoFenceController* const fence = master->geoFenceController();
    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);

    QVERIFY2(_selectTab(QStringLiteral("fence")), "Failed to open the Fence tab");
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceAddPolygon")), "Failed to click + Polygon");
    QVERIFY2(waitForCondition([&] { return fence->polygons()->count() == 1; }, 2000, QStringLiteral("polygon added")),
             "+ Polygon did not add a polygon");
    QObject* const polygon = fence->polygons()->get(0);
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("geoMapPolygonToolbar"), 2000), "No polygon tools toolbar");

    const auto vertices = [polygon] {
        QList<QGeoCoordinate> list;
        const QVariantList path = polygon->property("path").toList();
        for (const QVariant& vertex : path) {
            list.append(vertex.value<QGeoCoordinate>());
        }
        return list;
    };
    const auto center = [polygon] { return polygon->property("center").value<QGeoCoordinate>(); };
    // Every vertex at radius meters from the center, within tolerance meters
    const auto allAtRadius = [&](double radius, double tolerance = 1.0) {
        const auto list = vertices();
        return !list.isEmpty() && std::all_of(list.cbegin(), list.cend(), [&](const QGeoCoordinate& vertex) {
            return qAbs(center().distanceTo(vertex) - radius) < tolerance;
        });
    };
    const auto meanDistance = [&] {
        const auto list = vertices();
        double total = 0;
        for (const QGeoCoordinate& vertex : list) {
            total += center().distanceTo(vertex);
        }
        return total / list.count();
    };
    const auto windowPoint = [map](const QGeoCoordinate& coordinate) {
        QVariant point;
        QMetaObject::invokeMethod(map, "fromCoordinate", Q_RETURN_ARG(QVariant, point),
                                  Q_ARG(QVariant, QVariant::fromValue(coordinate)), Q_ARG(QVariant, false));
        return map->mapToScene(point.toPointF()).toPoint();
    };
    // Basic and Circular start over in view with the shapes a new fence gets: the polygon + Polygon just added, and
    // the circle that fits in it
    const QList<QGeoCoordinate> original = vertices();
    QCOMPARE(original.count(), 4);
    const QGeoCoordinate originalCenter = center();
    const double circleRadius = qMin(original[0].distanceTo(original[1]), original[1].distanceTo(original[2])) / 2;
    const auto matchesOriginal = [&] {
        const auto list = vertices();
        if (list.count() != original.count()) {
            return false;
        }
        for (int i = 0; i < list.count(); i++) {
            if (list[i].distanceTo(original[i]) > 1.0) {
                return false;
            }
        }
        return true;
    };

    const auto clickCircular = [&] {
        QVERIFY2(clickButton(QStringLiteral("geoMapPolygonCircular")), "Failed to click Circular");
        QVERIFY2(waitForCondition([&] { return vertices().count() == 16 && allAtRadius(circleRadius); }, 2000,
                                  QStringLiteral("circle")),
                 "Circular did not start over with the circle a new fence gets");
    };

    if (tool == QStringLiteral("basic")) {
        clickCircular();
        if (QTest::currentTestFailed())
            return;
        QVERIFY2(clickButton(QStringLiteral("geoMapPolygonBasic")), "Failed to click Basic");
        QVERIFY2(waitForCondition(matchesOriginal, 2000, QStringLiteral("new polygon")),
                 "Basic did not start over with the polygon a new fence gets");
        // Back out of circle mode: vertices are edited again
        QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("geoMapPolygonVertexHandle"), 2000),
                 "The new polygon has no vertex handles");
        QVERIFY(!findVisibleItem(_rootItem, QStringLiteral("geoMapPolygonRadiusHandle"), 0));
    } else if (tool == QStringLiteral("circular")) {
        clickCircular();
        if (QTest::currentTestFailed())
            return;
        QVERIFY(center().distanceTo(originalCenter) < 1.0);
        // A circle is sized by its radius handle instead of its vertices
        QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("geoMapPolygonRadiusHandle"), 2000),
                 "The circle has no radius handle");
        QVERIFY(!findVisibleItem(_rootItem, QStringLiteral("geoMapPolygonVertexHandle"), 0));
        QVERIFY(!findVisibleItem(_rootItem, QStringLiteral("geoMapPolygonSplitHandle"), 0));
    } else if (tool == QStringLiteral("radiusDrag")) {
        clickCircular();
        if (QTest::currentTestFailed())
            return;
        QQuickItem* const handle = findVisibleItem(_rootItem, QStringLiteral("geoMapPolygonRadiusHandle"), 2000);
        QVERIFY(handle);
        // The circle is wider than the viewport: zoom out until its radius handle is on screen
        const QRectF viewport = map->property("centerViewport").toRectF();
        const auto handleInWindow = [&] {
            return handle->mapToScene(handle->property("anchorPoint").toPointF()).toPoint();
        };
        map->setProperty("zoomLevel", map->property("zoomLevel").toDouble() - 1);
        QVERIFY2(waitForCondition([&] { return viewport.contains(map->mapFromScene(handleInWindow())); }, 3000,
                                  QStringLiteral("radius handle on screen")),
                 "The radius handle is not on screen");
        const QPoint start = handleInWindow();
        const QPoint delta(40, 0);
        QTest::mousePress(_window, Qt::LeftButton, Qt::NoModifier, start);
        constexpr int steps = 10;
        for (int i = 1; i <= steps; i++) {
            QTest::mouseMove(_window, start + ((delta * i) / steps));
        }
        QTest::mouseRelease(_window, Qt::LeftButton, Qt::NoModifier, start + delta);
        QVariant cursor;
        QMetaObject::invokeMethod(map, "toCoordinate", Q_RETURN_ARG(QVariant, cursor),
                                  Q_ARG(QVariant, QVariant::fromValue(QPointF(map->mapFromScene(start + delta)))),
                                  Q_ARG(QVariant, false));
        const double expectedRadius = center().distanceTo(cursor.value<QGeoCoordinate>());
        QVERIFY(expectedRadius > circleRadius);
        // The handle keeps its sub-pixel grab offset, so the radius is within a pixel of the cursor's distance
        QVERIFY2(waitForCondition(
                     [&] { return vertices().count() == 16 && allAtRadius(expectedRadius, expectedRadius * 0.01); },
                     2000, QStringLiteral("circle resized")),
                 qPrintable(QStringLiteral("The circle is %1 m around after the radius drag instead of %2 m")
                                .arg(meanDistance())
                                .arg(expectedRadius)));
        QVERIFY(!polygon->property("vertexDrag").toBool());
        QVERIFY2(center().distanceTo(originalCenter) < 1.0, "The circle's center moved while its radius was dragged");
    } else if (tool == QStringLiteral("setRadius")) {
        clickCircular();
        if (QTest::currentTestFailed())
            return;
        QQuickItem* const handle = findVisibleItem(_rootItem, QStringLiteral("geoMapPolygonCenterHandle"), 2000);
        QVERIFY(handle);
        QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier,
                          handle->mapToScene(handle->property("anchorPoint").toPointF()).toPoint());
        QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("geoMapPolygonSetRadius"), 2000),
                 "The circle's center menu has no Set radius");
        QVERIFY2(clickButton(QStringLiteral("geoMapPolygonSetRadius")), "Failed to click Set radius");
        QVERIFY2(waitForDialog(QStringLiteral("Set Radius")), "Set radius did not open its dialog");
        QQuickItem* const radiusField = findVisibleItem(_rootItem, QStringLiteral("geoMapPolygonRadiusField"), 2000);
        QVERIFY(radiusField);
        radiusField->setProperty("text", QStringLiteral("123"));
        QVERIFY2(acceptDialog(), "Failed to save the radius");
        const double expectedRadius = FactMetaData::appSettingsHorizontalDistanceUnitsToMeters(123.0).toDouble();
        QVERIFY2(waitForCondition([&] { return vertices().count() == 16 && allAtRadius(expectedRadius); }, 2000,
                                  QStringLiteral("radius set")),
                 "Set radius did not resize the circle");
    } else if (tool.startsWith(QStringLiteral("trace"))) {
        const int clicks = (tool == QStringLiteral("trace")) ? 3 : 2;
        QVERIFY2(clickButton(QStringLiteral("geoMapPolygonTrace")), "Failed to click Trace");
        QVERIFY2(waitForCondition([&] { return polygon->property("traceMode").toBool() && vertices().isEmpty(); }, 2000,
                                  QStringLiteral("tracing")),
                 "Trace did not clear the polygon for tracing");
        const QList<QPointF> fractions = {QPointF(0.45, 0.45), QPointF(0.55, 0.45), QPointF(0.5, 0.55)};
        for (int i = 0; i < clicks; i++) {
            _clickMap(fractions[i].x(), fractions[i].y());
            QVERIFY2(waitForCondition([&] { return vertices().count() == i + 1; }, 2000, QStringLiteral("traced")),
                     "A map click did not add a vertex");
        }
        QVERIFY2(clickButton(QStringLiteral("geoMapPolygonTrace")), "Failed to click Done Tracing");
        QVERIFY2(waitForCondition([&] { return !polygon->property("traceMode").toBool(); }, 2000,
                                  QStringLiteral("tracing done")),
                 "Done Tracing did not end tracing");
        if (clicks < 3) {
            // Too few vertices for a polygon: the polygon from before tracing comes back
            QCOMPARE(vertices(), original);
        } else {
            QCOMPARE(vertices().count(), clicks);
        }
    } else if (tool == QStringLiteral("basicFromEmpty")) {
        // An empty polygon, like a new survey area: Basic gives it its first vertices
        QMetaObject::invokeMethod(polygon, "clear");
        QVERIFY(vertices().isEmpty());
        QVERIFY2(clickButton(QStringLiteral("geoMapPolygonBasic")), "Failed to click Basic");
        QVERIFY2(waitForCondition(matchesOriginal, 2000, QStringLiteral("new polygon")),
                 "Basic did not start over with the polygon a new fence gets");
    } else if (tool.startsWith(QStringLiteral("loadFile"))) {
        if (tool == QStringLiteral("loadFileNarrow")) {
            QQuickItem* const flickable =
                findVisibleItem(_rootItem, QStringLiteral("geoMapPolygonToolbarFlickable"), 2000);
            QVERIFY2(flickable, "The polygon toolbar does not scroll");
            QQuickItem* const loadButton = findVisibleItem(_rootItem, QStringLiteral("geoMapPolygonLoadFile"), 2000);
            QVERIFY(loadButton);
            const auto buttonInView = [&] {
                const QRectF shown(flickable->mapToScene(QPointF(0, 0)), flickable->size());
                return shown.contains(loadButton->mapToScene(loadButton->boundingRect().center()));
            };
            // Narrow the map well below the toolbar's width (the window has a minimum width): the last buttons are cut
            // off
            const double rowWidth = flickable->property("contentWidth").toDouble();
            const double viewportWidth = map->property("centerViewport").toRectF().width();
            _window->resize(_window->width() - qRound(viewportWidth - rowWidth / 2), _window->height());
            QVERIFY2(waitForCondition([&] { return flickable->width() < rowWidth && !buttonInView(); }, 2000,
                                      QStringLiteral("toolbar cut off")),
                     "The narrow map does not cut off the toolbar");
            // Swiping the toolbar sideways scrolls to them
            flickable->setProperty("contentX", rowWidth - flickable->width());
            QVERIFY2(waitForCondition(buttonInView, 2000, QStringLiteral("button scrolled into view")),
                     "Scrolling the toolbar did not bring Load KML/SHP into view");
        }
        QGCFileDialogController::setTestNextFileForAccept(QStringLiteral(":/unittest/PolygonGood.kml"));
        QVERIFY2(clickButton(QStringLiteral("geoMapPolygonLoadFile")), "Failed to click Load KML/SHP");
        QVERIFY2(waitForCondition([&] { return vertices().count() == 4 && vertices() != original; }, 2000,
                                  QStringLiteral("file loaded")),
                 "Loading the file did not replace the polygon");
        // The file's polygon is far from the map: the map moves to show it
        const QRectF viewport = map->property("centerViewport").toRectF();
        QVERIFY2(waitForCondition([&] { return viewport.contains(map->mapFromScene(windowPoint(center()))); }, 3000,
                                  QStringLiteral("map fit to the file")),
                 "The map did not move to the loaded polygon");
    }
}

void PlanPanelUITest::_testGeoMapBreachReturnPointDragsOnlyOnTheFenceLayer_data()
{
    QTest::addColumn<QString>("tab");

    QTest::newRow("fence tab moves the point") << QStringLiteral("fence");
    QTest::newRow("other tab pans the map") << QStringLiteral("mission");
}

void PlanPanelUITest::_testGeoMapBreachReturnPointDragsOnlyOnTheFenceLayer()
{
    QFETCH(QString, tab);
    const bool expectMove = (tab == QStringLiteral("fence"));

    // Only ArduPilot planes use a breach return point
    if (!apmFirmwareSupported()) {
        QSKIP("ArduPilot firmware support is not built");
    }
    const OfflineVehicleScope arduPlane(QGCMAVLink::FirmwareClassArduPilot, QGCMAVLink::VehicleClassFixedWing);

    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    _setHomeAndOpenMission();
    if (QTest::currentTestFailed())
        return;

    PlanMasterController* const master = _planMasterController();
    QVERIFY(master);
    GeoFenceController* const fence = master->geoFenceController();
    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    QVERIFY2(_selectTab(tab), "Failed to open the tab");

    const QGeoCoordinate mapCenter = map->property("center").value<QGeoCoordinate>();
    const double altitude = 55.0;
    fence->setBreachReturnPoint(
        QGeoCoordinate(mapCenter.latitude(), mapCenter.longitude(), altitude).atDistanceAndAzimuth(200, 225));
    QQuickItem* const marker = findVisibleItem(_rootItem, QStringLiteral("geoMapBreachReturnPoint"), 2000);
    QVERIFY2(marker, "The breach return point marker is not shown");

    const auto windowPoint = [map](const QGeoCoordinate& coordinate) {
        QVariant point;
        QMetaObject::invokeMethod(map, "fromCoordinate", Q_RETURN_ARG(QVariant, point),
                                  Q_ARG(QVariant, QVariant::fromValue(coordinate)), Q_ARG(QVariant, false));
        return map->mapToScene(point.toPointF()).toPoint();
    };
    const QGeoCoordinate before = fence->breachReturnPoint();
    const QPoint start = windowPoint(before);
    const QPoint delta(-60, 40);
    QTest::mousePress(_window, Qt::LeftButton, Qt::NoModifier, start);
    constexpr int steps = 10;
    for (int i = 1; i <= steps; i++) {
        QTest::mouseMove(_window, start + ((delta * i) / steps));
    }
    QTest::mouseRelease(_window, Qt::LeftButton, Qt::NoModifier, start + delta);

    if (expectMove) {
        // The point ends up under the cursor and keeps the altitude set in the inspector
        QVERIFY2(waitForCondition(
                     [&] { return (windowPoint(fence->breachReturnPoint()) - (start + delta)).manhattanLength() <= 4; },
                     2000, QStringLiteral("point dragged")),
                 "Dragging the breach return point did not move it under the cursor");
        QVERIFY(qFuzzyCompare(fence->breachReturnPoint().altitude(), altitude));
    } else {
        QVERIFY2(waitForCondition([&] { return (windowPoint(before) - start).manhattanLength() > 20; }, 2000,
                                  QStringLiteral("map panned")),
                 "Dragging the breach return point off the Fence tab did not pan the map");
        QCOMPARE(fence->breachReturnPoint(), before);
    }
}

void PlanPanelUITest::_testGeoMapFenceClickSelectsTheFenceUnderIt_data()
{
    QTest::addColumn<QString>("target");
    QTest::addColumn<QString>("expectedRow");
    QTest::addColumn<bool>("drillIn");

    QTest::newRow("polygon") << QStringLiteral("polygon") << QStringLiteral("planPanel_fenceRow_polygon_0") << false;
    QTest::newRow("smaller of overlapping fences")
        << QStringLiteral("nestedCircle") << QStringLiteral("planPanel_fenceRow_circle_0") << false;
    QTest::newRow("outside every fence keeps the selection")
        << QStringLiteral("outside") << QStringLiteral("planPanel_fenceRow_polygon_0") << false;
    // The marker sits inside the polygon: the click goes to the marker, not to the fence under it
    QTest::newRow("breach return point") << QStringLiteral("breach") << QStringLiteral("planPanel_fenceRow_breach")
                                         << false;
    // Where the list and the inspector take turns, a pick on the map shows the inspector as a row click does
    QTest::newRow("polygon in the drill-in layout")
        << QStringLiteral("polygon") << QStringLiteral("planPanel_fenceRow_polygon_0") << true;
    QTest::newRow("breach return point in the drill-in layout")
        << QStringLiteral("breach") << QStringLiteral("planPanel_fenceRow_breach") << true;
}

void PlanPanelUITest::_testGeoMapFenceClickSelectsTheFenceUnderIt()
{
    QFETCH(QString, target);
    QFETCH(QString, expectedRow);
    QFETCH(bool, drillIn);

    // Only ArduPilot planes use a breach return point
    std::optional<OfflineVehicleScope> arduPlane;
    if (target == QStringLiteral("breach")) {
        if (!apmFirmwareSupported()) {
            QSKIP("ArduPilot firmware support is not built");
        }
        arduPlane.emplace(QGCMAVLink::FirmwareClassArduPilot, QGCMAVLink::VehicleClassFixedWing);
    }

    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    _setHomeAndOpenMission();
    if (QTest::currentTestFailed())
        return;

    PlanMasterController* const master = _planMasterController();
    QVERIFY(master);
    GeoFenceController* const fence = master->geoFenceController();
    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);

    QVERIFY2(_selectTab(QStringLiteral("fence")), "Failed to open the Fence tab");
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceAddPolygon")), "Failed to click + Polygon");
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceAddCircle")), "Failed to click + Circle");
    QVERIFY2(waitForCondition([&] { return fence->polygons()->count() == 1 && fence->circles()->count() == 1; }, 2000,
                              QStringLiteral("fences added")),
             "The fences were not added");
    QObject* const polygon = fence->polygons()->get(0);
    QObject* const circle = fence->circles()->get(0);

    // A small circle inside the polygon, away from the polygon's handles
    const QGeoCoordinate polygonCenter = polygon->property("center").value<QGeoCoordinate>();
    QGeoCoordinate corner;
    QMetaObject::invokeMethod(polygon, "vertexCoordinate", Q_RETURN_ARG(QGeoCoordinate, corner), Q_ARG(int, 0));
    const double cornerDistance = polygonCenter.distanceTo(corner);
    const QGeoCoordinate circleCenter = polygonCenter.atDistanceAndAzimuth(cornerDistance * 0.4, 45);
    circle->setProperty("center", QVariant::fromValue(circleCenter));
    fence->circles()->value<QGCMapCircle*>(0)->radius()->setRawValue(cornerDistance * 0.15);

    // The polygon starts selected, except when the click goes to the circle nested in it
    QVERIFY2(clickButton(target == QStringLiteral("nestedCircle") ? QStringLiteral("planPanel_fenceRow_polygon_0")
                                                                  : QStringLiteral("planPanel_fenceRow_circle_0")),
             "Failed to select the starting fence");

    const QRectF viewport = map->property("centerViewport").toRectF();
    QPointF point;
    if (target == QStringLiteral("outside")) {
        // The new polygon is inset from the viewport edges
        point = QPointF(viewport.left() + viewport.width() * 0.04, viewport.center().y());
        QVERIFY2(clickButton(QStringLiteral("planPanel_fenceRow_polygon_0")), "Failed to select the polygon");
    } else {
        const QGeoCoordinate clickCoordinate = (target == QStringLiteral("nestedCircle"))
                                                   ? circleCenter
                                                   : polygonCenter.atDistanceAndAzimuth(cornerDistance * 0.5, 225);
        if (target == QStringLiteral("breach")) {
            fence->setBreachReturnPoint(clickCoordinate);
            QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("geoMapBreachReturnPoint"), 2000),
                     "The breach return point marker is not shown");
        }
        QVariant projected;
        QMetaObject::invokeMethod(map, "fromCoordinate", Q_RETURN_ARG(QVariant, projected),
                                  Q_ARG(QVariant, QVariant::fromValue(clickCoordinate)), Q_ARG(QVariant, false));
        point = projected.toPointF();
    }
    QVERIFY2(viewport.contains(point), "The click point is off screen");
    if (drillIn) {
        QQuickItem* const listInspector = findItem(_rootItem, QStringLiteral("planPanel_fenceListInspector"));
        QVERIFY(listInspector);
        listInspector->setProperty("layoutModeOverride", QStringLiteral("drillIn"));
        verifyVisibility(QStringLiteral("planPanel_fenceInspector_title"), false,
                         QStringLiteral("list shown before the map click"));
        if (QTest::currentTestFailed())
            return;
    }
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, map->mapToScene(point).toPoint());

    QCOMPARE(polygon->property("interactive").toBool(), expectedRow.contains(QStringLiteral("polygon")));
    QCOMPARE(circle->property("interactive").toBool(), expectedRow.contains(QStringLiteral("circle")));
    QString expectedTitle;
    if (expectedRow.contains(QStringLiteral("polygon"))) {
        expectedTitle = QStringLiteral("Polygon 1");
    } else if (expectedRow.contains(QStringLiteral("circle"))) {
        expectedTitle = QStringLiteral("Circle 1");
    } else {
        expectedTitle = QStringLiteral("Breach return point");
    }
    verifyText(QStringLiteral("planPanel_fenceInspector_title"), expectedTitle,
               QStringLiteral("inspector of the fence selected by the map click"));
    if (!drillIn) {
        verifyProperty(expectedRow, "selected", true, QStringLiteral("fence selected by the map click"));
    }
}

void PlanPanelUITest::_testGeoMapKeepsTabbedPanelWhenTheSettingIsTurnedOff()
{
    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    // The tree panel does not support the GeoMap engine, which stays loaded until the restart
    planViewSettings()->useNewPlanPanel()->setRawValue(false);
    verifyVisibility(QStringLiteral("planPanel_tabBar"), true, QStringLiteral("tabbed panel stays"));
    verifyVisibility(QLatin1String(kPlanTree), false, QStringLiteral("tree panel not shown"));
    verifyVisibility(QStringLiteral("geoMapModeButton"), true, QStringLiteral("GeoMap camera controls stay"));
}

void PlanPanelUITest::_testGeoMapModeButtonTogglesCameraMode()
{
    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;

    QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY(map);
    auto* const camera = map->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(camera, "GeoMapCamera not found");
    QCOMPARE(camera->mode(), GeoMapCamera::Mode::Mode2D);

    const QString modeButton = QStringLiteral("geoMapModeButton");
    verifyText(modeButton, QStringLiteral("3D"), QStringLiteral("button offers 3D from 2D"));
    QVERIFY2(clickButton(modeButton), "Failed to click the 2D/3D button");
    QVERIFY2(
        waitForCondition([&] { return camera->mode() == GeoMapCamera::Mode::Mode3D; }, 2000, QStringLiteral("3D mode")),
        "The 2D/3D button did not switch the Plan map to 3D");
    verifyText(modeButton, QStringLiteral("2D"), QStringLiteral("button offers 2D from 3D"));

    QVERIFY2(clickButton(modeButton), "Failed to click the 2D/3D button back");
    QVERIFY2(
        waitForCondition([&] { return camera->mode() == GeoMapCamera::Mode::Mode2D; }, 2000, QStringLiteral("2D mode")),
        "The 2D/3D button did not switch the Plan map back to 2D");
}

void PlanPanelUITest::_testVehicleClassComboChangesToolStrip_data()
{
    QTest::addColumn<int>("startClass");
    QTest::addColumn<int>("chosenClass");
    QTest::addColumn<bool>("takeoffVisible");

    QTest::newRow("multi-rotor to rover")
        << int(QGCMAVLink::VehicleClassMultiRotor) << int(QGCMAVLink::VehicleClassRoverBoat) << false;
    QTest::newRow("rover to multi-rotor")
        << int(QGCMAVLink::VehicleClassRoverBoat) << int(QGCMAVLink::VehicleClassMultiRotor) << true;
}

void PlanPanelUITest::_testVehicleClassComboChangesToolStrip()
{
    QFETCH(int, startClass);
    QFETCH(int, chosenClass);
    QFETCH(bool, takeoffVisible);

    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    Fact* const vehicleClass = appSettings->offlineEditingVehicleClass();
    Fact* const firmwareClass = appSettings->offlineEditingFirmwareClass();
    const QVariant savedVehicleClass = vehicleClass->rawValue();
    const QVariant savedFirmwareClass = firmwareClass->rawValue();
    const auto restoreGuard = qScopeGuard([=] {
        vehicleClass->setRawValue(savedVehicleClass);
        firmwareClass->setRawValue(savedFirmwareClass);
    });
    firmwareClass->setRawValue(QGCMAVLink::FirmwareClassPX4);
    vehicleClass->setRawValue(startClass);

    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;

    QQuickItem* const combo = findVisibleItem(_rootItem, QStringLiteral("planPanel_vehicleTypeCombo"), 2000);
    QVERIFY2(combo, "Vehicle type combo not shown for an empty offline plan");
    int chosenIndex = -1;
    const QVariantList enumValues = vehicleClass->enumValues();
    for (int i = 0; i < enumValues.size(); i++) {
        if (enumValues.at(i).toInt() == chosenClass) {
            chosenIndex = i;
        }
    }
    QVERIFY2(chosenIndex >= 0, "Chosen vehicle class is not offered by the combo");
    QVERIFY(QMetaObject::invokeMethod(combo, "activated", Q_ARG(int, chosenIndex)));
    QVERIFY2(waitForCondition([&] { return vehicleClass->rawValue().toInt() == chosenClass; }, 2000,
                              QStringLiteral("vehicle class")),
             "Choosing a vehicle type did not change the offline vehicle class");

    _setHomeAndOpenMission();
    if (QTest::currentTestFailed())
        return;
    verifyVisibility(QLatin1String(kWaypointButton), true, QStringLiteral("tool strip after changing vehicle"));
    verifyVisibility(QLatin1String(kTakeoffButton), takeoffVisible, QStringLiteral("takeoff after changing vehicle"));
}

void PlanPanelUITest::_testComplexItemEditorFitsInspector()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;

    _clickMap(0.4, 0.5);
    if (QTest::currentTestFailed())
        return;
    MissionController* const controller = _missionController();
    QVERIFY(controller);
    QVERIFY2(waitForCondition([&] { return controller->homePositionSet(); }, 2000, QStringLiteral("home set")),
             "Home was not set");

    // The Survey template inserts settings, takeoff, the survey and land
    QVERIFY2(clickButton(QStringLiteral("planCreator_Survey")), "Failed to click the Survey template");
    QVERIFY2(waitForCondition([&] { return controller->visualItems()->count() == 4; }, 3000,
                              QStringLiteral("survey template")),
             "Survey template did not create the plan");
    QVERIFY2(waitForCondition([&] { return _currentTab() == QStringLiteral("mission"); }, 2000,
                              QStringLiteral("mission tab")),
             "Survey template did not open the Mission tab");
    VisualMissionItem* const survey = controller->visualItems()->value<VisualMissionItem*>(2);
    QVERIFY(survey);
    QVERIFY(survey->isSurveyItem());
    const int surveySeq = survey->sequenceNumber();
    QVERIFY2(clickButton(rowObjectName(surveySeq)), "Failed to click the survey row");
    QVERIFY2(waitForCondition([&] { return controller->currentPlanViewItem() == survey; }, 2000,
                              QStringLiteral("survey selected")),
             "Clicking the survey row did not select it");

    verifyText(QString::fromLatin1(kInspectorTitle), survey->commandName(), QStringLiteral("survey inspector title"));
    QQuickItem* const editorLoader = findVisibleItem(_rootItem, QStringLiteral("planPanel_inspectorEditor"), 2000);
    QQuickItem* const flickable = findVisibleItem(_rootItem, QStringLiteral("planPanel_inspectorFlickable"), 2000);
    QVERIFY(editorLoader);
    QVERIFY(flickable);
    QVERIFY2(waitForCondition([&] { return editorLoader->property("item").value<QQuickItem*>() != nullptr; }, 2000,
                              QStringLiteral("survey editor loaded")),
             "Survey editor did not load in the inspector");
    QQuickItem* const editor = editorLoader->property("item").value<QQuickItem*>();
    QVERIFY2(waitForCondition([&] { return editor->width() > 0 && editor->width() <= flickable->width(); }, 2000,
                              QStringLiteral("survey editor fits inspector")),
             qPrintable(QStringLiteral("Survey editor is %1 wide in a %2 wide inspector")
                            .arg(editor->width())
                            .arg(flickable->width())));
    QVERIFY2(flickable->property("contentHeight").toReal() > 0, "Survey editor has no content height");
}

void PlanPanelUITest::_testOpenedPlanShowsMissionTab()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;

    _buildMission(1);
    if (QTest::currentTestFailed())
        return;
    PlanMasterController* const masterController = _planMasterController();
    QVERIFY(masterController);
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString planFile = tempDir.filePath(QStringLiteral("PlanPanelUITest.plan"));
    QVERIFY(masterController->saveToFile(planFile));

    masterController->removeAll();
    QVERIFY2(
        waitForCondition([&] { return _currentTab() == QStringLiteral("plan"); }, 2000, QStringLiteral("cleared plan")),
        "Clearing the plan did not return to the Plan tab");
    MissionController* const controller = _missionController();
    QVERIFY(controller);
    QCOMPARE(controller->visualItems()->count(), 1);

    QGCFileDialogController::setTestNextFileForAccept(planFile);
    QVERIFY2(clickButton(QStringLiteral("planToolbar_openButton")), "Failed to click Open");
    QVERIFY2(waitForCondition([] { return !QGCFileDialogController::testHookArmed(); }, 5000,
                              QStringLiteral("file dialog shim consumed")),
             "Open: file dialog shim was not consumed");
    QVERIFY2(
        waitForCondition([&] { return controller->visualItems()->count() == 3; }, 3000, QStringLiteral("plan loaded")),
        "Open did not load the saved plan");
    QVERIFY2(waitForCondition([&] { return _currentTab() == QStringLiteral("mission"); }, 2000,
                              QStringLiteral("mission tab")),
             "Opening a plan with items did not show the Mission tab");
    const int lastSeq =
        controller->visualItems()->value<VisualMissionItem*>(controller->visualItems()->count() - 1)->sequenceNumber();
    QVERIFY2(findVisibleItem(_rootItem, rowObjectName(lastSeq), 2000), "Loaded plan has no row for its last item");
}

void PlanPanelUITest::_testConnectedVehicleFixesVehicleChoiceAndEnablesVehicleActions()
{
    setUseTabbedPlanPanel(true);
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [this](QPointer<MockLink>, Vehicle* vehicle) {
            _openPlanView();
            if (QTest::currentTestFailed())
                return;

            // A connected vehicle decides firmware and vehicle type, so the offline combos become labels
            verifyVisibility(QStringLiteral("planPanel_firmwareCombo"), false,
                             QStringLiteral("firmware combo while connected"));
            verifyVisibility(QStringLiteral("planPanel_vehicleTypeCombo"), false,
                             QStringLiteral("vehicle type combo while connected"));
            verifyText(QStringLiteral("planPanel_firmwareLabel"), vehicle->firmwareTypeString(),
                       QStringLiteral("firmware label"));
            verifyText(QStringLiteral("planPanel_vehicleTypeLabel"), vehicle->vehicleTypeString(),
                       QStringLiteral("vehicle type label"));
            if (QTest::currentTestFailed())
                return;

            _buildMission(1);
            if (QTest::currentTestFailed())
                return;
            verifyEnabled(QStringLiteral("planToolbar_uploadButton"), true,
                          QStringLiteral("upload with items and a vehicle"));
            if (QTest::currentTestFailed())
                return;

            MissionController* const controller = _missionController();
            QVERIFY(controller);
            VisualMissionItem* const item = controller->currentPlanViewItem();
            QVERIFY(item);
            QVERIFY2(waitForCondition([&] { return vehicle->coordinate().isValid(); }, 5000,
                                      QStringLiteral("vehicle position")),
                     "MockLink vehicle never reported a position");
            QVERIFY2(item->coordinate().distanceTo(vehicle->coordinate()) > 1.0,
                     "Item must start away from the vehicle");

            QVERIFY2(clickButton(QLatin1String(kInspectorMenu)), "Failed to click the inspector menu button");
            QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("missionItemMenu_moveToVehicle"), 2000),
                     "Item menu did not open");
            verifyEnabled(QStringLiteral("missionItemMenu_moveToVehicle"), true, QStringLiteral("move to vehicle"));
            if (QTest::currentTestFailed())
                return;
            QVERIFY2(clickButton(QStringLiteral("missionItemMenu_moveToVehicle")), "Failed to click Move to vehicle");
            QVERIFY2(waitForCondition([&] { return item->coordinate().distanceTo(vehicle->coordinate()) < 1.0; }, 2000,
                                      QStringLiteral("item moved")),
                     "Move to vehicle did not move the inspected item");
        });
}

PlanMasterController* PlanPanelUITest::_planMasterController()
{
    QQuickItem* const planView = findVisibleItem(_rootItem, QStringLiteral("mainView_plan"));
    return planView ? qvariant_cast<PlanMasterController*>(planView->property("_planMasterController")) : nullptr;
}

void PlanPanelUITest::_testFenceTabAddsSelectsAndEditsFences()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;
    _setHome();
    if (QTest::currentTestFailed())
        return;

    PlanMasterController* const master = _planMasterController();
    QVERIFY(master);
    GeoFenceController* const fence = master->geoFenceController();
    QVERIFY2(fence->supported(), "Offline vehicle should support GeoFence");

    QVERIFY2(_selectTab(QStringLiteral("fence")), "Failed to open the Fence tab");
    verifyVisibility(QStringLiteral("planPanel_fenceInspector_title"), false,
                     QStringLiteral("no inspector without selection"));
    if (QTest::currentTestFailed())
        return;

    // A new polygon becomes the selected fence and the one edited on the map
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceAddPolygon")), "Failed to click + Polygon");
    QVERIFY2(waitForCondition([&] { return fence->polygons()->count() == 1; }, 2000, QStringLiteral("polygon added")),
             "+ Polygon did not add a polygon");
    QObject* const polygon = fence->polygons()->get(0);
    verifyVisibility(QStringLiteral("planPanel_fenceRow_polygon_0"), true, QStringLiteral("polygon row"));
    verifyProperty(QStringLiteral("planPanel_fenceRow_polygon_0"), "selected", true,
                   QStringLiteral("new polygon selected"));
    verifyText(QStringLiteral("planPanel_fenceInspector_title"), QStringLiteral("Polygon 1"),
               QStringLiteral("polygon title"));
    QVERIFY(polygon->property("interactive").toBool());
    if (QTest::currentTestFailed())
        return;

    // Include/Exclude follows the fence in both directions
    verifyProperty(QStringLiteral("planPanel_fenceInclusion"), "currentIndex", 0, QStringLiteral("new fences include"));
    polygon->setProperty("inclusion", false);
    verifyProperty(QStringLiteral("planPanel_fenceInclusion"), "currentIndex", 1,
                   QStringLiteral("inclusion change shown"));
    verifyText(QStringLiteral("planPanel_fenceRowMeta"), QStringLiteral("Exclude"),
               QStringLiteral("row meta follows inclusion"));
    if (QTest::currentTestFailed())
        return;

    // A new circle takes the selection and the edit state from the polygon
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceAddCircle")), "Failed to click + Circle");
    QVERIFY2(waitForCondition([&] { return fence->circles()->count() == 1; }, 2000, QStringLiteral("circle added")),
             "+ Circle did not add a circle");
    QObject* const circle = fence->circles()->get(0);
    verifyProperty(QStringLiteral("planPanel_fenceRow_circle_0"), "selected", true,
                   QStringLiteral("new circle selected"));
    verifyProperty(QStringLiteral("planPanel_fenceRow_polygon_0"), "selected", false,
                   QStringLiteral("polygon deselected"));
    verifyText(QStringLiteral("planPanel_fenceInspector_title"), QStringLiteral("Circle 1"),
               QStringLiteral("circle title"));
    verifyVisibility(QStringLiteral("planPanel_fenceRadius"), true, QStringLiteral("circle radius field"));
    QVERIFY(circle->property("interactive").toBool());
    QVERIFY(!polygon->property("interactive").toBool());
    if (QTest::currentTestFailed())
        return;

    // An invalid radius blocks selecting another fence until it is corrected
    QQuickItem* const radiusField = findItem(_rootItem, QStringLiteral("planPanel_fenceRadius"));
    QVERIFY(radiusField);
    QObject* const radiusTextField = radiusField->property("textField").value<QObject*>();
    QVERIFY(radiusTextField);
    const QVariant validRadius = radiusTextField->property("fact").value<QObject*>()->property("valueString");
    QVERIFY2(enterInvalidValue(radiusTextField), "Non-numeric radius was accepted");
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceRow_polygon_0")), "Failed to click the polygon row");
    verifyProperty(QStringLiteral("planPanel_fenceRow_circle_0"), "selected", true,
                   QStringLiteral("selection blocked by the invalid radius"));
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceAddPolygon")), "Failed to click + Polygon");
    QCOMPARE(fence->polygons()->count(), 1);
    verifyProperty(QStringLiteral("planPanel_fenceRow_circle_0"), "selected", true,
                   QStringLiteral("adding a fence blocked by the invalid radius"));
    QVERIFY(circle->property("interactive").toBool());
    QVERIFY(restoreValidValue(radiusTextField, validRadius));
    if (QTest::currentTestFailed())
        return;

    // Clicking a row selects that fence for map editing
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceRow_polygon_0")), "Failed to click the polygon row");
    QVERIFY2(waitForCondition(
                 [&] { return polygon->property("interactive").toBool() && !circle->property("interactive").toBool(); },
                 2000, QStringLiteral("row selects fence")),
             "Clicking a fence row did not make it the interactive fence");
    verifyVisibility(QStringLiteral("planPanel_fenceRadius"), false, QStringLiteral("radius hidden for polygons"));
    if (QTest::currentTestFailed())
        return;

    // Deleting the selected fence removes its row and closes the inspector
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceRow_polygon_0")), "Failed to select the polygon");
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceInspector_delete")), "Failed to click delete");
    QVERIFY2(waitForCondition([&] { return fence->polygons()->count() == 0; }, 2000, QStringLiteral("polygon deleted")),
             "Delete did not remove the polygon");
    verifyVisibility(QStringLiteral("planPanel_fenceInspector_title"), false,
                     QStringLiteral("inspector closed after delete"));
    QVERIFY2(
        waitForCondition([&] { return !findVisibleItem(_rootItem, QStringLiteral("planPanel_fenceRow_polygon_0"), 0); },
                         2000, QStringLiteral("polygon row removed")),
        "Deleted polygon still has a row");

    // The selection does not outlive its fences, so a plan loaded later does not open on a stale selection
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceRow_circle_0")), "Failed to select the circle");
    verifyProperty(QStringLiteral("planPanel_page_fence"), "selectedKind", QStringLiteral("circle"),
                   QStringLiteral("circle selected before clearing"));
    fence->removeAll();
    verifyProperty(QStringLiteral("planPanel_page_fence"), "selectedKind", QString(),
                   QStringLiteral("selection cleared with the fences"));
}

void PlanPanelUITest::_testBreachReturnPointOnlyForArduPilotPlanes_data()
{
    QTest::addColumn<int>("firmwareClass");
    QTest::addColumn<int>("vehicleClass");
    QTest::addColumn<bool>("supported");

    // PX4 ignores the point; of ArduPilot only Plane flies to it
    QTest::newRow("PX4 multi-rotor") << int(QGCMAVLink::FirmwareClassPX4) << int(QGCMAVLink::VehicleClassMultiRotor)
                                     << false;
    QTest::newRow("PX4 fixed-wing") << int(QGCMAVLink::FirmwareClassPX4) << int(QGCMAVLink::VehicleClassFixedWing)
                                    << false;
    QTest::newRow("ArduPilot multi-rotor")
        << int(QGCMAVLink::FirmwareClassArduPilot) << int(QGCMAVLink::VehicleClassMultiRotor) << false;
    QTest::newRow("ArduPilot rover") << int(QGCMAVLink::FirmwareClassArduPilot)
                                     << int(QGCMAVLink::VehicleClassRoverBoat) << false;
    QTest::newRow("ArduPilot fixed-wing")
        << int(QGCMAVLink::FirmwareClassArduPilot) << int(QGCMAVLink::VehicleClassFixedWing) << true;
    QTest::newRow("ArduPilot VTOL") << int(QGCMAVLink::FirmwareClassArduPilot) << int(QGCMAVLink::VehicleClassVTOL)
                                    << true;
}

void PlanPanelUITest::_testBreachReturnPointOnlyForArduPilotPlanes()
{
    QFETCH(int, firmwareClass);
    QFETCH(int, vehicleClass);
    QFETCH(bool, supported);

    if ((firmwareClass == QGCMAVLink::FirmwareClassArduPilot) && !apmFirmwareSupported()) {
        QSKIP("ArduPilot firmware support is not built");
    }
    const OfflineVehicleScope offlineVehicle(firmwareClass, vehicleClass);

    _startPlanPanel(true /* geoMapEngine */);
    if (QTest::currentTestFailed())
        return;
    _setHome();
    if (QTest::currentTestFailed())
        return;

    GeoFenceController* const fence = _planMasterController()->geoFenceController();
    QCOMPARE(fence->breachReturnSupported(), supported);
    QVERIFY2(_selectTab(QStringLiteral("fence")), "Failed to open the Fence tab");
    // The polygon row is the barrier that the list is built
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceAddPolygon")), "Failed to click + Polygon");
    verifyVisibility(QStringLiteral("planPanel_fenceRow_polygon_0"), true, QStringLiteral("polygon row"));
    verifyVisibility(QStringLiteral("planPanel_fenceRow_breach"), supported, QStringLiteral("breach row"));
    if (QTest::currentTestFailed())
        return;

    if (!supported) {
        // A point from a plan file is kept but not shown
        QQuickItem* const map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
        QVERIFY(map);
        fence->setBreachReturnPoint(map->property("center").value<QGeoCoordinate>());
        QVERIFY(!findVisibleItem(_rootItem, QStringLiteral("geoMapBreachReturnPoint"), 0));
        return;
    }

    // Set and removed from its inspector, which has no altitude: Plane returns at FENCE_RET_ALT
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceRow_breach")), "Failed to click the breach row");
    verifyText(QStringLiteral("planPanel_fenceInspector_title"), QStringLiteral("Breach return point"),
               QStringLiteral("breach title"));
    verifyVisibility(QStringLiteral("planPanel_fenceInspector_delete"), false,
                     QStringLiteral("breach cannot be deleted"));
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceBreachToggle")), "Failed to set the breach return point");
    QVERIFY2(waitForCondition([&] { return fence->breachReturnPoint().isValid(); }, 2000, QStringLiteral("breach set")),
             "Set at map center did not set the breach return point");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("geoMapBreachReturnPoint"), 2000),
             "The breach return point is not shown on the map");
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceBreachToggle")), "Failed to remove the breach return point");
    QVERIFY2(
        waitForCondition([&] { return !fence->breachReturnPoint().isValid(); }, 2000, QStringLiteral("breach removed")),
        "Remove did not clear the breach return point");
}

void PlanPanelUITest::_testFenceInspectorStepsAcrossEntriesInDrillIn()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;
    _setHome();
    if (QTest::currentTestFailed())
        return;

    GeoFenceController* const fence = _planMasterController()->geoFenceController();
    QVERIFY2(_selectTab(QStringLiteral("fence")), "Failed to open the Fence tab");
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceAddPolygon")), "Failed to add a polygon");
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceAddCircle")), "Failed to add a circle");
    QVERIFY2(waitForCondition([&] { return fence->polygons()->count() == 1 && fence->circles()->count() == 1; }, 2000,
                              QStringLiteral("fences added")),
             "Fences were not added");

    QQuickItem* const listInspector = findItem(_rootItem, QStringLiteral("planPanel_fenceListInspector"));
    QVERIFY(listInspector);
    QCOMPARE(listInspector->property("layoutMode").toString(), QStringLiteral("split"));
    verifyVisibility(QStringLiteral("planPanel_fenceInspector_back"), false,
                     QStringLiteral("no back button when split"));
    listInspector->setProperty("layoutModeOverride", QStringLiteral("drillIn"));
    verifyVisibility(QStringLiteral("planPanel_fenceList"), true, QStringLiteral("list first in drill-in"));
    verifyVisibility(QStringLiteral("planPanel_fenceInspector_title"), false,
                     QStringLiteral("inspector hidden in drill-in"));
    if (QTest::currentTestFailed())
        return;

    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceRow_polygon_0")), "Failed to open the polygon");
    verifyText(QStringLiteral("planPanel_fenceInspector_title"), QStringLiteral("Polygon 1"),
               QStringLiteral("polygon opened"));
    verifyVisibility(QStringLiteral("planPanel_fenceList"), false, QStringLiteral("list hidden while inspecting"));
    verifyEnabled(QStringLiteral("planPanel_fenceInspector_previous"), false,
                  QStringLiteral("no entry before the first"));
    if (QTest::currentTestFailed())
        return;

    // Stepping walks polygons, then circles, in list order
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceInspector_next")), "Failed to step to the circle");
    verifyText(QStringLiteral("planPanel_fenceInspector_title"), QStringLiteral("Circle 1"),
               QStringLiteral("stepped to circle"));
    verifyEnabled(QStringLiteral("planPanel_fenceInspector_next"), false, QStringLiteral("no entry after the last"));
    QVERIFY(fence->circles()->get(0)->property("interactive").toBool());
    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceInspector_previous")), "Failed to step back");
    verifyText(QStringLiteral("planPanel_fenceInspector_title"), QStringLiteral("Polygon 1"),
               QStringLiteral("stepped back"));
    if (QTest::currentTestFailed())
        return;

    QVERIFY2(clickButton(QStringLiteral("planPanel_fenceInspector_back")), "Failed to go back to the list");
    verifyVisibility(QStringLiteral("planPanel_fenceList"), true, QStringLiteral("list after back"));
    verifyVisibility(QStringLiteral("planPanel_fenceInspector_title"), false, QStringLiteral("inspector after back"));
}

void PlanPanelUITest::_testRallyTabListsSelectsAndDeletesPoints()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;
    _setHome();
    if (QTest::currentTestFailed())
        return;

    RallyPointController* const rally = _planMasterController()->rallyPointController();
    QVERIFY2(rally->supported(), "Offline vehicle should support rally points");

    QVERIFY2(_selectTab(QStringLiteral("rally")), "Failed to open the Rally tab");
    verifyVisibility(QStringLiteral("planPanel_rallyInspector_title"), false,
                     QStringLiteral("no inspector without points"));
    if (QTest::currentTestFailed())
        return;

    // Map clicks on the Rally tab add points and select the newest
    _clickMap(0.3, 0.4);
    if (QTest::currentTestFailed())
        return;
    _clickMap(0.3, 0.6);
    if (QTest::currentTestFailed())
        return;
    QVERIFY2(waitForCondition([&] { return rally->points()->count() == 2; }, 2000, QStringLiteral("points added")),
             "Map clicks did not add rally points");
    verifyVisibility(QStringLiteral("planPanel_rallyRow_0"), true, QStringLiteral("first row"));
    verifyVisibility(QStringLiteral("planPanel_rallyRow_1"), true, QStringLiteral("second row"));
    verifyProperty(QStringLiteral("planPanel_rallyRow_1"), "selected", true, QStringLiteral("newest point selected"));
    verifyText(QStringLiteral("planPanel_rallyInspector_title"), QStringLiteral("Rally Point 2"),
               QStringLiteral("newest title"));
    if (QTest::currentTestFailed())
        return;

    QVERIFY2(clickButton(QStringLiteral("planPanel_rallyRow_0")), "Failed to click the first row");
    QVERIFY2(waitForCondition([&] { return rally->currentRallyPoint() == rally->points()->get(0); }, 2000,
                              QStringLiteral("row selects point")),
             "Clicking a rally row did not select the point");
    verifyProperty(QStringLiteral("planPanel_rallyRow_0"), "selected", true, QStringLiteral("clicked row selected"));
    verifyProperty(QStringLiteral("planPanel_rallyRow_1"), "selected", false, QStringLiteral("other row deselected"));
    verifyText(QStringLiteral("planPanel_rallyInspector_title"), QStringLiteral("Rally Point 1"),
               QStringLiteral("clicked title"));
    if (QTest::currentTestFailed())
        return;

    QVERIFY2(clickButton(QStringLiteral("planPanel_rallyInspector_delete")), "Failed to delete the point");
    QVERIFY2(waitForCondition([&] { return rally->points()->count() == 1; }, 2000, QStringLiteral("point deleted")),
             "Delete did not remove the rally point");
    QVERIFY2(waitForCondition([&] { return !findVisibleItem(_rootItem, QStringLiteral("planPanel_rallyRow_1"), 0); },
                              2000, QStringLiteral("row removed")),
             "Deleted point still has a row");

    // The inspector closes when the last point goes
    QVERIFY2(clickButton(QStringLiteral("planPanel_rallyInspector_delete")), "Failed to delete the last point");
    QVERIFY2(
        waitForCondition([&] { return rally->points()->count() == 0; }, 2000, QStringLiteral("last point deleted")),
        "Delete did not remove the last rally point");
    verifyVisibility(QStringLiteral("planPanel_rallyInspector_title"), false, QStringLiteral("inspector closed"));
}

void PlanPanelUITest::_testRallyInspectorStepsInDrillIn()
{
    _startPlanPanel();
    if (QTest::currentTestFailed())
        return;
    _setHome();
    if (QTest::currentTestFailed())
        return;

    RallyPointController* const rally = _planMasterController()->rallyPointController();
    QVERIFY2(_selectTab(QStringLiteral("rally")), "Failed to open the Rally tab");
    for (int i = 0; i < 3; i++) {
        _clickMap(0.25 + (0.05 * i), 0.3 + (0.1 * i));
        if (QTest::currentTestFailed())
            return;
    }
    QVERIFY2(waitForCondition([&] { return rally->points()->count() == 3; }, 2000, QStringLiteral("points added")),
             "Map clicks did not add rally points");

    QQuickItem* const listInspector = findItem(_rootItem, QStringLiteral("planPanel_rallyListInspector"));
    QVERIFY(listInspector);
    listInspector->setProperty("layoutModeOverride", QStringLiteral("drillIn"));
    verifyVisibility(QStringLiteral("planPanel_rallyList"), true, QStringLiteral("list first in drill-in"));
    verifyVisibility(QStringLiteral("planPanel_rallyInspector_title"), false,
                     QStringLiteral("inspector hidden in drill-in"));
    if (QTest::currentTestFailed())
        return;

    QVERIFY2(clickButton(QStringLiteral("planPanel_rallyRow_1")), "Failed to open the middle point");
    verifyText(QStringLiteral("planPanel_rallyInspector_title"), QStringLiteral("Rally Point 2"),
               QStringLiteral("middle point opened"));
    verifyVisibility(QStringLiteral("planPanel_rallyList"), false, QStringLiteral("list hidden while inspecting"));

    QVERIFY2(clickButton(QStringLiteral("planPanel_rallyInspector_next")), "Failed to step forward");
    verifyText(QStringLiteral("planPanel_rallyInspector_title"), QStringLiteral("Rally Point 3"),
               QStringLiteral("stepped forward"));
    verifyEnabled(QStringLiteral("planPanel_rallyInspector_next"), false, QStringLiteral("no point after the last"));
    QVERIFY2(clickButton(QStringLiteral("planPanel_rallyInspector_previous")), "Failed to step back");
    QVERIFY2(clickButton(QStringLiteral("planPanel_rallyInspector_previous")), "Failed to step back to the first");
    verifyText(QStringLiteral("planPanel_rallyInspector_title"), QStringLiteral("Rally Point 1"),
               QStringLiteral("stepped to first"));
    verifyEnabled(QStringLiteral("planPanel_rallyInspector_previous"), false,
                  QStringLiteral("no point before the first"));
    QCOMPARE(rally->currentRallyPoint(), rally->points()->get(0));
    if (QTest::currentTestFailed())
        return;

    QVERIFY2(clickButton(QStringLiteral("planPanel_rallyInspector_back")), "Failed to go back");
    verifyVisibility(QStringLiteral("planPanel_rallyList"), true, QStringLiteral("list after back"));
    verifyVisibility(QStringLiteral("planPanel_rallyInspector_title"), false, QStringLiteral("inspector after back"));
    if (QTest::currentTestFailed())
        return;

    // An invalid field keeps the inspector open until it is corrected
    QVERIFY2(clickButton(QStringLiteral("planPanel_rallyRow_0")), "Failed to reopen the first point");
    verifyText(QStringLiteral("planPanel_rallyInspector_title"), QStringLiteral("Rally Point 1"),
               QStringLiteral("first point reopened"));
    verifyVisibility(QStringLiteral("rallyPointAltitudeField"), true, QStringLiteral("altitude field"));
    verifyVisibility(QStringLiteral("rallyPointAltitudeFrameCombo"), true, QStringLiteral("altitude frame selector"));
    verifyVisibility(QStringLiteral("rallyPointAltitudeHelp"), true, QStringLiteral("altitude help"));
    QQuickItem* const field = findItem(_rootItem, QStringLiteral("planPanel_rallyField_0"));
    QVERIFY(field);
    QObject* const textField = field->property("textField").value<QObject*>();
    QVERIFY(textField);
    QObject* const fact = textField->property("fact").value<QObject*>();
    QVERIFY(fact);
    const QVariant validText = fact->property("valueString");
    QVERIFY2(enterInvalidValue(textField), "Non-numeric value was accepted");

    QVERIFY2(clickButton(QStringLiteral("planPanel_rallyInspector_back")), "Failed to click Back");
    verifyVisibility(QStringLiteral("planPanel_rallyInspector_title"), true,
                     QStringLiteral("back blocked by the invalid field"));
    QVERIFY2(clickButton(QStringLiteral("planPanel_rallyInspector_next")), "Failed to click Next");
    verifyText(QStringLiteral("planPanel_rallyInspector_title"), QStringLiteral("Rally Point 1"),
               QStringLiteral("step blocked by the invalid field"));
    _clickMap(0.3, 0.4);
    if (QTest::currentTestFailed())
        return;
    QCOMPARE(rally->currentRallyPoint(), rally->points()->get(0));
    QCOMPARE(rally->points()->count(), 3);
    listInspector->setProperty("layoutModeOverride", QStringLiteral("split"));
    listInspector->setProperty("layoutModeOverride", QStringLiteral("drillIn"));
    verifyVisibility(QStringLiteral("planPanel_rallyInspector_title"), true,
                     QStringLiteral("inspector kept when entering drill-in"));
    if (QTest::currentTestFailed())
        return;

    QVERIFY(restoreValidValue(textField, validText));
    QVERIFY2(clickButton(QStringLiteral("planPanel_rallyInspector_back")), "Failed to go back after correcting");
    verifyVisibility(QStringLiteral("planPanel_rallyList"), true, QStringLiteral("list after correcting"));
}
