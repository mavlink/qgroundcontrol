#ifndef QGCACTIONBUTTON_H
#define QGCACTIONBUTTON_H

#include "QGCToolWidgetItem.h"

namespace Ui {
    class QGCActionButton;
}

class QGCActionButton : public QGCToolWidgetItem
{
    Q_OBJECT

public:
    explicit QGCActionButton(QWidget *parent = 0);
    ~QGCActionButton();

public slots:
    void startEditMode();
    void endEditMode();

private:
    Ui::QGCActionButton *ui;
};

#endif // QGCACTIONBUTTON_H
