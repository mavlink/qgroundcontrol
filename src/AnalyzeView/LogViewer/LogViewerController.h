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

    Q_PROPERTY(SourceType   sourceType     READ sourceType     NOTIFY sourceTypeChanged)
    Q_PROPERTY(QString      currentLogPath READ currentLogPath NOTIFY currentLogPathChanged)
    Q_PROPERTY(bool         hasLoadedLog   READ hasLoadedLog   NOTIFY currentLogPathChanged)
    Q_PROPERTY(QVariantList fieldRows      READ fieldRows      NOTIFY fieldRowsChanged)
    Q_PROPERTY(QStringList  selectedFields READ selectedFields NOTIFY selectedFieldsChanged)

public:
    enum class SourceType {
        None,
        TLog,
        Bin,
        ULog,
    };
    Q_ENUM(SourceType)

    explicit LogViewerController(QObject *parent = nullptr);
    ~LogViewerController();

    SourceType sourceType() const { return _sourceType; }
    QString currentLogPath() const { return _currentLogPath; }
    bool hasLoadedLog() const { return !_currentLogPath.isEmpty(); }
    QVariantList fieldRows() const { return _fieldRows; }
    QStringList selectedFields() const { return _selectedFields; }

    Q_INVOKABLE void clear();
    Q_INVOKABLE void openTLog(const QString &path);
    Q_INVOKABLE void openBinLog(const QString &path);
    Q_INVOKABLE void openULogFile(const QString &path);
    Q_INVOKABLE void setPlottableFields(const QStringList &fieldNames);
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void toggleGroupExpanded(const QString &groupName);
    Q_INVOKABLE bool isGroupExpanded(const QString &groupName) const;
    Q_INVOKABLE void setFieldSelected(const QString &fieldName, bool selected);
    Q_INVOKABLE bool isFieldSelected(const QString &fieldName) const;
    /// Returns a deterministic hash-based color for unselected fields.
    /// For selected fields, QML assigns index-based colors from its own palette.
    Q_INVOKABLE QString fieldColor(const QString &fieldName) const;
    Q_INVOKABLE QString eventColor(const QString &eventType) const;
    Q_INVOKABLE QString modeColor(const QString &modeName) const;
    Q_INVOKABLE QStringList modeLegendEntries(const QVariantList &modeSegments) const;

signals:
    void sourceTypeChanged();
    void currentLogPathChanged();
    void fieldRowsChanged();
    void selectedFieldsChanged();

private:
    void _rebuildFieldRows();
    QString _assignColorForKey(const QString &key) const;
    void _setLog(SourceType sourceType, const QString &path);

    SourceType _sourceType = SourceType::None;
    QString _currentLogPath;
    QStringList _plottableFields;
    QVariantList _fieldRows;
    QStringList _selectedFields;
    QSet<QString> _expandedGroups;
};
