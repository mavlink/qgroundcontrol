#ifndef SETUPWIDGETHOLDER_H
#define SETUPWIDGETHOLDER_H

#include <QWidget>

namespace Ui {
class SetupWidgetHolder;
}

class SetupWidgetHolder : public QWidget
{
    Q_OBJECT

public:
    explicit SetupWidgetHolder(QWidget *parent = 0);
    ~SetupWidgetHolder();

private:
    Ui::SetupWidgetHolder *ui;
};

#endif // SETUPWIDGETHOLDER_H
