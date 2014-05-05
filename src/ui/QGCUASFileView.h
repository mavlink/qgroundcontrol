#ifndef QGCUASFILEVIEW_H
#define QGCUASFILEVIEW_H

#include <QWidget>
#include "uas/QGCUASFileManager.h"

namespace Ui {
class QGCUASFileView;
}

class QGCUASFileView : public QWidget
{
    Q_OBJECT

public:
    explicit QGCUASFileView(QWidget *parent, QGCUASFileManager *manager);
    ~QGCUASFileView();

protected:
    QGCUASFileManager* _manager;

private:
    Ui::QGCUASFileView *ui;
};

#endif // QGCUASFILEVIEW_H
