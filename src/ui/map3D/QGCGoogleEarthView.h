#ifndef QGCGOOGLEEARTHVIEW_H
#define QGCGOOGLEEARTHVIEW_H

#include <QWidget>

namespace Ui {
    class QGCGoogleEarthView;
}

class QGCGoogleEarthView : public QWidget
{
    Q_OBJECT

public:
    explicit QGCGoogleEarthView(QWidget *parent = 0);
    ~QGCGoogleEarthView();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::QGCGoogleEarthView *ui;
};

#endif // QGCGOOGLEEARTHVIEW_H
