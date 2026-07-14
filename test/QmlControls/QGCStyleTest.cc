#include "QGCStyleTest.h"

#include <QtCore/QScopeGuard>
#include <QtCore/QScopedPointer>
#include <QtCore/QSet>
#include <QtCore/QtMath>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QFontDatabase>
#include <QtGui/QPointingDevice>
#include <QtGui/QWindow>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <array>

#include "AppSettings.h"
#include "Fact.h"
#include "SettingsManager.h"

void QGCStyleTest::init()
{
    _originalTheme = QGCPalette::globalTheme();
}

void QGCStyleTest::cleanup()
{
    QGCPalette::setGlobalTheme(_originalTheme);
}

void QGCStyleTest::_galleryLoads_data()
{
    QTest::addColumn<int>("theme");

    QTest::newRow("light") << static_cast<int>(QGCPalette::Light);
    QTest::newRow("dark") << static_cast<int>(QGCPalette::Dark);
}

void QGCStyleTest::_galleryLoads()
{
    QFETCH(int, theme);

    QGCPalette::setGlobalTheme(static_cast<QGCPalette::Theme>(theme));

    static constexpr char qml[] = R"(
import QGCStyle.Testing as QGCStyleTesting

QGCStyleTesting.QGCStyleGallery {
}
)";

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(qml, QUrl(QStringLiteral("qrc:/tests/QGCStyleGalleryLoadTest.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> gallery(component.create());
    QVERIFY2(gallery, qPrintable(component.errorString()));
}

void QGCStyleTest::_galleryRenders_data()
{
    QTest::addColumn<int>("theme");
    QTest::addColumn<double>("scaleFactor");
    QTest::addColumn<bool>("touchMode");
    QTest::addColumn<bool>("mirrored");
    QTest::addColumn<QSize>("viewportSize");

    static constexpr std::array scaleFactors = {0.5, 1., 1.25, 1.5, 1.75, 2.};
    for (const auto theme : {QGCPalette::Light, QGCPalette::Dark}) {
        for (const double scaleFactor : scaleFactors) {
            const QString name = QStringLiteral("desktop-%1-%2-percent")
                                     .arg(theme == QGCPalette::Light ? QStringLiteral("light") : QStringLiteral("dark"))
                                     .arg(qRound(scaleFactor * 100));
            QTest::newRow(qPrintable(name))
                << static_cast<int>(theme) << scaleFactor << false << false << QSize(1200, 900);
        }

        for (const bool touchMode : {false, true}) {
            for (const double scaleFactor : {1., 2.}) {
                const QString name =
                    QStringLiteral("narrow-%1-%2-%3-percent")
                        .arg(theme == QGCPalette::Light ? QStringLiteral("light") : QStringLiteral("dark"))
                        .arg(touchMode ? QStringLiteral("touch") : QStringLiteral("pointer"))
                        .arg(qRound(scaleFactor * 100));
                QTest::newRow(qPrintable(name))
                    << static_cast<int>(theme) << scaleFactor << touchMode << false << QSize(480, 900);
            }
        }

        const QString themeName = theme == QGCPalette::Light ? QStringLiteral("light") : QStringLiteral("dark");
        QTest::newRow(qPrintable(QStringLiteral("rtl-wide-%1-125-percent").arg(themeName)))
            << static_cast<int>(theme) << 1.25 << false << true << QSize(1200, 900);
        QTest::newRow(qPrintable(QStringLiteral("rtl-narrow-%1-touch-175-percent").arg(themeName)))
            << static_cast<int>(theme) << 1.75 << true << true << QSize(480, 900);
    }
}

void QGCStyleTest::_galleryRenders()
{
    QFETCH(int, theme);
    QFETCH(double, scaleFactor);
    QFETCH(bool, touchMode);
    QFETCH(bool, mirrored);
    QFETCH(QSize, viewportSize);

    QGCPalette::setGlobalTheme(static_cast<QGCPalette::Theme>(theme));

    static constexpr char qml[] = R"(
import QtQuick
import Qt.labs.StyleKit as Labs
import QGCStyle as QGCStyle
import QGCStyle.Testing as QGCStyleTesting

Item {
    id: root

    required property real requestedScaleFactor
    required property bool requestedMirrored
    required property bool requestedTouchMode
    property real originalScaleFactor: 1
    property int originalTouchModeOverride: QGCStyle.StylePreferences.Automatic

    readonly property int contentMargin: QGCStyle.StyleMetrics.contentMargin
    readonly property int controlHeight: QGCStyle.StyleMetrics.controlHeight
    readonly property int controlWidth: QGCStyle.StyleMetrics.controlWidth
    readonly property int galleryScalePercent: gallery.uiScalePercent
    readonly property bool galleryMirroredLayout: gallery.mirroredLayout
    readonly property bool galleryNarrowLayout: gallery.narrowLayout
    readonly property int indicatorSize: QGCStyle.StyleMetrics.indicatorSize
    readonly property int nominalTouchTarget: QGCStyle.StyleMetrics.nominalTouchTarget
    readonly property int toolbarIconSize: QGCStyle.StyleMetrics.toolbarIconSize
    readonly property bool touchMode: QGCStyle.StylePreferences.touchMode

    Labs.StyleKit.style: qgcStyle
    LayoutMirroring.childrenInherit: true
    LayoutMirroring.enabled: requestedMirrored

    function isStyleLoaded(): bool {
        return Labs.StyleKit.styleLoaded()
    }

    Component.onCompleted: {
        originalScaleFactor = QGCStyle.StyleTypography.scaleFactor
        originalTouchModeOverride = QGCStyle.StylePreferences.touchModeOverride
        QGCStyle.StylePreferences.setTouchModePreference(requestedTouchMode ? QGCStyle.StylePreferences.Touch
                                                                           : QGCStyle.StylePreferences.Pointer)
        QGCStyle.StyleTypography.scaleFactor = requestedScaleFactor
    }
    Component.onDestruction: {
        QGCStyle.StylePreferences.setTouchModePreference(originalTouchModeOverride)
        QGCStyle.StyleTypography.scaleFactor = originalScaleFactor
    }

    QGCStyleTesting.QGCStyleGallery {
        id: gallery

        anchors.fill: parent
    }

    QGCStyle.QGCStyleKit {
        id: qgcStyle
    }
}
)";

    QQuickView view;
    view.engine()->addImportPath(QStringLiteral("qrc:/qml"));
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(viewportSize);

    const QUrl testUrl(QStringLiteral("qrc:/tests/QGCStyleGalleryScaleTest.qml"));
    QQmlComponent component(view.engine());
    component.setData(qml, testUrl);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QObject* const root = component.createWithInitialProperties({
        {QStringLiteral("requestedScaleFactor"), scaleFactor},
        {QStringLiteral("requestedMirrored"), mirrored},
        {QStringLiteral("requestedTouchMode"), touchMode},
    });
    QVERIFY2(root, qPrintable(component.errorString()));
    view.setContent(testUrl, &component, root);

    QStringList errors;
    for (const QQmlError& error : view.errors()) {
        errors.append(error.toString());
    }
    QVERIFY2(view.status() == QQuickView::Ready, qPrintable(errors.join(QLatin1Char('\n'))));

    view.show();
    QTRY_VERIFY_WITH_TIMEOUT(view.isExposed(), 5000);
    const auto styleLoaded = [root]() {
        bool result = false;
        return QMetaObject::invokeMethod(root, "isStyleLoaded", Q_RETURN_ARG(bool, result)) && result;
    };
    QTRY_VERIFY_WITH_TIMEOUT(styleLoaded(), 5000);
    const int densityControlHeight = touchMode ? 48 : 40;
    const int densityControlWidth = touchMode ? 120 : 100;
    const int densityContentMargin = touchMode ? 16 : 12;
    const int densityIndicatorSize = touchMode ? 32 : 28;
    QTRY_COMPARE_WITH_TIMEOUT(root->property("touchMode").toBool(), touchMode, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(root->property("controlHeight").toInt(), qRound(densityControlHeight * scaleFactor),
                              5000);
    QCOMPARE(root->property("controlWidth").toInt(), qRound(densityControlWidth * scaleFactor));
    QCOMPARE(root->property("contentMargin").toInt(), qRound(densityContentMargin * scaleFactor));
    QCOMPARE(root->property("galleryScalePercent").toInt(), qRound(scaleFactor * 100));
    QCOMPARE(root->property("galleryMirroredLayout").toBool(), mirrored);
    QCOMPARE(root->property("galleryNarrowLayout").toBool(),
             viewportSize.width() < qRound(densityControlWidth * scaleFactor) * 6);
    QCOMPARE(root->property("indicatorSize").toInt(), qRound(densityIndicatorSize * scaleFactor));
    QCOMPARE(root->property("nominalTouchTarget").toInt(), qRound(48 * scaleFactor));
    QCOMPARE(root->property("toolbarIconSize").toInt(), qRound(24 * scaleFactor));

    QQuickItem* const contentLayout = root->findChild<QQuickItem*>(QStringLiteral("contentLayout"));
    QQuickItem* const preferenceLayout = root->findChild<QQuickItem*>(QStringLiteral("preferenceLayout"));
    QQuickItem* const controlLayout = root->findChild<QQuickItem*>(QStringLiteral("controlLayout"));
    QQuickItem* const densityComboBox = root->findChild<QQuickItem*>(QStringLiteral("densityComboBox"));
    QQuickItem* const reducedMotionSwitch = root->findChild<QQuickItem*>(QStringLiteral("reducedMotionSwitch"));
    QVERIFY(contentLayout);
    QVERIFY(preferenceLayout);
    QVERIFY(controlLayout);
    QVERIFY(densityComboBox);
    QVERIFY(reducedMotionSwitch);
    QCOMPARE(preferenceLayout->property("columns").toInt(), root->property("galleryNarrowLayout").toBool() ? 1 : 4);
    QCOMPARE(controlLayout->property("columns").toInt(), root->property("galleryNarrowLayout").toBool() ? 2 : 4);
    if (!root->property("galleryNarrowLayout").toBool()) {
        QCOMPARE(densityComboBox->x() > reducedMotionSwitch->x(), mirrored);
    }

    const auto childrenFitWidth = [](const QQuickItem* layout) {
        constexpr qreal tolerance = 0.5;
        for (const QQuickItem* const child : layout->childItems()) {
            if (!child->isVisible()) {
                continue;
            }
            const QRectF bounds = child->mapRectToItem(layout, child->boundingRect());
            if (bounds.left() < -tolerance || bounds.right() > layout->width() + tolerance) {
                return false;
            }
        }
        return true;
    };
    QTRY_VERIFY_WITH_TIMEOUT(childrenFitWidth(contentLayout), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(childrenFitWidth(preferenceLayout), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(childrenFitWidth(controlLayout), 5000);

    const QImage image = view.grabWindow().convertToFormat(QImage::Format_ARGB32);
    QVERIFY(!image.isNull());
    QCOMPARE(image.size(), view.size());

    QSet<QRgb> sampledColors;
    for (int y = 0; y < image.height(); y += 16) {
        for (int x = 0; x < image.width(); x += 16) {
            sampledColors.insert(image.pixel(x, y));
        }
    }
    QVERIFY2(sampledColors.size() >= 8, "The style gallery rendered as an unexpectedly uniform frame");
}

void QGCStyleTest::_previewRenders_data()
{
    QTest::addColumn<double>("scaleFactor");
    QTest::addColumn<QSize>("viewportSize");
    QTest::addColumn<bool>("narrowLayout");

    QTest::newRow("wide") << 1. << QSize(1200, 500) << false;
    QTest::newRow("narrow-scaled") << 2. << QSize(480, 900) << true;
}

void QGCStyleTest::_previewRenders()
{
    QFETCH(double, scaleFactor);
    QFETCH(QSize, viewportSize);
    QFETCH(bool, narrowLayout);

    static constexpr char qml[] = R"(
import QtQuick
import Qt.labs.StyleKit as Labs
import QGCStyle as QGCStyle
import QGCStyle.Testing as QGCStyleTesting

Item {
    id: root

    required property real requestedScaleFactor
    property real originalScaleFactor: 1
    readonly property bool narrowLayout: preview.narrowLayout

    Labs.StyleKit.style: qgcStyle

    Component.onCompleted: {
        originalScaleFactor = QGCStyle.StyleTypography.scaleFactor
        QGCStyle.StyleTypography.scaleFactor = requestedScaleFactor
    }
    Component.onDestruction: QGCStyle.StyleTypography.scaleFactor = originalScaleFactor

    QGCStyleTesting.QGCStyleKitPreview {
        id: preview

        anchors.fill: parent
    }

    QGCStyle.QGCStyleKit {
        id: qgcStyle
    }
}
)";

    QQuickView view;
    view.engine()->addImportPath(QStringLiteral("qrc:/qml"));
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(viewportSize);

    const QUrl testUrl(QStringLiteral("qrc:/tests/QGCStylePreviewScaleTest.qml"));
    QQmlComponent component(view.engine());
    component.setData(qml, testUrl);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QObject* const root = component.createWithInitialProperties({{QStringLiteral("requestedScaleFactor"), scaleFactor}});
    QVERIFY2(root, qPrintable(component.errorString()));
    view.setContent(testUrl, &component, root);
    view.show();
    QTRY_VERIFY_WITH_TIMEOUT(view.isExposed(), 5000);
    QTRY_COMPARE_WITH_TIMEOUT(root->property("narrowLayout").toBool(), narrowLayout, 5000);

    QObject* const controlLayout = root->findChild<QObject*>(QStringLiteral("previewControlLayout"));
    QObject* const progressLayout = root->findChild<QObject*>(QStringLiteral("previewProgressLayout"));
    QObject* const variationLayout = root->findChild<QObject*>(QStringLiteral("previewVariationLayout"));
    QVERIFY(controlLayout);
    QVERIFY(progressLayout);
    QVERIFY(variationLayout);
    QCOMPARE(controlLayout->property("columns").toInt(), narrowLayout ? 1 : 5);
    QCOMPARE(progressLayout->property("columns").toInt(), narrowLayout ? 1 : 2);
    QCOMPARE(variationLayout->property("columns").toInt(), narrowLayout ? 1 : 3);
}

void QGCStyleTest::_layoutProfile()
{
    static constexpr char qml[] = R"(
import QtQml
import QGCStyle as QGCStyle

QtObject {
    id: root

    property real availableHeight: 500
    property real availableWidth: 500
    property bool mobileLayout: false
    property real pixelDensity: 2

    readonly property real fallbackTouchTarget: QGCStyle.StyleMetrics.fallbackTouchTargetHeight
    readonly property bool isShort: layoutProfile.isShort
    readonly property bool isTiny: layoutProfile.isTiny
    readonly property real minimumTouchTarget: layoutProfile.minimumTouchTarget

    property QGCStyle.LayoutProfile layoutProfile: QGCStyle.LayoutProfile {
        availableHeight: root.availableHeight
        availableWidth: root.availableWidth
        mobileLayout: root.mobileLayout
        pixelDensity: root.pixelDensity
    }
}
)";

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(qml, QUrl(QStringLiteral("qrc:/tests/QGCStyleLayoutProfileTest.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    QVERIFY(!root->property("isTiny").toBool());
    QVERIFY(!root->property("isShort").toBool());
    QCOMPARE(root->property("minimumTouchTarget").toReal(), 10.);

    QVERIFY(root->setProperty("availableWidth", 200.));
    QTRY_VERIFY_WITH_TIMEOUT(root->property("isTiny").toBool(), 5000);

    QVERIFY(root->setProperty("availableWidth", 500.));
    QVERIFY(root->setProperty("availableHeight", 250.));
    QVERIFY(root->setProperty("pixelDensity", 1.));
    QVERIFY(root->setProperty("mobileLayout", true));
    QTRY_VERIFY_WITH_TIMEOUT(root->property("isShort").toBool(), 5000);

    QVERIFY(root->setProperty("availableHeight", 50.));
    QVERIFY(root->setProperty("pixelDensity", 2.));
    QTRY_COMPARE_WITH_TIMEOUT(root->property("minimumTouchTarget"), root->property("fallbackTouchTarget"), 5000);
}

void QGCStyleTest::_liveUiScaleReflow()
{
    Fact* const uiScalePercentFact = SettingsManager::instance()->appSettings()->uiScalePercent();
    QVERIFY(uiScalePercentFact);
    const QVariant originalUiScalePercent = uiScalePercentFact->rawValue();
    const auto restoreUiScalePercent = qScopeGuard(
        [uiScalePercentFact, originalUiScalePercent]() { uiScalePercentFact->setRawValue(originalUiScalePercent); });
    uiScalePercentFact->setRawValue(25);

    static constexpr char qml[] = R"(
import QtQuick
import QGCStyle as QGCStyle
import QGroundControl
import QGroundControl.Controls

Item {
    readonly property var styleEnvironment: QGCStyleEnvironment

    readonly property real bodyPointSize: QGCStyle.StyleTypography.bodyPointSize
    readonly property real buttonImplicitWidth: button.background.implicitWidth
    readonly property int controlHeight: QGCStyle.StyleMetrics.controlHeight
    readonly property real styleScaleFactor: QGCStyle.StyleTypography.scaleFactor
    readonly property int uiScalePercent: QGroundControl.settingsManager.appSettings.uiScalePercent.value

    function setUiScalePercent(percent: int) {
        QGroundControl.settingsManager.appSettings.uiScalePercent.value = percent
    }

    QGCButton {
        id: button

        text: qsTr("Scale test")
    }
}
)";

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(qml, QUrl(QStringLiteral("qrc:/tests/QGCLiveUiScaleTest.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    QTRY_COMPARE_WITH_TIMEOUT(root->property("uiScalePercent").toInt(), 100, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(root->property("styleScaleFactor").toReal(), 1., 5000);
    QTRY_COMPARE_WITH_TIMEOUT(root->property("controlHeight").toInt(), 40, 5000);
    const double baseBodyPointSize = root->property("bodyPointSize").toReal();
    const double baseButtonImplicitWidth = root->property("buttonImplicitWidth").toReal();

    QVERIFY(QMetaObject::invokeMethod(root.data(), "setUiScalePercent", Q_ARG(int, 200)));
    QTRY_COMPARE_WITH_TIMEOUT(root->property("styleScaleFactor").toReal(), 2., 5000);
    QTRY_COMPARE_WITH_TIMEOUT(root->property("bodyPointSize").toReal(), baseBodyPointSize * 2., 5000);
    QTRY_COMPARE_WITH_TIMEOUT(root->property("controlHeight").toInt(), 80, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(root->property("buttonImplicitWidth").toReal() > baseButtonImplicitWidth, 5000);
}

void QGCStyleTest::_stylePreferencesTouchMode()
{
    static constexpr char qml[] = R"(
import QtQuick
import QGCStyle as QGCStyle
import QGroundControl

QtObject {
    readonly property int automaticTouchMode: QGCStyle.StylePreferences.Automatic
    readonly property bool forceHighContrast: QGCStyle.StylePreferences.forceHighContrast
    readonly property bool inputTouchDeviceAvailable: QGCStyle.InputCapabilities.touchDeviceAvailable
    readonly property bool platformTouchMode: QGCStyle.StylePreferences.platformTouchMode
    readonly property int pointerTouchMode: QGCStyle.StylePreferences.Pointer
    readonly property bool reducedMotion: QGCStyle.StylePreferences.reducedMotion
    readonly property bool settingsForceHighContrast: QGroundControl.settingsManager.appSettings.forceHighContrast.rawValue
    readonly property bool settingsReducedMotion: QGroundControl.settingsManager.appSettings.reducedMotion.rawValue
    readonly property int settingsTouchModeOverride: QGroundControl.settingsManager.appSettings.touchModeOverride.rawValue
    readonly property int touchDensityMode: QGCStyle.StylePreferences.Touch
    readonly property bool touchMode: QGCStyle.StylePreferences.touchMode
    readonly property int touchModeOverride: QGCStyle.StylePreferences.touchModeOverride

    function setForceHighContrast(enabled: bool) {
        QGCStyle.StylePreferences.setForceHighContrast(enabled)
    }

    function setReducedMotion(enabled: bool) {
        QGCStyle.StylePreferences.setReducedMotion(enabled)
    }

    function setTouchInputObserved(observed: bool) {
        QGCStyle.StylePreferences.touchInputObserved = observed
    }

    function setTouchMode(enabled: bool) {
        QGCStyle.StylePreferences.setTouchMode(enabled)
    }

    function setTouchModePreference(preference: int) {
        QGCStyle.StylePreferences.setTouchModePreference(preference)
    }

    function useAutomaticTouchMode() {
        QGCStyle.StylePreferences.useAutomaticTouchMode()
    }
}
)";

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(qml, QUrl(QStringLiteral("qrc:/tests/QGCStylePreferencesTest.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    const bool originalForceHighContrast = root->property("forceHighContrast").toBool();
    const bool originalReducedMotion = root->property("reducedMotion").toBool();
    const int originalTouchModeOverride = root->property("touchModeOverride").toInt();
    const int automaticTouchMode = root->property("automaticTouchMode").toInt();
    const int pointerTouchMode = root->property("pointerTouchMode").toInt();
    const int touchDensityMode = root->property("touchDensityMode").toInt();
    const auto restoreStylePreferences = qScopeGuard([&root, originalForceHighContrast, originalReducedMotion,
                                                      originalTouchModeOverride]() {
        if (root) {
            (void) QMetaObject::invokeMethod(root.data(), "setForceHighContrast",
                                             Q_ARG(bool, originalForceHighContrast));
            (void) QMetaObject::invokeMethod(root.data(), "setReducedMotion", Q_ARG(bool, originalReducedMotion));
            (void) QMetaObject::invokeMethod(root.data(), "setTouchModePreference",
                                             Q_ARG(int, originalTouchModeOverride));
        }
    });

    QCOMPARE(root->property("settingsForceHighContrast").toBool(), originalForceHighContrast);
    QCOMPARE(root->property("settingsReducedMotion").toBool(), originalReducedMotion);
    QCOMPARE(root->property("settingsTouchModeOverride").toInt(), originalTouchModeOverride);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "setForceHighContrast", Q_ARG(bool, !originalForceHighContrast)));
    QTRY_COMPARE_WITH_TIMEOUT(root->property("forceHighContrast").toBool(), !originalForceHighContrast, 5000);
    QCOMPARE(root->property("settingsForceHighContrast").toBool(), !originalForceHighContrast);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "setReducedMotion", Q_ARG(bool, !originalReducedMotion)));
    QTRY_COMPARE_WITH_TIMEOUT(root->property("reducedMotion").toBool(), !originalReducedMotion, 5000);
    QCOMPARE(root->property("settingsReducedMotion").toBool(), !originalReducedMotion);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "useAutomaticTouchMode"));
    QCOMPARE(root->property("touchModeOverride").toInt(), automaticTouchMode);
    QCOMPARE(root->property("settingsTouchModeOverride").toInt(), automaticTouchMode);
    QVERIFY(QMetaObject::invokeMethod(root.data(), "setTouchInputObserved", Q_ARG(bool, true)));
    QTRY_VERIFY_WITH_TIMEOUT(root->property("touchMode").toBool(), 5000);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "setTouchInputObserved", Q_ARG(bool, false)));
    QTRY_COMPARE_WITH_TIMEOUT(
        root->property("touchMode").toBool(),
        root->property("platformTouchMode").toBool() || root->property("inputTouchDeviceAvailable").toBool(), 5000);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "setTouchInputObserved", Q_ARG(bool, true)));
    QVERIFY(QMetaObject::invokeMethod(root.data(), "setTouchMode", Q_ARG(bool, false)));
    QCOMPARE(root->property("touchModeOverride").toInt(), pointerTouchMode);
    QTRY_VERIFY_WITH_TIMEOUT(!root->property("touchMode").toBool(), 5000);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "setTouchMode", Q_ARG(bool, true)));
    QCOMPARE(root->property("touchModeOverride").toInt(), touchDensityMode);
    QTRY_VERIFY_WITH_TIMEOUT(root->property("touchMode").toBool(), 5000);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "setTouchModePreference", Q_ARG(int, 42)));
    QCOMPARE(root->property("touchModeOverride").toInt(), automaticTouchMode);
    QCOMPARE(root->property("settingsTouchModeOverride").toInt(), automaticTouchMode);
}

