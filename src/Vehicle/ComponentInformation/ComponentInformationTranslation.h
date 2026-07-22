#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QString>
class QGCCachedFileDownload;

class ComponentInformationTranslation : public QObject
{
    Q_OBJECT
#ifdef QGC_UNITTEST_BUILD
    friend class ComponentInformationTranslationTest; // Unit test
#endif

public:
    ComponentInformationTranslation(QObject* parent, QGCCachedFileDownload* cachedFileDownload);

    /// Download translation file according to the currently set locale and translate the json file.
    /// emits downloadComplete() when done (with a temporary file that should be deleted)
    ///     @param summaryJsonFile json file with url's to translation files (.ts)
    ///     @param toTranslateJsonFile json file to be translated
    ///     @param maxCacheAgeSec Maximum age of cached item in seconds
    ///     @param maximumFileBytes Maximum translation download and decompressed size (0 = unlimited)
    /// @return true: Asynchronous download has started, false: Download initialization failed
    bool downloadAndTranslate(const QString& summaryJsonFile, const QString& toTranslateJsonFile, int maxCacheAgeSec,
                              const QString& componentName, qint64 maximumFileBytes = 0);

    void cancel();

    bool running() const { return _running; }

    QString translateJsonUsingTS(const QString& toTranslateJsonFile, const QString& tsFile,
                                 qint64 maximumOutputBytes = 0);

signals:
    void downloadComplete(QString translatedJsonTempFile, QString errorMsg);

private slots:
    void onDownloadCompleted(bool success, const QString& localFile, QString errorMsg, bool fromCache);

private:
    struct TranslationBudget {
        qint64 remainingBytes = 0;
        bool exceeded = false;

        bool consumeReplacement(const QString& oldValue, const QString& newValue);
    };

    QString getUrlFromSummaryJson(const QString& summaryJsonFile, const QString& locale, const QString& componentName);

    static QJsonObject translate(const QJsonObject& translationObj, const QHash<QString, QString>& translations,
                                 QJsonObject doc, TranslationBudget* budget);

    static QJsonObject translateItems(const QString& prefix, const QJsonObject& defs, const QJsonObject& translationObj,
                                      const QHash<QString, QString>& translations, QJsonObject jsonData,
                                      TranslationBudget* budget);
    static QJsonValue translateTranslationItems(const QString& prefix, const QJsonObject& defs,
                                                const QJsonObject& translationObj,
                                                const QHash<QString, QString>& translations, QJsonValue jsonData,
                                                TranslationBudget* budget);
    static QString getRefName(const QString& ref);

    QGCCachedFileDownload* _cachedFileDownload = nullptr;
    QMetaObject::Connection _downloadConnection;
    QString _toTranslateJsonFile;
    qint64 _maximumFileBytes = 0;
    quint64 _generation = 0;
    bool _running = false;
};
