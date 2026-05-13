#include "SprigCorePlugin.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QFile>
#include <QtGui/QFont>
#include <QtGui/QFontDatabase>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlFile>

#include "AppSettings.h"
#include "QGCLoggingCategory.h"
#include "QGCMAVLink.h"
#include "QGCPalette.h"
#include "QmlComponentInfo.h"
#include "VideoSettings.h"

QGC_LOGGING_CATEGORY(SprigLog, "Sprig.SprigCorePlugin")

Q_APPLICATION_STATIC(SprigCorePlugin, _sprigPluginInstance);

SprigFlyViewOptions::SprigFlyViewOptions(SprigOptions* options, QObject* parent) : QGCFlyViewOptions(options, parent)
{
    qCDebug(SprigLog) << this;
}

SprigOptions::SprigOptions(SprigCorePlugin* plugin, QObject* parent)
    : QGCOptions(parent), _plugin(plugin), _flyViewOptions(new SprigFlyViewOptions(this, this))
{
    Q_CHECK_PTR(_plugin);
}

/*===========================================================================*/

SprigCorePlugin::SprigCorePlugin(QObject* parent) : QGCCorePlugin(parent), _options(new SprigOptions(this, this))
{
    qCDebug(SprigLog) << this;

    _showAdvancedUI = false;
    (void) connect(this, &QGCCorePlugin::showAdvancedUIChanged, this, &SprigCorePlugin::_advancedChanged);
}

QGCCorePlugin* SprigCorePlugin::instance()
{
    return _sprigPluginInstance();
}

void SprigCorePlugin::init()
{
    qCInfo(SprigLog) << "Sprig GCS core plugin active";

    // Load Sprig fonts from QRC into QFontDatabase
    static const QList<QString> kFontResources = {
        QStringLiteral(":/Custom/fonts/BebasNeue-Regular.ttf"), QStringLiteral(":/Custom/fonts/Barlow-Regular.ttf"),
        QStringLiteral(":/Custom/fonts/Barlow-Medium.ttf"),     QStringLiteral(":/Custom/fonts/Barlow-SemiBold.ttf"),
        QStringLiteral(":/Custom/fonts/Barlow-Bold.ttf"),
    };

    for (const QString& res : kFontResources) {
        QFile f(res);
        if (!f.open(QIODevice::ReadOnly)) {
            qCWarning(SprigLog) << "Font not found in QRC:" << res;
            continue;
        }
        const int id = QFontDatabase::addApplicationFontFromData(f.readAll());
        if (id < 0) {
            qCWarning(SprigLog) << "Failed to register font:" << res;
        } else {
            qCInfo(SprigLog) << "Registered font:" << res << "families:" << QFontDatabase::applicationFontFamilies(id);
        }
    }

    // Set Barlow as the application default font.
    // Point size 11 matches Qt's default; adjust if visual sizing regresses.
    const QFont appFont(QStringLiteral("Barlow"), 11);
    QGuiApplication::setFont(appFont);
    qCInfo(SprigLog) << "App default font set to:" << QGuiApplication::font().family()
                     << "pt:" << QGuiApplication::font().pointSize();
}

// Placeholder -- actual auto-update infra is Phase 7
QString SprigCorePlugin::stableVersionCheckFileUrl() const
{
    return QStringLiteral("https://updates.sprigaerospace.com/qgc/stable/version.json");
}

QString SprigCorePlugin::stableDownloadLocation() const
{
    return QStringLiteral("https://updates.sprigaerospace.com/qgc/stable/");
}

void SprigCorePlugin::cleanup()
{
    if (_qmlEngine) {
        _qmlEngine->removeUrlInterceptor(_selector);
    }

    delete _selector;
}

void SprigCorePlugin::_advancedChanged(bool changed)
{
    // Firmware Upgrade page is only show in Advanced mode
    emit _options->showFirmwareUpgradeChanged(changed);
}

void SprigCorePlugin::_addSettingsEntry(const QString& title, const char* qmlFile, const char* iconFile)
{
    Q_CHECK_PTR(qmlFile);
    // 'this' instance will take ownership on the QmlComponentInfo instance
    _customSettingsList.append(QVariant::fromValue(new QmlComponentInfo(
        title, QUrl::fromUserInput(qmlFile), !iconFile ? QUrl() : QUrl::fromUserInput(iconFile), this)));
}

