#ifndef QGCTABBEDINFOVIEW_H
#define QGCTABBEDINFOVIEW_H

#include <QWidget>
#include "ui_QGCTabbedInfoView.h"

class QGCTabbedInfoView : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCTabbedInfoView(QWidget *parent = 0);
    ~QGCTabbedInfoView();
    
private:
    Ui::QGCTabbedInfoView ui;
};

#endif // QGCTABBEDINFOVIEW_H
