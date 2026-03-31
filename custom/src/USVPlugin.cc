/****************************************************************************
 *
 * USV (Unmanned Surface Vehicle) Custom Plugin Implementation
 * 无人船专用定制插件实现
 *
 ****************************************************************************/

#include "USVPlugin.h"
#include "USVOptions.h"
#include "USVPayloadFactGroup.h"

#include "QGCPalette.h"
#include "QGCMAVLink.h"
#include "AppSettings.h"
#include "BrandImageSettings.h"
#include "FirmwareUpgradeSettings.h"
#include "MavlinkActionsSettings.h"
#include "SettingsManager.h"
#include "Vehicle.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QLoggingCategory>
#include <QtCore/QCoreApplication>
#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QtCore/QDir>
#include <QtQml/QQmlApplicationEngine>
#include <QtCore/QFile>

Q_LOGGING_CATEGORY(USVPluginLog, "USV.Plugin")

Q_APPLICATION_STATIC(USVPlugin, _usvPluginInstance);

/*===========================================================================*/
// USVPlugin Implementation
/*===========================================================================*/

USVPlugin::USVPlugin(QObject *parent)
    : QGCCorePlugin(parent)
    , _usvOptions(new USVOptions(this, this))
    , _usvTranslator(new QTranslator(this))
{
    qCDebug(USVPluginLog) << "USVPlugin created";

    // 默认显示高级 UI（包含分析视图）
    _showAdvancedUI = true;

    connect(this, &QGCCorePlugin::showAdvancedUIChanged,
            this, &USVPlugin::_advancedChanged);
}

USVPlugin::~USVPlugin()
{
    qCDebug(USVPluginLog) << "USVPlugin destroyed";
}

QGCCorePlugin *USVPlugin::instance()
{
    return _usvPluginInstance();
}

void USVPlugin::init()
{
    qCInfo(USVPluginLog) << "USV Custom Plugin initialized - 无人船专用模式";

    // 加载 USV 专用翻译文件 (覆盖默认翻译中的航空术语)
    _loadUSVTranslations();

    // 设置默认的 MAVLink Actions 文件（水质采样）
    _setupDefaultMavlinkActions();

    QGCCorePlugin::init();
}

void USVPlugin::_loadUSVTranslations()
{
    QLocale locale = QLocale::system();

    // 尝试加载 USV 专用翻译文件
    // 翻译文件命名格式: usv_zh_CN.qm
    // Qt 会自动尝试: usv_zh_CN.qm, usv_zh.qm, usv_.qm

    // 首先尝试完整 locale 名称
    QString fullPath = QStringLiteral(":/i18n/usv_%1.qm").arg(locale.name());
    qCDebug(USVPluginLog) << "Trying to load USV translation from:" << fullPath;
    qCDebug(USVPluginLog) << "File exists:" << QFile::exists(fullPath);

    if (_usvTranslator->load(locale, QStringLiteral("usv_"), QString(), QStringLiteral(":/i18n"))) {
        QCoreApplication::installTranslator(_usvTranslator);
        qCInfo(USVPluginLog) << "USV translations loaded for locale:" << locale.name();
    } else {
        qCDebug(USVPluginLog) << "No USV translations found for locale:" << locale.name();

        // 列出 :/i18n 目录下的所有文件用于调试
        QDir i18nDir(":/i18n");
        if (i18nDir.exists()) {
            qCDebug(USVPluginLog) << "Available translation files in :/i18n:" << i18nDir.entryList();
        } else {
            qCDebug(USVPluginLog) << ":/i18n directory does not exist";
        }
    }
}

void USVPlugin::_setupDefaultMavlinkActions()
{
    // 内置的 USV Actions 资源路径
    const QString usvActionsPath = QStringLiteral(":/usv/actions/usv_actions.json");

    // 检查资源文件是否存在
    if (!QFile::exists(usvActionsPath)) {
        qCWarning(USVPluginLog) << "Built-in USV actions file not found:" << usvActionsPath;
        return;
    }

    auto* mavlinkActionsSettings = SettingsManager::instance()->mavlinkActionsSettings();

    // 如果用户未配置 flyViewActionsFile，设置为内置资源路径
    Fact* flyViewFact = mavlinkActionsSettings->flyViewActionsFile();
    if (flyViewFact->rawValue().toString().isEmpty()) {
        flyViewFact->setRawValue(usvActionsPath);
        qCInfo(USVPluginLog) << "Set flyViewActionsFile to:" << usvActionsPath;
    }

    // 如果用户未配置 joystickActionsFile，设置为内置资源路径
    Fact* joystickFact = mavlinkActionsSettings->joystickActionsFile();
    if (joystickFact->rawValue().toString().isEmpty()) {
        joystickFact->setRawValue(usvActionsPath);
        qCInfo(USVPluginLog) << "Set joystickActionsFile to:" << usvActionsPath;
    }
}

