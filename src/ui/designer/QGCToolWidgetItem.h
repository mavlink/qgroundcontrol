#ifndef QGCTOOLWIDGETITEM_H
#define QGCTOOLWIDGETITEM_H

#include <QWidget>
#include <QAction>
#include <QSettings>

#include "UASInterface.h"

class QGCToolWidgetItem : public QWidget
{
    Q_OBJECT
public:
    QGCToolWidgetItem(const QString& name, QWidget *parent = 0);
    ~QGCToolWidgetItem();

    int component() {
        return _component;
    }

public slots:
    virtual void startEditMode() {}
    virtual void endEditMode() {}
    virtual void setComponent(int comp) {
        _component = comp;
    }
    virtual void writeSettings(QSettings& settings) = 0;
    virtual void readSettings(const QSettings& settings) = 0;
    virtual void setActiveUAS(UASInterface *uas);

signals:
    void editingFinished();

protected:
    QAction* startEditAction;
    QAction* stopEditAction;
    QAction* deleteAction;
    bool isInEditMode;
    QString qgcToolWidgetItemName;
    UASInterface* uas;
    int _component;          ///< The MAV component (the process or device ID)

    void contextMenuEvent (QContextMenuEvent* event);

};

#endif // QGCTOOLWIDGETITEM_H
