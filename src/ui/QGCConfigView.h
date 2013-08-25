#ifndef QGCCONFIGVIEW_H
#define QGCCONFIGVIEW_H

#include <QWidget>
#include <UASInterface.h>

namespace Ui {
class QGCConfigView;
}

class QGCConfigView : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCConfigView(QWidget *parent = 0);
    ~QGCConfigView();

public slots:
    void activeUASChanged(UASInterface* uas);
    
private:
    Ui::QGCConfigView *ui;
    QWidget *config;
    UASInterface* mav;

};

#endif // QGCCONFIGVIEW_H
