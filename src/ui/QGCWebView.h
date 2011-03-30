#ifndef QGCWEBVIEW_H
#define QGCWEBVIEW_H

#include <QWidget>

namespace Ui
{
class QGCWebView;
}

class QGCWebView : public QWidget
{
    Q_OBJECT

public:
    explicit QGCWebView(QWidget *parent = 0);
    ~QGCWebView();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::QGCWebView *ui;
};

#endif // QGCWEBVIEW_H
