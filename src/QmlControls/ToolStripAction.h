/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QVariant>
#include <QQmlComponent>

class ToolStripAction : public QObject
{
    Q_OBJECT
    
public:
    ToolStripAction(QObject* parent = nullptr);
    
    Q_PROPERTY(bool             enabled             READ enabled                WRITE setEnabled                NOTIFY enabledChanged)
    Q_PROPERTY(bool             visible             READ visible                WRITE setVisible                NOTIFY visibleChanged)
    Q_PROPERTY(bool             checkable           READ checkable              WRITE setCheckable              NOTIFY checkableChanged)
    Q_PROPERTY(bool             checked             READ checked                WRITE setChecked                NOTIFY checkedChanged)
    Q_PROPERTY(bool             showAlternateIcon   READ showAlternateIcon      WRITE setShowAlternateIcon      NOTIFY showAlternateIconChanged)
    Q_PROPERTY(QString          text                READ text                   WRITE setText                   NOTIFY textChanged)
    Q_PROPERTY(QString          iconSource          READ iconSource             WRITE setIconSource             NOTIFY iconSourceChanged)
    Q_PROPERTY(QString          alternateIconSource READ alternateIconSource    WRITE setAlternateIconSource    NOTIFY alternateIconSourceChanged)
    Q_PROPERTY(QQmlComponent*   dropPanelComponent  READ dropPanelComponent     WRITE setDropPanelComponent     NOTIFY dropPanelComponentChanged)

    bool            enabled             (void) const { return _enabled; }
    bool            visible             (void) const { return _visible; }
    bool            checkable           (void) const { return _checkable; }
    bool            checked             (void) const { return _checked; }
    bool            showAlternateIcon   (void) const { return _showAlternateIcon; }
    QString         text                (void) const { return _text; }
    QString         iconSource          (void) const { return _iconSource; }
    QString         alternateIconSource (void) const { return _alternateIconSource; }
    QQmlComponent* dropPanelComponent   (void) const { return _dropPanelComponent; }

    void setEnabled             (bool enabled);
    void setVisible             (bool visible);
    void setCheckable           (bool checkable);
    void setChecked             (bool checked);
    void setShowAlternateIcon   (bool showAlternateIcon);
    void setText                (const QString& text);
    void setIconSource          (const QString& iconSource);
    void setAlternateIconSource (const QString& alternateIconSource);
    void setDropPanelComponent  (QQmlComponent* dropPanelComponent);

signals:
    void enabledChanged             (bool enabled);
    void visibleChanged             (bool visible);
    void checkableChanged           (bool checkable);
    void checkedChanged             (bool checked);
    void showAlternateIconChanged   (bool showAlternateIcon);
    void textChanged                (QString text);
    void iconSourceChanged          (QString iconSource);
    void alternateIconSourceChanged (QString alternateIconSource);
    void triggered                  (QObject* source);
    void dropPanelComponentChanged  (void);

protected:
    bool            _enabled =              true;
    bool            _visible =              true;
    bool            _checkable =            false;
    bool            _checked =              false;
    bool            _showAlternateIcon =    false;
    QString         _text;
    QString         _iconSource;
    QString         _alternateIconSource;
    QQmlComponent*  _dropPanelComponent =   nullptr;
};
