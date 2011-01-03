#ifndef QGCTOOLWIDGETITEM_H
#define QGCTOOLWIDGETITEM_H

#include <QWidget>
#include <QAction>

class QGCToolWidgetItem : public QWidget
{
    Q_OBJECT
public:
    explicit QGCToolWidgetItem(QWidget *parent = 0);
    ~QGCToolWidgetItem();

    int component() {return _component;}

public slots:
    virtual void startEditMode() {}
    virtual void endEditMode() {}
    virtual void setComponent(int comp) {_component = comp;}

protected:
    QAction* startEditAction;
    QAction* stopEditAction;

    bool isInEditMode;
    int _component;          ///< The MAV component (the process or device ID)

    void contextMenuEvent (QContextMenuEvent* event);

};

#endif // QGCTOOLWIDGETITEM_H
