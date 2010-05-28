#ifndef WATCHDOGVIEW_H
#define WATCHDOGVIEW_H

#include <QtGui/QWidget>

namespace Ui {
    class WatchdogView;
}

class WatchdogView : public QWidget {
    Q_OBJECT
public:
    WatchdogView(QWidget *parent = 0);
    ~WatchdogView();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::WatchdogView *m_ui;
};

#endif // WATCHDOGVIEW_H
