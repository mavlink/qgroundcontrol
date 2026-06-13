#include "QmlUITestBase.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QVariant>
#include <QtQml/QQmlApplicationEngine>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuickControls2/QQuickStyle>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AppSettings.h"
#include "ColoredSvgImageProvider.h"
#include "MAVLinkProtocol.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "QGCFileDialogController.h"
#include "QGCImageProvider.h"
#include "SettingsManager.h"
#include "Vehicle.h"

void QmlUITestBase::startUI()
{
    // Initialise subsystems needed for the full QML UI
    // setStyle() must only be called once per process; subsequent calls after
    // any QML engine has loaded produce an "must be called before loading QML"
    // warning that would trip the strict-mode log check.
    static bool s_styleSet = false;
    if (!s_styleSet) {
        QQuickStyle::setStyle("Basic");
        s_styleSet = true;
    }
    QGCCorePlugin::instance()->init();
    MAVLinkProtocol::instance()->init();
    MultiVehicleManager::instance()->init();

    // Suppress first-run prompts so they don't block the UI
    AppSettings *appSettings = SettingsManager::instance()->appSettings();
    const QList<int> promptIds = QGCCorePlugin::instance()->firstRunPromptStdIds();
    for (int id : promptIds) {
        appSettings->firstRunPromptIdsMarkIdAsShown(id);
    }

    QVERIFY2(QGCCorePlugin::instance()->showAdvancedUI(), "Test requires Advanced UI mode");

    // Ignore benign Qt platform warnings that cannot be avoided in offscreen mode
    ignoreLogMessage("default", QtWarningMsg,
                     QRegularExpression(QStringLiteral("This plugin does not support propagateSizeHints")));
    ignoreLogMessage("qt.qpa.fonts", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Populating font family aliases")));
    ignoreLogMessage("default", QtWarningMsg,
                     QRegularExpression(QStringLiteral("QRhiGles2")));
#ifdef QT_DEBUG
    // Debug builds on macOS are ad-hoc signed with an unbound Info.plist, so
    // macOS never shows the camera permission dialog and silently denies access.
    ignoreLogMessage("default", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Access to camera not granted")));
#endif

    _engine = QGCCorePlugin::instance()->createQmlApplicationEngine(this);
    QVERIFY(_engine);

    _engine->addImageProvider(QStringLiteral("QGCImages"),               new QGCImageProvider());
    _engine->addImageProvider(QLatin1String(ColoredSvgImageProvider::ProviderId), new ColoredSvgImageProvider());

    _engine->load(QUrl(QStringLiteral("qrc:/qml/QGroundControl/MainWindow.qml")));
    QVERIFY(!_engine->rootObjects().isEmpty());

    _window = qobject_cast<QQuickWindow *>(_engine->rootObjects().first());
    QVERIFY(_window);

    QVERIFY(QTest::qWaitForWindowExposed(_window));

    _viewDelay = (qApp->platformName() != QLatin1String("offscreen")) ? 700 : 0;
    _pageDelay = (qApp->platformName() != QLatin1String("offscreen")) ? 400 : 0;

    _rootItem = _window->contentItem();
}

void QmlUITestBase::ignoreAPMMockLinkWarnings()
{
    // ArduPilot MockLink does not serve COMP_METADATA_TYPE_GENERAL.
    ignoreLogMessage(
        "ComponentInformation.RequestMetaDataTypeStateMachine",
        QtWarningMsg,
        QRegularExpression(QStringLiteral("\"COMP_METADATA_TYPE_GENERAL\" : failed to load metadata \\(primary and fallback\\) \"\"")));
}

void QmlUITestBase::closeUIWindow()
{
    if (_window) {
        _window->close();
        (void) QTest::qWaitFor([this] { return !_window->isVisible(); }, 1000);
        for (int i = 0; i < 5; ++i) {
            QCoreApplication::processEvents();
            QTest::qWait(20);
        }
    }
}

void QmlUITestBase::destroyUIEngine()
{
    if (_engine) {
        // Give asynchronous QML item creation/destruction a brief drain window
        // before engine teardown to avoid strict-mode warnings at shutdown.
        for (int i = 0; i < 10; ++i) {
            _engine->collectGarbage();
            QCoreApplication::processEvents();
            QTest::qWait(10);
        }

        // Force GC and event processing so QML releases references to C++ singletons
        // (e.g. SettingsFacts) before the engine is destroyed.  Without this,
        // QQmlData attached to those objects fires stale binding updates during
        // Q_APPLICATION_STATIC teardown and crashes.
        _engine->collectGarbage();
        QCoreApplication::processEvents();
        _engine->clearComponentCache();
        QCoreApplication::processEvents();
    }
    delete _engine;
    _engine   = nullptr;
    _window   = nullptr;
    _rootItem = nullptr;
    QCoreApplication::processEvents();
}

void QmlUITestBase::stopUI()
{
    closeUIWindow();
    destroyUIEngine();
}

void QmlUITestBase::_verifyFileDialogTestHookConsumed()
{
    if (QGCFileDialogController::testHookArmed()) {
        QGCFileDialogController::takeTestNextFile();  // clear so later tests aren't contaminated
        QTest::qFail("file dialog test hook was armed but never consumed by a dialog", __FILE__, __LINE__);
    }
}

bool QmlUITestBase::clickButton(const QString &objectName)
{
    QQuickItem *btn = findVisibleItem(_rootItem, objectName);
    if (!btn) {
        return false;
    }
    const QPointF center = btn->mapToScene(QPointF(btn->width() / 2, btn->height() / 2));
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
    return true;
}

// Format a property value for failure messages: quote strings, otherwise use
// QVariant's string form ("true"/"false" for bools).
static QString _displayValue(const QVariant &value)
{
    if (value.typeId() == QMetaType::QString) {
        return QStringLiteral("'%1'").arg(value.toString());
    }
    return value.toString();
}

bool QmlUITestBase::_verifyItemProperty(const QString &objectName, const char *propertyName,
                                        const QVariant &expectedValue, const QString &context)
{
    // Item discovery is as asynchronous as state propagation (view transitions,
    // Loaders); poll for the item with the same ceiling as the state check.
    // findVisibleItem returns immediately once the item exists.
    QQuickItem *item = findVisibleItem(_rootItem, objectName, 2000);
    if (!item) {
        QTest::qFail(qPrintable(QStringLiteral("%1: item not found: %2").arg(context, objectName)),
                     __FILE__, __LINE__);
        return false;
    }

    // An invalid QVariant silently converts to false/"", which would make a
    // false/empty expected value pass vacuously on an item without the property.
    if (!item->property(propertyName).isValid()) {
        QTest::qFail(qPrintable(QStringLiteral("%1: %2 has no '%3' property")
                                    .arg(context, objectName, QLatin1String(propertyName))),
                     __FILE__, __LINE__);
        return false;
    }

    // State changes propagate through bindings; allow them to settle
    const bool matched = waitForCondition(
        [item, propertyName, expectedValue] { return item->property(propertyName) == expectedValue; },
        2000,
        QStringLiteral("%1 %2 == %3").arg(objectName, QLatin1String(propertyName), _displayValue(expectedValue)));
    if (!matched) {
        QTest::qFail(qPrintable(QStringLiteral("%1: %2 expected %3=%4 but was %5")
                                    .arg(context, objectName, QLatin1String(propertyName),
                                         _displayValue(expectedValue),
                                         _displayValue(item->property(propertyName)))),
                     __FILE__, __LINE__);
        return false;
    }
    return true;
}

bool QmlUITestBase::verifyEnabled(const QString &objectName, bool expectedEnabled, const QString &context)
{
    return _verifyItemProperty(objectName, "enabled", expectedEnabled, context);
}

bool QmlUITestBase::verifyPrimary(const QString &objectName, bool expectedPrimary, const QString &context)
{
    return _verifyItemProperty(objectName, "primary", expectedPrimary, context);
}

bool QmlUITestBase::verifyChecked(const QString &objectName, bool expectedChecked, const QString &context)
{
    return _verifyItemProperty(objectName, "checked", expectedChecked, context);
}

bool QmlUITestBase::verifyText(const QString &objectName, const QString &expectedText, const QString &context)
{
    return _verifyItemProperty(objectName, "text", expectedText, context);
}

void QmlUITestBase::scrollIntoView(QQuickItem *item, const QString &flickableObjectName)
{
    if (!item || !_rootItem || !_window) {
        return;
    }

    QQuickItem *flickable = findVisibleItem(_rootItem, flickableObjectName);
    if (!flickable) {
        return;
    }

    // Check whether the item's centre is already within the flickable's visible viewport.
    const QPointF sceneCenter        = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
    const QPointF flickableScenePos  = flickable->mapToScene(QPointF(0, 0));
    const QRectF  flickableViewport(flickableScenePos, QSizeF(flickable->width(), flickable->height()));
    if (flickableViewport.contains(sceneCenter)) {
        return; // already within the flickable's clip region
    }

    // mapToItem gives a viewport-relative position (already offset by contentY).
    // Add current contentY to get the absolute content-space position.
    const QPointF itemInFlickable = item->mapToItem(flickable, QPointF(0, 0));
    const double currentContentY = flickable->property("contentY").toDouble();
    const double absoluteY = itemInFlickable.y() + currentContentY;
    // Target contentY that places the item's centre in the middle of the flickable.
    const double targetY = absoluteY + item->height() / 2.0 - flickable->height() / 2.0;
    const double maxContentY = flickable->property("contentHeight").toDouble() - flickable->height();
    flickable->setProperty("contentY", qBound(0.0, targetY, qMax(0.0, maxContentY)));
    QTest::qWait(50);
}

void QmlUITestBase::runWithMockLink(
    const std::function<MockLink *()> &factory,
    const std::function<void(QPointer<MockLink>, Vehicle *)> &body)
{
    startUI();
    if (QTest::currentTestFailed()) return;

    Vehicle *vehicle = nullptr;
    QPointer<MockLink> mockLink = connectMockLinkAndWaitReady(factory, vehicle);
    if (!mockLink) return;

    const auto cleanup = qScopeGuard([&] {
        disconnectMockLink(mockLink);
        closeUIWindow();
        destroyUIEngine();
    });

    body(mockLink, vehicle);
}

void QmlUITestBase::disconnectMockLink(QPointer<MockLink> mockLink)
{
    if (!mockLink) return;

    QSignalSpy spyDisconnect(MultiVehicleManager::instance(),
                             &MultiVehicleManager::activeVehicleChanged);
    mockLink->disconnect();
    if (spyDisconnect.isValid()) {
        (void)waitForSignal(spyDisconnect, 5000, QStringLiteral("activeVehicleChanged"));
    }
}

QPointer<MockLink> QmlUITestBase::connectMockLinkAndWaitReady(
    const std::function<MockLink *()> &factory,
    Vehicle *&vehicleOut)
{
    vehicleOut = nullptr;

    QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    if (!spyVehicle.isValid()) {
        QTest::qFail("Failed to create spy for activeVehicleChanged", __FILE__, __LINE__);
        return {};
    }

    QPointer<MockLink> mockLink = factory();
    if (!mockLink) {
        QTest::qFail("Failed to start MockLink", __FILE__, __LINE__);
        return {};
    }

    // Helper: disconnect the MockLink and return {} so callers never receive a
    // live link they cannot clean up (the caller's qScopeGuard is not yet active).
    const auto failAndDisconnect = [&](const char *msg) -> QPointer<MockLink> {
        QTest::qFail(msg, __FILE__, __LINE__);
        mockLink->disconnect();
        return {};
    };

    if (!waitForSignal(spyVehicle, 10000, QStringLiteral("activeVehicleChanged"))) {
        return failAndDisconnect("Timeout waiting for vehicle connection");
    }

    Vehicle *vehicle = MultiVehicleManager::instance()->activeVehicle();
    if (!vehicle) {
        return failAndDisconnect("No active vehicle after MockLink connection");
    }

    QSignalSpy spyConnect(vehicle, &Vehicle::initialConnectComplete);
    if (!spyConnect.isValid()) {
        return failAndDisconnect("Failed to create spy for initialConnectComplete");
    }
    if (!vehicle->isInitialConnectComplete()) {
        if (!waitForSignal(spyConnect, 10000, QStringLiteral("initialConnectComplete"))) {
            return failAndDisconnect("Timeout waiting for initial connect");
        }
    }

    QSignalSpy spyParamsReady(MultiVehicleManager::instance(),
                              &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    if (!spyParamsReady.isValid()) {
        return failAndDisconnect("Failed to create spy for parameterReadyVehicleAvailableChanged");
    }
    if (!MultiVehicleManager::instance()->parameterReadyVehicleAvailable()) {
        if (!waitForSignal(spyParamsReady, 15000,
                           QStringLiteral("parameterReadyVehicleAvailableChanged"))) {
            return failAndDisconnect("Timeout waiting for parameters to be ready");
        }
    }
    if (!MultiVehicleManager::instance()->parameterReadyVehicleAvailable()) {
        return failAndDisconnect("Parameters should be ready after signal");
    }

    vehicleOut = vehicle;
    return mockLink;
}