void QGCStyleTest::_styleTypographyMetrics()
{
    static constexpr char qml[] = R"(
import QtQuick
import QGCStyle as QGCStyle

QtObject {
    readonly property FontInfo _bodyFontInfo: FontInfo {
        font: QGCStyle.StyleTypography.bodyFont
    }
    readonly property FontMetrics _captionFontMetrics: FontMetrics {
        font: QGCStyle.StyleTypography.captionFont
    }
    readonly property TextMetrics _captionTextMetrics: TextMetrics {
        font: QGCStyle.StyleTypography.captionFont
        text: "X"
    }
    readonly property FontInfo _fixedFontInfo: FontInfo {
        font: QGCStyle.StyleTypography.fixedFont
    }
    readonly property FontInfo _monospaceFontInfo: FontInfo {
        font.family: "monospace"
    }
    readonly property FontInfo _systemFixedFontInfo: FontInfo {
        font: QGCStyle.SystemFonts.fixedFont
    }
    readonly property FontMetrics _headingFontMetrics: FontMetrics {
        font: QGCStyle.StyleTypography.headingFont
    }
    readonly property TextMetrics _headingTextMetrics: TextMetrics {
        font: QGCStyle.StyleTypography.headingFont
        text: "X"
    }
    readonly property TextMetrics _numericNarrowMetrics: TextMetrics {
        font: QGCStyle.StyleTypography.numericFont
        text: "1111"
    }
    readonly property TextMetrics _numericWideMetrics: TextMetrics {
        font: QGCStyle.StyleTypography.numericFont
        text: "8888"
    }
    readonly property FontMetrics _titleFontMetrics: FontMetrics {
        font: QGCStyle.StyleTypography.titleFont
    }
    readonly property TextMetrics _titleTextMetrics: TextMetrics {
        font: QGCStyle.StyleTypography.titleFont
        text: "X"
    }
    readonly property real bodyAscent: QGCStyle.StyleTypography.bodyAscent
    readonly property real bodyAverageCharacterWidth: QGCStyle.StyleTypography.bodyAverageCharacterWidth
    readonly property real bodyBaselineOffset: QGCStyle.StyleTypography.bodyBaselineOffset
    readonly property real bodyCapitalHeight: QGCStyle.StyleTypography.bodyCapitalHeight
    readonly property real bodyDescent: QGCStyle.StyleTypography.bodyDescent
    readonly property string bodyFontFamily: QGCStyle.StyleTypography.bodyFont.family
    readonly property real bodyFontHeight: QGCStyle.StyleTypography.bodyFontHeight
    readonly property real bodyFontPointSize: QGCStyle.StyleTypography.bodyFont.pointSize
    readonly property real bodyLineSpacing: QGCStyle.StyleTypography.bodyLineSpacing
    readonly property real bodyPixelHeight: QGCStyle.StyleTypography.bodyPixelHeight
    readonly property real bodyPixelWidth: QGCStyle.StyleTypography.bodyPixelWidth
    readonly property real bodyPointSize: QGCStyle.StyleTypography.bodyPointSize
    readonly property real captionAscent: QGCStyle.StyleTypography.captionAscent
    readonly property real captionBaselineOffset: QGCStyle.StyleTypography.captionBaselineOffset
    readonly property real captionDescent: QGCStyle.StyleTypography.captionDescent
    readonly property real captionFontHeight: QGCStyle.StyleTypography.captionFontHeight
    readonly property real captionLineSpacing: QGCStyle.StyleTypography.captionLineSpacing
    readonly property real captionPixelHeight: QGCStyle.StyleTypography.captionPixelHeight
    readonly property real captionPixelWidth: QGCStyle.StyleTypography.captionPixelWidth
    readonly property real captionPointSize: QGCStyle.StyleTypography.captionPointSize
    readonly property int controlContentHeight: QGCStyle.StyleMetrics.controlContentHeight
    readonly property int controlHeight: QGCStyle.StyleMetrics.controlHeight
    readonly property int controlVerticalPadding: QGCStyle.StyleMetrics.controlVerticalPadding
    readonly property real defaultDialogControlSpacing: QGCStyle.StyleMetrics.defaultDialogControlSpacing
    readonly property string fixedFontFamily: QGCStyle.StyleTypography.fixedFontFamily
    readonly property bool fixedFontIsFixedPitch: _fixedFontInfo.fixedPitch
    readonly property string monospaceFontFamily: _monospaceFontInfo.family
    readonly property bool monospaceFontIsFixedPitch: _monospaceFontInfo.fixedPitch
    readonly property real headingAscent: QGCStyle.StyleTypography.headingAscent
    readonly property real headingBaselineOffset: QGCStyle.StyleTypography.headingBaselineOffset
    readonly property real headingDescent: QGCStyle.StyleTypography.headingDescent
    readonly property real headingFontHeight: QGCStyle.StyleTypography.headingFontHeight
    readonly property real headingLineSpacing: QGCStyle.StyleTypography.headingLineSpacing
    readonly property real headingPixelHeight: QGCStyle.StyleTypography.headingPixelHeight
    readonly property real headingPixelWidth: QGCStyle.StyleTypography.headingPixelWidth
    readonly property real headingPointSize: QGCStyle.StyleTypography.headingPointSize
    readonly property int inlineIconSize: QGCStyle.StyleMetrics.inlineIconSize
    readonly property int minimumInteractiveHeight: QGCStyle.StyleMetrics.minimumInteractiveHeight
    readonly property int numericTabularFeature: QGCStyle.StyleTypography.numericFont.features["tnum"]
    readonly property real numericWideWidth: _numericWideMetrics.advanceWidth
    readonly property real numericNarrowWidth: _numericNarrowMetrics.advanceWidth
    readonly property string resolvedBodyFontFamily: _bodyFontInfo.family
    readonly property font systemFixedFont: QGCStyle.SystemFonts.fixedFont
    readonly property string systemFixedFontFamily: _systemFixedFontInfo.family
    readonly property bool systemFixedFontIsFixedPitch: _systemFixedFontInfo.fixedPitch
    readonly property real titleAscent: QGCStyle.StyleTypography.titleAscent
    readonly property real titleBaselineOffset: QGCStyle.StyleTypography.titleBaselineOffset
    readonly property real titleDescent: QGCStyle.StyleTypography.titleDescent
    readonly property real titleFontHeight: QGCStyle.StyleTypography.titleFontHeight
    readonly property real titleLineSpacing: QGCStyle.StyleTypography.titleLineSpacing
    readonly property real titlePixelHeight: QGCStyle.StyleTypography.titlePixelHeight
    readonly property real titlePixelWidth: QGCStyle.StyleTypography.titlePixelWidth
    readonly property real titlePointSize: QGCStyle.StyleTypography.titlePointSize

    readonly property real expectedCaptionPixelHeight: Math.round(_captionFontMetrics.height / 2) * 2
    readonly property real expectedCaptionPixelWidth: Math.round(_captionTextMetrics.advanceWidth / 2) * 2
    readonly property real expectedHeadingPixelHeight: Math.round(_headingFontMetrics.height / 2) * 2
    readonly property real expectedHeadingPixelWidth: Math.round(_headingTextMetrics.advanceWidth / 2) * 2
    readonly property real expectedTitlePixelHeight: Math.round(_titleFontMetrics.height / 2) * 2
    readonly property real expectedTitlePixelWidth: Math.round(_titleTextMetrics.advanceWidth / 2) * 2

    function setTypographyScale(platformPointSize: real, scaleFactor: real) {
        QGCStyle.StyleTypography.platformPointSize = platformPointSize
        QGCStyle.StyleTypography.scaleFactor = scaleFactor
    }

    function setTypographyFontFamily(fontFamily: string) {
        QGCStyle.StyleTypography.normalFontFamily = fontFamily
    }
}
)";

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(qml, QUrl(QStringLiteral("qrc:/tests/QGCStyleTypographyTest.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    QVERIFY(QMetaObject::invokeMethod(root.data(), "setTypographyScale", Q_ARG(double, 8.), Q_ARG(double, 1.)));
    QTRY_COMPARE_WITH_TIMEOUT(root->property("bodyPointSize").toReal(), 8., 5000);
    QTRY_COMPARE_WITH_TIMEOUT(root->property("bodyFontPointSize").toReal(), 8., 5000);
    QVERIFY(
        QMetaObject::invokeMethod(root.data(), "setTypographyFontFamily", Q_ARG(QString, QStringLiteral("Open Sans"))));
    QTRY_COMPARE_WITH_TIMEOUT(root->property("bodyFontFamily").toString(), QStringLiteral("Open Sans"), 5000);
    QCOMPARE(root->property("bodyFontFamily"), root->property("resolvedBodyFontFamily"));
    QVERIFY(root->property("bodyAscent").toReal() > 0.);
    QCOMPARE(root->property("bodyBaselineOffset"), root->property("bodyAscent"));
    QVERIFY(root->property("bodyAverageCharacterWidth").toReal() > 0.);
    QVERIFY(root->property("bodyCapitalHeight").toReal() > 0.);
    QVERIFY(root->property("bodyFontHeight").toReal() > 0.);
    QVERIFY(root->property("bodyLineSpacing").toReal() >= root->property("bodyFontHeight").toReal());
    QCOMPARE(root->property("captionPointSize").toReal(), 6.);
    QCOMPARE(root->property("headingPointSize").toReal(), 10.);
    QCOMPARE(root->property("titlePointSize").toReal(), 12.);
    QCOMPARE(root->property("captionBaselineOffset"), root->property("captionAscent"));
    QCOMPARE(root->property("headingBaselineOffset"), root->property("headingAscent"));
    QCOMPARE(root->property("titleBaselineOffset"), root->property("titleAscent"));
    QVERIFY(root->property("captionDescent").toReal() >= 0.);
    QVERIFY(root->property("headingDescent").toReal() >= 0.);
    QVERIFY(root->property("titleDescent").toReal() >= 0.);
    QVERIFY(root->property("captionLineSpacing").toReal() >= root->property("captionFontHeight").toReal());
    QVERIFY(root->property("headingLineSpacing").toReal() >= root->property("headingFontHeight").toReal());
    QVERIFY(root->property("titleLineSpacing").toReal() >= root->property("titleFontHeight").toReal());
    QCOMPARE(root->property("captionPixelHeight"), root->property("expectedCaptionPixelHeight"));
    QCOMPARE(root->property("captionPixelWidth"), root->property("expectedCaptionPixelWidth"));
    QCOMPARE(root->property("headingPixelHeight"), root->property("expectedHeadingPixelHeight"));
    QCOMPARE(root->property("headingPixelWidth"), root->property("expectedHeadingPixelWidth"));
    QCOMPARE(root->property("titlePixelHeight"), root->property("expectedTitlePixelHeight"));
    QCOMPARE(root->property("titlePixelWidth"), root->property("expectedTitlePixelWidth"));
    QCOMPARE(root->property("defaultDialogControlSpacing").toInt(),
             qMax(1, qRound(root->property("bodyLineSpacing").toReal() / 2.)));
    QTRY_VERIFY_WITH_TIMEOUT(root->property("bodyPixelWidth").toReal() > 0., 5000);
    QVERIFY(root->property("fixedFontIsFixedPitch").toBool());
    QCOMPARE(root->property("fixedFontFamily"), root->property("systemFixedFontFamily"));
    QVERIFY(root->property("monospaceFontIsFixedPitch").toBool());
    QCOMPARE(root->property("systemFixedFont").value<QFont>(), QFontDatabase::systemFont(QFontDatabase::FixedFont));
    QVERIFY(root->property("systemFixedFontIsFixedPitch").toBool());
    QCOMPARE(root->property("numericTabularFeature").toInt(), 1);
    QVERIFY(qAbs(root->property("numericNarrowWidth").toReal() - root->property("numericWideWidth").toReal()) < 0.01);
    QCOMPARE(root->property("controlContentHeight").toInt(), qCeil(root->property("bodyFontHeight").toReal()));
    QCOMPARE(root->property("controlHeight").toInt(), qMax(root->property("minimumInteractiveHeight").toInt(),
                                                           root->property("controlContentHeight").toInt() +
                                                               root->property("controlVerticalPadding").toInt() * 2));
    QCOMPARE(root->property("inlineIconSize").toInt(), qMax(1, qCeil(root->property("bodyCapitalHeight").toReal())));
    const double compactBodyPixelWidth = root->property("bodyPixelWidth").toReal();
    const double compactCaptionPixelHeight = root->property("captionPixelHeight").toReal();
    const double compactHeadingPixelHeight = root->property("headingPixelHeight").toReal();
    const double compactTitlePixelHeight = root->property("titlePixelHeight").toReal();
    QVERIFY(QMetaObject::invokeMethod(root.data(), "setTypographyScale", Q_ARG(double, 12.), Q_ARG(double, 1.25)));
    QTRY_COMPARE_WITH_TIMEOUT(root->property("bodyPointSize").toReal(), 15., 5000);
    QTRY_VERIFY_WITH_TIMEOUT(root->property("bodyPixelHeight").toReal() > 0., 5000);
    QTRY_VERIFY_WITH_TIMEOUT(root->property("bodyPixelWidth").toReal() > compactBodyPixelWidth, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(root->property("captionPixelHeight").toReal() > compactCaptionPixelHeight, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(root->property("headingPixelHeight").toReal() > compactHeadingPixelHeight, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(root->property("titlePixelHeight").toReal() > compactTitlePixelHeight, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(root->property("bodyDescent").toReal() >= 0., 5000);
}

void QGCStyleTest::_styleKitApplies()
{
    QGCPalette::setGlobalTheme(QGCPalette::Light);

    static constexpr char qml[] = R"(
import QtQuick
import Qt.labs.StyleKit as Labs
import QGCStyle as QGCStyle
import QGroundControl.Controls

QGCStyle.ApplicationWindow {
    id: root

    width: 640
    height: 480
    visible: true

    readonly property int animationDuration: QGCStyle.StyleMetrics.animationDuration
    readonly property var activeStyle: Labs.StyleKit.style
    readonly property string activeThemeName: activeStyle.themeName
    readonly property bool controlsLoaded: controlsLoader.status === Loader.Ready
    readonly property string expectedFontFamily: QGCStyle.StyleTypography.normalFontFamily
    readonly property real expectedFontPointSize: QGCStyle.StyleTypography.bodyPointSize
    readonly property int fastAnimationDuration: QGCStyle.StyleMetrics.fastAnimationDuration
    readonly property int normalAnimationDuration: QGCStyle.StyleMetrics.normalAnimationDuration
    readonly property bool reducedMotion: QGCStyle.StylePreferences.reducedMotion
    readonly property int slowAnimationDuration: QGCStyle.StyleMetrics.slowAnimationDuration
    readonly property color styleHighlightColor: activeStyle.palette.highlight
    readonly property bool touchDeviceAvailable: QGCStyle.InputCapabilities.touchDeviceAvailable
    readonly property bool touchInputObserved: QGCStyle.StylePreferences.touchInputObserved
    readonly property bool transitionsEnabled: Labs.StyleKit.transitionsEnabled
    readonly property string windowFontFamily: root.font.family
    readonly property real windowFontPointSize: root.font.pointSize
    readonly property color windowColor: root.palette.window

    function isStyleLoaded(): bool {
        return Labs.StyleKit.styleLoaded()
    }

    function loadControls() {
        controlsLoader.active = true
    }

    function setReducedMotion(enabled: bool) {
        QGCStyle.StylePreferences.setReducedMotion(enabled)
    }

    Loader {
        id: controlsLoader

        active: false
        sourceComponent: Component {
            Item {
                objectName: "styleKitControls"

                readonly property real compactBackgroundHeight: compactButton.background.delegateStyle.implicitHeight
                readonly property real expectedScrollBarExtent: QGCStyle.StyleMetrics.scrollBarExtent
                readonly property real expectedCompactBackgroundHeight: QGCStyle.StyleMetrics.iconSize + QGCStyle.StyleMetrics.spacing
                readonly property string expectedFontFamily: QGCStyle.StyleTypography.normalFontFamily
                readonly property color expectedLabelColor: root.palette.windowText
                readonly property real expectedFontPointSize: QGCStyle.StyleTypography.bodyPointSize
                readonly property color labelColor: qgcLabel.color
                readonly property string labelFontFamily: qgcLabel.font.family
                readonly property real labelFontPointSize: qgcLabel.font.pointSize
                readonly property real expectedNormalBackgroundHeight: QGCStyle.StyleMetrics.controlHeight
                readonly property real normalBackgroundHeight: normalButton.background.delegateStyle.implicitHeight
                readonly property string normalFontFamily: normalButton.font.family
                readonly property real normalFontPointSize: normalButton.font.pointSize
                readonly property int numericFieldTabularFeature: numericField.font.features["tnum"]
                readonly property color primaryBackgroundColor: primaryButton.background.delegateStyle.color
                readonly property bool primaryFontBold: primaryButton.font.bold
                readonly property real scrollBarBackgroundHeight: horizontalScrollBar.background.delegateStyle.implicitHeight
                readonly property real scrollIndicatorBackgroundHeight: horizontalScrollIndicator.background.delegateStyle.implicitHeight

                Column {
                    QGCLabel {
                        id: qgcLabel
                        text: qsTr("QGC label")
                    }
                    QGCTextField {
                        id: numericField
                        numericValuesOnly: true
                        text: "1234"
                    }
                    Labs.Button {
                        id: normalButton
                        text: qsTr("Button")
                    }
                    Labs.Button {
                        id: primaryButton
                        text: qsTr("Primary")
                        Labs.StyleVariation.variations: ["primary"]
                    }
                    Labs.Button {
                        id: compactButton
                        text: qsTr("Compact")
                        Labs.StyleVariation.variations: ["compact"]
                    }
                    Labs.CheckBox { text: qsTr("Check box"); checked: true }
                    Labs.ComboBox { model: [qsTr("One"), qsTr("Two")] }
                    Labs.ProgressBar { value: 0.5 }
                    Labs.RadioButton { text: qsTr("Radio"); checked: true }
                    Labs.Slider { value: 0.5 }
                    Labs.Switch { text: qsTr("Switch"); checked: true }
                    Labs.TextArea { text: qsTr("Text area") }
                    Labs.TextField { text: qsTr("Text field") }
                    Labs.ScrollBar {
                        id: horizontalScrollBar
                        orientation: Qt.Horizontal
                    }
                    Labs.ScrollIndicator {
                        id: horizontalScrollIndicator
                        orientation: Qt.Horizontal
                    }
                }
            }
        }
    }
}
)";

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(qml, QUrl(QStringLiteral("qrc:/tests/QGCStyleKitTest.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    const bool originalReducedMotion = root->property("reducedMotion").toBool();
    const auto restoreReducedMotion = qScopeGuard([&root, originalReducedMotion]() {
        if (root) {
            (void) QMetaObject::invokeMethod(root.data(), "setReducedMotion", Q_ARG(bool, originalReducedMotion));
        }
    });

    const auto styleLoaded = [&root]() {
        bool result = false;
        return QMetaObject::invokeMethod(root.data(), "isStyleLoaded", Q_RETURN_ARG(bool, result)) && result;
    };
    QVERIFY(root->property("activeStyle").value<QObject*>());
    QTRY_VERIFY_WITH_TIMEOUT(styleLoaded(), 5000);
    QCOMPARE(root->property("activeThemeName").toString(), QStringLiteral("QGCPalette"));
    QCOMPARE(root->property("windowFontFamily"), root->property("expectedFontFamily"));
    QCOMPARE(root->property("windowFontPointSize"), root->property("expectedFontPointSize"));
    const QColor lightWindowColor = root->property("windowColor").value<QColor>();
    const bool originalTouchDeviceAvailable = root->property("touchDeviceAvailable").toBool();

    QWindow* const window = qobject_cast<QWindow*>(root.data());
    QVERIFY(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QScopedPointer<QPointingDevice> touchDevice(QTest::createTouchDevice());
    QVERIFY(touchDevice);
    QTest::touchEvent(window, touchDevice.data()).press(0, QPoint(10, 10));
    QTRY_VERIFY_WITH_TIMEOUT(root->property("touchDeviceAvailable").toBool(), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(root->property("touchInputObserved").toBool(), 5000);
    QTest::touchEvent(window, touchDevice.data()).release(0, QPoint(10, 10));
    touchDevice.reset();
    QTRY_COMPARE_WITH_TIMEOUT(root->property("touchDeviceAvailable").toBool(), originalTouchDeviceAvailable, 5000);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "loadControls"));
    QTRY_VERIFY_WITH_TIMEOUT(root->property("controlsLoaded").toBool(), 5000);

    QObject* const controls = root->findChild<QObject*>(QStringLiteral("styleKitControls"));
    QVERIFY(controls);
    QCOMPARE(controls->property("normalBackgroundHeight"), controls->property("expectedNormalBackgroundHeight"));
    QCOMPARE(controls->property("compactBackgroundHeight"), controls->property("expectedCompactBackgroundHeight"));
    QCOMPARE(controls->property("scrollBarBackgroundHeight"), controls->property("expectedScrollBarExtent"));
    QCOMPARE(controls->property("scrollIndicatorBackgroundHeight"), controls->property("expectedScrollBarExtent"));
    QVERIFY(controls->property("compactBackgroundHeight").toReal() <
            controls->property("normalBackgroundHeight").toReal());
    QVERIFY(controls->property("primaryFontBold").toBool());
    QCOMPARE(controls->property("primaryBackgroundColor"), root->property("styleHighlightColor"));
    QCOMPARE(controls->property("normalFontFamily"), controls->property("expectedFontFamily"));
    QCOMPARE(controls->property("normalFontPointSize"), controls->property("expectedFontPointSize"));
    QCOMPARE(controls->property("numericFieldTabularFeature").toInt(), 1);
    QCOMPARE(controls->property("labelColor"), controls->property("expectedLabelColor"));
    QCOMPARE(controls->property("labelFontFamily"), controls->property("expectedFontFamily"));
    QCOMPARE(controls->property("labelFontPointSize"), controls->property("expectedFontPointSize"));

    QGCPalette::setGlobalTheme(QGCPalette::Dark);
    QTRY_VERIFY_WITH_TIMEOUT(root->property("windowColor").value<QColor>() != lightWindowColor, 5000);

    QGCPalette::setGlobalTheme(QGCPalette::Light);
    QTRY_COMPARE_WITH_TIMEOUT(root->property("windowColor").value<QColor>(), lightWindowColor, 5000);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "setReducedMotion", Q_ARG(bool, false)));
    QTRY_VERIFY_WITH_TIMEOUT(root->property("transitionsEnabled").toBool(), 5000);
    QVERIFY(root->property("animationDuration").toInt() > 0);
    QVERIFY(root->property("fastAnimationDuration").toInt() > 0);
    QVERIFY(root->property("normalAnimationDuration").toInt() > 0);
    QVERIFY(root->property("slowAnimationDuration").toInt() > 0);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "setReducedMotion", Q_ARG(bool, true)));
    QTRY_VERIFY_WITH_TIMEOUT(!root->property("transitionsEnabled").toBool(), 5000);
    QTRY_COMPARE_WITH_TIMEOUT(root->property("animationDuration").toInt(), 0, 5000);
    QCOMPARE(root->property("fastAnimationDuration").toInt(), 0);
    QCOMPARE(root->property("normalAnimationDuration").toInt(), 0);
    QCOMPARE(root->property("slowAnimationDuration").toInt(), 0);
}

UT_REGISTER_TEST(QGCStyleTest, TestLabel::Unit)
