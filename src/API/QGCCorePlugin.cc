/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCCorePlugin.h"
#include "AppMessages.h"
#include "AppSettings.h"
#include "MavlinkSettings.h"
#include "FactMetaData.h"
#ifdef QGC_GST_STREAMING
#include "GStreamer.h"
#endif
#include "HorizontalFactValueGrid.h"
#include "InstrumentValueData.h"
#include "JoystickManager.h"
#include "LogDownloadController.h"
#include "MAVLinkLib.h"
#include "QGCLoggingCategory.h"
#include "QGCOptions.h"
#include "QmlComponentInfo.h"
#include "QmlObjectListModel.h"
#ifdef QGC_QT_STREAMING
#include "QtMultimediaReceiver.h"
#endif
#include "SettingsManager.h"
#include "VideoReceiver.h"

#ifdef QGC_CUSTOM_BUILD
#include CUSTOMHEADER
#endif

#include <QtCore/qapplicationstatic.h>
#include <QtCore/QFile>
#include <QtQml/qqml.h>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>

QGC_LOGGING_CATEGORY(QGCCorePluginLog, "qgc.api.qgccoreplugin");

#ifndef QGC_CUSTOM_BUILD
Q_APPLICATION_STATIC(QGCCorePlugin, _qgcCorePluginInstance);
#endif

QGCCorePlugin::QGCCorePlugin(QObject *parent)
    : QObject(parent)
    , _defaultOptions(new QGCOptions(this))
    , _emptyCustomMapItems(new QmlObjectListModel(this))
{
    // qCDebug(QGCCorePluginLog) << Q_FUNC_INFO << this;
}

QGCCorePlugin::~QGCCorePlugin()
{
    // qCDebug(QGCCorePluginLog) << Q_FUNC_INFO << this;
}

QGCCorePlugin *QGCCorePlugin::instance()
{
#ifndef QGC_CUSTOM_BUILD
    return _qgcCorePluginInstance();
#else
    return CUSTOMCLASS::instance();
#endif
}

void QGCCorePlugin::registerQmlTypes()
{
    (void) qmlRegisterUncreatableType<QGCCorePlugin>("QGroundControl", 1, 0, "QGCCorePlugin", QStringLiteral("Reference only"));
    (void) qmlRegisterUncreatableType<QGCOptions>("QGroundControl", 1, 0, "QGCOptions", QStringLiteral("Reference only"));
    (void) qmlRegisterUncreatableType<QGCFlyViewOptions>("QGroundControl", 1, 0, "QGCFlyViewOptions", QStringLiteral("Reference only"));
}

const QVariantList &QGCCorePlugin::analyzePages()
{
    static const QVariantList analyzeList = {
        QVariant::fromValue(new QmlComponentInfo(
            tr("Log Download"),
            QUrl::fromUserInput(QStringLiteral("qrc:/qml/LogDownloadPage.qml")),
            QUrl::fromUserInput(QStringLiteral("qrc:/qmlimages/LogDownloadIcon.svg")))),
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        QVariant::fromValue(new QmlComponentInfo(
            tr("GeoTag Images"),
            QUrl::fromUserInput(QStringLiteral("qrc:/qml/GeoTagPage.qml")),
            QUrl::fromUserInput(QStringLiteral("qrc:/qmlimages/GeoTagIcon.svg")))),
#endif
        QVariant::fromValue(new QmlComponentInfo(
            tr("MAVLink Console"),
            QUrl::fromUserInput(QStringLiteral("qrc:/qml/MAVLinkConsolePage.qml")),
            QUrl::fromUserInput(QStringLiteral("qrc:/qmlimages/MAVLinkConsoleIcon.svg")))),
#ifndef QGC_DISABLE_MAVLINK_INSPECTOR
        QVariant::fromValue(new QmlComponentInfo(
            tr("MAVLink Inspector"),
            QUrl::fromUserInput(QStringLiteral("qrc:/qml/MAVLinkInspectorPage.qml")),
            QUrl::fromUserInput(QStringLiteral("qrc:/qmlimages/MAVLinkInspector.svg")))),
#endif
        QVariant::fromValue(new QmlComponentInfo(
            tr("Vibration"),
            QUrl::fromUserInput(QStringLiteral("qrc:/qml/VibrationPage.qml")),
            QUrl::fromUserInput(QStringLiteral("qrc:/qmlimages/VibrationPageIcon")))),
    };

    return analyzeList;
}

