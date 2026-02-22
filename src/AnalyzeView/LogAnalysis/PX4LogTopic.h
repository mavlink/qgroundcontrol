#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QStringList>
#include <QtQmlIntegration/QtQmlIntegration>

#include <ulog_cpp/subscription.hpp>

Q_DECLARE_LOGGING_CATEGORY(PX4LogTopicLog)

class PX4LogTopic : public QObject
{
    Q_OBJECT
    QML_UNCREATABLE("Reference only")

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QStringList fieldNames READ fieldNames CONSTANT)
    Q_PROPERTY(int sampleCount READ sampleCount CONSTANT)

public:
    explicit PX4LogTopic(const std::string &name,
                         std::shared_ptr<ulog_cpp::Subscription> subscription,
                         QObject *parent = nullptr);
    ~PX4LogTopic();

    QString name() const { return _name; }
    QStringList fieldNames() const { return _fieldNames; }
    int sampleCount() const;

    /// Returns time-series data for a field as (timestamp_us, value) pairs, ready for Qt Graphs.
    Q_INVOKABLE QList<QPointF> fieldSeries(const QString &fieldName) const;

    /// Returns all timestamps (microseconds since log start) for this topic.
    Q_INVOKABLE QList<qreal> timestamps() const;

    /// Returns raw values for a single field across all samples.
    Q_INVOKABLE QList<qreal> fieldValues(const QString &fieldName) const;

    const std::shared_ptr<ulog_cpp::Subscription> &subscription() const { return _subscription; }

private:
    QString _name;
    QStringList _fieldNames;
    std::shared_ptr<ulog_cpp::Subscription> _subscription;
};
