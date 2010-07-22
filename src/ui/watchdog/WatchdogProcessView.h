#ifndef WATCHDOGPROCESSVIEW_H
#define WATCHDOGPROCESSVIEW_H

#include <QtGui/QWidget>
#include <QMap>

namespace Ui {
    class WatchdogProcessView;
}

class WatchdogProcessView : public QWidget {
    Q_OBJECT
public:
    WatchdogProcessView(int processid, QWidget *parent = 0);
    ~WatchdogProcessView();

protected:
    void changeEvent(QEvent *e);
    int processid;

private:
    Ui::WatchdogProcessView *m_ui;
};

#endif // WATCHDOGPROCESSVIEW_H