void SprigCorePlugin::adjustSettingMetaData(const QString& settingsGroup, FactMetaData& metaData, bool& userVisible)
{
    QGCCorePlugin::adjustSettingMetaData(settingsGroup, metaData, userVisible);

    if (settingsGroup == AppSettings::settingsGroup) {
        // This tells QGC than when you are creating Plans while not connected to a vehicle
        // the specific firmware/vehicle the plan is for.
        if (metaData.name() == AppSettings::offlineEditingFirmwareClassName) {
            metaData.setRawDefaultValue(QGCMAVLink::FirmwareClassPX4);
            userVisible = false;
            return;
        } else if (metaData.name() == AppSettings::offlineEditingVehicleClassName) {
            metaData.setRawDefaultValue(QGCMAVLink::VehicleClassMultiRotor);
            userVisible = false;
            return;
        } else if (metaData.name() == QStringLiteral("indoorPalette")) {
            // Force default palette to Dark for industrial/outdoor use case.
            // indoorPalette: 1 = Indoor (Dark), 0 = Outdoor (Light)
            metaData.setRawDefaultValue(1);
            return;
        }
    }

    if (settingsGroup == VideoSettings::settingsGroup) {
        if (metaData.name() == VideoSettings::videoSourceName) {
            // Hide vendor-specific video presets that are not relevant to Sprig GCS.
            static const QSet<QString> kAllowedSources = {
                VideoSettings::videoSourceNoVideo, VideoSettings::videoDisabled,      VideoSettings::videoSourceRTSP,
                VideoSettings::videoSourceUDPH264, VideoSettings::videoSourceUDPH265, VideoSettings::videoSourceTCP,
                VideoSettings::videoSourceMPEGTS,
            };

            const QStringList origStrings = metaData.enumStrings();
            const QVariantList origValues = metaData.enumValues();

            QStringList filteredStrings;
            QVariantList filteredValues;
            for (int i = 0; i < origValues.size(); ++i) {
                if (kAllowedSources.contains(origValues[i].toString())) {
                    filteredStrings.append(origStrings[i]);
                    filteredValues.append(origValues[i]);
                }
            }

            metaData.setEnumInfo(filteredStrings, filteredValues);
            return;
        }
    }
}