void USVPlugin::cleanup()
{
    if (_qmlEngine && _interceptor) {
        _qmlEngine->removeUrlInterceptor(_interceptor);
    }

    delete _interceptor;
    _interceptor = nullptr;

    // 移除 USV 翻译
    if (_usvTranslator) {
        QCoreApplication::removeTranslator(_usvTranslator);
    }

    QGCCorePlugin::cleanup();
}

QGCOptions *USVPlugin::options()
{
    return _usvOptions;
}

QString USVPlugin::brandImageIndoor() const
{
    // 无人船品牌 Logo (深色背景用白色版本)
    return QStringLiteral("/custom/img/usv-logo-white.png");
}

QString USVPlugin::brandImageOutdoor() const
{
    return QStringLiteral("/custom/img/usv-logo-black.png");
}

bool USVPlugin::overrideSettingsGroupVisibility(const QString &name)
{
    // 隐藏品牌图片设置 (已自定义)
    if (name == BrandImageSettings::name) {
        return false;
    }

    // 固件升级仅在高级模式下显示
    if (name == FirmwareUpgradeSettings::name) {
        return _showAdvancedUI;
    }

    return true;
}

void USVPlugin::adjustSettingMetaData(const QString &settingsGroup,
                                       FactMetaData &metaData,
                                       bool &visible)
{
    // 先调用父类方法
    QGCCorePlugin::adjustSettingMetaData(settingsGroup, metaData, visible);

    if (settingsGroup == AppSettings::settingsGroup) {
        const QString &name = metaData.name();

        // ========== 锁定载具类型为 Rover/Boat ==========
        if (name == QStringLiteral("offlineEditingVehicleClass")) {
            // MAV_TYPE_GROUND_ROVER = 10, 包含 Rover 和 Boat
            metaData.setRawDefaultValue(QGCMAVLink::VehicleClassRoverBoat);
            visible = false;  // 隐藏选择器，用户无法更改
            qCDebug(USVPluginLog) << "Locked vehicle class to RoverBoat";
            return;
        }

        // ========== 固件类型：允许选择 ArduPilot 或 PX4 ==========
        if (name == QStringLiteral("offlineEditingFirmwareClass")) {
            // 默认使用 ArduPilot，但允许用户选择 PX4
            metaData.setRawDefaultValue(QGCMAVLink::FirmwareClassArduPilot);
            // 修改枚举选项，仅保留 ArduPilot 和 PX4
            metaData.setEnumInfo(
                QStringList() << QStringLiteral("ArduPilot") << QStringLiteral("PX4"),
                QVariantList() << QGCMAVLink::FirmwareClassArduPilot << QGCMAVLink::FirmwareClassPX4
            );
            visible = true;  // 允许用户选择
            qCDebug(USVPluginLog) << "Firmware class: ArduPilot/PX4 selectable";
            return;
        }

        // ========== 隐藏不适用于无人船的速度设置 ==========
        // 悬停速度 - 无人船不悬停
        if (name == QStringLiteral("offlineEditingHoverSpeed")) {
            visible = false;
            return;
        }

        // 上升速度 - 无人船不上升
        if (name == QStringLiteral("offlineEditingAscentSpeed")) {
            visible = false;
            return;
        }

        // 下降速度 - 无人船不下降
        if (name == QStringLiteral("offlineEditingDescentSpeed")) {
            visible = false;
            return;
        }

        // ========== 调整巡航速度默认值 ==========
        if (name == QStringLiteral("offlineEditingCruiseSpeed")) {
            // 无人船典型巡航速度 5 m/s (约 10 节)
            metaData.setRawDefaultValue(5.0);
            return;
        }

        // ========== 调整默认任务高度 ==========
        if (name == QStringLiteral("defaultMissionItemAltitude")) {
            // 无人船高度设为 0 (水面)
            metaData.setRawDefaultValue(0.0);
            return;
        }
    }

    // ========== MavlinkActions 设置 ==========
    if (settingsGroup == MavlinkActionsSettings::name) {
        const QString &name = metaData.name();

        // 设置默认的 FlyView Actions 文件为内置水质采样配置
        if (name == QStringLiteral("flyViewActionsFile")) {
            metaData.setRawDefaultValue(QStringLiteral(":/usv/actions/usv_actions.json"));
            qCDebug(USVPluginLog) << "Set flyViewActionsFile default to built-in USV actions";
            return;
        }

        // 设置默认的 Joystick Actions 文件
        if (name == QStringLiteral("joystickActionsFile")) {
            metaData.setRawDefaultValue(QStringLiteral(":/usv/actions/usv_actions.json"));
            qCDebug(USVPluginLog) << "Set joystickActionsFile default to built-in USV actions";
            return;
        }
    }
}

