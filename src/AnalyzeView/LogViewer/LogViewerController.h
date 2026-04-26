#pragma once

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

class LogViewerController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(SourceType sourceType READ sourceType NOTIFY sourceTypeChanged)
    Q_PROPERTY(QString currentLogPath READ currentLogPath NOTIFY currentLogPathChanged)
    Q_PROPERTY(bool hasLoadedLog READ hasLoadedLog NOTIFY currentLogPathChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QVariantList signalRows READ signalRows NOTIFY signalRowsChanged)
    Q_PROPERTY(QStringList selectedSignals READ selectedSignals NOTIFY selectedSignalsChanged)

public:
    enum class SourceType {
        None,
        TLog,
        Bin,
    };
    Q_ENUM(SourceType)

    explicit LogViewerController(QObject *parent = nullptr);
    ~LogViewerController();

    SourceType sourceType() const { return _sourceType; }
    QString currentLogPath() const { return _currentLogPath; }
    bool hasLoadedLog() const { return !_currentLogPath.isEmpty(); }
    QString statusText() const { return _statusText; }
    QVariantList signalRows() const { return _signalRows; }
    QStringList selectedSignals() const { return _selectedSignals; }

    Q_INVOKABLE void clear();
    Q_INVOKABLE void openTLog(const QString &path);
    Q_INVOKABLE void openBinLog(const QString &path);
    Q_INVOKABLE void setPlottableSignals(const QStringList &signalNames);
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void toggleGroupExpanded(const QString &groupName);
    Q_INVOKABLE bool isGroupExpanded(const QString &groupName) const;
    Q_INVOKABLE void toggleSignal(const QString &signalName);
    Q_INVOKABLE bool isSignalSelected(const QString &signalName) const;
    Q_INVOKABLE QString signalColor(const QString &signalName) const;
    Q_INVOKABLE QString eventColor(const QString &eventType) const;
    Q_INVOKABLE QString modeColor(const QString &modeName) const;
    Q_INVOKABLE QStringList modeLegendEntries(const QVariantList &modeSegments) const;

signals:
    void sourceTypeChanged();
    void currentLogPathChanged();
    void statusTextChanged();
    void signalRowsChanged();
    void selectedSignalsChanged();

private:
    void _rebuildSignalRows();
    QString _assignColorForKey(const QString &key) const;
    void _setLog(SourceType sourceType, const QString &path, const QString &statusText);

    SourceType _sourceType = SourceType::None;
    QString _currentLogPath;
    QString _statusText;
    QStringList _plottableSignals;
    QVariantList _signalRows;
    QStringList _selectedSignals;
    QSet<QString> _expandedGroups;
};
