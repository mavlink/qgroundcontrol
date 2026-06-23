#pragma once

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QVariantList>
#include <QtCore/QVector>
#include <QtCore/QtGlobal>
#include <QtQmlIntegration/QtQmlIntegration>

class APMDataFlashLogParser : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool        parseComplete              READ parseComplete              NOTIFY parseCompleteChanged)
    Q_PROPERTY(QString      parseError          READ parseError          NOTIFY parseErrorChanged)
    Q_PROPERTY(QStringList  availableFields     READ availableFields     NOTIFY availableFieldsChanged)
    Q_PROPERTY(QVariantList parameters          READ parameters          NOTIFY parametersChanged)
    Q_PROPERTY(QVariantList events              READ events              NOTIFY eventsChanged)
    Q_PROPERTY(QVariantList messages            READ messages            NOTIFY messagesChanged)
    Q_PROPERTY(QStringList  plottableFields     READ plottableFields     NOTIFY plottableFieldsChanged)
    Q_PROPERTY(QVariantList modeSegments        READ modeSegments        NOTIFY modeSegmentsChanged)
    Q_PROPERTY(QString      detectedVehicleType READ detectedVehicleType NOTIFY detectedVehicleTypeChanged)
    Q_PROPERTY(double       minTimestamp        READ minTimestamp        NOTIFY timeRangeChanged)
    Q_PROPERTY(double       maxTimestamp        READ maxTimestamp        NOTIFY timeRangeChanged)
    Q_PROPERTY(int          sampleCount         READ sampleCount         NOTIFY sampleCountChanged)

public:
    explicit APMDataFlashLogParser(QObject *parent = nullptr);
    ~APMDataFlashLogParser();

    bool parseComplete() const { return _parseComplete; }
    QString parseError() const { return _parseError; }
    QStringList availableFields() const { return _availableFields; }
    QVariantList parameters() const { return _parameters; }
    QVariantList events() const { return _events; }
    QVariantList messages() const { return _messages; }
    QStringList plottableFields() const { return _plottableFields; }
    QVariantList modeSegments() const { return _modeSegments; }
    QString detectedVehicleType() const { return _detectedVehicleType; }
    double minTimestamp() const { return _minTimestamp; }
    double maxTimestamp() const { return _maxTimestamp; }
    int sampleCount() const { return _sampleCount; }

    Q_INVOKABLE bool parseFile(const QString &filePath);
    Q_INVOKABLE void parseFileAsync(const QString &filePath);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QVariantList fieldSamples(const QString &fieldName) const;
    Q_INVOKABLE double fieldValueAt(const QString &fieldName, double timestampSeconds) const;
    Q_INVOKABLE QString modeAt(double timestampSeconds) const;
    Q_INVOKABLE QVariantList eventsNear(double timestampSeconds, double thresholdSeconds) const;

signals:
    void parseCompleteChanged();
    void parseErrorChanged();
    void availableFieldsChanged();
    void parametersChanged();
    void eventsChanged();
    void messagesChanged();
    void plottableFieldsChanged();
    void modeSegmentsChanged();
    void detectedVehicleTypeChanged();
    void timeRangeChanged();
    void sampleCountChanged();
    void parseFileFinished(const QString &filePath, bool ok, const QString &errorMessage);

private:
    void _setParseError(const QString &error);

    bool _parseComplete = false;
    QString _parseError;
    QStringList _availableFields;
    QStringList _plottableFields;
    QVariantList _parameters;
    QVariantList _events;
    QVariantList _messages;
    QVariantList _modeSegments;
    QString _detectedVehicleType;
    QHash<QString, QVector<QPointF>> _fieldSamples;
    double _minTimestamp = -1.0;
    double _maxTimestamp = -1.0;
    int _sampleCount = 0;
    quint64 _parseRequestId = 0;
};
