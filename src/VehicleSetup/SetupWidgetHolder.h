#ifndef SETUPWIDGETHOLDER_H
#define SETUPWIDGETHOLDER_H

#include <QDialog>

namespace Ui {
class SetupWidgetHolder;
}

class SetupWidgetHolder : public QDialog
{
    Q_OBJECT

public:
    explicit SetupWidgetHolder(QWidget *parent = 0);
    ~SetupWidgetHolder();
    
    void setInnerWidget(QWidget* widget);

private:
    Ui::SetupWidgetHolder *ui;
};

#endif // SETUPWIDGETHOLDER_H
