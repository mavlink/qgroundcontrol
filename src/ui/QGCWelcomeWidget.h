#ifndef QGCWELCOMEWIDGET_H
#define QGCWELCOMEWIDGET_H

#include <QWidget>

namespace Ui {
class QGCWelcomeWidget;
}

class QGCWelcomeWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCWelcomeWidget(QWidget *parent = 0);
    ~QGCWelcomeWidget();
    
private:
    Ui::QGCWelcomeWidget *ui;
};

#endif // QGCWELCOMEWIDGET_H