QGCOptions *QGCCorePlugin::options()
{
    return _defaultOptions;
}

const QmlObjectListModel *QGCCorePlugin::customMapItems()
{
    return _emptyCustomMapItems;
}

bool QGCCorePlugin::adjustSettingMetaData(const QString &settingsGroup, FactMetaData &metaData)
{
    if (settingsGroup == AppSettings::settingsGroup) {
        if (metaData.name() == AppSettings::indoorPaletteName) {
            QVariant outdoorPalette;
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
            outdoorPalette = 0;
#else
            outdoorPalette = 1;
#endif
            metaData.setRawDefaultValue(outdoorPalette);
            return true;
        }
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        else if (metaData.name() == MavlinkSettings::telemetrySaveName) {
            metaData.setRawDefaultValue(false);
            return true;
        }
#endif
#ifndef Q_OS_ANDROID
        else if (metaData.name() == AppSettings::androidSaveToSDCardName) {
            return false;
        }
#endif
    }

    return true;
}

QString QGCCorePlugin::showAdvancedUIMessage() const
{
    return tr("WARNING: You are about to enter Advanced Mode. "
              "If used incorrectly, this may cause your vehicle to malfunction thus voiding your warranty. "
              "You should do so only if instructed by customer support. "
              "Are you sure you want to enable Advanced Mode?");
}

void QGCCorePlugin::factValueGridCreateDefaultSettings(const QString &defaultSettingsGroup)
{
    HorizontalFactValueGrid *const factValueGrid = new HorizontalFactValueGrid(defaultSettingsGroup);

    const bool includeFWValues = ((factValueGrid->vehicleClass() == QGCMAVLink::VehicleClassFixedWing) || (factValueGrid->vehicleClass() == QGCMAVLink::VehicleClassVTOL) || (factValueGrid->vehicleClass() == QGCMAVLink::VehicleClassAirship));

    factValueGrid->setFontSize(FactValueGrid::LargeFontSize);

    (void) factValueGrid->appendColumn();
    (void) factValueGrid->appendColumn();
    (void) factValueGrid->appendColumn();
    if (includeFWValues) {
        (void) factValueGrid->appendColumn();
    }
    factValueGrid->appendRow();

    int rowIndex = 0;
    QmlObjectListModel *column = factValueGrid->columns()->value<QmlObjectListModel*>(0);

    InstrumentValueData *value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact(QStringLiteral("Vehicle"), QStringLiteral("AltitudeRelative"));
    value->setIcon(QStringLiteral("arrow-thick-up.svg"));
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(true);

    value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact(QStringLiteral("Vehicle"), QStringLiteral("DistanceToHome"));
    value->setIcon(QStringLiteral("bookmark copy 3.svg"));
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(true);

    rowIndex = 0;
    column = factValueGrid->columns()->value<QmlObjectListModel*>(1);

    value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact(QStringLiteral("Vehicle"), QStringLiteral("ClimbRate"));
    value->setIcon(QStringLiteral("arrow-simple-up.svg"));
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(true);

    value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact(QStringLiteral("Vehicle"), QStringLiteral("GroundSpeed"));
    value->setIcon(QStringLiteral("arrow-simple-right.svg"));
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(true);

    if (includeFWValues) {
        rowIndex = 0;
        column = factValueGrid->columns()->value<QmlObjectListModel*>(2);

        value = column->value<InstrumentValueData*>(rowIndex++);
        value->setFact(QStringLiteral("Vehicle"), QStringLiteral("AirSpeed"));
        value->setText(QStringLiteral("AirSpd"));
        value->setShowUnits(true);

        value = column->value<InstrumentValueData*>(rowIndex++);
        value->setFact(QStringLiteral("Vehicle"), QStringLiteral("ThrottlePct"));
        value->setText(QStringLiteral("Thr"));
        value->setShowUnits(true);
    }

    rowIndex = 0;
    column = factValueGrid->columns()->value<QmlObjectListModel*>(includeFWValues ? 3 : 2);

    value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact(QStringLiteral("Vehicle"), QStringLiteral("FlightTime"));
    value->setIcon(QStringLiteral("timer.svg"));
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(false);

    value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact(QStringLiteral("Vehicle"), QStringLiteral("FlightDistance"));
    value->setIcon(QStringLiteral("travel-walk.svg"));
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(true);

    factValueGrid->deleteLater();
}

