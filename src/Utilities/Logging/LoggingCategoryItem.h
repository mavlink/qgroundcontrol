#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include <memory>

class QmlObjectListModel;

/// Represents a single logging category for QML binding.
class LoggingCategoryItem : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString shortName READ shortName CONSTANT)
    Q_PROPERTY(QString fullName READ fullName CONSTANT)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(QmlObjectListModel* children READ children CONSTANT)

public:
    explicit LoggingCategoryItem(const QString& shortName,
                                 const QString& fullName,
                                 bool enabled,
                                 QObject* parent = nullptr);

    QString shortName() const { return _shortName; }
    QString fullName() const { return _fullName; }
    bool isEnabled() const { return _enabled; }
    bool isExpanded() const { return _expanded; }
    QmlObjectListModel* children() const { return _children.get(); }

    void setEnabled(bool enabled);
    void setExpanded(bool expanded);

    /// Sets enabled state without notifying the manager (used by manager to avoid recursion).
    void setEnabledFromManager(bool enabled);

    /// Ensures child model exists (always valid since constructor creates it).
    void ensureChildModel();

    /// Returns true if this is a parent category (has children).
    bool isParent() const;

signals:
    void enabledChanged();
    void expandedChanged();

private:
    QString _shortName;
    QString _fullName;
    bool _enabled = false;
    bool _expanded = false;
    std::unique_ptr<QmlObjectListModel> _children;
};
