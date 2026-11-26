#pragma once

#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

class QGCCachedFileDownload;

class ComponentInformationTranslation : public QObject
{
    Q_OBJECT
#ifdef QGC_UNITTEST_BUILD
    friend class ComponentInformationTranslationTest; // Unit test
#endif

public:
    /// @param cachedFileDownload Downloads the translation file. The translated file is written where it writes its
    ///        downloads (QGCFileDownload::outputDirectory())
    ComponentInformationTranslation(QObject* parent, QGCCachedFileDownload* cachedFileDownload);

    /// What translateJsonUsingTS() made
    struct Translation
    {
        QString fileName;      ///< The translated temporary file, or empty if the translation failed
        QStringList warnings;  ///< What went wrong, for the caller to log
    };

    /// Download translation file according to the currently set locale and translate the json file.
    /// The summary is read and the json file translated on worker threads. Emits downloadComplete() when done, on the
    /// thread of this object, unless cancel() or another call comes first: they drop the request in progress.
    ///     @param summaryJsonFile json file with url's to translation files (.ts)
    ///     @param toTranslateJsonFile json file to be translated
    ///     @param maxCacheAgeSec Maximum age of cached item in seconds
    /// @return true: the request has started, downloadComplete() follows. false: there is nothing to translate for an
    ///         English locale
    bool downloadAndTranslate(const QString& summaryJsonFile, const QString& toTranslateJsonFile, int maxCacheAgeSec, const QString& componentName);

    /// Drops the request in progress: no downloadComplete() follows for it, and its download is cancelled. A file
    /// that its translation still writes is removed once written.
    void cancel();

    /// Translates @p toTranslateJsonFile with the TS file @p tsFile into a new temporary file in @p outputDirectory.
    /// Both files are decompressed if they are compressed, and held to Compression::kMaxSmallFileBytes. It uses
    /// nothing but its arguments, so that it can run on a worker thread.
    static Translation translateJsonUsingTS(const QString& toTranslateJsonFile, const QString& tsFile,
                                            const QString& outputDirectory);

signals:
    /// @param translatedJsonTempFile The translated file, for the receiver to remove, or empty
    /// @param errorMsg Why the translation failed. Empty also when the summary names no translation for the locale or
    ///        the download could not start, which is logged.
    void downloadComplete(QString translatedJsonTempFile, QString errorMsg);

private slots:
    void onDownloadCompleted(bool success, const QString &localFile, QString errorMsg, bool fromCache);
private:
    /// What _urlFromSummaryJson() found
    struct SummaryEntry
    {
        QString url;      ///< The translation file for the locale, or empty
        QString warning;  ///< Why there is none
    };

    static SummaryEntry _urlFromSummaryJson(const QString& summaryJsonFile, const QString& locale,
                                            const QString& componentName);
    void _downloadTranslation(const SummaryEntry& entry, int maxCacheAgeSec);
    void _stopDownload();

    static QJsonObject translate(const QJsonObject& translationObj, const QHash<QString, QString>& translations, QJsonObject doc);

    static QJsonObject translateItems(const QString& prefix, const QJsonObject& defs, const QJsonObject& translationObj,
                                      const QHash<QString, QString>& translations, QJsonObject jsonData);
    static QJsonValue translateTranslationItems(const QString& prefix, const QJsonObject& defs, const QJsonObject& translationObj,
                                                const QHash<QString, QString>& translations, QJsonValue jsonData);
    static QString getRefName(const QString& ref);

    QGCCachedFileDownload* _cachedFileDownload = nullptr;
    QString _toTranslateJsonFile;
    bool _downloading = false;
    /// Reading the summary, then translating: the jobs of the request in progress. Deleting one drops its result.
    QPointer<QObject> _summaryJob;
    QPointer<QObject> _translationJob;
};
