#pragma once

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QVariantList>
#include <QtCore/QDateTime>
#include <QtCore/QVector>
#include <QtCore/QtGlobal>
#include <QtQmlIntegration/QtQmlIntegration>

#include <atomic>
#include <memory>

/// \brief Unified log file parser for both DataFlash (.bin/.log) and PX4 ULog (.ulg) files.
///
/// Dispatches by file extension, verifies the header magic bytes match the expected
/// format, then parses the file into a canonical set of properties that the log
/// viewer UI consumes identically for both formats:
///
///  - availableFields / plottableFields — two-level "Type.Field" hierarchy
///  - fieldSamples(name) — time-series (QPointF) for charting
///  - modeSegments — flight-mode bands for the chart timeline
///  - events — timestamped events / errors / warnings
///  - parameters — parameter name/value pairs from the log
///  - messages — free-text log messages
///  - dropouts — (ULog only) data-dropout intervals rendered as chart overlays
///
class LogFileParser : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool         parsing             READ parsing             NOTIFY parsingChanged)
    Q_PROPERTY(float        parseProgress       READ parseProgress       NOTIFY parseProgressChanged)
    Q_PROPERTY(bool         parseComplete       READ parseComplete       NOTIFY parseCompleteChanged)
    Q_PROPERTY(QString      parseError          READ parseError          NOTIFY parseErrorChanged)
    Q_PROPERTY(QStringList  availableFields     READ availableFields     NOTIFY availableFieldsChanged)
    Q_PROPERTY(QVariantList parameters          READ parameters          NOTIFY parametersChanged)
    Q_PROPERTY(QVariantList events              READ events              NOTIFY eventsChanged)
    Q_PROPERTY(QVariantList messages            READ messages            NOTIFY messagesChanged)
    Q_PROPERTY(QStringList  plottableFields     READ plottableFields     NOTIFY plottableFieldsChanged)
    Q_PROPERTY(QVariantList modeSegments        READ modeSegments        NOTIFY modeSegmentsChanged)
    Q_PROPERTY(QStringList  modeNames           READ modeNames           NOTIFY modeNamesChanged)
    Q_PROPERTY(QVariantList dropouts            READ dropouts            NOTIFY dropoutsChanged)
    Q_PROPERTY(QString      detectedVehicleType READ detectedVehicleType NOTIFY detectedVehicleTypeChanged)
    Q_PROPERTY(double       minTimestamp        READ minTimestamp        NOTIFY timeRangeChanged)
    Q_PROPERTY(double       maxTimestamp        READ maxTimestamp        NOTIFY timeRangeChanged)
    Q_PROPERTY(int          sampleCount         READ sampleCount         NOTIFY sampleCountChanged)
    Q_PROPERTY(QDateTime    startTime           READ startTime           NOTIFY startTimeChanged)

public:
    explicit LogFileParser(QObject *parent = nullptr);
    ~LogFileParser();

    bool parseComplete() const { return _parseComplete; }
    QString parseError() const { return _parseError; }
    QStringList availableFields() const { return _availableFields; }
    QVariantList parameters() const { return _parameters; }
    QVariantList events() const { return _events; }
    QVariantList messages() const { return _messages; }
    QStringList plottableFields() const { return _plottableFields; }
    QVariantList modeSegments() const { return _modeSegments; }
    QStringList modeNames() const { return _modeNames; }
    QVariantList dropouts() const { return _dropouts; }
    QString detectedVehicleType() const { return _detectedVehicleType; }
    double minTimestamp() const { return _minTimestamp; }
    double maxTimestamp() const { return _maxTimestamp; }
    int sampleCount() const { return _sampleCount; }
    QDateTime startTime() const { return _startTime; }
    bool parsing() const { return _parsing; }
    float parseProgress() const { return _parseProgress; }

    Q_INVOKABLE bool parseFile(const QString &filePath);
    Q_INVOKABLE void startParsingAsync(const QString &filePath);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QVariantList fieldSamples(const QString &fieldName) const;
    Q_INVOKABLE QVariantList fieldSamplesFiltered(const QString &fieldName, double minX, double maxX, int pixelWidth) const;
    Q_INVOKABLE QVariantMap  fieldMinMax(const QString &fieldName) const;
    Q_INVOKABLE double fieldValueAt(const QString &fieldName, double timestampSeconds) const;
    Q_INVOKABLE QString modeAt(double timestampSeconds) const;
    Q_INVOKABLE QString modeColor(const QString &modeName) const;
    Q_INVOKABLE QVariantList eventsNear(double timestampSeconds, double thresholdSeconds) const;

    /// Returns a list of GPS path points as QVariantMap entries with `latitude` and `longitude` keys
    /// (compatible with QML MapPolyline.path via implicit coordinate coercion).
    /// Tries known field name patterns for both PX4 ULog and APM DataFlash.
    /// Returns an empty list if no GPS data is found.
    Q_INVOKABLE QVariantList gpsPath() const;

    /// Returns the field name used for altitude data paired with the GPS path source,
    /// e.g. "vehicle_global_position.alt". Returns empty string if no GPS data was found.
    /// Must be called after gpsPath() has been evaluated.
    Q_INVOKABLE QString gpsAltitudeFieldName() const;

    /// Returns {latitude, longitude} for the GPS position nearest to timestampSeconds.
    /// Returns an empty map if GPS data is not available.
    Q_INVOKABLE QVariantMap gpsCoordAt(double timestampSeconds) const;

signals:
    void parseCompleteChanged();
    void parseErrorChanged();
    void availableFieldsChanged();
    void parametersChanged();
    void eventsChanged();
    void messagesChanged();
    void plottableFieldsChanged();
    void modeSegmentsChanged();
    void modeNamesChanged();
    void dropoutsChanged();
    void detectedVehicleTypeChanged();
    void timeRangeChanged();
    void sampleCountChanged();
    void startTimeChanged();
    void parsingChanged();
    void parseProgressChanged();
    void parseFileFinished(const QString &filePath, bool ok, const QString &errorMessage);

private:
    void _setParseError(const QString &error);
    void _applyResult(const struct LogParseResult &result);

    bool _parseComplete = false;
    QString _parseError;
    QStringList _availableFields;
    QStringList _plottableFields;
    QVariantList _parameters;
    QVariantList _events;
    QVariantList _messages;
    QVariantList _modeSegments;
    QVariantList _dropouts;
    QString _detectedVehicleType;
    QHash<QString, QVector<QPointF>> _fieldSamples;
    double _minTimestamp = -1.0;
    double _maxTimestamp = -1.0;
    int _sampleCount = 0;
    quint64 _parseRequestId = 0;
    QDateTime _startTime;
    bool _parsing = false;
    float _parseProgress = 0.f;
    std::shared_ptr<std::atomic<bool>> _cancelToken;

    QStringList _modeNames;
    QHash<QString, int> _modeColorCache;

    // Cached GPS field names, set by gpsPath() when a valid candidate is found.
    mutable QString _gpsLatField;
    mutable QString _gpsLonField;
    mutable QString _gpsAltField;
};
