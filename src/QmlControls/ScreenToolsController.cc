/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
/// @author Gus Grubba <gus@auterion.com>

#include "ScreenToolsController.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QtCore/QLocale>
#include <QtCore/QStringList>
#include <QtGui/QCursor>
#include <QtGui/QFontDatabase>
#include <QtGui/QFontMetrics>
#include <QtGui/QInputDevice>

#if defined(Q_OS_IOS)
#include <sys/utsname.h>
#endif

QGC_LOGGING_CATEGORY(ScreenToolsControllerLog, "qgc.qmlcontrols.screentoolscontroller")

namespace {
QString firstAvailableFontFamily(const QStringList &candidateFamilies)
{
    const QStringList availableFamilies = QFontDatabase::families();

    for (const QString &candidate: candidateFamilies) {
        for (const QString &available: availableFamilies) {
            if (available.compare(candidate, Qt::CaseInsensitive) == 0) {
                return available;
            }
        }
    }

    return QString();
}

QLocale::Language configuredUiLanguage()
{
    const QLocale::Language language = static_cast<QLocale::Language>(
        SettingsManager::instance()->appSettings()->qLocaleLanguage()->rawValue().toInt());

    return language == QLocale::AnyLanguage ? QLocale::system().language() : language;
}
}

ScreenToolsController::ScreenToolsController(QObject *parent)
    : QObject(parent)
{
    // qCDebug(ScreenToolsControllerLog) << Q_FUNC_INFO << this;
}

ScreenToolsController::~ScreenToolsController()
{
    // qCDebug(ScreenToolsControllerLog) << Q_FUNC_INFO << this;
}

int ScreenToolsController::mouseX()
{
    return QCursor::pos().x();
}

int ScreenToolsController::mouseY()
{
    return QCursor::pos().y();
}

bool ScreenToolsController::hasTouch()
{
    for (const auto &inputDevice: QInputDevice::devices()) {
        if (inputDevice->type() == QInputDevice::DeviceType::TouchScreen) {
            return true;
        }
    }
    return false;
}

QString ScreenToolsController::iOSDevice()
{
#if defined(Q_OS_IOS)
    struct utsname systemInfo;
    uname(&systemInfo);
    return QString(systemInfo.machine);
#else
    return QString();
#endif
}

QString ScreenToolsController::fixedFontFamily()
{
    return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
}

QString ScreenToolsController::normalFontFamily()
{
    //-- See App.SettinsGroup.json for index
    const QLocale::Language language = configuredUiLanguage();
    if (language == QLocale::Korean) {
        return QStringLiteral("NanumGothic");
    }
    if (language == QLocale::Chinese) {
        const QString chineseFontFamily = firstAvailableFontFamily({
            QStringLiteral("Source Han Sans SC"),
            QStringLiteral("Source Han Sans CN"),
            QStringLiteral("Source Han Sans"),
            QStringLiteral("Noto Sans SC"),
            QStringLiteral("Microsoft YaHei UI"),
            QStringLiteral("Microsoft YaHei"),
            QStringLiteral("PingFang SC"),
            QStringLiteral("WenQuanYi Micro Hei"),
        });
        if (!chineseFontFamily.isEmpty()) {
            return chineseFontFamily;
        }
    }

    return QStringLiteral("Open Sans");
}

double ScreenToolsController::defaultFontDescent(int pointSize)
{
    return QFontMetrics(QFont(normalFontFamily(), pointSize)).descent();
}

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
bool ScreenToolsController::fakeMobile()
{
    return qgcApp()->fakeMobile();
}
#endif
