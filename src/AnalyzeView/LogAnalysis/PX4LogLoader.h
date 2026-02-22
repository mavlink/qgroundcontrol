#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QVariantMap>
#include <QtQmlIntegration/QtQmlIntegration>

#include <memory>

Q_DECLARE_LOGGING_CATEGORY(PX4LogLoaderLog)

namespace ulog_cpp {
class DataContainer;
}

class PX4LogTopic;

class PX4LogLoader : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("PX4LogTopic.h")

    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QStringList topicNames READ topicNames NOTIFY loadedChanged)
    Q_PROPERTY(QVariantMap parameters READ parameters NOTIFY loadedChanged)
    Q_PROPERTY(QVariantMap infoMessages READ infoMessages NOTIFY loadedChanged)
    Q_PROPERTY(QStringList logMessages READ logMessages NOTIFY loadedChanged)
    Q_PROPERTY(int dropoutCount READ dropoutCount NOTIFY loadedChanged)
    Q_PROPERTY(quint64 durationUs READ durationUs NOTIFY loadedChanged)

public:
    explicit PX4LogLoader(QObject *parent = nullptr);
    ~PX4LogLoader();

    Q_INVOKABLE bool load(const QString &filePath);
    Q_INVOKABLE void clear();
    Q_INVOKABLE PX4LogTopic *topic(const QString &name, int multiId = 0) const;

    bool loaded() const { return _loaded; }
    QString errorMessage() const { return _errorMessage; }
    QStringList topicNames() const { return _topicNames; }
    QVariantMap parameters() const { return _parameters; }
    QVariantMap infoMessages() const { return _infoMessages; }
    QStringList logMessages() const { return _logMessages; }
    int dropoutCount() const { return _dropoutCount; }
    quint64 durationUs() const { return _durationUs; }

signals:
    void loadedChanged();
    void errorMessageChanged();

private:
    void _extractMetadata();

    bool _loaded = false;
    QString _errorMessage;

    std::shared_ptr<ulog_cpp::DataContainer> _dataContainer;
    QMap<QString, PX4LogTopic *> _topics;

    QStringList _topicNames;
    QVariantMap _parameters;
    QVariantMap _infoMessages;
    QStringList _logMessages;
    int _dropoutCount = 0;
    quint64 _durationUs = 0;
};
