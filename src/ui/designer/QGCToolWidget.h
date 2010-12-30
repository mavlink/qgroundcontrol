#ifndef QGCTOOLWIDGET_H
#define QGCTOOLWIDGET_H

#include <QWidget>
#include <QAction>
#include <QVBoxLayout>

#include "UAS.h"

namespace Ui {
    class QGCToolWidget;
}

class QGCToolWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QGCToolWidget(QWidget *parent = 0);
    ~QGCToolWidget();

public slots:
    void addUAS(UASInterface* uas);

protected:
    QAction* addParamAction;
    QAction* addButtonAction;
    QAction* setTitleAction;
    QVBoxLayout* toolLayout;
    UAS* mav;

    void contextMenuEvent(QContextMenuEvent* event);
    void createActions();

protected slots:
    void addParam();
    void addAction();
    void setTitle();


private:
    Ui::QGCToolWidget *ui;
};

#endif // QGCTOOLWIDGET_H
