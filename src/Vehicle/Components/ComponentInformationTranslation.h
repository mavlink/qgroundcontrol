/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(ComponentInformationTranslationLog)

class QGCCachedFileDownload;

class ComponentInformationTranslation : public QObject
{
    Q_OBJECT
    
public:
    ComponentInformationTranslation(QObject* parent, QGCCachedFileDownload* cachedFileDownload);

    /// Download translation file according to the currently set locale and translate the json file.
    /// emits downloadComplete() when done (with a temporary file that should be deleted)
    ///     @param summaryJsonFile json file with url's to translation files (.ts)
    ///     @param toTranslateJsonFile json file to be translated
    ///     @param maxCacheAgeSec Maximum age of cached item in seconds
    /// @return true: Asynchronous download has started, false: Download initialization failed
    bool downloadAndTranslate(const QString& summaryJsonFile, const QString& toTranslateJsonFile, int maxCacheAgeSec);

    QString translateJsonUsingTS(const QString& toTranslateJsonFile, const QString& tsFile);

signals:
    void downloadComplete(QString translatedJsonTempFile, QString errorMsg);

private slots:
    void onDownloadCompleted(QString remoteFile, QString localFile, QString errorMsg);
private:
    QString getUrlFromSummaryJson(const QString& summaryJsonFile, const QString& locale);

    static QJsonObject translate(const QJsonObject& translationObj, const QHash<QString, QString>& translations, QJsonObject doc);

    static QJsonObject translateItems(const QString& prefix, const QJsonObject& defs, const QJsonObject& translationObj,
                                      const QHash<QString, QString>& translations, QJsonObject jsonData);
    static QJsonValue translateTranslationItems(const QString& prefix, const QJsonObject& defs, const QJsonObject& translationObj,
                                                const QHash<QString, QString>& translations, QJsonValue jsonData);
    static QString getRefName(const QString& ref);

    QGCCachedFileDownload* _cachedFileDownload = nullptr;
    QString _toTranslateJsonFile;
};
