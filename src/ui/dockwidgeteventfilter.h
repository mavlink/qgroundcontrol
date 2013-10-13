#ifndef DOCKWIDGETEVENTFILTER_H
#define DOCKWIDGETEVENTFILTER_H

#include <QObject>

/** Event filter to update a QLabel titleBarWidget if the window's title changes */
class DockWidgetEventFilter : public QObject
{
    Q_OBJECT
public:
    DockWidgetEventFilter(QObject *parent = 0);
protected:
    virtual bool eventFilter(QObject *object,QEvent *event) override;
};

#endif // DOCKWIDGETEVENTFILTER_H