QQmlApplicationEngine *QGCCorePlugin::createQmlApplicationEngine(QObject *parent)
{
    QQmlApplicationEngine *const qmlEngine = new QQmlApplicationEngine(parent);
    qmlEngine->addImportPath(QStringLiteral("qrc:/qml"));
    qmlEngine->rootContext()->setContextProperty(QStringLiteral("joystickManager"), JoystickManager::instance());
    qmlEngine->rootContext()->setContextProperty(QStringLiteral("debugMessageModel"), AppMessages::getModel());
    qmlEngine->rootContext()->setContextProperty(QStringLiteral("logDownloadController"), LogDownloadController::instance());
    return qmlEngine;
}

void QGCCorePlugin::createRootWindow(QQmlApplicationEngine *qmlEngine)
{
    qmlEngine->load(QUrl(QStringLiteral("qrc:/qml/MainRootWindow.qml")));
}

VideoReceiver *QGCCorePlugin::createVideoReceiver(QObject *parent)
{
#ifdef QGC_GST_STREAMING
    return GStreamer::createVideoReceiver(parent);
#elif defined(QGC_QT_STREAMING)
    return QtMultimediaReceiver::createVideoReceiver(parent);
#else
    return nullptr;
#endif
}

void *QGCCorePlugin::createVideoSink(QObject *parent, QQuickItem *widget)
{
#ifdef QGC_GST_STREAMING
    return GStreamer::createVideoSink(parent, widget);
#elif defined(QGC_QT_STREAMING)
    return QtMultimediaReceiver::createVideoSink(parent, widget);
#else
    Q_UNUSED(parent);
    Q_UNUSED(widget);
    return nullptr;
#endif
}

void QGCCorePlugin::releaseVideoSink(void *sink)
{
#ifdef QGC_GST_STREAMING
    GStreamer::releaseVideoSink(sink);
#elif defined(QGC_QT_STREAMING)
    QtMultimediaReceiver::releaseVideoSink(sink);
#else
    Q_UNUSED(sink);
#endif
}

const QVariantList &QGCCorePlugin::toolBarIndicators()
{
    static const QVariantList toolBarIndicatorList = QVariantList(
        {
            QVariant::fromValue(QUrl::fromUserInput(QStringLiteral("qrc:/toolbar/RTKGPSIndicator.qml"))),
        }
    );

    return toolBarIndicatorList;
}

QVariantList QGCCorePlugin::firstRunPromptsToShow()
{
    QList<int> rgIdsToShow;

    rgIdsToShow.append(firstRunPromptStdIds());
    rgIdsToShow.append(firstRunPromptCustomIds());

    const QList<int> rgAlreadyShownIds = AppSettings::firstRunPromptsIdsVariantToList(SettingsManager::instance()->appSettings()->firstRunPromptIdsShown()->rawValue());
    for (int idToRemove: rgAlreadyShownIds) {
        (void) rgIdsToShow.removeOne(idToRemove);
    }

    QVariantList rgVarIdsToShow;
    for (int id: rgIdsToShow) {
        rgVarIdsToShow.append(id);
    }

    return rgVarIdsToShow;
}

QString QGCCorePlugin::firstRunPromptResource(int id) const
{
    switch (id) {
    case kUnitsFirstRunPromptId:
        return QStringLiteral("/FirstRunPromptDialogs/UnitsFirstRunPrompt.qml");
    case kOfflineVehicleFirstRunPromptId:
        return QStringLiteral("/FirstRunPromptDialogs/OfflineVehicleFirstRunPrompt.qml");
    default:
        return QString();
    }
}

void QGCCorePlugin::_setShowTouchAreas(bool show)
{
    if (show != _showTouchAreas) {
        _showTouchAreas = show;
        emit showTouchAreasChanged(show);
    }
}

void QGCCorePlugin::_setShowAdvancedUI(bool show)
{
    if (show != _showAdvancedUI) {
        _showAdvancedUI = show;
        emit showAdvancedUIChanged(show);
    }
}
