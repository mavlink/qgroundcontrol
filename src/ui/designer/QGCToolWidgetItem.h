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

    int componentId() {
        return _component;
    }

    virtual void setEditMode(bool editMode);
    bool isEditMode() const { return isInEditMode; }
public slots:
    void startEditMode() { setEditMode(true); }
    void endEditMode() { setEditMode(false); }

    virtual void setComponent(int comp) {
        _component = comp;
    }
    virtual void writeSettings(QSettings& settings) = 0;
    virtual void readSettings(const QSettings& settings) = 0;
    virtual void readSettings(const QString& pre,const QVariantMap& settings) = 0;
    virtual void setActiveUAS(UASInterface *uas);

signals:
    void editingFinished();

protected:
    void contextMenuEvent (QContextMenuEvent* event);
    void init();
    UASInterface* uas;

private:
    QAction* startEditAction;
    QAction* stopEditAction;
    QAction* deleteAction;
    bool isInEditMode;
    QString qgcToolWidgetItemName;
    int _component;          ///< The MAV component (the process or device ID)



};

#endif // QGCTOOLWIDGETITEM_H