void SprigCorePlugin::paletteOverride(const QString& colorName, QGCPalette::PaletteColorInfo_t& colorInfo)
{
    // -----------------------------------------------------------------------
    // SPRIG PALETTE - edit ONLY this constant block to change brand colors.
    // Phase 1 ships Dark theme only; Light theme stubs use the same values.
    // Replace placeholders with official Sprig hex values before phase exit.
    // -----------------------------------------------------------------------
    static const QColor kPrimary("#5BA86A");      // Sprig green - main action color
    static const QColor kAccent("#84C58F");       // lighter green accent
    static const QColor kBgDark("#1A1F26");       // darkest background / window shade
    static const QColor kBgLight("#232830");      // slightly lighter background
    static const QColor kSurface("#232830");      // card / widget surface
    static const QColor kError("#D9534F");        // error / alert red
    static const QColor kWarning("#F0AD4E");      // warning amber
    static const QColor kSuccess("#5CB85C");      // success green
    static const QColor kTextOnDark("#EAEEF3");   // primary text on dark backgrounds
    static const QColor kTextOnLight("#1A1F26");  // text on light surfaces
    static const QColor kDisabled("#6B7480");     // disabled / muted text and icons
    // -----------------------------------------------------------------------

    // Helper to assign all 4 slots: [Dark][Enabled], [Dark][Disabled],
    //                                [Light][Enabled], [Light][Disabled]
    // Light theme is stubbed with the same values; replace when Light theme is designed.
    auto set4 = [&](const QColor& de, const QColor& dd, const QColor& le, const QColor& ld) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled] = de;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled] = dd;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled] = le;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = ld;
    };

    // ---- Every color name from DECLARE_QGC_COLOR / DECLARE_QGC_NONTHEMED_COLOR /
    //      DECLARE_QGC_SINGLE_COLOR in src/QmlControls/QGCPalette.cc _buildMap(). ----

    // Window / background
    if (colorName == QStringLiteral("window"))
        set4(kBgDark, kBgDark, kBgLight, kBgLight);
    else if (colorName == QStringLiteral("windowTransparent"))
        set4(QColor(26, 31, 38, 204), QColor(26, 31, 38, 204), QColor(35, 40, 48, 204), QColor(35, 40, 48, 204));
    else if (colorName == QStringLiteral("windowShadeLight"))
        set4(QColor("#3A4250"), QColor("#3A4250"), QColor("#3A4250"), QColor("#3A4250"));
    else if (colorName == QStringLiteral("windowShade"))
        set4(QColor("#2B3038"), QColor("#2B3038"), QColor("#2B3038"), QColor("#2B3038"));
    else if (colorName == QStringLiteral("windowShadeDark"))
        set4(QColor("#151920"), QColor("#151920"), QColor("#151920"), QColor("#151920"));

    // Text
    else if (colorName == QStringLiteral("text"))
        set4(kTextOnDark, kDisabled, kTextOnLight, kDisabled);
    else if (colorName == QStringLiteral("warningText"))
        set4(kWarning, kWarning.darker(130), kWarning, kWarning.darker(130));

    // Buttons
    else if (colorName == QStringLiteral("button"))
        set4(kSurface, kSurface.darker(120), kSurface, kSurface.darker(120));
    else if (colorName == QStringLiteral("buttonBorder"))
        set4(kAccent, kDisabled, kAccent, kDisabled);
    else if (colorName == QStringLiteral("buttonText"))
        set4(kTextOnDark, kDisabled, kTextOnLight, kDisabled);
    else if (colorName == QStringLiteral("buttonHighlight"))
        set4(kAccent, kAccent.darker(130), kAccent, kAccent.darker(130));
    else if (colorName == QStringLiteral("buttonHighlightText"))
        set4(kTextOnDark, kDisabled, kTextOnLight, kDisabled);
    else if (colorName == QStringLiteral("primaryButton"))
        set4(kPrimary, kPrimary.darker(130), kPrimary, kPrimary.darker(130));
    else if (colorName == QStringLiteral("primaryButtonText"))
        set4(kTextOnDark, kDisabled, kTextOnDark, kDisabled);
    else if (colorName == QStringLiteral("textField"))
        set4(kBgLight, kBgLight.darker(110), kBgLight, kBgLight.darker(110));
    else if (colorName == QStringLiteral("textFieldText"))
        set4(kTextOnDark, kDisabled, kTextOnLight, kDisabled);

    // Map overlay
    else if (colorName == QStringLiteral("mapButton"))
        set4(kBgDark, kDisabled, kBgDark, kDisabled);
    else if (colorName == QStringLiteral("mapButtonHighlight"))
        set4(kAccent, kDisabled, kAccent, kDisabled);
    else if (colorName == QStringLiteral("mapIndicator"))
        set4(kPrimary, kDisabled, kPrimary, kDisabled);
    else if (colorName == QStringLiteral("mapIndicatorChild"))
        set4(kPrimary.darker(130), kDisabled, kPrimary.darker(130), kDisabled);

    // Status colors
    else if (colorName == QStringLiteral("colorGreen"))
        set4(kSuccess, kSuccess.darker(130), kSuccess, kSuccess.darker(130));
    else if (colorName == QStringLiteral("colorYellow"))
        set4(QColor("#F5E642"), QColor("#F5E642").darker(130), QColor("#F5E642"), QColor("#F5E642").darker(130));
    else if (colorName == QStringLiteral("colorYellowGreen"))
        set4(QColor("#9DBE2F"), QColor("#9DBE2F").darker(130), QColor("#9DBE2F"), QColor("#9DBE2F").darker(130));
    else if (colorName == QStringLiteral("colorOrange"))
        set4(kWarning, kWarning.darker(130), kWarning, kWarning.darker(130));
    else if (colorName == QStringLiteral("colorRed"))
        set4(kError, kError.darker(130), kError, kError.darker(130));
    else if (colorName == QStringLiteral("colorGrey"))
        set4(kDisabled, kDisabled.darker(120), kDisabled, kDisabled.darker(120));
    else if (colorName == QStringLiteral("colorBlue"))
        set4(kAccent, kAccent.darker(130), kAccent, kAccent.darker(130));

    // Alert bar
    else if (colorName == QStringLiteral("alertBackground"))
        set4(kError.darker(115), kError.darker(130), kError.darker(115), kError.darker(130));
    else if (colorName == QStringLiteral("alertBorder"))
        set4(kError, kError.darker(130), kError, kError.darker(130));
    else if (colorName == QStringLiteral("alertText"))
        set4(kTextOnDark, kDisabled, kTextOnDark, kDisabled);

    // Mission item editor
    else if (colorName == QStringLiteral("missionItemEditor"))
        set4(kSurface, kBgDark, kSurface, kBgDark);

    // Tool strip / hover
    else if (colorName == QStringLiteral("toolStripHoverColor"))
        set4(kAccent.lighter(115), kDisabled, kAccent.lighter(115), kDisabled);

    // Status text
    else if (colorName == QStringLiteral("statusFailedText"))
        set4(kTextOnDark, kDisabled, kTextOnLight, kDisabled);
    else if (colorName == QStringLiteral("statusPassedText"))
        set4(kTextOnDark, kDisabled, kTextOnLight, kDisabled);
    else if (colorName == QStringLiteral("statusPendingText"))
        set4(kTextOnDark, kDisabled, kTextOnLight, kDisabled);

    // Toolbar
    else if (colorName == QStringLiteral("toolbarBackground"))
        set4(QColor(26, 31, 38, 0), QColor(26, 31, 38, 0), QColor(26, 31, 38, 0), QColor(26, 31, 38, 0));

    // Group border
    else if (colorName == QStringLiteral("groupBorder"))
        set4(kDisabled, kPrimary, kDisabled, kDisabled);

    // Modified parameter value
    else if (colorName == QStringLiteral("modifiedParamValue"))
        set4(kWarning, kWarning.darker(130), kWarning, kWarning.darker(130));

    // Branding - remap QGC purple/blue to Sprig green; QML references these names directly.
    else if (colorName == QStringLiteral("brandingPurple"))
        set4(kPrimary, kDisabled, kPrimary, kDisabled);
    else if (colorName == QStringLiteral("brandingBlue"))
        set4(kAccent, kDisabled, kAccent, kDisabled);

    // Non-themed utility colors
    else if (colorName == QStringLiteral("toolStripFGColor"))
        set4(kTextOnDark, kDisabled, kTextOnDark, kDisabled);
    else if (colorName == QStringLiteral("photoCaptureButtonColor"))
        set4(kTextOnDark, kDisabled, kTextOnDark, kDisabled);
    else if (colorName == QStringLiteral("videoCaptureButtonColor"))
        set4(kError, kError.darker(130), kError, kError.darker(130));

    // Single-value colors (same across all theme/group slots)
    else if (colorName == QStringLiteral("mapWidgetBorderLight"))
        set4(kTextOnDark, kTextOnDark, kTextOnDark, kTextOnDark);
    else if (colorName == QStringLiteral("mapWidgetBorderDark"))
        set4(kBgDark, kBgDark, kBgDark, kBgDark);
    else if (colorName == QStringLiteral("mapMissionTrajectory"))
        set4(kWarning, kWarning, kWarning, kWarning);
    else if (colorName == QStringLiteral("surveyPolygonInterior"))
        set4(kPrimary, kPrimary, kPrimary, kPrimary);
    else if (colorName == QStringLiteral("surveyPolygonTerrainCollision"))
        set4(kError, kError, kError, kError);

    // If we get here, fall through to QGC default (no modification to colorInfo).
}