void USVPlugin::paletteOverride(const QString &colorName,
                                 QGCPalette::PaletteColorInfo_t &colorInfo)
{
    // 可选：自定义无人船主题颜色
    // 使用蓝色系代表水域

    if (colorName == QStringLiteral("brandingBlue")) {
        // 海洋蓝主题色
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#0077b6");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#48cae4");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#0077b6");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#48cae4");
    } else if (colorName == QStringLiteral("colorGreen")) {
        // 安全/正常状态 - 保持绿色
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#2d9d78");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#2d9d78");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#2d9d78");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#2d9d78");
    }
}

QQmlApplicationEngine *USVPlugin::createQmlApplicationEngine(QObject *parent)
{
    _qmlEngine = QGCCorePlugin::createQmlApplicationEngine(parent);

    // 添加 USV QML 模块导入路径
    _qmlEngine->addImportPath("qrc:/qml");

    // 安装 URL 拦截器，用于替换默认 QML 文件
    _interceptor = new USVQmlOverrideInterceptor();
    _qmlEngine->addUrlInterceptor(_interceptor);

    return _qmlEngine;
}

const QVariantList &USVPlugin::analyzePages()
{
    // 使用默认分析页面
    return QGCCorePlugin::analyzePages();
}

const QVariantList &USVPlugin::toolBarIndicators()
{
    // 使用默认工具栏指示器
    return QGCCorePlugin::toolBarIndicators();
}

QList<int> USVPlugin::firstRunPromptStdIds()
{
    // 保留单位设置提示，移除离线载具选择提示（已锁定）
    return QList<int>({ kUnitsFirstRunPromptId });
}

QStringList USVPlugin::complexMissionItemNames(Vehicle *vehicle,
                                                const QStringList &complexMissionItemNames)
{
    Q_UNUSED(vehicle);

    QStringList filteredItems;

    for (const QString &item : complexMissionItemNames) {
        // 保留适用于无人船的复杂任务项
        if (item == QStringLiteral("Survey") ||           // 测量任务 - 可用于水域测量
            item == QStringLiteral("Corridor Scan")) {    // 走廊扫描 - 可用于河道巡检
            filteredItems.append(item);
        }
        // 排除:
        // - "Fixed Wing Landing" - 固定翼降落
        // - "VTOL Landing" - 垂直起降
        // - "Structure Scan" - 建筑扫描（需要高度变化）
    }

    qCDebug(USVPluginLog) << "Filtered complex mission items:" << filteredItems;
    return filteredItems;
}

void USVPlugin::_advancedChanged(bool advanced)
{
    // 高级模式变化时，通知固件升级设置可见性变化
    emit _usvOptions->showFirmwareUpgradeChanged(advanced);
}

/*===========================================================================*/
// USVQmlOverrideInterceptor Implementation
/*===========================================================================*/

USVQmlOverrideInterceptor::USVQmlOverrideInterceptor()
    : QQmlAbstractUrlInterceptor()
{
    qCInfo(USVPluginLog) << "USVQmlOverrideInterceptor created";

    // 启动时检查关键覆盖资源是否存在
    QStringList checkPaths = {
        QStringLiteral(":/USV/qml/QGroundControl/FlyView/FlyViewCustomLayer.qml"),
        QStringLiteral(":/USV/qml/QGroundControl/FlightMap/Widgets/IntegratedCompassAttitude.qml"),
    };

    for (const QString &path : checkPaths) {
        bool exists = QFile::exists(path);
        qCInfo(USVPluginLog) << "Override resource check:" << path << "exists:" << exists;
    }
}

QUrl USVQmlOverrideInterceptor::intercept(const QUrl &url,
                                           QQmlAbstractUrlInterceptor::DataType type)
{
    // 仅拦截 QML 文件和 URL 字符串（包括资源文件路径）
    switch (type) {
    case QQmlAbstractUrlInterceptor::QmlFile:
    case QQmlAbstractUrlInterceptor::UrlString:
        if (url.scheme() == QStringLiteral("qrc")) {
            const QString origPath = url.path();

            // 检查是否有 USV 定制版本
            // 例如: /qml/QGroundControl/FlyView/FlyViewCustomLayer.qml
            //    -> :/USV/qml/QGroundControl/FlyView/FlyViewCustomLayer.qml
            const QString overrideRes = QStringLiteral(":/USV%1").arg(origPath);

            if (QFile::exists(overrideRes)) {
                // 构建新的 qrc URL
                const QString relPath = overrideRes.mid(2);  // 移除 ":/"
                QUrl result;
                result.setScheme(QStringLiteral("qrc"));
                result.setPath('/' + relPath);
                qCInfo(USVPluginLog) << "Resource override:" << origPath << "->" << result.path();
                return result;
            }

            // 调试：记录关键 QML 文件的加载（仅记录 FlyView 和 FlightMap 相关）
            if (origPath.contains(QStringLiteral("FlyView")) ||
                origPath.contains(QStringLiteral("FlightMap"))) {
                qCDebug(USVPluginLog) << "QML loading (no override):" << origPath;
            }
        }
        break;
    default:
        break;
    }

    return url;
}
