#ifndef UNCONNECTEDUASINFOWIDGET_H
#define UNCONNECTEDUASINFOWIDGET_H

#include <QGroupBox>

namespace Ui {
    class UnconnectedUASInfoWidget;
}

class UnconnectedUASInfoWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit UnconnectedUASInfoWidget(QWidget *parent = 0);
    ~UnconnectedUASInfoWidget();

private:
    Ui::UnconnectedUASInfoWidget *ui;
};

#endif // UNCONNECTEDUASINFOWIDGET_H
