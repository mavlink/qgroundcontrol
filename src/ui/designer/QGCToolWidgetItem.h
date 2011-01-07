#ifndef QGCTOOLWIDGETITEM_H
#define QGCTOOLWIDGETITEM_H

#include <QWidget>
#include <QAction>

#include "UASInterface.h"

class QGCToolWidgetItem : public QWidget
{
    Q_OBJECT
public:
    QGCToolWidgetItem(const QString& name, QWidget *parent = 0);
    ~QGCToolWidgetItem();

    int component() {return _component;}

public slots:
    virtual void startEditMode() {}
    virtual void endEditMode() {}
    virtual void setComponent(int comp) {_component = comp;}
    void setActiveUAS(UASInterface *uas);

protected:
    QAction* startEditAction;
    QAction* stopEditAction;
    bool isInEditMode;
    QString qgcToolWidgetItemName;
    UASInterface* uas;
    int _component;          ///< The MAV component (the process or device ID)

    void contextMenuEvent (QContextMenuEvent* event);

};

#endif // QGCTOOLWIDGETITEM_H
