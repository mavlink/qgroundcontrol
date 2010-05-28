#ifndef WATCHDOGPROCESSVIEW_H
#define WATCHDOGPROCESSVIEW_H

#include <QtGui/QWidget>

namespace Ui {
    class WatchdogProcessView;
}

class WatchdogProcessView : public QWidget {
    Q_OBJECT
public:
    WatchdogProcessView(QWidget *parent = 0);
    ~WatchdogProcessView();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::WatchdogProcessView *m_ui;
};

#endif // WATCHDOGPROCESSVIEW_H
