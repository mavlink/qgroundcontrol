#ifndef QGCACTIONBUTTON_H
#define QGCACTIONBUTTON_H

#include "QGCToolWidgetItem.h"

namespace Ui {
    class QGCActionButton;
}

class UASInterface;

class QGCActionButton : public QGCToolWidgetItem
{
    Q_OBJECT

public:
    explicit QGCActionButton(QWidget *parent = 0);
    ~QGCActionButton();

public slots:
    void sendAction();
    void setActionButtonName(QString text);
    void startEditMode();
    void endEditMode();

private:
    Ui::QGCActionButton *ui;
    UASInterface* uas;
};

#endif // QGCACTIONBUTTON_H
