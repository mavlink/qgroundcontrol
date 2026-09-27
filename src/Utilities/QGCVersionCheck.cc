#include "QGCVersionCheck.h"

#include <optional>

#include <QtCore/QApplicationStatic>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QTextStream>
#include <QtCore/QVersionNumber>

#include "AppSettings.h"
#include "Fact.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "QGCFileDownload.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(QGCVersionCheckLog, "Utilities.QGCVersionCheck")

Q_APPLICATION_STATIC(QGCVersionCheck, _qgcVersionCheckInstance);

namespace {

std::optional<QVersionNumber> _parseVersion(const QString& versionString)
{
    static const QRegularExpression regExp(QStringLiteral("^v(\\d+)\\.(\\d+)\\.(\\d+)"));
    const QRegularExpressionMatch match = regExp.match(versionString.trimmed());
    if (!match.hasMatch()) {
        return std::nullopt;
    }

    return QVersionNumber(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt());
}

}  // namespace

QGCVersionCheck::QGCVersionCheck(QObject* parent)
    : QObject(parent)
{}

QGCVersionCheck* QGCVersionCheck::instance()
{
    return _qgcVersionCheckInstance();
}

bool QGCVersionCheck::isNewerVersion(const QString& currentVersion, const QString& latestVersion)
{
    const std::optional<QVersionNumber> current = _parseVersion(currentVersion);
    const std::optional<QVersionNumber> latest = _parseVersion(latestVersion);
    return current && latest && (*latest > *current);
}

bool QGCVersionCheck::shouldNotify(const QString& latestVersion, const QString& lastNotifiedVersion)
{
    const std::optional<QVersionNumber> latest = _parseVersion(latestVersion);
    if (!latest) {
        return false;
    }

    const std::optional<QVersionNumber> lastNotified = _parseVersion(lastNotifiedVersion);
    return !lastNotified || (*latest > *lastNotified);
}

void QGCVersionCheck::start()
{
    if (!_parseVersion(QCoreApplication::applicationVersion())) {
        return;
    }

    const QString versionCheckFile = QGCCorePlugin::instance()->stableVersionCheckFileUrl();
    if (versionCheckFile.isEmpty()) {
        return;
    }

    QGCFileDownload* const download = new QGCFileDownload(this);
    (void) connect(download, &QGCFileDownload::finished, this,
                   [this, download](bool success, const QString& localFile, const QString& errorMsg) {
                       download->deleteLater();
                       _downloadComplete(success, localFile, errorMsg);
                   });
    if (!download->start(versionCheckFile)) {
        qCDebug(QGCVersionCheckLog) << "Download QGC stable version failed to start" << download->errorString();
        download->deleteLater();
    }
}

void QGCVersionCheck::_downloadComplete(bool success, const QString& localFile, const QString& errorMsg)
{
    if (!success) {
        if (!errorMsg.isEmpty()) {
            qCDebug(QGCVersionCheckLog) << "Download QGC stable version failed" << errorMsg;
        }
        return;
    }

    QFile versionFile(localFile);
    if (!versionFile.open(QIODevice::ReadOnly)) {
        return;
    }

    QTextStream textStream(&versionFile);
    const QString version = textStream.readLine();
    qCDebug(QGCVersionCheckLog) << version;

    if (!isNewerVersion(QCoreApplication::applicationVersion(), version)) {
        return;
    }

    _newStableVersion = version;
    emit newStableVersionChanged();

    Fact* const lastNotifiedFact = SettingsManager::instance()->appSettings()->lastNotifiedStableVersion();
    if (shouldNotify(version, lastNotifiedFact->rawValue().toString())) {
        lastNotifiedFact->setRawValue(version);
        const QString url = QGCCorePlugin::instance()->stableDownloadUrl();
        qgcApp()->showAppMessage(
            tr("There is a newer version of %1 available. You can download it from <a href=\"%2\">%2</a>.")
                .arg(QCoreApplication::applicationName(), url),
            tr("New Version Available"));
    }
}
