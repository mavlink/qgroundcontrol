#ifndef DOCKWIDGETTITLEBAREVENTFILTER_H
#define DOCKWIDGETTITLEBAREVENTFILTER_H

#include <QObject>

class DockWidgetTitleBarEventFilter : public QObject
{
    Q_OBJECT
public:
    explicit DockWidgetTitleBarEventFilter(QObject *parent = 0);
protected:
    bool eventFilter(QObject *object,QEvent *event);
signals:
    
public slots:
    
};

#endif // DOCKWIDGETTITLEBAREVENTFILTER_H