QQmlApplicationEngine* SprigCorePlugin::createQmlApplicationEngine(QObject* parent)
{
    _qmlEngine = QGCCorePlugin::createQmlApplicationEngine(parent);
    _qmlEngine->addImportPath("qrc:/qml/Custom/Widgets");
    // TODO: Investigate _qmlEngine->setExtraSelectors({"custom"})

    _selector = new SprigOverrideInterceptor();
    _qmlEngine->addUrlInterceptor(_selector);

    return _qmlEngine;
}

/*===========================================================================*/

SprigOverrideInterceptor::SprigOverrideInterceptor() : QQmlAbstractUrlInterceptor() {}

QUrl SprigOverrideInterceptor::intercept(const QUrl& url, QQmlAbstractUrlInterceptor::DataType type)
{
    switch (type) {
        case QQmlAbstractUrlInterceptor::QmlFile:
        case QQmlAbstractUrlInterceptor::UrlString:
            if (url.scheme() == QStringLiteral("qrc")) {
                const QString origPath = url.path();
                const QString overrideRes = QStringLiteral(":/Custom%1").arg(origPath);
                if (QFile::exists(overrideRes)) {
                    const QString relPath = overrideRes.mid(2);
                    QUrl result;
                    result.setScheme(QStringLiteral("qrc"));
                    result.setPath('/' + relPath);
                    return result;
                }
            }
            break;
        default:
            break;
    }

    return url;
}
