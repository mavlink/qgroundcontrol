#pragma once

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

class DataFlashLogParser : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool parsed READ parsed NOTIFY parsedChanged)
    Q_PROPERTY(QString parseError READ parseError NOTIFY parseErrorChanged)
    Q_PROPERTY(QStringList availableSignals READ availableSignals NOTIFY availableSignalsChanged)
    Q_PROPERTY(QVariantList parameters READ parameters NOTIFY parametersChanged)
    Q_PROPERTY(QVariantList events READ events NOTIFY eventsChanged)
    Q_PROPERTY(QStringList plottableSignals READ plottableSignals NOTIFY plottableSignalsChanged)
    Q_PROPERTY(QVariantList modeSegments READ modeSegments NOTIFY modeSegmentsChanged)
    Q_PROPERTY(QString detectedVehicleType READ detectedVehicleType NOTIFY detectedVehicleTypeChanged)
    Q_PROPERTY(double minTimestamp READ minTimestamp NOTIFY timeRangeChanged)
    Q_PROPERTY(double maxTimestamp READ maxTimestamp NOTIFY timeRangeChanged)
    Q_PROPERTY(int sampleCount READ sampleCount NOTIFY sampleCountChanged)

public:
    explicit DataFlashLogParser(QObject *parent = nullptr);
    ~DataFlashLogParser();

    bool parsed() const { return _parsed; }
    QString parseError() const { return _parseError; }
    QStringList availableSignals() const { return _availableSignals; }
    QVariantList parameters() const { return _parameters; }
    QVariantList events() const { return _events; }
    QStringList plottableSignals() const { return _plottableSignals; }
    QVariantList modeSegments() const { return _modeSegments; }
    QString detectedVehicleType() const { return _detectedVehicleType; }
    double minTimestamp() const { return _minTimestamp; }
    double maxTimestamp() const { return _maxTimestamp; }
    int sampleCount() const { return _sampleCount; }

    Q_INVOKABLE bool parseFile(const QString &filePath);
    Q_INVOKABLE void parseFileAsync(const QString &filePath);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QVariantList signalSamples(const QString &signalName) const;
    Q_INVOKABLE double signalValueAt(const QString &signalName, double timestampSeconds) const;
    Q_INVOKABLE QString modeAt(double timestampSeconds) const;
    Q_INVOKABLE QVariantList eventsNear(double timestampSeconds, double thresholdSeconds) const;

signals:
    void parsedChanged();
    void parseErrorChanged();
    void availableSignalsChanged();
    void parametersChanged();
    void eventsChanged();
    void plottableSignalsChanged();
    void modeSegmentsChanged();
    void detectedVehicleTypeChanged();
    void timeRangeChanged();
    void sampleCountChanged();
    void parseFileFinished(const QString &filePath, bool ok, const QString &errorMessage);

private:
    static double _extractTimestampSeconds(const QMap<QString, QVariant> &values);
    void _appendEvent(double timestampSecs, const QString &type, const QString &description);
    void _setParseError(const QString &error);

    bool _parsed = false;
    QString _parseError;
    QStringList _availableSignals;
    QStringList _plottableSignals;
    QVariantList _parameters;
    QVariantList _events;
    QVariantList _modeSegments;
    QString _detectedVehicleType;
    QHash<QString, QVariantList> _signalSamples;
    double _minTimestamp = -1.0;
    double _maxTimestamp = -1.0;
    int _sampleCount = 0;
};
