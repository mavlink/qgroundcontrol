#ifndef QGCPARAMSLIDER_H
#define QGCPARAMSLIDER_H

#include <QWidget>
#include <QtDesigner/QDesignerExportWidget>

namespace Ui {
    class QGCParamSlider;
}

class QDESIGNER_WIDGET_EXPORT QGCParamSlider : public QWidget
{
    Q_OBJECT

public:
    explicit QGCParamSlider(QWidget *parent = 0);
    ~QGCParamSlider();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::QGCParamSlider *ui;
};

#endif // QGCPARAMSLIDER_H
