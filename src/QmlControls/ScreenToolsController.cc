#include "ScreenToolsController.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "AppSettings.h"
#include "SettingsManager.h"

#include <QtGui/QCursor>
#include <QtGui/QFontDatabase>
#include <QtGui/QFontMetrics>
#include <QtGui/QInputDevice>

#if defined(Q_OS_IOS)
#include <sys/utsname.h>
#endif

QGC_LOGGING_CATEGORY(ScreenToolsControllerLog, "QMLControls.ScreenToolsController")

ScreenToolsController::ScreenToolsController(QObject *parent)
    : QObject(parent)
{
    if (QGCApplication *const app = qgcApp()) {
        (void) connect(app, &QGCApplication::languageChanged, this, [this](const QLocale &) {
            emit fontFamiliesChanged();
        });
    }

    qCDebug(ScreenToolsControllerLog) << this;
}

ScreenToolsController::~ScreenToolsController()
{
    qCDebug(ScreenToolsControllerLog) << this;
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
    if (uname(&systemInfo) == 0) {
        return QString(systemInfo.machine);
    }
    qCWarning(ScreenToolsControllerLog) << "uname failed while reading iOS device";
    return QString();
#else
    return QString();
#endif
}

QString ScreenToolsController::fixedFontFamily() const
{
    return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
}

QLocale::Language ScreenToolsController::effectiveLanguage()
{
    if (const QGCApplication *const app = qgcApp()) {
        const QLocale locale = app->getCurrentLanguage();
        if (locale.language() != QLocale::AnyLanguage) {
            return locale.language();
        }
    }

    if (SettingsManager *const settingsManager = SettingsManager::instance()) {
        if (AppSettings *const appSettings = settingsManager->appSettings()) {
            const auto language = static_cast<QLocale::Language>(appSettings->qLocaleLanguage()->rawValue().toInt());
            if (language != QLocale::AnyLanguage) {
                return language;
            }
        }
    }

    return QLocale::system().language();
}

QString ScreenToolsController::normalFontFamilyForLanguage(const QLocale::Language language)
{
    if (language == QLocale::Korean) {
        return QStringLiteral("NanumGothic");
    }

    return QStringLiteral("Open Sans");
}

QString ScreenToolsController::normalFontFamily() const
{
    return normalFontFamilyForLanguage(effectiveLanguage());
}

double ScreenToolsController::defaultFontDescent(int pointSize)
{
    return QFontMetrics(QFont(normalFontFamilyForLanguage(effectiveLanguage()), pointSize)).descent();
}

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
bool ScreenToolsController::fakeMobile()
{
    return qgcApp()->fakeMobile();
}
#endif
